// SPDX-License-Identifier: GPL-2.0
#ifndef TYPEENUM_H
#define TYPEENUM_H
#include <QObject>

class typeEnum : public QObject {
	Q_OBJECT

public:
	// Duplicated enums, these enums are properly defined in the C/C++ structure
	// but duplicated here to make them available to QML.

	// Duplicating the enums poses a slight risk for forgetting to update
	// them if the proper enum is changed (e.g. assigning a new start value).

	// remark please do not use these enums outside the C++/QML interface.
	enum UNIT_SYSTEM {
		METRIC,
		IMPERIAL,
		PERSONALIZE
	};
	Q_ENUM(UNIT_SYSTEM);

	enum LENGTH {
		METERS,
		FEET
	};
	Q_ENUM(LENGTH);

	enum VOLUME {
		LITER,
		CUFT
	};
	Q_ENUM(VOLUME);

	enum PRESSURE {
		BAR,
		PSI,
		PASCALS
	};
	Q_ENUM(PRESSURE);

	enum TEMPERATURE {
		CELSIUS,
		FAHRENHEIT,
		KELVIN
	};
	Q_ENUM(TEMPERATURE);

	enum WEIGHT {
		KG,
		LBS
	};
	Q_ENUM(WEIGHT);

	enum TIME {
		SECONDS,
		MINUTES
	};
	Q_ENUM(TIME);

	enum DURATION {
		MIXED,
		MINUTES_ONLY,
		ALWAYS_HOURS
	};
	Q_ENUM(DURATION);

	enum CLOUD_STATUS {
		CS_UNKNOWN,
		CS_INCORRECT_USER_PASSWD,
		CS_NEED_TO_VERIFY,
		CS_VERIFIED,
		CS_NOCLOUD
	};
	Q_ENUM(CLOUD_STATUS);

	enum DECO_MODE {
		BUEHLMANN,
		RECREATIONAL,
		VPMB
	};
	Q_ENUM(DECO_MODE);

	enum DIVE_MODE {
		OC,
		CCR,
		PSCR
	};
	Q_ENUM(DIVE_MODE);

private:
	typeEnum() {}
};
#endif
