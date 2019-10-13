// SPDX-License-Identifier: GPL-2.0
#include "TabDiveInformation.h"
#include "ui_TabDiveInformation.h"
#include "../tagwidget.h"
#include "core/units.h"
#include "core/dive.h"
#include "desktop-widgets/command.h"
#include "core/qthelper.h"
#include "core/statistics.h"
#include "core/display.h"
#include "core/divelist.h"

#define COMBO_CHANGED 0
#define TEXT_EDITED 1

TabDiveInformation::TabDiveInformation(QWidget *parent) : TabBase(parent), ui(new Ui::TabDiveInformation())
{
	ui->setupUi(this);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &TabDiveInformation::divesChanged);
	QStringList atmPressTypes { "mbar", get_depth_unit() ,"use dc"};
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

	if (current_dive->surface_pressure.mbar == 0) {
		ui->atmPressVal->clear();			// If no atm pressure for dive then clear text box
	} else {

		ui->atmPressVal->setEnabled(true);
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

void TabDiveInformation::updateSalinity()
{
	if (current_dive->salinity)
		ui->salinityText->setText(QString("%1g/ℓ").arg(current_dive->salinity / 10.0));
	else
		ui->salinityText->clear();
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
	updateSalinity();

	ui->atmPressType->setEditable(true);
	ui->atmPressType->setItemText(1, get_depth_unit());  // Check for changes in depth unit (imperial/metric)
	ui->atmPressType->setEditable(false);
	ui->atmPressType->setCurrentIndex(0);  // Set the atmospheric pressure combo box to mbar
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveInformation::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If the current dive is not in list of changed dives, do nothing
	if (!current_dive || !dives.contains(current_dive))
		return;

	if (field.duration || field.depth || field.mode)
		updateProfile();
	if (field.air_temp)
		ui->airTemperatureText->setText(get_temperature_string(current_dive->airtemp, true));
	if (field.water_temp)
		ui->waterTemperatureText->setText(get_temperature_string(current_dive->watertemp, true));
	if (field.atm_press)
		ui->atmPressVal->setText(ui->atmPressVal->text().sprintf("%d",current_dive->surface_pressure.mbar));
	if (field.datetime)
		updateWhen();
	if (field.salinity)
		updateSalinity();
}

void TabDiveInformation::on_atmPressType_currentIndexChanged(int index) { updateTextBox(COMBO_CHANGED); }

void TabDiveInformation::on_atmPressVal_editingFinished() { updateTextBox(TEXT_EDITED); }

void TabDiveInformation::updateTextBox(int event) // Either the text box has been edited or the pressure type has changed.
{                                       // Either way this gets a numeric value and puts it on the text box atmPressVal,
	pressure_t atmpress = { 0 };    // then stores it in dive->surface_pressure.The undo stack for the text box content is
	double altitudeVal;             // maintained even though two independent events trigger saving the text box contents.
	if (current_dive) {
		switch (ui->atmPressType->currentIndex()) {
		case 0:		// If an atm pressure has been specified in mbar:
			if (event == TEXT_EDITED)         // this is only triggered by on_atmPressVal_editingFinished()
				atmpress.mbar = ui->atmPressVal->text().toInt();    // use the specified mbar pressure
			break;
		case 1:		// If an altitude has been specified:
			if (event == TEXT_EDITED) {	// this is only triggered by on_atmPressVal_editingFinished()
				altitudeVal = (ui->atmPressVal->text().toFloat());    // get altitude from text box
				if (prefs.units.length == units::FEET)         // if altitude in feet
					altitudeVal = feet_to_mm(altitudeVal); // imperial: convert altitude from feet to mm
				else
					altitudeVal = altitudeVal * 1000;     // metric: convert altitude from meters to mm
				atmpress.mbar = altitude_to_pressure((int32_t) altitudeVal); // convert altitude (mm) to pressure (mbar)
				ui->atmPressVal->setText(ui->atmPressVal->text().sprintf("%d",atmpress.mbar));
				ui->atmPressType->setCurrentIndex(0);    // reset combobox to mbar
			} else { // i.e. event == COMBO_CHANGED, that is, "m" or "ft" was selected from combobox
				ui->atmPressVal->clear();   // Clear the text box so that altitude can be typed
			}
			break;
		case 2:          // i.e. event = COMBO_CHANGED, that is, the option "Use dc" was selected from combobox
			atmpress = calculate_surface_pressure(current_dive);	// re-calculate air pressure from dc data
			ui->atmPressVal->setText(QString::number(atmpress.mbar)); // display it in text box
			ui->atmPressType->setCurrentIndex(0);          // reset combobox to mbar
			break;
		default:
			atmpress.mbar = 1013;    // This line should never execute
			break;
		}
		if (atmpress.mbar)
			Command::editAtmPress(atmpress.mbar, false);      // and save the pressure for undo
	}
}


