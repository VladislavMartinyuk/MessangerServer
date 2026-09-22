#ifndef CHAT_H
#define CHAT_H

#include <string>
#include <optional>

class Chat {
public:
    int id{0};
    std::string uuid;
    std::string name;
    int typeId;
    std::string createdAt;
    std::optional<std::string> updatedAt;
    std::optional<std::string> deletedAt;
    std::string createdBy;

    bool operator<(const Chat &other) { return id < other.id; }
};

#endif // CHAT_H
