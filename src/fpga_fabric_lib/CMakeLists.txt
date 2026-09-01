# fpga_fabric_lib — FPGA (PL) fabric management and validation
add_library(fpga_fabric_lib fpga_fabric_lib.cpp fpga_fabric_lib.h)
target_include_directories(fpga_fabric_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fpga_fabric_lib PUBLIC lib_base hal_platform ps_pl_backend)
install(TARGETS fpga_fabric_lib LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES fpga_fabric_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zion_hal)