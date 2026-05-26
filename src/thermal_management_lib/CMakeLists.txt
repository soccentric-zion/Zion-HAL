add_library(thermal_management_lib thermal_management_lib.cpp thermal_management_lib.h)
target_include_directories(thermal_management_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(thermal_management_lib PUBLIC lib_base)
install(TARGETS thermal_management_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES thermal_management_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
