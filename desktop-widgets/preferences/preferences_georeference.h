#ifndef PREFERENCES_GEOREFERENCE_H
#define PREFERENCES_GEOREFERENCE_H

#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesGeoreference;
}

class PreferencesGeoreference : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesGeoreference();
	virtual ~PreferencesGeoreference();
	virtual void refreshSettings();
	virtual void syncSettings();
private:
	Ui::PreferencesGeoreference *ui;
};

#endif