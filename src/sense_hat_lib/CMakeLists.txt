add_library(sense_hat_lib sense_hat_lib.cpp sense_hat_lib.h)
target_include_directories(sense_hat_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sense_hat_lib PUBLIC lib_base)
install(TARGETS sense_hat_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES sense_hat_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
