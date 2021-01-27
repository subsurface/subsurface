// SPDX-License-Identifier: GPL-2.0
#include "TabDiveInformation.h"
#include "ui_TabDiveInformation.h"
#include "desktop-widgets/mainwindow.h" // TODO: Only used temporarilly for edit mode changes
#include "profile-widget/profilewidget2.h"
#include "../tagwidget.h"
#include "commands/command.h"
#include "core/subsurface-string.h"
#include "core/units.h"
#include "core/dive.h"
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
	connect(&diveListNotifier, &DiveListNotifier::cylinderAdded, this, &TabDiveInformation::cylinderChanged);
	connect(&diveListNotifier, &DiveListNotifier::cylinderRemoved, this, &TabDiveInformation::cylinderChanged);
	connect(&diveListNotifier, &DiveListNotifier::cylinderEdited, this, &TabDiveInformation::cylinderChanged);

	QStringList atmPressTypes { "mbar", get_depth_unit() ,tr("Use DC")};
	ui->atmPressType->insertItems(0, atmPressTypes);
	pressTypeIndex = 0;
	ui->waterTypeCombo->insertItems(0, getWaterTypesAsString());

	// This needs to be the same order as enum dive_comp_type in dive.h!
	QStringList types;
	for (int i = 0; i < NUM_DIVEMODE; i++)
		types.append(gettextFromC::tr(divemode_text_ui[i]));
	ui->diveType->insertItems(0, types);
	connect(ui->diveType, SIGNAL(currentIndexChanged(int)), this, SLOT(diveModeChanged(int)));
	if (!prefs.extraEnvironmentalDefault) // if extraEnvironmental preference is turned off
		showCurrentWidget(false, 0);  // Show current star widget at lefthand side
	QAction *action = new QAction(tr("OK"), this);
	connect(action, &QAction::triggered, this, &TabDiveInformation::closeWarning);
	ui->multiDiveWarningMessage->addAction(action);
	action = new QAction(tr("Undo"), this);
	connect(action, &QAction::triggered, Command::undoAction(this), &QAction::trigger);
	connect(action, &QAction::triggered, this, &TabDiveInformation::closeWarning);
	ui->multiDiveWarningMessage->addAction(action);
	ui->multiDiveWarningMessage->hide();
	manualDive = false;
	updateWaterTypeWidget();
	QPixmap warning (":salinity-warning-icon");
	ui->salinityOverWrittenIcon->setPixmap(warning);
	ui->salinityOverWrittenIcon->setToolTip("Water type differs from that of dc");
	ui->salinityOverWrittenIcon->setToolTipDuration(2500);
	ui->salinityOverWrittenIcon->setVisible(false);
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
	ui->diveTimeText->clear();
	ui->surfaceIntervalText->clear();
	ui->maximumDepthText->clear();
	ui->averageDepthText->clear();
	ui->watertemp->clear();
	ui->airtemp->clear();
	ui->atmPressVal->clear();
	ui->salinityText->clear();
	ui->waterTypeText->clear();
	ui->waterTypeCombo->setCurrentIndex(0);
}

void TabDiveInformation::divesEdited(int i)
{
	// No warning if only one dive was edited
	if (i <= 1)
		return;
	ui->multiDiveWarningMessage->setCloseButtonVisible(false);
	ui->multiDiveWarningMessage->setText(tr("Warning: edited %1 dives").arg(i));
	ui->multiDiveWarningMessage->show();
}

void TabDiveInformation::closeWarning()
{
	ui->multiDiveWarningMessage->hide();
}

void TabDiveInformation::updateWaterTypeWidget()
{    // Decide on whether to show the water type/salinity combobox or not
	if (prefs.salinityEditDefault || manualDive)
	{         // if the preference setting has been checked or this is a manually-entered dive
		ui->waterTypeText->setVisible(false);
		ui->waterTypeCombo->setVisible(true); // show combobox
	} else {  // if the preference setting has not been set
		ui->waterTypeCombo->setVisible(false);
		ui->waterTypeText->setVisible(true);  // show water type as text label
	}
}

