set(FACEBOOK_PLUGIN_SRCS facebook_integration.cpp)

add_library(facebook_integration ${FACEBOOK_PLUGIN_SRCS})

target_link_libraries(facebook_integration subsurface_core ${QT_LIBRARIES})
