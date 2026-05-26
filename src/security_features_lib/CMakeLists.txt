add_library(security_features_lib security_features_lib.cpp security_features_lib.h)
target_include_directories(security_features_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(security_features_lib PUBLIC lib_base)
install(TARGETS security_features_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES security_features_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
