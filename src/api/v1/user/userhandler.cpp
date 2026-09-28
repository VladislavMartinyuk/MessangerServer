#include "userhandler.h"

#include <spdlog/spdlog.h>

#include "repos/chatrepo.h"
#include "repos/userrepo.h"

api::v1::UserHandler::UserHandler(const std::shared_ptr<asio::io_context> &ioc,
                                  const AppServices &services)
    : IApiHandler(ioc, services) {}

grpc::ServerUnaryReactor *api::v1::UserHandler::UserChatList(grpc::CallbackServerContext *context,
                                                             const ChatListRequest *request,
                                                             ChatListResponse *response) {
    class ChatListReactor : public grpc::ServerUnaryReactor {
    public:
        ChatListReactor(const std::shared_ptr<asio::io_context> &ioc,
                        const AppServices &services,
                        const ChatListRequest *request,
                        ChatListResponse *response)
            : services(services)
            , response(response) {
            asio::co_spawn(*ioc, asyncUserChatList(request->user_uuid()), asio::detached);
        }

    private:
        const AppServices &services;
        ChatListResponse *response;

        void OnCancel() override { spdlog::debug("rpc UserChatList cancel"); }

        void OnDone() override {
            spdlog::debug("rpc UserChatList done");
            delete this;
        }

        asio::awaitable<void> asyncUserChatList(std::string userUuid) {
            spdlog::debug("rpc UserChatList started");

            try {
                ChatRepo repo(services.dbService->dataBase("messenger"));

                auto chats = co_await repo.getByUserUuid(userUuid);
                for (const auto &chat : chats) {
                    auto userChat = response->add_chats();
                    userChat->set_id(chat.id);
                    userChat->set_uuid(chat.uuid);
                    userChat->set_name(chat.name);
                    userChat->set_type_id(chat.typeId);
                    userChat->set_created_at(chat.createdAt);
                    userChat->set_updated_at(chat.updatedAt.value_or(""));
                    userChat->set_deleted_at(chat.deletedAt.value_or(""));
                    userChat->set_created_by(chat.createdBy);
                }

                response->set_result(true);
                Finish(grpc::Status::OK);
            } catch (const std::exception &e) {
                spdlog::warn("rpc UserChatList exception: {}", e.what());
                response->set_result(false);
                response->set_error_string(e.what());
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, e.what()));
            }
        }
    };

    return new ChatListReactor(m_ioc, m_appServices, request, response);
}

grpc::ServerUnaryReactor *api::v1::UserHandler::FindUser(
    grpc::CallbackServerContext *, const FindUserRequest *request,
    FindUserResponse *response) {
    class FindUserReactor final : public grpc::ServerUnaryReactor {
    public:
        FindUserReactor(const std::shared_ptr<asio::io_context> &ioc,
                        const AppServices &services, std::string login,
                        FindUserResponse *response)
            : services(services), response(response) {
            asio::co_spawn(*ioc, lookup(std::move(login)), asio::detached);
        }

    private:
        const AppServices &services;
        FindUserResponse *response;

        void OnDone() override { delete this; }

        asio::awaitable<void> lookup(std::string login) {
            if (login.empty()) {
                Finish(grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Login is required"));
                co_return;
            }
            try {
                UserRepo storage(services.dbService->dataBase("messenger"));
                auto user = co_await storage.findByLogin(login);
                if (!user) {
                    Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found"));
                } else {
                    response->set_user_uuid(user->uuid);
                    response->set_login(user->login);
                    response->set_name(user->name);
                    Finish(grpc::Status::OK);
                }
            } catch (const std::exception &error) {
                spdlog::warn("rpc FindUser exception: {}", error.what());
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Database error"));
            }
        }
    };

    return new FindUserReactor(m_ioc, m_appServices, request->login(), response);
}
