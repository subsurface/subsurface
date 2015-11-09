
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(FACEBOOK_PLUGIN_UI
  facebookconnectwidget.ui
  socialnetworksdialog.ui
)

set(FACEBOOK_PLUGIN_SRCS
  facebook_integration.cpp
  facebookconnectwidget.cpp
)

qt5_wrap_ui(FACEBOOK_PLUGIN_UI_SRCS ${FACEBOOK_PLUGIN_UI})
add_library(facebook_integration STATIC ${FACEBOOK_PLUGIN_SRCS} ${FACEBOOK_PLUGIN_UI_SRCS})
target_link_libraries(facebook_integration ${QT_LIBRARIES})
add_dependencies(facebook_integration subsurface_corelib)