// SPDX-License-Identifier: GPL-2.0
#ifndef TYPEENUM_H
#define TYPEENUM_H

// *** REMARK ***
// This feature is currently not used
// but is left to allow for future use.

//FUTURE This header file can be included twice
//FUTURE 1 time as enum only (TYPEENUM_COMPILE_AS_ENUM defined)
//FUTURE 1 time as class (TYPEENUM_COMPILE_AS_ENUM not defined)
//FUTURE #if !defined(TYPEENUM_ALLOW_ENUM) || !defined(TYPEENUM_ALLOW_CLASS)
//FUTURE #if defined(TYPEENUM_COMPILE_AS_ENUM)
//FUTURE #define TYPEENUM_ALLOW_ENUM
//FUTURE #else
//FUTURE #define TYPEENUM_ALLOW_CLASS
//FUTURE #endif

//FUTURE This header is shared between the C/C++ part of subsurface and
//FUTURE indirectly with QML.
//FUTURE it secures that shared enums are defined identical for QML
//FUTURE and C++ by having the shared enums defined only once in this
//FUTURE header.
//
//FUTURE TYPEENUM_COMPILE_AS_ENUM is set where it is included, to NOT
//FUTURE generate the class (which is only needed for the QML interface)
//FUTURE that can be registred to make the enums available in QML

//FUTURE #ifndef TYPEENUM_COMPILE_AS_ENUM
//FUTURE  This part is only relevant when generating the QML accessible
//FUTURE  class.
#include <QObject>

class typeEnum : public QObject {
	Q_OBJECT

public:

private:
	typeEnum() {}

public:
//FUTURE #endif // TYPEENUM_COMPILE_AS_ENUM

	// Shared enums
	//FUTURE accessed in C/C++ just with the name.value
	// accessed in QML with TypeEnum.name.value
	enum CLOUD_STATUS {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};

	// Do not activate these enums until code is changed
	enum LENGTH {
		METERS,
		FEET
	};
	enum VOLUME {
		LITER,
		CUFT
	};
	enum PRESSURE {
		BAR,
		PSI,
		PASCALS
	};
	enum TEMPERATURE {
		CELSIUS,
		FAHRENHEIT,
		KELVIN
	};
	enum WEIGHT {
		KG,
		LBS
	};
	enum TIME {
		SECONDS,
		MINUTES
	};
	enum DURATION {
		MIXED,
		MINUTES_ONLY,
		ALWAYS_HOURS
	};

//FUTURE #ifndef TYPEENUM_COMPILE_AS_ENUM
	Q_ENUM(CLOUD_STATUS);
	Q_ENUM(LENGTH);
	Q_ENUM(VOLUME);
	Q_ENUM(PRESSURE);
	Q_ENUM(TEMPERATURE);
	Q_ENUM(WEIGHT);
	Q_ENUM(TIME);
	Q_ENUM(DURATION);
//FUTURE #endif
};
//FUTURE #endif // TYPEENUM_COMPILE_AS_ENUM

//FUTURE  Clear TYPEENUM_COMPILE_AS_ENUM to allow multple includes
//FUTURE #undef TYPEENUM_COMPILE_AS_ENUM
#endif
