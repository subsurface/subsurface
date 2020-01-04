// SPDX-License-Identifier: GPL-2.0
#ifndef QMLINTERFACE_H
#define QMLINTERFACE_H
#include <QObject>
#include <QQmlContext>
// This class is a pure interface class and may not contain any implementation code
// Allowed are:
//     header
//        Q_PROPERTY
//        signal/slot for Q_PROPERTY functions
//        the functions may contain either
//             a) a function call to the implementation
//             b) a reference to a global variable like e.g. prefs.
//        Q_INVOCABLE functions
//        the functions may contain
//             a) a function call to the implementation
//    source
//        connect signal/signal to pass signals from implementation


class QMLInterface : public QObject {
	Q_OBJECT

	// Q_PROPERTY used in QML

public:
	static QMLInterface *instance();

	// function to do the needed setup and do connect of signal/signal
	static void setup(QQmlContext *ct);

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

private:
	QMLInterface() {}
};
#endif // QMLINTERFACE_H

