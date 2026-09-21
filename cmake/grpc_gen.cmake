option(protobuf_MODULE_COMPAITABLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using Protobuf ${Protobuf_VERSION}")

if(CMAKE_CROSSCOMPILING)
    find_program(PROTOBUF_PROTOC protoc)
else()
    set(PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

if (CMAKE_CROSSCOMPILING)
    find_program(GRPC_CPP_PLUGIN_EXE grpc_cpp_plugin)
else()
    set(GRPC_CPP_PLUGIN_EXE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif ()

find_package(absl CONFIG REQUIRED)
message(STATUS "Using absl ${absl_VERSION}")

function(generate_proto_library TARGET_NAME PROTO_DIR)
    # Создаем базовую директорию для сгенерированных файлов
    set(PROTO_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto")
    file(MAKE_DIRECTORY ${PROTO_OUT_DIR})

    #  Ищем все .proto файлы в директории и ее поддиректориях
    file(GLOB_RECURSE PROTO_FILES "${PROTO_DIR}/*.proto")
    if (NOT PROTO_FILES)
        message(FATAL_ERROR "No .proto files found in directory: ${PROTO_DIR}")
    endif ()

    set(GENERATED_SRCS "")
    set(GENERATED_HDRS "")

    foreach (PROTO_FILE ${PROTO_FILES})
        # Вычисляем относительный путь от PROTO_DIR (например, "order/v1/order_service.proto")
        file(RELATIVE_PATH REL_PROTO_PATH "${PROTO_DIR}" "${PROTO_FILE}")

        # Получаем путь без расширения (например, "order/v1/order_service")
        get_filename_component(REL_DIR_AND_NAME ${REL_PROTO_PATH} LAST_EXT)
        string(REGEX REPLACE "\\.proto$" "" REL_BASE_PATH "${REL_PROTO_PATH}")

        set(PB_CPP      "${PROTO_OUT_DIR}/${REL_BASE_PATH}.pb.cc")
        set(PB_H        "${PROTO_OUT_DIR}/${REL_BASE_PATH}.pb.h")
        set(GRPC_PB_CPP "${PROTO_OUT_DIR}/${REL_BASE_PATH}.grpc.pb.cc")
        set(GRPC_PB_H   "${PROTO_OUT_DIR}/${REL_BASE_PATH}.grpc.pb.h")

        # Создаем целевую поддиректорию перед запуском protoc, чтобы избежать ошибок записи
        get_filename_component(REL_DIR ${REL_BASE_PATH} DIRECTORY)
        file(MAKE_DIRECTORY "${PROTO_OUT_DIR}/${REL_DIR}")

        list(APPEND GENERATED_SRCS ${PB_CPP} ${GRPC_PB_CPP})
        list(APPEND GENERATED_HDRS ${PB_H} ${GRPC_PB_H})

        add_custom_command(
                OUTPUT "${PB_CPP}" "${PB_H}" "${GRPC_PB_CPP}" "${GRPC_PB_H}"
                COMMAND ${PROTOBUF_PROTOC}
                ARGS --grpc_out ${PROTO_OUT_DIR}
                --cpp_out ${PROTO_OUT_DIR}
                -I ${PROTO_DIR}
                --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXE}
                ${PROTO_FILE}
                DEPENDS ${PROTO_FILE}
                COMMENT "Generating C++ and gRPC code from ${REL_PROTO_PATH}"
                VERBATIM
        )
    endforeach ()

    add_library(${TARGET_NAME} STATIC ${GENERATED_SRCS} ${GENERATED_HDRS})

    target_include_directories(${TARGET_NAME} PUBLIC ${PROTO_OUT_DIR} ${PROTO_DIR})
    target_link_libraries(${TARGET_NAME} PUBLIC
            gRPC::grpc++
            gRPC::grpc++_reflection
            gRPC::grpc
            protobuf::libprotobuf)

endfunction()