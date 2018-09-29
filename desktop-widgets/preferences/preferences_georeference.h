// SPDX-License-Identifier: GPL-2.0
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
	~PreferencesGeoreference();
	void refreshSettings() override;
	void syncSettings() override;
private:
	Ui::PreferencesGeoreference *ui;
};

#endif
