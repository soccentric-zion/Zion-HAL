add_library(display_interfaces display_interfaces.cpp display_interfaces.h)
target_include_directories(display_interfaces PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(display_interfaces PUBLIC lib_base)
install(TARGETS display_interfaces
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES display_interfaces.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
