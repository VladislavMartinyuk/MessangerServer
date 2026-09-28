#ifndef USERREPO_H
#define USERREPO_H

#include "entities/user.h"
#include "services/db/database.h"
#include <optional>

struct PublicUser {
    std::string uuid;
    std::string login;
    std::string name;
};

class UserRepo {
public:
    UserRepo(const std::shared_ptr<DataBase> &db);

    asio::awaitable<void> insertNewUser(const User &user) const;
    asio::awaitable<bool> containsUser(const User &user) const;
    asio::awaitable<std::optional<std::string>> checkUserByLoginAndPass(
        std::string_view login, std::string_view pass) const;
    asio::awaitable<std::optional<PublicUser>> findByLogin(std::string_view login) const;

private:
    std::shared_ptr<DataBase> m_db;
};

#endif // USERREPO_H
