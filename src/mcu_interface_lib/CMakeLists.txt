add_library(mcu_interface_lib mcu_interface_lib.cpp mcu_interface_lib.h)
target_include_directories(mcu_interface_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mcu_interface_lib PUBLIC lib_base)
install(TARGETS mcu_interface_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES mcu_interface_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
