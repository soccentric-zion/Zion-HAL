add_library(gnss_lib gnss_lib.cpp gnss_lib.h)
target_include_directories(gnss_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gnss_lib PUBLIC lib_base)
install(TARGETS gnss_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES gnss_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
