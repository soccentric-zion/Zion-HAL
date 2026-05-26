add_library(environmental_lib environmental_lib.cpp environmental_lib.h)
target_include_directories(environmental_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(environmental_lib PUBLIC lib_base)
install(TARGETS environmental_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES environmental_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
