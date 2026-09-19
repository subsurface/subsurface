vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO libmtp/libmtp
    REF "v1.1.22"
    SHA512 8518d1c201b24aefbb81ca4cffe8a9ab01c3b120f2dece954983ed57c2934b71fcf95cefa3591276812088c3040781e438d8cc6af35e75a66615f63e9632173f
)

# Replace upstream build system and source with an MSVC-compatible libmtp ABI
# shim backed by Windows Portable Devices. Upstream libmtp's Windows support is
# MinGW/libusb-oriented; Subsurface only needs a narrow read-only subset.
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/msvc_wpd.cpp" DESTINATION "${SOURCE_PATH}/src")

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/COPYING"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
     RENAME copyright)
