add_library(camera_interfaces_lib camera_interfaces_lib.cpp camera_interfaces_lib.h)
target_include_directories(camera_interfaces_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(camera_interfaces_lib PUBLIC lib_base)
install(TARGETS camera_interfaces_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES camera_interfaces_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
