#include "chatrepo.h"

#include <algorithm>
#include <exception>

#include "shared/shared.h"

namespace {

Chat toChat(const mysql::row_view &row) {
    Chat chat;
    chat.id = static_cast<int>(row[0].as_uint64());
    chat.uuid = std::string(row[1].as_string());
    chat.name = std::string(row[2].as_string());
    chat.typeId = static_cast<int>(row[3].as_uint64());
    chat.createdAt = std::string(row[4].as_string());
    if (!row[5].is_null()) chat.updatedAt = std::string(row[5].as_string());
    if (!row[6].is_null()) chat.deletedAt = std::string(row[6].as_string());
    chat.createdBy = std::string(row[7].as_string());
    return chat;
}

ChatMessage toMessage(const mysql::row_view &row) {
    ChatMessage message;
    message.id = row[0].as_uint64();
    message.uuid = std::string(row[1].as_string());
    message.chatUuid = std::string(row[2].as_string());
    message.senderUuid = std::string(row[3].as_string());
    message.text = std::string(row[4].as_string());
    message.createdAt = std::string(row[5].as_string());
    return message;
}

} // namespace

ChatRepo::ChatRepo(const std::shared_ptr<DataBase> &db) : m_db(db) {}

asio::awaitable<std::vector<Chat>> ChatRepo::getByUserUuid(std::string_view userUuid) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params(
            "SELECT c.id, c.uuid, c.name, c.type_id, "
            "DATE_FORMAT(c.created_at, '%Y-%m-%dT%H:%i:%s.%f'), "
            "DATE_FORMAT(c.updated_at, '%Y-%m-%dT%H:%i:%s.%f'), "
            "DATE_FORMAT(c.deleted_at, '%Y-%m-%dT%H:%i:%s.%f'), c.created_by "
            "FROM chat c JOIN user_chats uc ON uc.chat_uuid = c.uuid "
            "WHERE uc.user_uuid = {} AND c.deleted_at IS NULL ORDER BY c.id DESC",
            userUuid), result);
    std::vector<Chat> chats;
    for (const auto &row : result.rows()) chats.push_back(toChat(row));
    co_return chats;
}

asio::awaitable<bool> ChatRepo::userExists(std::string_view userUuid) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params("SELECT 1 FROM users WHERE uuid = {} AND deleted_at IS NULL LIMIT 1", userUuid),
        result);
    co_return !result.rows().empty();
}

asio::awaitable<bool> ChatRepo::isMember(std::string_view chatUuid,
                                         std::string_view userUuid) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params(
            "SELECT 1 FROM user_chats uc JOIN chat c ON c.uuid = uc.chat_uuid "
            "WHERE uc.chat_uuid = {} AND uc.user_uuid = {} AND c.deleted_at IS NULL LIMIT 1",
            chatUuid, userUuid), result);
    co_return !result.rows().empty();
}

asio::awaitable<std::vector<ChatMemberData>> ChatRepo::getMembers(
    std::string_view chatUuid) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params(
            "SELECT u.uuid, u.login, u.name FROM user_chats uc "
            "JOIN users u ON u.uuid = uc.user_uuid "
            "WHERE uc.chat_uuid = {} AND u.deleted_at IS NULL ORDER BY u.name, u.uuid",
            chatUuid), result);
    std::vector<ChatMemberData> members;
    for (const auto &row : result.rows()) {
        members.push_back({std::string(row[0].as_string()),
                           std::string(row[1].as_string()),
                           std::string(row[2].as_string())});
    }
    co_return members;
}

asio::awaitable<std::string> ChatRepo::createPersonal(std::string firstUuid,
                                                        std::string secondUuid) const {
    const std::string key = std::min(firstUuid, secondUuid) + ":" +
                            std::max(firstUuid, secondUuid);
    const std::string newUuid = generateUuid();
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute("START TRANSACTION", result);
    std::string chatUuid;
    std::exception_ptr error;
    try {
        co_await connection->async_execute(
            mysql::with_params(
                "INSERT INTO chat (uuid, name, type_id, created_by, personal_key) "
                "VALUES ({}, '', 1, {}, {}) ON DUPLICATE KEY UPDATE uuid = uuid",
                newUuid, firstUuid, key), result);
        co_await connection->async_execute(
            mysql::with_params("SELECT uuid FROM chat WHERE personal_key = {}", key), result);
        chatUuid = std::string(result.rows()[0][0].as_string());
        co_await connection->async_execute(
            mysql::with_params(
                "INSERT IGNORE INTO user_chats (user_uuid, chat_uuid) VALUES ({}, {})",
                firstUuid, chatUuid), result);
        co_await connection->async_execute(
            mysql::with_params(
                "INSERT IGNORE INTO user_chats (user_uuid, chat_uuid) VALUES ({}, {})",
                secondUuid, chatUuid), result);
    } catch (...) {
        error = std::current_exception();
    }
    if (error) {
        co_await connection->async_execute("ROLLBACK", result);
        std::rethrow_exception(error);
    }
    co_await connection->async_execute("COMMIT", result);
    co_return chatUuid;
}

