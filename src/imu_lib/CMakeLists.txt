add_library(imu_lib imu_lib.cpp imu_lib.h)
target_include_directories(imu_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(imu_lib PUBLIC lib_base)
install(TARGETS imu_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES imu_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
