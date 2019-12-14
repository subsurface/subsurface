// SPDX-License-Identifier: GPL-2.0
#ifndef PLANNERSHARED_H
#define PLANNERSHARED_H
#include <QObject>

// This is a shared class (mobile/desktop), and contains the core of the diveplanner
// without UI entanglement.
// It make variables and functions available to QML, these are referenced directly
// in the desktop version
//
// The mobile diveplanner shows all diveplans, but the editing functionality is
// limited to keep the UI simpler.

class plannerShared: public QObject {
	Q_OBJECT

	// Ascend/Descend data, converted to meter/feet depending on user selection
	// Settings these will automatically update the corresponding qPrefDivePlanner
	// Variables
	Q_PROPERTY(int ascratelast6m READ ascratelast6m WRITE set_ascratelast6m NOTIFY ascratelast6mChanged);
	Q_PROPERTY(int ascratestops READ ascratestops WRITE set_ascratestops NOTIFY ascratestopsChanged);
	Q_PROPERTY(int ascrate50 READ ascrate50 WRITE set_ascrate50 NOTIFY ascrate50Changed);
	Q_PROPERTY(int ascrate75 READ ascrate75 WRITE set_ascrate75 NOTIFY ascrate75Changed);
	Q_PROPERTY(int descrate READ descrate WRITE set_descrate NOTIFY descrateChanged);
public:
	static plannerShared *instance();

	// Ascend/Descend data, converted to meter/feet depending on user selection
	static int ascratelast6m();
	static int ascratestops();
	static int ascrate50();
	static int ascrate75();
	static int descrate();

public slots:
	// Ascend/Descend data, converted to meter/feet depending on user selection
	static void set_ascratelast6m(int value);
	static void set_ascratestops(int value);
	static void set_ascrate50(int value);
	static void set_ascrate75(int value);
	static void set_descrate(int value);

signals:
	// Ascend/Descend data, converted to meter/feet depending on user selection
	void ascratelast6mChanged(int value);
	void ascratestopsChanged(int value);
	void ascrate50Changed(int value);
	void ascrate75Changed(int value);
	void descrateChanged(int value);

private:
	plannerShared() {}
};

#endif // PLANNERSHARED_H
