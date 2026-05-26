add_library(pcie_lib pcie_lib.cpp pcie_lib.h)
target_include_directories(pcie_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pcie_lib PUBLIC lib_base)
install(TARGETS pcie_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES pcie_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
