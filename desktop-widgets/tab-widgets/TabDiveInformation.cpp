// SPDX-License-Identifier: GPL-2.0
#include "TabDiveInformation.h"
#include "ui_TabDiveInformation.h"
#include "../tagwidget.h"
#include "core/dive.h"
#include "desktop-widgets/command.h"

#include <core/qthelper.h>
#include <core/statistics.h>
#include <core/display.h>

TabDiveInformation::TabDiveInformation(QWidget *parent) : TabBase(parent), ui(new Ui::TabDiveInformation())
{
	ui->setupUi(this);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &TabDiveInformation::divesChanged);
	QStringList atmPressTypes = QStringList();
	atmPressTypes.append("mbar");
	atmPressTypes.append(get_depth_unit());
	ui->atmPressType->insertItems(0, atmPressTypes);
	pressTypeIndex = 0;
}

TabDiveInformation::~TabDiveInformation()
{
	delete ui;
}

void TabDiveInformation::clear()
{
	ui->sacText->clear();
	ui->otuText->clear();
	ui->maxcnsText->clear();
	ui->oxygenHeliumText->clear();
	ui->gasUsedText->clear();
	ui->dateText->clear();
	ui->diveTimeText->clear();
	ui->surfaceIntervalText->clear();
	ui->maximumDepthText->clear();
	ui->averageDepthText->clear();
	ui->waterTemperatureText->clear();
	ui->airTemperatureText->clear();
	ui->atmPressVal->clear();
	ui->salinityText->clear();
}

// Update fields that depend on the dive profile
void TabDiveInformation::updateProfile()
{
	ui->maxcnsText->setText(QString("%1\%").arg(current_dive->maxcns));
	ui->otuText->setText(QString("%1").arg(current_dive->otu));
	ui->maximumDepthText->setText(get_depth_string(current_dive->maxdepth, true));
	ui->averageDepthText->setText(get_depth_string(current_dive->meandepth, true));

	volume_t gases[MAX_CYLINDERS] = {};
	get_gas_used(current_dive, gases);
	QString volumes;
	int mean[MAX_CYLINDERS], duration[MAX_CYLINDERS];
	per_cylinder_mean_depth(current_dive, select_dc(current_dive), mean, duration);
	volume_t sac;
	QString gaslist, SACs, separator;

	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!is_cylinder_used(current_dive, i))
			continue;
		gaslist.append(separator); volumes.append(separator); SACs.append(separator);
		separator = "\n";

		gaslist.append(gasname(current_dive->cylinder[i].gasmix));
		if (!gases[i].mliter)
			continue;
		volumes.append(get_volume_string(gases[i], true));
		if (duration[i]) {
			sac.mliter = lrint(gases[i].mliter / (depth_to_atm(mean[i], current_dive) * duration[i] / 60));
			SACs.append(get_volume_string(sac, true).append(tr("/min")));
		}
	}
	ui->gasUsedText->setText(volumes);
	ui->oxygenHeliumText->setText(gaslist);

	ui->diveTimeText->setText(get_dive_duration_string(current_dive->duration.seconds, tr("h"), tr("min"), tr("sec"),
			" ", current_dive->dc.divemode == FREEDIVE));

	ui->sacText->setText( mean[0] ? SACs : QString());

	if (current_dive->surface_pressure.mbar == 0)
		ui->atmPressVal->clear();			// If no atm pressure for dive then clear text box
	else {
		QString pressStr;
		pressStr.sprintf("%d",current_dive->surface_pressure.mbar);
		ui->atmPressVal->setText(pressStr);		// else display atm pressure
	}

}

// Update fields that depend on start of dive
void TabDiveInformation::updateWhen()
{
	ui->dateText->setText(get_short_dive_date_string(current_dive->when));
	timestamp_t surface_interval = get_surface_interval(current_dive->when);
	if (surface_interval >= 0)
		ui->surfaceIntervalText->setText(get_dive_surfint_string(surface_interval, tr("d"), tr("h"), tr("min")));
	else
		ui->surfaceIntervalText->clear();
}

void TabDiveInformation::updateData()
{
	if (!current_dive) {
		clear();
		return;
	}

	updateProfile();
	updateWhen();
	ui->waterTemperatureText->setText(get_temperature_string(current_dive->watertemp, true));
	ui->airTemperatureText->setText(get_temperature_string(current_dive->airtemp, true));

	if (current_dive->salinity)
		ui->salinityText->setText(QString("%1g/ℓ").arg(current_dive->salinity / 10.0));
	else
		ui->salinityText->clear();

	ui->atmPressType->setEditable(true);
	ui->atmPressType->setItemText(1, get_depth_unit());  // Check for changes in depth unit (imperial/metric)
	ui->atmPressType->setEditable(false);
	ui->atmPressType->setCurrentIndex(0);  // Set the atmospheric pressure combo box to mbar
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveInformation::divesChanged(dive_trip *trip, const QVector<dive *> &dives, DiveField field)
{
	// If the current dive is not in list of changed dives, do nothing
	if (!current_dive || !dives.contains(current_dive))
		return;

	switch(field) {
	case DiveField::DURATION:
	case DiveField::DEPTH:
	case DiveField::MODE:
		updateProfile();
		break;
	case DiveField::AIR_TEMP:
		ui->airTemperatureText->setText(get_temperature_string(current_dive->airtemp, true));
		break;
	case DiveField::WATER_TEMP:
		ui->waterTemperatureText->setText(get_temperature_string(current_dive->watertemp, true));
		break;
	case DiveField::ATM_PRESS:
		ui->atmPressVal->setText(ui->atmPressVal->text().sprintf("%d",current_dive->surface_pressure.mbar));

//get_temperature_string(current_dive->surface_pressure, true));
//pressStr.sprintf("%d",atmpress);

		break;
	case DiveField::DATETIME:
		updateWhen();
		break;
	default:
		break;
	}
}

void TabDiveInformation::on_atmPressVal_editingFinished()
{
	int32_t atmpress = 0;
	QString pressStr;
	if (current_dive) {
		if (ui->atmPressType->currentIndex() == 1) {		// If altitude has been specified:
			atmpress = (int)(0.5 + ui->atmPressVal->text().toFloat());	// get altitude from text box
			if (prefs.units.length == units::FEET)		// if altitude in feet
				atmpress = (int)(atmpress / 3.28084); 	// 	convert feet -> meters
			atmpress = altitude_to_pressure(atmpress);	// convert altitude to pressure
			pressStr.sprintf("%d",atmpress);
			ui->atmPressVal->setText(pressStr);		// Write pressure to text box
			ui->atmPressType->setCurrentIndex(0);		// set combobox to mbar
		}
		else
			atmpress = ui->atmPressVal->text().toInt();	// get pressure that has been specified
		Command::editAtmPress(atmpress, false);
	}
}



