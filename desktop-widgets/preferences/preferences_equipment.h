// SPDX-License-Identifier: GPL-2.0
#ifndef PREFERENCES_EQUIPMENT_H
#define PREFERENCES_EQUIPMENT_H

#include <QMap>
#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesEquipment;
}

class PreferencesEquipment : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesEquipment();
	~PreferencesEquipment();
	void refreshSettings() override;
	void syncSettings() override;
private:
	Ui::PreferencesEquipment *ui;
public slots:

};

#endif
