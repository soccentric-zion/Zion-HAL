# can_bus_lib — PS CAN 2.0B (SocketCAN) validation
add_library(can_bus_lib can_bus_lib.cpp can_bus_lib.h)
target_include_directories(can_bus_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(can_bus_lib PUBLIC lib_base hal_platform)
install(TARGETS can_bus_lib ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES can_bus_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zion_hal)