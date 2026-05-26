add_library(serializer_deserializer_lib serializer_deserializer_lib.cpp serializer_deserializer_lib.h)
target_include_directories(serializer_deserializer_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(serializer_deserializer_lib PUBLIC lib_base)
install(TARGETS serializer_deserializer_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES serializer_deserializer_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
