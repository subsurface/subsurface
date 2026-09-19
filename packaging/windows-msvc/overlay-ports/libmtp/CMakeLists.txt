cmake_minimum_required(VERSION 3.15)
project(libmtp CXX)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)

# Generate libmtp.h from the upstream template (fills in version strings etc.)
set(VERSION "1.1.22")
configure_file(src/libmtp.h.in "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" @ONLY)
if(MSVC)
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" LIBMTP_HEADER)
    foreach(_type int8_t uint8_t int16_t uint16_t int32_t uint32_t uint64_t)
        string(REGEX REPLACE "typedef[^;]*[ *]${_type};[\r\n]*" "" LIBMTP_HEADER "${LIBMTP_HEADER}")
    endforeach()

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" "${LIBMTP_HEADER}")
endif()

add_library(libmtp STATIC src/msvc_wpd.cpp)

target_include_directories(libmtp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_compile_features(libmtp PRIVATE cxx_std_17)
target_link_libraries(libmtp PRIVATE
    ole32
    portabledeviceguids
    propsys
)

install(TARGETS libmtp ARCHIVE DESTINATION lib)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libmtp.h" DESTINATION include)
