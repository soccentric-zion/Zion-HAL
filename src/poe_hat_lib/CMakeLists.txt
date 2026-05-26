add_library(poe_hat_lib poe_hat_lib.cpp poe_hat_lib.h)
target_include_directories(poe_hat_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(poe_hat_lib PUBLIC lib_base)
install(TARGETS poe_hat_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES poe_hat_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
