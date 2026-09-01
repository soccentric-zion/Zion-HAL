# test apps for the FPGA/PL fabric module
add_executable(test_fpga_fabric main.cpp)
target_link_libraries(test_fpga_fabric PRIVATE fpga_fabric_lib)
target_include_directories(test_fpga_fabric PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_fpga_fabric COMMAND test_fpga_fabric)
set_tests_properties(test_fpga_fabric PROPERTIES TIMEOUT 120)
install(TARGETS test_fpga_fabric RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})