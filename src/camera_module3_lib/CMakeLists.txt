add_library(camera_module3_lib camera_module3_lib.cpp camera_module3_lib.h)
target_include_directories(camera_module3_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(camera_module3_lib PUBLIC lib_base)
install(TARGETS camera_module3_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES camera_module3_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