// Update fields that depend on the dive profile
void TabDiveInformation::updateProfile()
{
	ui->maxcnsText->setText(QString("%L1\%").arg(current_dive->maxcns));
	ui->otuText->setText(QString("%L1").arg(current_dive->otu));
	ui->maximumDepthText->setText(get_depth_string(current_dive->maxdepth, true));
	ui->averageDepthText->setText(get_depth_string(current_dive->meandepth, true));

	volume_t *gases = get_gas_used(current_dive);
	QString volumes;
	std::vector<int> mean(current_dive->cylinders.nr), duration(current_dive->cylinders.nr);
	if (current_dive->cylinders.nr >= 0)
		per_cylinder_mean_depth(current_dive, select_dc(current_dive), mean.data(), duration.data());
	volume_t sac;
	QString gaslist, SACs, separator;

	for (int i = 0; i < current_dive->cylinders.nr; i++) {
		if (!is_cylinder_used(current_dive, i))
			continue;
		gaslist.append(separator); volumes.append(separator); SACs.append(separator);
		separator = "\n";

		gaslist.append(gasname(get_cylinder(current_dive, i)->gasmix));
		if (!gases[i].mliter)
			continue;
		volumes.append(get_volume_string(gases[i], true));
		if (duration[i]) {
			sac.mliter = lrint(gases[i].mliter / (depth_to_atm(mean[i], current_dive) * duration[i] / 60));
			SACs.append(get_volume_string(sac, true).append(tr("/min")));
		}
	}
	free(gases);
	ui->gasUsedText->setText(volumes);
	ui->oxygenHeliumText->setText(gaslist);

	ui->diveTimeText->setText(get_dive_duration_string(current_dive->duration.seconds, tr("h"), tr("min"), tr("sec"),
			" ", current_dive->dc.divemode == FREEDIVE));

	ui->sacText->setText(current_dive->cylinders.nr > 0 && mean[0] ? SACs : QString());

	if (current_dive->surface_pressure.mbar == 0) {
		ui->atmPressVal->clear();			// If no atm pressure for dive then clear text box
	} else {
		ui->atmPressVal->setEnabled(true);
		ui->atmPressVal->setText(QString::number(current_dive->surface_pressure.mbar));		// else display atm pressure
	}
}

// Update fields that depend on start of dive
void TabDiveInformation::updateWhen()
{
	timestamp_t surface_interval = get_surface_interval(current_dive->when);
	if (surface_interval >= 0)
		ui->surfaceIntervalText->setText(get_dive_surfint_string(surface_interval, tr("d"), tr("h"), tr("min")));
	else
		ui->surfaceIntervalText->clear();
}

// Provide an index for the combobox that corresponds to the salinity value
int TabDiveInformation::updateSalinityComboIndex(int salinity)
{
	if (salinity == 0)
		return -1; // we don't know
	else if (salinity < 10050)
		return FRESHWATER;
	else if (salinity < 10190)
		return BRACKISHWATER;
	else if (salinity < 10210)
		return EN13319WATER;
	else
		return SALTWATER;
}

// If dive->user_salinity != dive->salinity (i.e. dc value) then show the salinity-overwrite indicator
void TabDiveInformation::checkDcSalinityOverWritten()
{
	if (!current_dive)
		return;
	int dc_value = current_dc->salinity;
	int user_value = current_dive->user_salinity;
	bool show_indicator = false;
	if (!manualDive && user_value != 0 && user_value != dc_value)
		show_indicator = true;
	ui->salinityOverWrittenIcon->setVisible(show_indicator);
}

void TabDiveInformation::showCurrentWidget(bool show, int position)
{
	ui->groupBox_wavesize->setVisible(show);
	ui->groupBox_surge->setVisible(show);
	ui->groupBox_chill->setVisible(show);
	int layoutPosition = ui->diveInfoScrollAreaLayout->indexOf(ui->groupBox_current);
	ui->diveInfoScrollAreaLayout->takeAt(layoutPosition);
	ui->diveInfoScrollAreaLayout->addWidget(ui->groupBox_current, 6, position, 1, 1);
}

void TabDiveInformation::updateData()
{
	if (!current_dive) {
		clear();
		return;
	}

	int salinity_value;
	manualDive = same_string(current_dive->dc.model, "manually added dive");
	updateWaterTypeWidget();
	updateProfile();
	updateWhen();
	ui->watertemp->setText(get_temperature_string(current_dive->watertemp, true));
	ui->airtemp->setText(get_temperature_string(current_dive->airtemp, true));
	ui->atmPressType->setItemText(1, get_depth_unit());  // Check for changes in depth unit (imperial/metric)
	ui->atmPressType->setCurrentIndex(0);                // Set the atmospheric pressure combo box to mbar
	salinity_value = get_dive_salinity(current_dive);
	if (salinity_value) {			// Set water type indicator (EN13319 = 1.020 g/l)
		if (prefs.salinityEditDefault) {   //If edit-salinity is enabled then set correct water type in combobox:
			ui->waterTypeCombo->setCurrentIndex(updateSalinityComboIndex(salinity_value));
		} else {         // If water salinity is not editable: show water type as a text label
			ui->waterTypeText->setText(get_water_type_string(salinity_value));
		}
		ui->salinityText->setText(get_salinity_string(salinity_value));
	} else {
		ui->waterTypeCombo->setCurrentIndex(-1);
		ui->waterTypeText->setText(tr("unknown"));
		ui->salinityText->clear();
	}
	checkDcSalinityOverWritten();  // If exclamation is needed (i.e. salinity overwrite by user), then show it

	updateMode(current_dive);
	ui->visibility->setCurrentStars(current_dive->visibility);
	ui->wavesize->setCurrentStars(current_dive->wavesize);
	ui->current->setCurrentStars(current_dive->current);
	ui->surge->setCurrentStars(current_dive->surge);
	ui->chill->setCurrentStars(current_dive->chill);
	if (prefs.extraEnvironmentalDefault)
		showCurrentWidget(true, 2);   // Show current star widget at 3rd position
	else
		showCurrentWidget(false, 0);  // Show current star widget at lefthand side
}

