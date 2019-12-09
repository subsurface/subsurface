// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_LOG_H
#define PREFERENCES_LOG_H

#include "abstractpreferenceswidget.h"
#include "core/pref.h"

namespace Ui {
	class PreferencesLog;
}

class PreferencesLog : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesLog();
	~PreferencesLog();
	void refreshSettings() override;
	void syncSettings() override;
public slots:
	void on_chooseFile_clicked();
	void on_btnUseDefaultFile_toggled(bool toggled);
	void on_localDefaultFile_toggled(bool toggled);

private:
	Ui::PreferencesLog *ui;
};


#endif
