add_library(deep_learning_accelerator_lib deep_learning_accelerator_lib.cpp deep_learning_accelerator_lib.h)
target_include_directories(deep_learning_accelerator_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(deep_learning_accelerator_lib PUBLIC lib_base)
install(TARGETS deep_learning_accelerator_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES deep_learning_accelerator_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
