#ifndef CHATREPO_H
#define CHATREPO_H

#include <vector>

#include "services/db/database.h"
#include "entities/chat.h"

class ChatRepo {
public:
    ChatRepo(const std::shared_ptr<DataBase> &db);

    asio::awaitable<std::vector<Chat>> getByUserUuid(std::string_view userUuid) const;

private:
    std::shared_ptr<DataBase> m_db;
};

#endif // CHATREPO_H
