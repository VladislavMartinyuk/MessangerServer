#include "authuserhandler.h"

#include <spdlog/spdlog.h>

#include "repos/userrepo.h"

api::v1::AuthUserHandler::AuthUserHandler(const std::shared_ptr<asio::io_context> &ioc,
                                          const AppServices &appServices)
    : IApiHandler(ioc, appServices) {}

grpc::ServerUnaryReactor *
api::v1::AuthUserHandler::Authorizate(grpc::CallbackServerContext *context,
                                      const AuthRequest *request,
                                      AuthResponse *response) {
    class AuthorizateReactor : public grpc::ServerUnaryReactor {
    public:
        AuthorizateReactor(const std::shared_ptr<asio::io_context> &ioc,
                           const AppServices &services,
                           const AuthRequest *request,
                           AuthResponse *response)
            : services(services)
            , response(response) {
            asio::co_spawn(*ioc,
                           asyncAuthorizate(request->login(), request->password()),
                           asio::detached);
        }

    private:
        const AppServices &services;
        AuthResponse *response;

        void OnDone() override {
            spdlog::debug("rpc Authirizate done");
            delete this;
        }
        void OnCancel() override { spdlog::debug("rpc Authorize canceled"); }

        asio::awaitable<void> asyncAuthorizate(std::string login, std::string password) {
            spdlog::debug("rpc Authorizate start. Login: {}", login);

            try {
                UserRepo repo(services.dbService->dataBase("messenger"));

                auto containsUser = co_await repo.checkUserByLoginAndPass(login, password);
                if (containsUser) {
                    response->set_result(true);
                } else {
                    response->set_result(false);
                    response->set_erorr_string("Пользователь не найден");
                }

                Finish(grpc::Status::OK);
            } catch (const std::exception &e) {
                response->set_erorr_string(e.what());
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, e.what()));
            }
        }
    };

    return new AuthorizateReactor(m_ioc, m_appServices, request, response);
}
