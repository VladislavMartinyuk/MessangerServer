#ifndef REGISTERUSER_H
#define REGISTERUSER_H

#include <grpc++/grpc++.h>

#include "v1/register/register_service.grpc.pb.h"
#include "api/iapihandler.h"

namespace api::v1 {

class RegisterUserHandler : public RegisterService::CallbackService, public IApiHandler {
public:
    RegisterUserHandler(const std::shared_ptr<asio::io_context> &ioc,
                        const AppServices &appServices);

    grpc::ServerUnaryReactor *Registerate(grpc::CallbackServerContext *context,
                                          const RegisterRequest *request,
                                          RegisterResponse *response) override;
};

} // namespace api::v1

#endif