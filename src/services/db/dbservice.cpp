#include "dbservice.h"

DBService::DBService(const std::shared_ptr<asio::io_context> &ioc)
    : m_ioc(ioc) {}

void DBService::addDB(std::string_view username,
                      std::string_view pass,
                      std::string_view host,
                      std::string_view dbName,
                      unsigned short port) {
    auto db = std::make_shared<DataBase>(m_ioc);
    db->setupPoolParams(username, pass, host, dbName, port);
    std::lock_guard lock(m_mutex);
    m_dbs.insert({std::string{dbName}, std::move(db)});
}

std::shared_ptr<DataBase> DBService::dataBase(const std::string &dbName) const {
    std::shared_lock lock(m_mutex);

    auto it = m_dbs.find(std::string{dbName});

    if (it == m_dbs.end()) {
        throw std::runtime_error(std::format("Database '{}' is not registered", dbName));
    }

    return it->second;
}