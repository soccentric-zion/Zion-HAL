add_library(communication_interface_lib communication_interface_lib.cpp communication_interface_lib.h)
target_include_directories(communication_interface_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(communication_interface_lib PUBLIC lib_base)
install(TARGETS communication_interface_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES communication_interface_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
