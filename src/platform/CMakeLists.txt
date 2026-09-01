# hal_platform — runtime board detection shared across all HAL modules
add_library(hal_platform board_detect.cpp board_detect.h platform_kv260.h)
target_include_directories(hal_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
if(HAL_PLATFORM_UPPER)
    target_compile_definitions(hal_platform PUBLIC HAL_PLATFORM_${HAL_PLATFORM_UPPER})
endif()
install(TARGETS hal_platform ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES board_detect.h platform_kv260.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zion_hal)