void TabDiveInformation::updateUi(QString titleColor)
{
	QString CSSSetSmallLabel = "QLabel:enabled { color: ";
	CSSSetSmallLabel.append(titleColor + "; font-size: ");
	CSSSetSmallLabel.append(QString::number((int)(0.5 + ui->diveHeadingLabel->geometry().height() * 0.66)) + "px;}");
	ui->groupBox_visibility->setStyleSheet(ui->groupBox_visibility->styleSheet() + CSSSetSmallLabel);
	ui->groupBox_current->setStyleSheet(ui->groupBox_current->styleSheet() + CSSSetSmallLabel);
	ui->groupBox_wavesize->setStyleSheet(ui->groupBox_wavesize->styleSheet() + CSSSetSmallLabel);
	ui->groupBox_surge->setStyleSheet(ui->groupBox_surge->styleSheet() + CSSSetSmallLabel);
	ui->groupBox_chill->setStyleSheet(ui->groupBox_chill->styleSheet() + CSSSetSmallLabel);
	ui->salinityOverWrittenIcon->setToolTip(ui->salinityOverWrittenIcon->styleSheet() + CSSSetSmallLabel);
}

// From the index of the water type combo box, set the dive->salinity to an appropriate value
void TabDiveInformation::on_waterTypeCombo_activated(int index)
{
	Q_UNUSED(index)
	int combobox_salinity = 0;
	int dc_salinity = current_dc->salinity;
	switch(ui->waterTypeCombo->currentIndex()) {
	case FRESHWATER:
		combobox_salinity = FRESHWATER_SALINITY;
		break;
	case BRACKISHWATER:
		combobox_salinity = BRACKISH_SALINITY;
		break;
	case EN13319WATER:
		combobox_salinity = EN13319_SALINITY;
		break;
	case SALTWATER:
		combobox_salinity = SEAWATER_SALINITY;
		break;
	case DC_WATERTYPE:
		combobox_salinity = dc_salinity;
		ui->waterTypeCombo->setCurrentIndex(updateSalinityComboIndex(combobox_salinity));
		break;
	default:
		// the index was set to -1 to indicate an unknown water type
		combobox_salinity = 0;
		break;
	}
	// Save and display the new salinity value
	if (combobox_salinity)
		ui->salinityText->setText(get_salinity_string(combobox_salinity));
	else
		ui->salinityText->clear();
	divesEdited(Command::editWaterTypeUser(combobox_salinity, false));
	// If salinity differs from that downloaded from dc, show warning
	if (manualDive || dc_salinity == combobox_salinity)
		ui->salinityOverWrittenIcon->setVisible(false);
	else
		ui->salinityOverWrittenIcon->setVisible(true);
}

void TabDiveInformation::cylinderChanged(dive *d)
{
	// If this isn't the current dive, do nothing
	if (current_dive != d)
		return;
	updateProfile();
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveInformation::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	int salinity_value;

	// If the current dive is not in list of changed dives, do nothing
	if (!current_dive || !dives.contains(current_dive))
		return;

	if (field.visibility)
		ui->visibility->setCurrentStars(current_dive->visibility);
	if (field.wavesize)
		ui->wavesize->setCurrentStars(current_dive->wavesize);
	if (field.current)
		ui->current->setCurrentStars(current_dive->current);
	if (field.surge)
		ui->surge->setCurrentStars(current_dive->surge);
	if (field.chill)
		ui->chill->setCurrentStars(current_dive->chill);
	if (field.mode)
		updateMode(current_dive);
	if (field.duration || field.depth || field.mode)
		updateProfile();
	if (field.air_temp)
		ui->airtemp->setText(get_temperature_string(current_dive->airtemp, true));
	if (field.water_temp)
		ui->watertemp->setText(get_temperature_string(current_dive->watertemp, true));
	if (field.atm_press)
		ui->atmPressVal->setText(QString::number(current_dive->surface_pressure.mbar));
	if (field.salinity)
		checkDcSalinityOverWritten();
	if (current_dive->user_salinity)
		salinity_value = current_dive->user_salinity;
	else
		salinity_value = current_dive->salinity;
	ui->waterTypeCombo->setCurrentIndex(updateSalinityComboIndex(salinity_value));
	ui->salinityText->setText(QString("%L1g/ℓ").arg(salinity_value / 10.0));
}

