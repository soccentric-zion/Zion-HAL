cmake_minimum_required(VERSION 3.10)
project(ACUFullTest VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threading library
find_package(Threads REQUIRED)
include(GNUInstallDirs)

# Enable testing
enable_testing()

# Compiler warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(MAINFOLDER ${PROJECT_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH "${MAINFOLDER}/bin")
set(LIBRARY_OUTPUT_PATH "${MAINFOLDER}/lib")

# Collect all source files for formatting and analysis
file(
    GLOB_RECURSE
    ALL_SOURCES
    CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.h"
    "tests/*.cpp"
    "tests/*.h"
)

# Clang-format
find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    # cmake-format: off
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCES}
        COMMENT "Formatting code with clang-format"
    )
    # cmake-format: on
endif()

# Cppcheck
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    # cmake-format: off
    add_custom_target(cppcheck
        COMMAND ${CPPCHECK}
            --enable=all
            --std=c++17
            --language=c++
            --platform=unix64
            --suppress=missingIncludeSystem
            --suppress=missingInclude
            --inline-suppr
            --quiet
            --error-exitcode=1
            ${ALL_SOURCES}
        COMMENT "Running cppcheck static analysis"
    )
    # cmake-format: on
endif()

# Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # cmake-format: off
    add_custom_target(doc_doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    # cmake-format: on
else()
    message("Doxygen need to be installed to generate the doxygen documentation")
endif()

# Add proto layer (Protobuf + gRPC generated sources and hal_protos library)
add_subdirectory(protos)

# Add src directory
add_subdirectory(src)

# Collect all source files from src for combined libraries
file(GLOB_RECURSE SRC_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Create static library
add_library(acu_full_test STATIC ${SRC_SOURCES})
target_include_directories(acu_full_test PUBLIC src)
target_link_libraries(acu_full_test Threads::Threads)

# Create shared library
add_library(acu_full_test_shared SHARED ${SRC_SOURCES})
target_include_directories(acu_full_test_shared PUBLIC src)
target_link_libraries(acu_full_test_shared Threads::Threads)
set_target_properties(acu_full_test_shared PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Install rules for aggregate libraries and headers
install(TARGETS acu_full_test
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(TARGETS acu_full_test_shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal
    FILES_MATCHING PATTERN "*.h"
)

# Add tests directory
add_subdirectory(tests)

# ============================================================
# CPack — DEB / IPK / RPM packaging
# ============================================================

# --- Common metadata ---
set(CPACK_PACKAGE_NAME             "acadia-hal")
set(CPACK_PACKAGE_VENDOR           "Acadia")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Hardware Abstraction Layer for Raspberry Pi 5")
set(CPACK_PACKAGE_VERSION          "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR    "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR    "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH    "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT          "maintainer@acadia.example.com")
set(CPACK_STRIP_FILES              ON)

# Map CMake/system processor to package-manager architecture names
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
    set(HAL_DEB_ARCH "arm64")
    set(HAL_RPM_ARCH "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
    set(HAL_DEB_ARCH "amd64")
    set(HAL_RPM_ARCH "x86_64")
else()
    set(HAL_DEB_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
    set(HAL_RPM_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# ---------------------------------------------------------------
# DEB
# ---------------------------------------------------------------
set(CPACK_DEBIAN_PACKAGE_MAINTAINER  "Acadia <maintainer@acadia.example.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION     "embedded")
set(CPACK_DEBIAN_PACKAGE_PRIORITY    "optional")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${HAL_DEB_ARCH}")
set(CPACK_DEBIAN_PACKAGE_DEPENDS     "libstdc++6 (>= 10), libgcc-s1")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION [[Acadia HAL provides a C++17 hardware abstraction layer for the
Raspberry Pi 5 (BCM2712 / RP1) exposing 35 hardware modules via a
canonical API and a gRPC/Protobuf service interface.]])
set(CPACK_DEBIAN_COMPRESSION_TYPE    "gzip")
set(CPACK_DEBIAN_FILE_NAME           "DEB-DEFAULT")

# ---------------------------------------------------------------
# IPK  (Yocto / OpenWrt — structurally identical to DEB)
# Build with:  cmake --build <dir> --target package_ipk
# ---------------------------------------------------------------
add_custom_target(package_ipk
    COMMAND "${CMAKE_CPACK_COMMAND}" -G DEB
            --config "${CMAKE_BINARY_DIR}/CPackConfig.cmake"
    COMMAND "${CMAKE_COMMAND}"
            "-DDIR=${CMAKE_BINARY_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rename_deb_to_ipk.cmake"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Generating IPK package (DEB repackaged as .ipk)"
    VERBATIM
)

# ---------------------------------------------------------------
# RPM
# ---------------------------------------------------------------
set(CPACK_RPM_PACKAGE_LICENSE       "Proprietary")
set(CPACK_RPM_PACKAGE_GROUP         "System/Libraries")
set(CPACK_RPM_PACKAGE_ARCHITECTURE  "${HAL_RPM_ARCH}")
set(CPACK_RPM_PACKAGE_DESCRIPTION   "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
set(CPACK_RPM_PACKAGE_REQUIRES      "libstdc++ >= 10")
set(CPACK_RPM_PACKAGE_AUTOREQPROV   NO)
set(CPACK_RPM_COMPRESSION_TYPE      "gzip")
set(CPACK_RPM_FILE_NAME             "RPM-DEFAULT")

# ---------------------------------------------------------------
# Generators
# Run `cpack` to produce both DEB and RPM, or select one:
#   cpack -G DEB
#   cpack -G RPM
#   cmake --build <dir> --target package_ipk
# ---------------------------------------------------------------
set(CPACK_GENERATOR "DEB;RPM")
include(CPack)
