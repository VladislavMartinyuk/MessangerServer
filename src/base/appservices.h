#ifndef APPSERVICES_H
#define APPSERVICES_H

#include <memory>

#include "services/db/dbservice.h"

struct AppServices {
    std::shared_ptr<DBService> dbService;
};

#endif // APPSERVICES_H
