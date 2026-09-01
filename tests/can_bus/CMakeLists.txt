# test apps for the CAN bus (SocketCAN) module
add_executable(test_can_bus main.cpp)
target_link_libraries(test_can_bus PRIVATE can_bus_lib)
target_include_directories(test_can_bus PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_can_bus COMMAND test_can_bus)
set_tests_properties(test_can_bus PROPERTIES TIMEOUT 120)
install(TARGETS test_can_bus RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})