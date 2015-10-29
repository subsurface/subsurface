set(FACEBOOK_PLUGIN_SRCS facebook_integration.cpp)

add_library(facebook_integration SHARED ${FACEBOOK_PLUGIN_SRCS})

target_link_libraries(facebook_integration subsurface_corelib ${QT_LIBRARIES})
add_dependencies(facebook_integration subsurface_corelib)