#ifndef AUTHUSER_H
#define AUTHUSER_H

#include <grpc++/grpc++.h>

#include "v1/auth/auth_service.grpc.pb.h"
#include "api/iapihandler.h"

namespace api::v1 {

class AuthUserHandler : public AuthService::CallbackService, public IApiHandler {
public:
    AuthUserHandler(const std::shared_ptr<asio::io_context> &ioc, const AppServices &appServices);

    grpc::ServerUnaryReactor *Authorizate(::grpc::CallbackServerContext *context,
                                          const ::AuthRequest *request,
                                          ::AuthResponse *response) override;
};

} // namespace api::v1

#endif