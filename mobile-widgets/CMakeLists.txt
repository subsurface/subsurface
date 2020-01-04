# mobile backend functions

set(SUBSURFACE_MOBILE_SRCS
	qmlinterface.cpp
	qmlmanager.cpp
	qml/kirigami/src/columnview.cpp
	qml/kirigami/src/delegaterecycler.cpp
	qml/kirigami/src/enums.cpp
	qml/kirigami/src/formlayoutattached.cpp
	qml/kirigami/src/icon.cpp
	qml/kirigami/src/kirigamiplugin.cpp
	qml/kirigami/src/mnemonicattached.cpp
	qml/kirigami/src/scenepositionattached.cpp
	qml/kirigami/src/settings.cpp
	qml/kirigami/src/wheelhandler.cpp
	qml/kirigami/src/libkirigami/basictheme.cpp
	qml/kirigami/src/libkirigami/kirigamipluginfactory.cpp
	qml/kirigami/src/libkirigami/platformtheme.cpp
	qml/kirigami/src/libkirigami/tabletmodewatcher.cpp
)

if (SUBSURFACE_TARGET_EXECUTABLE MATCHES "MobileExecutable")
	include_directories(${CMAKE_SOURCE_DIR}/mobile-widgets/qml/kirigami/src/libkirigami)
	add_definitions(-DKIRIGAMI_BUILD_TYPE_STATIC)
	add_library(subsurface_mobile STATIC ${SUBSURFACE_MOBILE_SRCS})
	target_link_libraries(subsurface_mobile ${QT_LIBRARIES})
endif()
