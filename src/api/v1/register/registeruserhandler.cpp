#include "registeruserhandler.h"

#include <spdlog/spdlog.h>
#include <repos/userrepo.h>

#include "shared/shared.h"

api::v1::RegisterUserHandler::RegisterUserHandler(const std::shared_ptr<asio::io_context> &ioc,
                                                  const AppServices &appServices)
    : IApiHandler(ioc, appServices) {}

grpc::ServerUnaryReactor *
api::v1::RegisterUserHandler::Registerate(grpc::CallbackServerContext *context,
                                          const RegisterRequest *request,
                                          RegisterResponse *response) {
    class Reactor : public grpc::ServerUnaryReactor {
    public:
        Reactor(const std::shared_ptr<asio::io_context> &ioc,
                const AppServices &services,
                const RegisterRequest *request,
                RegisterResponse *response)
            : services(services)
            , response(response) {
            asio::co_spawn(*ioc,
                           asyncRegisterUser(request->login(),
                                             request->password(),
                                             request->username(),
                                             request->datebirth()),
                           asio::detached);
        }

    private:
        const AppServices &services;
        RegisterResponse *response;

        void OnDone() override {
            spdlog::debug("rpc Registrate finish");
            delete this;
        }

        void OnCancel() override { spdlog::info("rpc Registrate canceled"); }

        asio::awaitable<void> asyncRegisterUser(std::string login,
                                                std::string password,
                                                std::string userName,
                                                std::string dateBirth) {
            spdlog::debug("rpc Registrate start. Login: {}, UserName: {}, DateBirth: {}",
                          login,
                          userName,
                          dateBirth);

            try {
                UserRepo repo(services.dbService->dataBase("messenger"));

                User newUser;
                newUser.uuid = generateUuid();
                newUser.login = login;
                newUser.name = userName;
                newUser.password = password;
                newUser.dateBirth = dateBirth;
                if (co_await repo.containsUser(newUser)) {
                    response->set_result(false);
                    response->set_errorstring("Такой пользователь уже зарегистрирован в системе");
                } else {
                    co_await repo.insertNewUser(newUser);
                    response->set_result(true);
                }
                Finish(grpc::Status::OK);
            } catch (const std::exception &e) {
                spdlog::warn("Catch exception at rpc Registrate: {}", e.what());
                response->set_result(false);
                response->set_errorstring(e.what());
                Finish(grpc::Status(grpc::StatusCode::INTERNAL, "Database error"));
            }
        }
    };

    return new Reactor(m_ioc, m_appServices, request, response);
}
