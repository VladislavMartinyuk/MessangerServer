#ifndef DATABASE_H
#define DATABASE_H

#include <boost/mysql.hpp>
#include <boost/asio.hpp>
#include <memory>

namespace mysql = boost::mysql;
namespace asio = boost::asio;

class DataBase {
public:
    DataBase(const std::shared_ptr<asio::io_context> &ioc);

    void setupPoolParams(std::string_view username,
                         std::string_view pass,
                         std::string_view hoost,
                         std::string_view dbName,
                         unsigned short port);
    asio::awaitable<mysql::pooled_connection> getConnection();

private:
    std::shared_ptr<asio::io_context> m_ioc{nullptr};
    std::unique_ptr<mysql::connection_pool> m_pool;
};

#endif
