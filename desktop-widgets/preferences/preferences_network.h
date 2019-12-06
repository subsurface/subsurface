// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_NETWORK_H
#define PREFERENCES_NETWORK_H

#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesNetwork;
}

class PreferencesNetwork : public AbstractPreferencesWidget {
	Q_OBJECT

public:
	PreferencesNetwork();
	~PreferencesNetwork();
	void refreshSettings() override;
	void syncSettings() override;

public slots:
	void proxyType_changed(int i);

private:
	Ui::PreferencesNetwork *ui;
};

#endif
