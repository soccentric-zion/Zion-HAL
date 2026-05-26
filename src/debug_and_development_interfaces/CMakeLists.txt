add_library(debug_and_development_interfaces debug_and_development_interfaces.cpp debug_and_development_interfaces.h)
target_include_directories(debug_and_development_interfaces PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(debug_and_development_interfaces PUBLIC lib_base)
install(TARGETS debug_and_development_interfaces
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES debug_and_development_interfaces.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
