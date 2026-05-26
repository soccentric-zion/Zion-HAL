add_library(usb_lib usb_lib.cpp usb_lib.h)
target_include_directories(usb_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(usb_lib PUBLIC lib_base)
install(TARGETS usb_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES usb_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
