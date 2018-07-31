// SPDX-License-Identifier: GPL-2.0
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
	~PreferencesUnits();
	void refreshSettings() override;
	void syncSettings() override;
private:
	Ui::PreferencesUnits *ui;
};

#endif
