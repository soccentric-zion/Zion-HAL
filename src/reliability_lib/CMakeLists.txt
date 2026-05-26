add_library(reliability_lib reliability_lib.cpp reliability_lib.h)
target_include_directories(reliability_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(reliability_lib PUBLIC lib_base)
install(TARGETS reliability_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES reliability_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
