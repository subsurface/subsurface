// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_CLOUD_H
#define PREFERENCES_CLOUD_H

#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesCloud;
}

class PreferencesCloud : public AbstractPreferencesWidget {
	Q_OBJECT

public:
	PreferencesCloud();
	~PreferencesCloud();
	void refreshSettings() override;
	void syncSettings() override;

public slots:
	void updateCloudAuthenticationState();
	void passwordUpdateSuccessful();
	void on_resetPassword_clicked();

private:
	Ui::PreferencesCloud *ui;
};

#endif
