#include "userrepo.h"

#include <spdlog/spdlog.h>

UserRepo::UserRepo(const std::shared_ptr<DataBase> &db)
    : m_db(db) {}

asio::awaitable<bool> UserRepo::containsUser(const User &user) const {
    auto connection = co_await m_db->getConnection();

    mysql::results result;

    co_await connection->async_execute(
        mysql::with_params("SELECT 1 FROM users WHERE login = {} LIMIT 1", user.login),
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

asio::awaitable<std::optional<std::string>> UserRepo::checkUserByLoginAndPass(
    std::string_view login, std::string_view pass) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params("SELECT uuid FROM users WHERE login = {} AND password_hash = {} "
                           "AND deleted_at IS NULL LIMIT 1",
                           login,
                           pass),
        result);

    if (result.rows().empty()) co_return std::nullopt;
    co_return std::string(result.rows()[0][0].as_string());
}

asio::awaitable<std::optional<PublicUser>> UserRepo::findByLogin(
    std::string_view login) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params(
            "SELECT uuid, login, name FROM users WHERE login = {} "
            "AND deleted_at IS NULL LIMIT 1", login), result);
    if (result.rows().empty()) co_return std::nullopt;
    const auto row = result.rows()[0];
    co_return PublicUser{std::string(row[0].as_string()),
                         std::string(row[1].as_string()),
                         std::string(row[2].as_string())};
}
