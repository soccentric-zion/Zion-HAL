# test apps for the RPU (Cortex-R5F) module
add_executable(test_rpu main.cpp)
target_link_libraries(test_rpu PRIVATE rpu_lib)
target_include_directories(test_rpu PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_rpu COMMAND test_rpu)
set_tests_properties(test_rpu PROPERTIES TIMEOUT 60)
install(TARGETS test_rpu RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})