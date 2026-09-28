#ifndef CHATHANDLER_H
#define CHATHANDLER_H

#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "base/appservices.h"
#include "repos/chatrepo.h"
#include "v1/chat/chat_service.grpc.pb.h"

namespace api::v1 {

class ChatHandler final : public ChatService::CallbackService {
public:
    ChatHandler(std::shared_ptr<asio::io_context> ioc, const AppServices &services);

    grpc::ServerUnaryReactor *CreatePersonalChat(
        grpc::CallbackServerContext *context, const CreatePersonalChatRequest *request,
        ChatIdResponse *response) override;
    grpc::ServerUnaryReactor *CreateGroupChat(
        grpc::CallbackServerContext *context, const CreateGroupChatRequest *request,
        ChatIdResponse *response) override;
    grpc::ServerUnaryReactor *AddGroupMember(
        grpc::CallbackServerContext *context, const AddGroupMemberRequest *request,
        AddGroupMemberResponse *response) override;
    grpc::ServerUnaryReactor *ListMembers(
        grpc::CallbackServerContext *context, const ListMembersRequest *request,
        ListMembersResponse *response) override;
    grpc::ServerUnaryReactor *SendMessage(
        grpc::CallbackServerContext *context, const SendMessageRequest *request,
        Message *response) override;
    grpc::ServerUnaryReactor *ListMessages(
        grpc::CallbackServerContext *context, const ListMessagesRequest *request,
        ListMessagesResponse *response) override;
    grpc::ServerWriteReactor<Message> *Subscribe(
        grpc::CallbackServerContext *context, const SubscribeRequest *request) override;

private:
    struct Subscriber {
        explicit Subscriber(asio::io_context &ioc)
            : strand(asio::make_strand(ioc)), wake(strand), writeWake(strand) {}

        asio::strand<asio::io_context::executor_type> strand;
        asio::steady_timer wake;
        asio::steady_timer writeWake;
        std::uint64_t generation{0};
        bool cancelled{false};
        bool writeDone{false};
        bool writeOk{false};
    };

    class SubscribeReactor;

    ChatRepo repo() const;
    void addSubscriber(const std::string &chatUuid,
                       const std::shared_ptr<Subscriber> &subscriber);
    void removeSubscriber(const std::string &chatUuid,
                          const std::shared_ptr<Subscriber> &subscriber);
    void publish(const std::string &chatUuid);
    static void fillMessage(const ChatMessage &source, Message *target);
    static grpc::Status internalError(const std::exception &error);

    std::shared_ptr<asio::io_context> m_ioc;
    const AppServices &m_services;
    std::mutex m_subscribersMutex;
    std::unordered_map<std::string, std::vector<std::weak_ptr<Subscriber>>> m_subscribers;
};

} // namespace api::v1

#endif // CHATHANDLER_H
