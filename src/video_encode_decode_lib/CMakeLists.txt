add_library(video_encode_decode_lib video_encode_decode_lib.cpp video_encode_decode_lib.h)
target_include_directories(video_encode_decode_lib PUBLIC ${CMAKE_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(video_encode_decode_lib PUBLIC lib_base)
install(TARGETS video_encode_decode_lib
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES video_encode_decode_lib.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/acadia_hal)
