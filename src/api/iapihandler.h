#ifndef IAPIHANDLER_H
#define IAPIHANDLER_H

#include "base/appservices.h"

namespace api {

class IApiHandler {
public:
    IApiHandler(const std::shared_ptr<asio::io_context> &ioc, const AppServices &services)
        : m_ioc(ioc)
        , m_appServices(services) {}

protected:
    std::shared_ptr<asio::io_context> m_ioc;
    const AppServices &m_appServices;
};

} // namespace api

#endif // IAPIHANDLER_H
