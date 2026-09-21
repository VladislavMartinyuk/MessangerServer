#include "apimanager.h"

api::ApiManager::ApiManager(const std::shared_ptr<asio::io_context> &ioc,
                            const AppServices &services)
    : m_v1_registerUserHandler(ioc, services)
    , m_v1_authUserHandler(ioc, services) {}

void api::ApiManager::registerApiHandlers(grpc::ServerBuilder &builder,
                                          const ApiVersions &version) {
    switch (version) {
    case ApiVersions::V1: {
        registerV1Handlers(builder);
        break;
    }
    }
}

void api::ApiManager::registerV1Handlers(grpc::ServerBuilder &builder) {
    builder.RegisterService(&m_v1_registerUserHandler);
    builder.RegisterService(&m_v1_authUserHandler);
}
