add_library(audio_interfaces audio_interfaces.cpp audio_interfaces.h)
target_include_directories(audio_interfaces PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(audio_interfaces PUBLIC lib_base)
install(TARGETS audio_interfaces
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES audio_interfaces.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
