add_library(ethernet_lib ethernet_lib.cpp ethernet_lib.h)
target_include_directories(ethernet_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ethernet_lib PUBLIC lib_base)
install(TARGETS ethernet_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ethernet_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
