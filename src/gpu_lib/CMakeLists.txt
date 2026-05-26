add_library(gpu_lib gpu_lib.cpp gpu_lib.h)
target_include_directories(gpu_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gpu_lib PUBLIC lib_base)
install(TARGETS gpu_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES gpu_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
