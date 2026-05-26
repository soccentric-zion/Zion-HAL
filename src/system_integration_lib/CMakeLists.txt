add_library(system_integration_lib system_integration_lib.cpp system_integration_lib.h)
target_include_directories(system_integration_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(system_integration_lib PUBLIC lib_base)
install(TARGETS system_integration_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES system_integration_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
