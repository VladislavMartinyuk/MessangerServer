#ifndef APIMANAGER_H
#define APIMANAGER_H

#include <grpc++/grpc++.h>

#include "api/v1/register/registeruserhandler.h"
#include "api/v1/auth/authuserhandler.h"

namespace api {

enum class ApiVersions { V1 = 1 };

class ApiManager {
public:
    ApiManager(const std::shared_ptr<asio::io_context> &ioc, const AppServices &services);

    void registerApiHandlers(grpc::ServerBuilder &builder, const ApiVersions &version);

private:
    // v1
    v1::RegisterUserHandler m_v1_registerUserHandler;
    v1::AuthUserHandler m_v1_authUserHandler;

    void registerV1Handlers(grpc::ServerBuilder &builder);
};

} // namespace api

#endif // APIMANAGER_H
