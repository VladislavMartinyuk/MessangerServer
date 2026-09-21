#include "userrepo.h"

#include <spdlog/spdlog.h>

UserRepo::UserRepo(const std::shared_ptr<DataBase> &db)
    : m_db(db) {}

asio::awaitable<bool> UserRepo::containsUser(const User &user) const {
    auto connection = co_await m_db->getConnection();

    mysql::results result;

    co_await connection->async_execute(
        mysql::with_params("SELECT 1 FROM users WHERE login = {} AND name = {} LIMIT 1",
                           user.login,
                           user.name),
        result);

    co_return !result.rows().empty();
}

asio::awaitable<void> UserRepo::insertNewUser(const User &user) const {
    auto connection = co_await m_db->getConnection();
    mysql::results resut;
    co_await connection->async_execute(
        mysql::with_params(
            "INSERT INTO users (uuid, login, password_hash, name, date_birth) VALUES "
            "({}, {}, {}, {}, {})",
            user.uuid,
            user.login,
            user.password,
            user.name,
            user.dateBirth),
        resut);
}

asio::awaitable<bool> UserRepo::checkUserByLoginAndPass(std::string_view login,
                                                        std::string_view pass) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params("SELECT 1 FROM users WHERE login = {} AND password_hash = {};",
                           login,
                           pass),
        result);

    co_return !result.rows().empty();
}