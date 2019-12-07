// SPDX-License-Identifier: GPL-2.0
#include "preferences_equipment.h"
#include "ui_preferences_equipment.h"
#include "core/settings/qPrefEquipment.h"
#include "core/qthelper.h"
#include "core/dive.h"

#include <QApplication>
#include <QMessageBox>
#include <QSortFilterProxyModel>

#include "qt-models/models.h"

PreferencesEquipment::PreferencesEquipment() : AbstractPreferencesWidget(tr("Equipment"), QIcon(":preferences-equipment-icon"), 4)
{
	ui = new Ui::PreferencesEquipment();
	ui->setupUi(this);
}

PreferencesEquipment::~PreferencesEquipment()
{
	delete ui;
}

void PreferencesEquipment::refreshSettings()
{
	ui->display_unused_tanks->setChecked(prefs.display_unused_tanks);
	ui->default_cylinder->clear();
	for (int i = 0; i < MAX_TANK_INFO && tank_info[i].name != NULL; i++) {
		ui->default_cylinder->addItem(tank_info[i].name);
		if (qPrefEquipment::default_cylinder() == tank_info[i].name)
			ui->default_cylinder->setCurrentIndex(i);
	}
}

void PreferencesEquipment::syncSettings()
{
	auto equipment = qPrefEquipment::instance();
	qPrefEquipment::set_display_unused_tanks(ui->display_unused_tanks->isChecked());
	equipment->set_default_cylinder(ui->default_cylinder->currentText());
}
