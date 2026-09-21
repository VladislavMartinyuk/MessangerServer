#include "appcore.h"

#include <spdlog/spdlog.h>

AppCore::AppCore()
    : m_ioc(std::make_shared<asio::io_context>())
    , m_signals(*m_ioc, SIGINT, SIGTERM)
    , m_dbService(std::make_shared<DBService>(m_ioc))
    , m_apiManager(m_ioc, m_appServices) {
#ifdef SIGBREAK
    m_signals.add(SIGBREAK);
#endif
#ifdef SIGQUIT
    m_signals.add(SIGQUIT);
#endif
}

AppCore::~AppCore() {
    for (auto &thread : m_ioThreads) {
        thread.join();
    }
}

void AppCore::start(const api::ApiVersions &apiVersion) {
    const std::string grpcServerAddr = std::format("0.0.0.0:{}", m_grpcServerPort);

    grpc::ServerBuilder builder;
    m_apiManager.registerApiHandlers(builder, apiVersion);
    builder.AddListeningPort(grpcServerAddr, grpc::InsecureServerCredentials());
    m_grpcServer = builder.BuildAndStart();
    spdlog::info("gRPC Server starts at: {}", grpcServerAddr);

    if (!m_grpcServer) {
        spdlog::error("Failed to start gRPC server");
        return;
    }

    m_signals.async_wait([this](const boost::system::error_code &ec, int signal) {
        if (ec) {
            return;
        }
        m_grpcServer->Shutdown();
        m_ioc->stop();
    });

    const unsigned int threadCount = std::max(std::thread::hardware_concurrency(), 2u);

    m_ioThreads.reserve(threadCount);

    for (unsigned int i = 0; i < threadCount; ++i) {
        m_ioThreads.emplace_back([this] { m_ioc->run(); });
    }

    spdlog::info("io_context workers run");

    m_grpcServer->Wait();
}
