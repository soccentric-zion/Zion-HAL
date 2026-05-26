add_library(clock_and_timing_lib clock_and_timing_lib.cpp clock_and_timing_lib.h)
target_include_directories(clock_and_timing_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(clock_and_timing_lib PUBLIC lib_base)
install(TARGETS clock_and_timing_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES clock_and_timing_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
