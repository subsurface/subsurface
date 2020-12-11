// SPDX-License-Identifier: GPL-2.0
#include "preferences_equipment.h"
#include "ui_preferences_equipment.h"
#include "core/settings/qPrefEquipment.h"
#include "core/qthelper.h"
#include "core/dive.h"

#include <QApplication>
#include <QMessageBox>
#include <QSortFilterProxyModel>

PreferencesEquipment::PreferencesEquipment() : AbstractPreferencesWidget(tr("Equipment"), QIcon(":preferences-equipment-icon"), 5)
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
	for (int i = 0; i < tank_info_table.nr; i++) {
		const tank_info &ti = tank_info_table.infos[i];
		ui->default_cylinder->addItem(ti.name);
		if (qPrefEquipment::default_cylinder() == ti.name)
			ui->default_cylinder->setCurrentIndex(i);
	}
}

void PreferencesEquipment::syncSettings()
{
	auto equipment = qPrefEquipment::instance();
	qPrefEquipment::set_display_unused_tanks(ui->display_unused_tanks->isChecked());
	equipment->set_default_cylinder(ui->default_cylinder->currentText());
}
