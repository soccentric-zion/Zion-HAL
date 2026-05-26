# cmake/rename_deb_to_ipk.cmake
#
# Copies every *.deb in DIR to a sibling *.ipk file.
# IPK packages (Yocto / OpenWrt) are structurally identical to .deb;
# only the file extension differs.
#
# Usage (invoked automatically by the package_ipk target):
#   cmake -DDIR=<cmake-binary-dir> -P rename_deb_to_ipk.cmake

if(NOT DEFINED DIR)
    message(FATAL_ERROR "DIR must be defined: cmake -DDIR=<path> -P rename_deb_to_ipk.cmake")
endif()

file(GLOB DEB_FILES "${DIR}/*.deb")

if(NOT DEB_FILES)
    message(WARNING "No .deb files found in ${DIR} — nothing to rename to .ipk")
    return()
endif()

foreach(DEB_FILE IN LISTS DEB_FILES)
    string(REPLACE ".deb" ".ipk" IPK_FILE "${DEB_FILE}")
    configure_file("${DEB_FILE}" "${IPK_FILE}" COPYONLY)
    message(STATUS "IPK created: ${IPK_FILE}")
endforeach()
