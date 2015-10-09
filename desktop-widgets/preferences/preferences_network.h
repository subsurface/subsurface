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
	virtual ~PreferencesNetwork();
	virtual void refreshSettings();
	virtual void syncSettings();

public slots:
	void proxyType_changed(int i);

private:
	Ui::PreferencesNetwork *ui;
	void cloudPinNeeded();
	void passwordUpdateSuccessfull();
};

#endif