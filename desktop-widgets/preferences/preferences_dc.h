// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_DC_H
#define PREFERENCES_DC_H

#include "abstractpreferenceswidget.h"
#include "core/pref.h"

namespace Ui {
	class PreferencesDc;
}

class PreferencesDc : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesDc();
	~PreferencesDc();
	void refreshSettings() override;
	void syncSettings() override;
public slots:
	void on_resetRememberedDCs_clicked();

private:
	Ui::PreferencesDc *ui;
};


#endif
