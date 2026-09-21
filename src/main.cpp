#include "base/appcore.h"

int main() {
    AppCore core;

    auto dbService = core.appServices().dbService;
    dbService->addDB("user", "secret", "localhost", "messenger", 3306);

    core.start(api::ApiVersions::V1);

    return 0;
}