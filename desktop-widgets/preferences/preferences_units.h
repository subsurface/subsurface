#ifndef PREFERENCES_UNITS_H
#define PREFERENCES_UNITS_H

#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesUnits;
}

class PreferencesUnits : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesUnits();
	virtual ~PreferencesUnits();
	virtual void refreshSettings();
	virtual void syncSettings();
private:
	Ui::PreferencesUnits *ui;
};

#endif