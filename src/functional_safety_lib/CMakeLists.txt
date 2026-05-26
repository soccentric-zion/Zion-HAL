add_library(functional_safety_lib functional_safety_lib.cpp functional_safety_lib.h)
target_include_directories(functional_safety_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(functional_safety_lib PUBLIC lib_base)
install(TARGETS functional_safety_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES functional_safety_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
