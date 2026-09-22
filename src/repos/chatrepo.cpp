#include "chatrepo.h"
#include <boost/mysql/pfr.hpp>
#include <unordered_map>

ChatRepo::ChatRepo(const std::shared_ptr<DataBase> &db)
    : m_db(db) {}

asio::awaitable<std::vector<Chat>> ChatRepo::getByUserUuid(std::string_view userUuid) const {
    std::vector<Chat> result;

    auto connection = co_await m_db->getConnection();

    mysql::static_results<mysql::pfr_by_name<Chat>> chatsResult;
    std::unordered_map<std::string, Chat> uniqueChats;
    co_await connection->async_execute(
        mysql::with_params("SELECT * FROM chat WHERE created_by = {}", userUuid),
        chatsResult);

    for (const auto &chat : chatsResult.rows()) {
        if (!uniqueChats.contains(chat.uuid)) {
            uniqueChats.insert({chat.uuid, chat});
        }
    }

    mysql::static_results<mysql::pfr_by_name<Chat>> userChatsResult;
    co_await connection->async_execute(
        mysql::with_params("SELECT * FROM chat JOIN user_chats ON user_chats.chat_uuid = chat.uuid "
                           "WHERE created_by = {}",
                           userUuid),
        userChatsResult);

    for (const auto &chat : userChatsResult.rows()) {
        if (!uniqueChats.contains(chat.uuid)) {
            uniqueChats.insert({chat.uuid, chat});
        }
    }

    for (const auto &[uuid, chat] : uniqueChats) {
        result.push_back(chat);
    }

    co_return result;
}
