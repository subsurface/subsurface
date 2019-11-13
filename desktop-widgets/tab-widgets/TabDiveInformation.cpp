// SPDX-License-Identifier: GPL-2.0
#include "TabDiveInformation.h"
#include "ui_TabDiveInformation.h"
#include "desktop-widgets/mainwindow.h" // TODO: Only used temporarilly for edit mode changes
#include "profile-widget/profilewidget2.h"
#include "../tagwidget.h"
#include "commands/command.h"
#include "core/units.h"
#include "core/dive.h"
#include "core/qthelper.h"
#include "core/statistics.h"
#include "core/display.h"
#include "core/divelist.h"

#define COMBO_CHANGED 0
#define TEXT_EDITED 1
#define CSS_SET_HEADING_BLUE "QLabel { color: mediumblue;} "

TabDiveInformation::TabDiveInformation(QWidget *parent) : TabBase(parent), ui(new Ui::TabDiveInformation())
{
	ui->setupUi(this);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &TabDiveInformation::divesChanged);
	QStringList atmPressTypes { "mbar", get_depth_unit() ,"use dc"};
	ui->atmPressType->insertItems(0, atmPressTypes);
	pressTypeIndex = 0;
	// This needs to be the same order as enum dive_comp_type in dive.h!
	QStringList types;
	for (int i = 0; i < NUM_DIVEMODE; i++)
		types.append(gettextFromC::tr(divemode_text_ui[i]));
	ui->diveType->insertItems(0, types);
	connect(ui->diveType, SIGNAL(currentIndexChanged(int)), this, SLOT(diveModeChanged(int)));
	QString CSSSetSmallLabel = "QLabel { color: mediumblue; font-size: " +                        /* // Using label height ... */
		QString::number((int)(0.5 + ui->diveHeadingLabel->geometry().height() * 0.66)) + "px;}"; // .. set CSS font size of star widget subscripts
	ui->scrollAreaWidgetContents_3->setStyleSheet("QGroupBox::title { color: mediumblue;} ");
	ui->diveModeBox->setStyleSheet("QGroupBox{ padding: 0;} ");
	ui->diveHeadingLabel->setStyleSheet(CSS_SET_HEADING_BLUE);
	ui->gasHeadingLabel->setStyleSheet(CSS_SET_HEADING_BLUE);
	ui->environmentHeadingLabel->setStyleSheet(CSS_SET_HEADING_BLUE);
	ui->groupBox_visibility->setStyleSheet(CSSSetSmallLabel);
	QAction *action = new QAction(tr("OK"), this);
	connect(action, &QAction::triggered, this, &TabDiveInformation::closeWarning);
	ui->multiDiveWarningMessage->addAction(action);
	action = new QAction(tr("Undo"), this);
	connect(action, &QAction::triggered, Command::undoAction(this), &QAction::trigger);
	connect(action, &QAction::triggered, this, &TabDiveInformation::closeWarning);
	ui->multiDiveWarningMessage->addAction(action);
	ui->multiDiveWarningMessage->hide();
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

// Update fields that depend on the dive profile
void TabDiveInformation::updateProfile()
{
	ui->maxcnsText->setText(QString("%1\%").arg(current_dive->maxcns));
	ui->otuText->setText(QString("%1").arg(current_dive->otu));
	ui->maximumDepthText->setText(get_depth_string(current_dive->maxdepth, true));
	ui->averageDepthText->setText(get_depth_string(current_dive->meandepth, true));

	volume_t *gases = get_gas_used(current_dive);
	QString volumes;
	std::vector<int> mean(current_dive->cylinders.nr), duration(current_dive->cylinders.nr);
	per_cylinder_mean_depth(current_dive, select_dc(current_dive), &mean[0], &duration[0]);
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
		QString pressStr;
		pressStr.sprintf("%d",current_dive->surface_pressure.mbar);
		ui->atmPressVal->setText(pressStr);		// else display atm pressure
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

void TabDiveInformation::updateSalinity()
{
	if (current_dive->salinity) {                 // Set up the salinity string:
		ui->salinityText->setText(QString("%1g/ℓ").arg(current_dive->salinity / 10.0));
		if (current_dive->salinity < 10050)   // Set water type indicator:
			ui->waterTypeText->setText(tr("Fresh"));
		else if (current_dive->salinity < 10190)
			ui->waterTypeText->setText(tr("Salty"));
		else if (current_dive->salinity < 10210) // (EN13319 = 1.019 - 1.021 g/l)
			ui->waterTypeText->setText(tr("EN13319"));
		else ui->waterTypeText->setText(tr("Salt"));
	} else {
		ui->salinityText->clear();
		ui->waterTypeText->clear();
	}
}

void TabDiveInformation::updateData()
{
	if (!current_dive) {
		clear();
		return;
	}

	updateProfile();
	updateWhen();
	ui->watertemp->setText(get_temperature_string(current_dive->watertemp, true));
	ui->airtemp->setText(get_temperature_string(current_dive->airtemp, true));
	ui->atmPressType->setItemText(1, get_depth_unit());  // Check for changes in depth unit (imperial/metric)
	ui->atmPressType->setCurrentIndex(0);                // Set the atmospheric pressure combo box to mbar
	updateMode(current_dive);
	updateSalinity();
	ui->visibility->setCurrentStars(current_dive->visibility);
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveInformation::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If the current dive is not in list of changed dives, do nothing
	if (!current_dive || !dives.contains(current_dive))
		return;

	if (field.visibility)
		ui->visibility->setCurrentStars(current_dive->visibility);
	if (field.mode)
		updateMode(current_dive);
	if (field.duration || field.depth || field.mode)
		updateProfile();
	if (field.air_temp)
		ui->airtemp->setText(get_temperature_string(current_dive->airtemp, true));
	if (field.water_temp)
		ui->watertemp->setText(get_temperature_string(current_dive->watertemp, true));
	if (field.atm_press)
		ui->atmPressVal->setText(ui->atmPressVal->text().sprintf("%d",current_dive->surface_pressure.mbar));
	if (field.salinity)
		updateSalinity();
}


void TabDiveInformation::on_visibility_valueChanged(int value)
{
	if (current_dive)
		divesEdited(Command::editVisibility(value, false));
}

void TabDiveInformation::updateMode(struct dive *d)
{
	ui->diveType->setCurrentIndex(get_dive_dc(d, dc_number)->divemode);
	MainWindow::instance()->graphics->replot();
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
void TabDiveInformation::on_atmPressType_currentIndexChanged(int index) { updateTextBox(COMBO_CHANGED); }

void TabDiveInformation::on_atmPressVal_editingFinished() { updateTextBox(TEXT_EDITED); }

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
				ui->atmPressVal->setText(ui->atmPressVal->text().sprintf("%d",atmpress.mbar));
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


