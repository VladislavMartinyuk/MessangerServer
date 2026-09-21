#ifndef APPCORE_H
#define APPCORE_H

#include <boost/asio.hpp>
#include <thread>
#include <vector>

#include "appservices.h"
#include "api/apimanager.h"

namespace asio = boost::asio;

class AppCore {
public:
    AppCore();
    ~AppCore();

    void start(const api::ApiVersions &apiVersion);
    const AppServices &appServices() { return m_appServices; }
    std::shared_ptr<asio::io_context> io_context() { return m_ioc; }
    void setGrpcServerPort(unsigned short port) { m_grpcServerPort = port; }

private:
    std::shared_ptr<asio::io_context> m_ioc{nullptr};
    asio::signal_set m_signals;
    std::vector<std::thread> m_ioThreads;

    // app services
    std::shared_ptr<DBService> m_dbService;

    AppServices m_appServices{m_dbService};

    // api
    unsigned short m_grpcServerPort{5001};
    std::unique_ptr<grpc::Server> m_grpcServer;
    api::ApiManager m_apiManager;
};

#endif