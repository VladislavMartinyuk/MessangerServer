#ifndef DBSERVICE_H
#define DBSERVICE_H

#include <unordered_map>
#include <boost/asio.hpp>
#include <shared_mutex>

#include "database.h"

namespace asio = boost::asio;

class DBService {
public:
    DBService(const std::shared_ptr<asio::io_context> &ioc);

    void addDB(std::string_view username,
               std::string_view pass,
               std::string_view host,
               std::string_view dbName,
               unsigned short port);
    std::shared_ptr<DataBase> dataBase(const std::string &dbName) const;

private:
    std::shared_ptr<asio::io_context> m_ioc;
    mutable std::shared_mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<DataBase>> m_dbs;
};

#endif // DBSERVICE_H
