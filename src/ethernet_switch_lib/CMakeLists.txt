add_library(ethernet_switch_lib ethernet_switch_lib.cpp ethernet_switch_lib.h)
target_include_directories(ethernet_switch_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ethernet_switch_lib PUBLIC lib_base)
install(TARGETS ethernet_switch_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ethernet_switch_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
