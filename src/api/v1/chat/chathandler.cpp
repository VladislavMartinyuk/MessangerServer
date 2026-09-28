#include "chathandler.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <set>
#include <spdlog/spdlog.h>

using grpc::Status;
using grpc::StatusCode;

namespace api::v1 {
namespace {

// The reactor completes the RPC when its asynchronous database work ends.
class UnaryReactor final : public grpc::ServerUnaryReactor {
public:
    using Work = std::function<asio::awaitable<Status>()>;

    UnaryReactor(asio::io_context &ioc, Work work) : m_work(std::move(work)) {
        asio::co_spawn(ioc, execute(), asio::detached);
    }

private:
    Work m_work;

    asio::awaitable<void> execute() {
        Status status;
        try {
            status = co_await m_work();
        } catch (const std::exception &error) {
            spdlog::error("Chat RPC failed: {}", error.what());
            status = {StatusCode::INTERNAL, "Database error"};
        }
        Finish(status);
    }

    void OnDone() override { delete this; }
};

} // namespace

ChatHandler::ChatHandler(std::shared_ptr<asio::io_context> ioc, const AppServices &services)
    : m_ioc(std::move(ioc)), m_services(services) {}

ChatRepo ChatHandler::repo() const {
    return ChatRepo(m_services.dbService->dataBase("messenger"));
}

void ChatHandler::addSubscriber(const std::string &chatUuid,
                                const std::shared_ptr<Subscriber> &subscriber) {
    std::lock_guard lock(m_subscribersMutex);
    m_subscribers[chatUuid].push_back(subscriber);
}

void ChatHandler::removeSubscriber(const std::string &chatUuid,
                                   const std::shared_ptr<Subscriber> &subscriber) {
    std::lock_guard lock(m_subscribersMutex);
    auto it = m_subscribers.find(chatUuid);
    if (it == m_subscribers.end()) return;
    auto &list = it->second;
    std::erase_if(list, [&](const auto &weak) {
        auto current = weak.lock();
        return !current || current == subscriber;
    });
    if (list.empty()) m_subscribers.erase(it);
}

void ChatHandler::publish(const std::string &chatUuid) {
    std::vector<std::shared_ptr<Subscriber>> active;
    {
        std::lock_guard lock(m_subscribersMutex);
        auto it = m_subscribers.find(chatUuid);
        if (it == m_subscribers.end()) return;
        auto &list = it->second;
        std::erase_if(list, [&](const auto &weak) {
            if (auto subscriber = weak.lock()) {
                active.push_back(std::move(subscriber));
                return false;
            }
            return true;
        });
        if (list.empty()) m_subscribers.erase(it);
    }
    for (auto &subscriber : active) {
        asio::post(subscriber->strand, [subscriber] {
            ++subscriber->generation;
            subscriber->wake.cancel();
        });
    }
}

void ChatHandler::fillMessage(const ChatMessage &source, Message *target) {
    target->set_id(source.id);
    target->set_uuid(source.uuid);
    target->set_chat_uuid(source.chatUuid);
    target->set_sender_uuid(source.senderUuid);
    target->set_text(source.text);
    target->set_created_at(source.createdAt);
}

Status ChatHandler::internalError(const std::exception &error) {
    spdlog::error("Chat RPC failed: {}", error.what());
    return {StatusCode::INTERNAL, "Database error"};
}

grpc::ServerUnaryReactor *ChatHandler::CreatePersonalChat(
    grpc::CallbackServerContext *, const CreatePersonalChatRequest *request,
    ChatIdResponse *response) {
    return new UnaryReactor(*m_ioc, [this, input = *request, response]() -> asio::awaitable<Status> {
        if (input.user_uuid().empty() || input.other_user_uuid().empty() ||
            input.user_uuid() == input.other_user_uuid()) {
            co_return Status{StatusCode::INVALID_ARGUMENT, "Two different user UUIDs are required"};
        }
        auto storage = repo();
        if (!(co_await storage.userExists(input.user_uuid())) ||
            !(co_await storage.userExists(input.other_user_uuid()))) {
            co_return Status{StatusCode::NOT_FOUND, "User not found"};
        }
        response->set_chat_uuid(co_await storage.createPersonal(input.user_uuid(),
                                                                 input.other_user_uuid()));
        co_return Status::OK;
    });
}

grpc::ServerUnaryReactor *ChatHandler::CreateGroupChat(
    grpc::CallbackServerContext *, const CreateGroupChatRequest *request,
    ChatIdResponse *response) {
    return new UnaryReactor(*m_ioc, [this, input = *request, response]() -> asio::awaitable<Status> {
        if (input.creator_uuid().empty() || input.name().empty() ||
            input.name().size() > 100 || input.member_uuids_size() > 50) {
            co_return Status{StatusCode::INVALID_ARGUMENT, "Invalid group name or members"};
        }
        std::set<std::string> unique(input.member_uuids().begin(), input.member_uuids().end());
        unique.insert(input.creator_uuid());
        if (unique.contains("")) {
            co_return Status{StatusCode::INVALID_ARGUMENT, "Member UUID is required"};
        }
        auto storage = repo();
        for (const auto &member : unique) {
            if (!(co_await storage.userExists(member))) {
                co_return Status{StatusCode::NOT_FOUND, "User not found: " + member};
            }
        }
        response->set_chat_uuid(co_await storage.createGroup(
            input.creator_uuid(), input.name(), {unique.begin(), unique.end()}));
        co_return Status::OK;
    });
}

grpc::ServerUnaryReactor *ChatHandler::AddGroupMember(
    grpc::CallbackServerContext *, const AddGroupMemberRequest *request,
    AddGroupMemberResponse *response) {
    return new UnaryReactor(*m_ioc, [this, input = *request, response]() -> asio::awaitable<Status> {
        if (input.chat_uuid().empty() || input.creator_uuid().empty() ||
            input.member_uuid().empty()) {
            co_return Status{StatusCode::INVALID_ARGUMENT, "Chat and user UUIDs are required"};
        }
        auto storage = repo();
        if (!(co_await storage.userExists(input.member_uuid()))) {
            co_return Status{StatusCode::NOT_FOUND, "User not found"};
        }
        if (!(co_await storage.addGroupMember(input.chat_uuid(), input.creator_uuid(),
                                               input.member_uuid()))) {
            co_return Status{StatusCode::PERMISSION_DENIED,
                             "Only the group creator can add members"};
        }
        response->set_added(true);
        co_return Status::OK;
    });
}

grpc::ServerUnaryReactor *ChatHandler::ListMembers(
    grpc::CallbackServerContext *, const ListMembersRequest *request,
    ListMembersResponse *response) {
    return new UnaryReactor(*m_ioc, [this, input = *request, response]() -> asio::awaitable<Status> {
        if (input.chat_uuid().empty() || input.user_uuid().empty()) {
            co_return Status{StatusCode::INVALID_ARGUMENT, "Chat and user UUIDs are required"};
        }
        auto storage = repo();
        if (!(co_await storage.isMember(input.chat_uuid(), input.user_uuid()))) {
            co_return Status{StatusCode::PERMISSION_DENIED, "User is not a chat member"};
        }
        for (const auto &member : co_await storage.getMembers(input.chat_uuid())) {
            auto *item = response->add_members();
            item->set_user_uuid(member.uuid);
            item->set_login(member.login);
            item->set_name(member.name);
        }
        co_return Status::OK;
    });
}

grpc::ServerUnaryReactor *ChatHandler::SendMessage(
    grpc::CallbackServerContext *, const SendMessageRequest *request, Message *response) {
    return new UnaryReactor(*m_ioc, [this, input = *request, response]() -> asio::awaitable<Status> {
        if (input.chat_uuid().empty() || input.sender_uuid().empty() ||
            input.text().empty() || input.text().size() > 60000) {
            co_return Status{StatusCode::INVALID_ARGUMENT, "Invalid chat, sender or message text"};
        }
        auto storage = repo();
        if (!(co_await storage.isMember(input.chat_uuid(), input.sender_uuid()))) {
            co_return Status{StatusCode::PERMISSION_DENIED, "User is not a chat member"};
        }
        auto message = co_await storage.sendMessage(input.chat_uuid(),
                                                    input.sender_uuid(), input.text());
        fillMessage(message, response);
        publish(message.chatUuid);
        co_return Status::OK;
    });
}

grpc::ServerUnaryReactor *ChatHandler::ListMessages(
    grpc::CallbackServerContext *, const ListMessagesRequest *request,
    ListMessagesResponse *response) {
    return new UnaryReactor(*m_ioc, [this, input = *request, response]() -> asio::awaitable<Status> {
        if (input.chat_uuid().empty() || input.user_uuid().empty()) {
            co_return Status{StatusCode::INVALID_ARGUMENT, "Chat and user UUIDs are required"};
        }
        auto storage = repo();
        if (!(co_await storage.isMember(input.chat_uuid(), input.user_uuid()))) {
            co_return Status{StatusCode::PERMISSION_DENIED, "User is not a chat member"};
        }
        const unsigned int limit = input.limit() == 0 ? 50 : std::min(input.limit(), 100u);
        for (const auto &message : co_await storage.listMessages(input.chat_uuid(),
                                                                 input.before_id(), limit)) {
            fillMessage(message, response->add_messages());
        }
        co_return Status::OK;
    });
}

class ChatHandler::SubscribeReactor final : public grpc::ServerWriteReactor<Message> {
public:
    SubscribeReactor(ChatHandler &owner, SubscribeRequest request)
        : m_owner(owner), m_request(std::move(request)),
          m_subscriber(std::make_shared<Subscriber>(*owner.m_ioc)) {
        asio::co_spawn(m_subscriber->strand, stream(), asio::detached);
    }

private:
    ChatHandler &m_owner;
    SubscribeRequest m_request;
    std::shared_ptr<Subscriber> m_subscriber;
    Message m_current;

