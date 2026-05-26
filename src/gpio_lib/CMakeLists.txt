add_library(gpio_lib gpio_lib.cpp gpio_lib.h)
target_include_directories(gpio_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gpio_lib PUBLIC lib_base)
install(TARGETS gpio_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES gpio_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
