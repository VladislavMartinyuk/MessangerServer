#ifndef USERHANDLER_H
#define USERHANDLER_H

#include "v1/user/user_service.grpc.pb.h"
#include "api/iapihandler.h"

namespace api::v1 {

class UserHandler : public UserService::CallbackService, public IApiHandler {
public:
    UserHandler(const std::shared_ptr<asio::io_context> &ioc, const AppServices &services);

    grpc::ServerUnaryReactor *UserChatList(grpc::CallbackServerContext *context,
                                           const ChatListRequest *request,
                                           ChatListResponse *response) override;
};

} // namespace api::v1

#endif // USERHANDLER_H
