#include "database.h"

DataBase::DataBase(const std::shared_ptr<asio::io_context> &ioc)
    : m_ioc(ioc) {}

void DataBase::setupPoolParams(std::string_view username,
                               std::string_view pass,
                               std::string_view host,
                               std::string_view dbName,
                               unsigned short port) {
    if (!m_ioc) {
        return;
    }

    mysql::pool_params params;
    params.username = username;
    params.server_address.emplace_host_and_port(std::string{host}, port);
    params.password = pass;
    params.database = dbName;

    m_pool = std::make_unique<mysql::connection_pool>(*m_ioc, std::move(params));

    m_pool->async_run(asio::detached);
}

asio::awaitable<mysql::pooled_connection> DataBase::getConnection() {
    co_return co_await m_pool->async_get_connection(asio::cancel_after(std::chrono::seconds(1)));
}
