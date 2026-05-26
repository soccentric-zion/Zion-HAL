add_library(cpu_lib cpu_lib.cpp cpu_lib.h)
target_include_directories(cpu_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cpu_lib PUBLIC Threads::Threads lib_base)
install(TARGETS cpu_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES cpu_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
