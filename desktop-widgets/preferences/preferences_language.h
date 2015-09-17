#ifndef PREFERENCES_LANGUAGE_H
#define PREFERENCES_LANGUAGE_H

#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesLanguage;
}

class PreferencesLanguage : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesLanguage();
	virtual ~PreferencesLanguage();
	virtual void refreshSettings();
	virtual void syncSettings();
private:
	Ui::PreferencesLanguage *ui;
};

#endif