    void OnWriteDone(bool ok) override {
        auto subscriber = m_subscriber;
        asio::post(subscriber->strand, [subscriber, ok] {
            subscriber->writeDone = true;
            subscriber->writeOk = ok;
            subscriber->writeWake.cancel();
        });
    }

    void OnCancel() override {
        auto subscriber = m_subscriber;
        asio::post(subscriber->strand, [subscriber] {
            subscriber->cancelled = true;
            subscriber->wake.cancel();
        });
    }

    void OnDone() override {
        m_owner.removeSubscriber(m_request.chat_uuid(), m_subscriber);
        delete this;
    }

    asio::awaitable<void> stream() {
        if (m_request.chat_uuid().empty() || m_request.user_uuid().empty()) {
            Finish({StatusCode::INVALID_ARGUMENT, "Chat and user UUIDs are required"});
            co_return;
        }
        try {
            auto storage = m_owner.repo();
            if (!(co_await storage.isMember(m_request.chat_uuid(), m_request.user_uuid()))) {
                Finish({StatusCode::PERMISSION_DENIED, "User is not a chat member"});
                co_return;
            }
            m_owner.addSubscriber(m_request.chat_uuid(), m_subscriber);
            std::uint64_t lastId = m_request.after_message_id();
            while (!m_subscriber->cancelled) {
                const auto observed = m_subscriber->generation;
                auto pending = co_await storage.messagesAfter(m_request.chat_uuid(), lastId, 100);
                for (const auto &item : pending) {
                    if (m_subscriber->cancelled) break;
                    fillMessage(item, &m_current);
                    m_subscriber->writeDone = false;
                    m_subscriber->writeOk = false;
                    m_subscriber->writeWake.expires_at(asio::steady_timer::time_point::max());
                    StartWrite(&m_current);
                    while (!m_subscriber->writeDone) {
                        boost::system::error_code ignored;
                        co_await m_subscriber->writeWake.async_wait(
                            asio::redirect_error(asio::use_awaitable, ignored));
                    }
                    if (!m_subscriber->writeOk || m_subscriber->cancelled) {
                        Finish(Status::OK);
                        co_return;
                    }
                    lastId = item.id;
                }
                if (pending.size() == 100) continue;
                if (m_subscriber->generation != observed) continue;
                m_subscriber->wake.expires_after(std::chrono::seconds(1));
                boost::system::error_code ignored;
                co_await m_subscriber->wake.async_wait(
                    asio::redirect_error(asio::use_awaitable, ignored));
            }
            Finish(Status::OK);
        } catch (const std::exception &error) {
            Finish(internalError(error));
        }
    }
};

grpc::ServerWriteReactor<Message> *ChatHandler::Subscribe(
    grpc::CallbackServerContext *, const SubscribeRequest *request) {
    return new SubscribeReactor(*this, *request);
}

} // namespace api::v1
