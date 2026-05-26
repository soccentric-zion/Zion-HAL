add_library(storage_lib storage_lib.cpp storage_lib.h)
target_include_directories(storage_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(storage_lib PUBLIC lib_base)
install(TARGETS storage_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES storage_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
