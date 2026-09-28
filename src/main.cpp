#include "base/appcore.h"
#include <spdlog/spdlog.h>

int main() {
    spdlog::info("Initializing messenger server");
    AppCore core;

    auto dbService = core.appServices().dbService;
    dbService->addDB("user", "secret", "localhost", "messenger", 3306);

    spdlog::info("Starting gRPC server");
    core.start(api::ApiVersions::V1);

    return 0;
}
