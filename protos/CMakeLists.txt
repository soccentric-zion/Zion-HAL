# protos/CMakeLists.txt
#
# Full Protobuf + gRPC build for the HAL proto layer.
# Both Protobuf and gRPC are REQUIRED — the build fails fast if either is absent.
#
# Produced targets
# ─────────────────────────────────────────────────────────────
#  hal_protos          STATIC  generated protobuf/gRPC C++
#  hal_protos_server   STATIC  gRPC service implementation (server-side)
#  hal_protos_client   STATIC  gRPC client wrapper
#  hal_server          EXEC    standalone HAL gRPC server
#
# Consumers
#   target_link_libraries(my_target PRIVATE hal_protos_client)  # client only
#   target_link_libraries(my_target PRIVATE hal_protos_server)  # server (pulls in all 35 HAL modules)
#
# Generated header include paths (propagated automatically):
#   #include "common.pb.h"
#   #include "hal_service.grpc.pb.h"
#   #include "modules/cpu.pb.h"

cmake_minimum_required(VERSION 3.10)

# ============================================================
# 1. Protobuf — REQUIRED
# ============================================================
find_package(Protobuf REQUIRED)
message(STATUS "[hal_protos] protobuf ${Protobuf_VERSION}  (${Protobuf_INCLUDE_DIRS})")
message(STATUS "[hal_protos] protoc   ${Protobuf_PROTOC_EXECUTABLE}")

# ============================================================
# 2. gRPC — REQUIRED (CMake config preferred; pkg-config fallback)
# ============================================================
find_package(gRPC CONFIG QUIET)
if(gRPC_FOUND)
    message(STATUS "[hal_protos] gRPC ${gRPC_VERSION} via CMake config")
    set(HAL_GRPC_LIBRARIES gRPC::grpc++ gRPC::grpc)
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GRPCPP REQUIRED grpc++)
    message(STATUS "[hal_protos] gRPC via pkg-config")
    set(HAL_GRPC_LIBRARIES ${GRPCPP_LIBRARIES})
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin
        HINTS
            /usr/local/bin
            /usr/bin
            /usr/lib/grpc
            /usr/lib/aarch64-linux-gnu
            /usr/lib/x86_64-linux-gnu
            ${GRPCPP_PREFIX}/bin
        DOC "Path to the grpc_cpp_plugin protoc plugin")
    if(NOT GRPC_CPP_PLUGIN)
        message(FATAL_ERROR
            "[hal_protos] grpc_cpp_plugin not found.\n"
            "Rebuild the Docker image (the Dockerfile now builds the plugin from source), "
            "or set -DGRPC_CPP_PLUGIN=/path/to/grpc_cpp_plugin on the cmake command line.")
    endif()
endif()

message(STATUS "[hal_protos] grpc_cpp_plugin: ${GRPC_CPP_PLUGIN}")

