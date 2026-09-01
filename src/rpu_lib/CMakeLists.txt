# rpu_lib — RPU (Cortex-R5F) remoteproc/rpmsg validation
add_library(rpu_lib rpu_lib.cpp rpu_lib.h)
target_include_directories(rpu_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rpu_lib PUBLIC lib_base hal_platform)
install(TARGETS rpu_lib ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES rpu_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zion_hal)