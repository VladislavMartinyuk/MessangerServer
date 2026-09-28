#ifndef CHAT_H
#define CHAT_H

#include <string>
#include <optional>
#include <cstdint>

class Chat {
public:
    int id{0};
    std::string uuid;
    std::string name;
    int typeId{0};
    std::string createdAt;
    std::optional<std::string> updatedAt;
    std::optional<std::string> deletedAt;
    std::string createdBy;

};

struct ChatMessage {
    std::uint64_t id{0};
    std::string uuid;
    std::string chatUuid;
    std::string senderUuid;
    std::string text;
    std::string createdAt;
};

struct ChatMemberData {
    std::string uuid;
    std::string login;
    std::string name;
};

#endif // CHAT_H