# ============================================================
# 3. Output directories for generated sources
# ============================================================
set(PROTO_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${PROTO_BINARY_DIR}")
file(MAKE_DIRECTORY "${PROTO_BINARY_DIR}/modules")

# ============================================================
# 4. Collect proto files
# ============================================================
set(COMMON_PROTO  "${CMAKE_CURRENT_SOURCE_DIR}/common.proto")
set(SERVICE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/hal_service.proto")
file(GLOB MODULE_PROTOS "${CMAKE_CURRENT_SOURCE_DIR}/modules/*.proto")
set(ALL_PROTOS ${COMMON_PROTO} ${SERVICE_PROTO} ${MODULE_PROTOS})

# ============================================================
# 5. Generate protobuf C++ sources for every proto file
# ============================================================
set(PROTO_SRCS "")
set(PROTO_HDRS "")

foreach(PROTO_FILE IN LISTS ALL_PROTOS)
    file(RELATIVE_PATH PROTO_REL "${CMAKE_CURRENT_SOURCE_DIR}" "${PROTO_FILE}")
    get_filename_component(PROTO_REL_DIR "${PROTO_REL}" DIRECTORY)
    get_filename_component(PROTO_NAME_WE "${PROTO_FILE}" NAME_WE)

    if(PROTO_REL_DIR)
        set(OUT_SUBDIR "${PROTO_BINARY_DIR}/${PROTO_REL_DIR}")
    else()
        set(OUT_SUBDIR "${PROTO_BINARY_DIR}")
    endif()
    file(MAKE_DIRECTORY "${OUT_SUBDIR}")

    set(PROTO_SRC "${OUT_SUBDIR}/${PROTO_NAME_WE}.pb.cc")
    set(PROTO_HDR "${OUT_SUBDIR}/${PROTO_NAME_WE}.pb.h")

    add_custom_command(
        OUTPUT  "${PROTO_SRC}" "${PROTO_HDR}"
        COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
                "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}"
                "--proto_path=${Protobuf_INCLUDE_DIRS}"
                "--cpp_out=${PROTO_BINARY_DIR}"
                "${PROTO_FILE}"
        DEPENDS "${PROTO_FILE}" "${COMMON_PROTO}"
        COMMENT "[protoc pb]   ${PROTO_REL}"
        VERBATIM
    )
    list(APPEND PROTO_SRCS "${PROTO_SRC}")
    list(APPEND PROTO_HDRS "${PROTO_HDR}")
endforeach()

# ============================================================
# 6. Generate gRPC stubs for hal_service.proto
# ============================================================
set(GRPC_SRC "${PROTO_BINARY_DIR}/hal_service.grpc.pb.cc")
set(GRPC_HDR "${PROTO_BINARY_DIR}/hal_service.grpc.pb.h")

add_custom_command(
    OUTPUT  "${GRPC_SRC}" "${GRPC_HDR}"
    COMMAND "${Protobuf_PROTOC_EXECUTABLE}"
            "--proto_path=${CMAKE_CURRENT_SOURCE_DIR}"
            "--proto_path=${Protobuf_INCLUDE_DIRS}"
            "--grpc_out=${PROTO_BINARY_DIR}"
            "--plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}"
            "${SERVICE_PROTO}"
    DEPENDS "${SERVICE_PROTO}" "${COMMON_PROTO}"
    COMMENT "[protoc grpc] hal_service.proto"
    VERBATIM
)

# ============================================================
# 7. hal_protos — generated protobuf + gRPC sources
# ============================================================
add_library(hal_protos STATIC
    ${PROTO_SRCS}
    ${GRPC_SRC}
)

# Suppress warnings in protoc-generated code.
target_compile_options(hal_protos PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:   -w>
    $<$<CXX_COMPILER_ID:Clang>: -w>
)

target_include_directories(hal_protos
    PUBLIC
        "${PROTO_BINARY_DIR}"       # #include "common.pb.h", "modules/cpu.pb.h"
        "${Protobuf_INCLUDE_DIRS}"  # google/protobuf/*.h
)

target_link_libraries(hal_protos
    PUBLIC
        protobuf::libprotobuf
        ${HAL_GRPC_LIBRARIES}
)

# ============================================================
# 8. hal_protos_server — gRPC service implementation
# ============================================================
add_library(hal_protos_server STATIC
    server/hal_service_impl.cpp
    server/module_registry_all.cpp
)

target_include_directories(hal_protos_server
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"  # #include "server/hal_service_impl.h"
        "${CMAKE_SOURCE_DIR}/src"      # #include "cpu_lib/cpu_lib.h"
)

target_link_libraries(hal_protos_server
    PUBLIC
        hal_protos
        acu_full_test   # all 35 HAL module implementations
)

# ============================================================
# 9. hal_protos_client — gRPC client wrapper
# ============================================================
add_library(hal_protos_client STATIC
    client/hal_client.cpp
)

target_include_directories(hal_protos_client
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"  # #include "client/hal_client.h"
)

target_link_libraries(hal_protos_client
    PUBLIC
        hal_protos
)

# ============================================================
# 10. hal_server — standalone server executable
# ============================================================
add_executable(hal_server
    server/hal_server_main.cpp
)

target_link_libraries(hal_server
    PRIVATE
        hal_protos_server
)

install(TARGETS hal_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ============================================================
# 11. Install rules for libraries and generated headers
# ============================================================
install(TARGETS hal_protos hal_protos_server hal_protos_client
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
    DIRECTORY   "${PROTO_BINARY_DIR}/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/hal_protos"
    FILES_MATCHING PATTERN "*.pb.h"
)
