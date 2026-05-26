add_library(watchdog_and_reset_lib watchdog_and_reset_lib.cpp watchdog_and_reset_lib.h)
target_include_directories(watchdog_and_reset_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(watchdog_and_reset_lib PUBLIC lib_base)
install(TARGETS watchdog_and_reset_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES watchdog_and_reset_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
