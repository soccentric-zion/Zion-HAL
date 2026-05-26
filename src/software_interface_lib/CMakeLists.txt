add_library(software_interface_lib software_interface_lib.cpp software_interface_lib.h)
target_include_directories(software_interface_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(software_interface_lib PUBLIC lib_base)
install(TARGETS software_interface_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES software_interface_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
