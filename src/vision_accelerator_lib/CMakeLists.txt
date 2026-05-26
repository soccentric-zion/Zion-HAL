add_library(vision_accelerator_lib vision_accelerator_lib.cpp vision_accelerator_lib.h)
target_include_directories(vision_accelerator_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(vision_accelerator_lib PUBLIC lib_base)
install(TARGETS vision_accelerator_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES vision_accelerator_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
