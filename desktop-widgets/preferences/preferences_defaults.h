// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_DEFAULTS_H
#define PREFERENCES_DEFAULTS_H

#include "abstractpreferenceswidget.h"
#include "core/pref.h"

namespace Ui {
	class PreferencesDefaults;
}

class PreferencesDefaults : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesDefaults();
	~PreferencesDefaults();
	void refreshSettings() override;
	void syncSettings() override;

public slots:

private:
	Ui::PreferencesDefaults *ui;
};


#endif