asio::awaitable<std::string> ChatRepo::createGroup(std::string creatorUuid,
                                                     std::string name,
                                                     std::vector<std::string> members) const {
    const std::string chatUuid = generateUuid();
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute("START TRANSACTION", result);
    std::exception_ptr error;
    try {
        co_await connection->async_execute(
            mysql::with_params(
                "INSERT INTO chat (uuid, name, type_id, created_by) VALUES ({}, {}, 2, {})",
                chatUuid, name, creatorUuid), result);
        for (const auto &member : members) {
            co_await connection->async_execute(
                mysql::with_params(
                    "INSERT INTO user_chats (user_uuid, chat_uuid) VALUES ({}, {})",
                    member, chatUuid), result);
        }
    } catch (...) {
        error = std::current_exception();
    }
    if (error) {
        co_await connection->async_execute("ROLLBACK", result);
        std::rethrow_exception(error);
    }
    co_await connection->async_execute("COMMIT", result);
    co_return chatUuid;
}

asio::awaitable<bool> ChatRepo::addGroupMember(std::string chatUuid,
                                                 std::string creatorUuid,
                                                 std::string memberUuid) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params(
            "SELECT 1 FROM chat WHERE uuid = {} AND type_id = 2 AND created_by = {} "
            "AND deleted_at IS NULL LIMIT 1", chatUuid, creatorUuid), result);
    if (result.rows().empty()) co_return false;
    co_await connection->async_execute(
        mysql::with_params("INSERT IGNORE INTO user_chats (user_uuid, chat_uuid) VALUES ({}, {})",
                           memberUuid, chatUuid), result);
    co_return true;
}

asio::awaitable<ChatMessage> ChatRepo::sendMessage(std::string chatUuid,
                                                     std::string senderUuid,
                                                     std::string text) const {
    const std::string uuid = generateUuid();
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params(
            "INSERT INTO messages (uuid, chat_uuid, sender_uuid, message_text) "
            "VALUES ({}, {}, {}, {})", uuid, chatUuid, senderUuid, text), result);
    co_await connection->async_execute(
        mysql::with_params(
            "SELECT id, uuid, chat_uuid, sender_uuid, message_text, "
            "DATE_FORMAT(created_at, '%Y-%m-%dT%H:%i:%s.%f') "
            "FROM messages WHERE uuid = {}", uuid), result);
    co_return toMessage(result.rows()[0]);
}

asio::awaitable<std::vector<ChatMessage>> ChatRepo::listMessages(
    std::string chatUuid, std::uint64_t beforeId, unsigned int limit) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    if (beforeId == 0) {
        co_await connection->async_execute(
            mysql::with_params(
                "SELECT id, uuid, chat_uuid, sender_uuid, message_text, "
                "DATE_FORMAT(created_at, '%Y-%m-%dT%H:%i:%s.%f') "
                "FROM messages WHERE chat_uuid = {} AND deleted_at IS NULL "
                "ORDER BY id DESC LIMIT {}", chatUuid, limit), result);
    } else {
        co_await connection->async_execute(
            mysql::with_params(
                "SELECT id, uuid, chat_uuid, sender_uuid, message_text, "
                "DATE_FORMAT(created_at, '%Y-%m-%dT%H:%i:%s.%f') "
                "FROM messages WHERE chat_uuid = {} AND id < {} AND deleted_at IS NULL "
                "ORDER BY id DESC LIMIT {}", chatUuid, beforeId, limit), result);
    }
    std::vector<ChatMessage> messages;
    for (const auto &row : result.rows()) messages.push_back(toMessage(row));
    std::reverse(messages.begin(), messages.end());
    co_return messages;
}

asio::awaitable<std::vector<ChatMessage>> ChatRepo::messagesAfter(
    std::string chatUuid, std::uint64_t afterId, unsigned int limit) const {
    auto connection = co_await m_db->getConnection();
    mysql::results result;
    co_await connection->async_execute(
        mysql::with_params(
            "SELECT id, uuid, chat_uuid, sender_uuid, message_text, "
            "DATE_FORMAT(created_at, '%Y-%m-%dT%H:%i:%s.%f') "
            "FROM messages WHERE chat_uuid = {} AND id > {} AND deleted_at IS NULL "
            "ORDER BY id ASC LIMIT {}", chatUuid, afterId, limit), result);
    std::vector<ChatMessage> messages;
    for (const auto &row : result.rows()) messages.push_back(toMessage(row));
    co_return messages;
}
