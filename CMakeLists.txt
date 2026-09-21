cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("MessengerServer")

include(cmake/grpc_gen.cmake)

generate_proto_library(proto_lib ${CMAKE_CURRENT_SOURCE_DIR}/proto)

find_package(Boost CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable (MessengerServer
    src/main.cpp
    src/api/v1/auth/authuserhandler.h src/api/v1/auth/authuserhandler.cpp
    src/api/v1/register/registeruserhandler.h src/api/v1/register/registeruserhandler.cpp
    src/base/appcore.h src/base/appcore.cpp
    src/services/db/database.h src/services/db/database.cpp
    src/services/db/dbservice.h src/services/db/dbservice.cpp
    src/base/appservices.h
    src/api/iapihandler.h
    src/entities/user.h
    src/repos/userrepo.h src/repos/userrepo.cpp
    src/shared/shared.h
    src/api/apimanager.h src/api/apimanager.cpp
)

target_link_libraries(MessengerServer PRIVATE
    proto_lib
    boost::boost
    Boost::charconv
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog)

target_include_directories(MessengerServer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)