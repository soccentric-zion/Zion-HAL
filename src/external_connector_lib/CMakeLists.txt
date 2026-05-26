add_library(external_connector_lib external_connector_lib.cpp external_connector_lib.h)
target_include_directories(external_connector_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(external_connector_lib PUBLIC lib_base)
install(TARGETS external_connector_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES external_connector_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