void TabDiveInformation::on_visibility_valueChanged(int value)
{
	if (current_dive)
		divesEdited(Command::editVisibility(value, false));
}

void TabDiveInformation::on_wavesize_valueChanged(int value)
{
	if (current_dive)
		divesEdited(Command::editWaveSize(value, false));
}

void TabDiveInformation::on_current_valueChanged(int value)
{
	if (current_dive)
		divesEdited(Command::editCurrent(value, false));
}

void TabDiveInformation::on_surge_valueChanged(int value)
{
	if (current_dive)
		divesEdited(Command::editSurge(value, false));
}

void TabDiveInformation::on_chill_valueChanged(int value)
{
	if (current_dive)
		divesEdited(Command::editChill(value, false));
}

void TabDiveInformation::updateMode(struct dive *d)
{
	ui->diveType->setCurrentIndex(get_dive_dc(d, dc_number)->divemode);
}

void TabDiveInformation::diveModeChanged(int index)
{
	if (current_dive)
		divesEdited(Command::editMode(dc_number, (enum divemode_t)index, false));
}

void TabDiveInformation::on_airtemp_editingFinished()
{
	// If the field wasn't modified by the user, don't post a new undo command.
	// Owing to rounding errors, this might lead to undo commands that have
	// no user visible effects. These can be very confusing.
	if (ui->airtemp->isModified() && current_dive)
		divesEdited(Command::editAirTemp(parseTemperatureToMkelvin(ui->airtemp->text()), false));
}

void TabDiveInformation::on_watertemp_editingFinished()
{
	// If the field wasn't modified by the user, don't post a new undo command.
	// Owing to rounding errors, this might lead to undo commands that have
	// no user visible effects. These can be very confusing.
	if (ui->watertemp->isModified() && current_dive)
		divesEdited(Command::editWaterTemp(parseTemperatureToMkelvin(ui->watertemp->text()), false));
}

void TabDiveInformation::on_atmPressType_currentIndexChanged(int index)
{
	Q_UNUSED(index)
	updateTextBox(COMBO_CHANGED);
}

void TabDiveInformation::on_atmPressVal_editingFinished()
{
	updateTextBox(TEXT_EDITED);
}

void TabDiveInformation::updateTextBox(int event) // Either the text box has been edited or the pressure type has changed.
{                                       // Either way this gets a numeric value and puts it on the text box atmPressVal,
	pressure_t atmpress = { 0 };    // then stores it in dive->surface_pressure.The undo stack for the text box content is
	double altitudeVal;             // maintained even though two independent events trigger saving the text box contents.
	if (current_dive) {
		switch (ui->atmPressType->currentIndex()) {
		case 0:		// If atm pressure in mbar has been selected:
			if (event == TEXT_EDITED)         // this is only triggered by on_atmPressVal_editingFinished()
				atmpress.mbar = ui->atmPressVal->text().toInt();    // use the specified mbar pressure
			else                              // if no pressure has been typed, then show existing dive pressure
				ui->atmPressVal->setText(QString::number(current_dive->surface_pressure.mbar));
			break;
		case 1:		// If an altitude has been specified:
			if (event == TEXT_EDITED) {	// this is only triggered by on_atmPressVal_editingFinished()
				altitudeVal = (ui->atmPressVal->text().toFloat());    // get altitude from text box
				if (prefs.units.length == units::FEET)         // if altitude in feet
					altitudeVal = feet_to_mm(altitudeVal); // imperial: convert altitude from feet to mm
				else
					altitudeVal = altitudeVal * 1000;     // metric: convert altitude from meters to mm
				atmpress.mbar = altitude_to_pressure((int32_t) altitudeVal); // convert altitude (mm) to pressure (mbar)
				ui->atmPressVal->setText(QString::number(atmpress.mbar));
				ui->atmPressType->setCurrentIndex(0);    // reset combobox to mbar
			} else { // i.e. event == COMBO_CHANGED, that is, "m" or "ft" was selected from combobox
				 // Show estimated altitude
				bool ok;
				double convertVal = 0.0010;	// Metric conversion fro mm to m
				int pressure_as_integer = ui->atmPressVal->text().toInt(&ok,10);
				if (ok && ui->atmPressVal->text().length()) {  // Show existing atm press as an altitude:
					if (prefs.units.length == units::FEET) // For imperial units
						convertVal = mm_to_feet(1);    // convert from mm to ft
					ui->atmPressVal->setText(QString::number((int)(pressure_to_altitude(pressure_as_integer) * convertVal)));
				}
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
			divesEdited(Command::editAtmPress(atmpress.mbar, false));      // and save the pressure for undo
	}
}
