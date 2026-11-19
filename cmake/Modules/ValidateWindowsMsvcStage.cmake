# AI-generated (Claude)
# Validate the Qt 6 runtime payload before NSIS packages the staged tree.
# The TLS backend is always required (cloud storage and any other HTTPS access
# need it); the map payload is required only when MAPSUPPORT is enabled.

if(NOT DEFINED STAGING OR STAGING STREQUAL "")
	message(FATAL_ERROR "STAGING must name the Windows installer staging directory")
endif()

if(NOT DEFINED MAPSUPPORT)
	set(MAPSUPPORT ON)
endif()

set(_required_tls_payload
	"plugins/tls/qschannelbackend.dll"
)

set(_required_map_payload
	"qml/QtLocation/qmldir"
	"qml/QtLocation/declarative_locationplugin.dll"
	"qml/QtPositioning/qmldir"
	"qml/QtPositioning/positioningquickplugin.dll"
	"Qt6Location.dll"
	"Qt6Positioning.dll"
	"Qt6PositioningQuick.dll"
	"plugins/geoservices/qtgeoservices_googlemaps.dll"
)

set(_required_payload ${_required_tls_payload})
if(MAPSUPPORT)
	list(APPEND _required_payload ${_required_map_payload})
endif()

set(_missing_or_empty "")
foreach(_relative_path ${_required_payload})
	set(_path "${STAGING}/${_relative_path}")
	if(NOT EXISTS "${_path}" OR IS_DIRECTORY "${_path}")
		list(APPEND _missing_or_empty "${_relative_path}")
	else()
		file(SIZE "${_path}" _size)
		if(_size EQUAL 0)
			list(APPEND _missing_or_empty "${_relative_path}")
		endif()
	endif()
endforeach()

if(_missing_or_empty)
	string(JOIN "\n  " _missing_or_empty_text ${_missing_or_empty})
	message(FATAL_ERROR
		"Windows MSVC Qt staging validation failed. Missing or empty required paths:\n"
		"  ${_missing_or_empty_text}")
endif()

message(STATUS "Windows MSVC Qt staging validation passed.")
