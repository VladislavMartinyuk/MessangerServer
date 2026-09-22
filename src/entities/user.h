#ifndef USER_H
#define USER_H

#include <string>

class User {
public:
    int id{0};
    std::string uuid;
    std::string login;
    std::string password;
    std::string dateBirth;
    std::string name;
};

#endif // USER_H
