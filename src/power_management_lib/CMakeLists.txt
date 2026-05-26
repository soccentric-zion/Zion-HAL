add_library(power_management_lib power_management_lib.cpp power_management_lib.h)
target_include_directories(power_management_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(power_management_lib PUBLIC lib_base)
install(TARGETS power_management_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES power_management_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
