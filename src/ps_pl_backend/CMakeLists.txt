add_library(ps_pl_backend ps_pl_backend.cpp ps_pl_backend.h)
target_include_directories(ps_pl_backend PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ps_pl_backend PUBLIC lib_base)
install(TARGETS ps_pl_backend
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ps_pl_backend.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zion_hal)
