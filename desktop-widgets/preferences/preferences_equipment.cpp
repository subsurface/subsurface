// SPDX-License-Identifier: GPL-2.0
#include "preferences_equipment.h"
#include "ui_preferences_equipment.h"
#include "core/settings/qPrefEquipment.h"
#include "core/qthelper.h"
#include "core/dive.h"
#include "qt-models/tankinfomodel.h"
#include "managetankinfodialog.h"

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
	ui->include_unused_tanks->setChecked(prefs.include_unused_tanks);
	ui->display_default_tank_infos->setChecked(prefs.display_default_tank_infos);
	refreshCylinderCombo(qPrefEquipment::default_cylinder());
}

// Repopulate the "default cylinder" combo box from tank_info_table, trying
// to keep the given name selected if it is still present.
void PreferencesEquipment::refreshCylinderCombo(const QString &wantedSelection)
{
	ui->default_cylinder->clear();
	int wantedIndex = -1;
	int i = 0;
	for (const tank_info &ti: tank_info_table) {
		ui->default_cylinder->addItem(QString::fromStdString(ti.name));
		if (wantedSelection == QString::fromStdString(ti.name))
			wantedIndex = i;
		++i;
	}
	ui->default_cylinder->setCurrentIndex(wantedIndex);
}

void PreferencesEquipment::syncSettings()
{
	auto equipment = qPrefEquipment::instance();
	qPrefEquipment::set_include_unused_tanks(ui->include_unused_tanks->isChecked());
	qPrefEquipment::set_display_default_tank_infos(ui->display_default_tank_infos->isChecked());
	equipment->set_default_cylinder(ui->default_cylinder->currentText());

	// In case the user changed the tank info settings,
	// reset the tank_info_table. It is somewhat questionable
	// to do this here. Moreover, it is a bit crude, as this
	// will be called for *any* preferences change.
	reset_tank_info_table(tank_info_table);
}

void PreferencesEquipment::on_manageCylinderTypes_clicked()
{
	QString previousSelection = ui->default_cylinder->currentText();

	ManageTankInfoDialog dlg(previousSelection, this);
	dlg.exec();

	// Types may have been renamed, deleted or reordered: refresh the
	// default-cylinder combo, without touching the other (pending,
	// not-yet-applied) settings on this page. Use the dialog's
	// trackedName() rather than the pre-dialog previousSelection directly:
	// trackedName() follows the previously selected entry through any
	// rename made while the dialog was open, whereas previousSelection
	// itself would go stale the moment that exact entry gets renamed,
	// silently losing the selection to the combo's first entry.
	refreshCylinderCombo(dlg.trackedName());
}
