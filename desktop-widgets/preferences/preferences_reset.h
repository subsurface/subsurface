// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_RESET_H
#define PREFERENCES_RESET_H

#include "abstractpreferenceswidget.h"
#include "core/pref.h"

namespace Ui {
	class PreferencesReset;
}

class PreferencesReset : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesReset();
	~PreferencesReset();
	void refreshSettings() override;
	void syncSettings() override;

public slots:
	void on_resetSettings_clicked();

private:
	Ui::PreferencesReset *ui;

};

#endif

