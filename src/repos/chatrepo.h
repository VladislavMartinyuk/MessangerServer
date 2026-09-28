#ifndef CHATREPO_H
#define CHATREPO_H

#include <vector>
#include <cstdint>

#include "services/db/database.h"
#include "entities/chat.h"

class ChatRepo {
public:
    ChatRepo(const std::shared_ptr<DataBase> &db);

    asio::awaitable<std::vector<Chat>> getByUserUuid(std::string_view userUuid) const;
    asio::awaitable<bool> userExists(std::string_view userUuid) const;
    asio::awaitable<bool> isMember(std::string_view chatUuid, std::string_view userUuid) const;
    asio::awaitable<std::vector<ChatMemberData>> getMembers(std::string_view chatUuid) const;
    asio::awaitable<std::string> createPersonal(std::string firstUuid,
                                                 std::string secondUuid) const;
    asio::awaitable<std::string> createGroup(std::string creatorUuid,
                                              std::string name,
                                              std::vector<std::string> members) const;
    asio::awaitable<bool> addGroupMember(std::string chatUuid,
                                          std::string creatorUuid,
                                          std::string memberUuid) const;
    asio::awaitable<ChatMessage> sendMessage(std::string chatUuid,
                                               std::string senderUuid,
                                               std::string text) const;
    asio::awaitable<std::vector<ChatMessage>> listMessages(std::string chatUuid,
                                                             std::uint64_t beforeId,
                                                             unsigned int limit) const;
    asio::awaitable<std::vector<ChatMessage>> messagesAfter(std::string chatUuid,
                                                               std::uint64_t afterId,
                                                               unsigned int limit) const;

private:
    std::shared_ptr<DataBase> m_db;
};

#endif // CHATREPO_H
