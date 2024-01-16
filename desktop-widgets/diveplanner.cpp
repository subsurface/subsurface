// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/diveplanner.h"
#include "desktop-widgets/modeldelegates.h"
#include "desktop-widgets/mainwindow.h"
#include "core/planner.h"
#include "core/qthelper.h"
#include "core/units.h"
#include "core/selection.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/gettextfromc.h"
#include "backend-shared/plannershared.h"

#include "qt-models/cylindermodel.h"
#include "qt-models/models.h"
#include "profile-widget/profilescene.h"
#include "qt-models/diveplannermodel.h"

#include <QShortcut>
#ifndef NO_PRINTING
#include <QPrintDialog>
#include <QPrinter>
#include <QBuffer>
#endif

DivePlannerWidget::DivePlannerWidget(dive &planned_dive, PlannerWidgets *parent)
{
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	CylindersModel *cylinders = DivePlannerPointsModel::instance()->cylindersModel();

	ui.setupUi(this);
	ui.tableWidget->setTitle(tr("Dive planner points"));
	ui.tableWidget->setModel(plannerModel);
	connect(ui.tableWidget, &TableView::itemClicked, plannerModel, &DivePlannerPointsModel::remove);
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::GAS, new AirTypesDelegate(parent->gasModel.get(), this));
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::DIVEMODE, new DiveTypesDelegate(parent->diveTypeModel.get(), this));
	ui.cylinderTableWidget->setTitle(tr("Available gases"));
	ui.cylinderTableWidget->setBtnToolTip(tr("Add cylinder"));
	ui.cylinderTableWidget->setModel(cylinders);
	connect(ui.cylinderTableWidget, &TableView::itemClicked, cylinders, &CylindersModel::remove);
	ui.waterType->setItemData(0, FRESHWATER_SALINITY);
	ui.waterType->setItemData(1, SEAWATER_SALINITY);
	ui.waterType->setItemData(2, EN13319_SALINITY);
	waterTypeUpdateTexts();

	QTableView *view = ui.cylinderTableWidget->view();
	view->setColumnHidden(CylindersModel::START, true);
	view->setColumnHidden(CylindersModel::END, true);
	view->setColumnHidden(CylindersModel::DEPTH, false);
	view->setColumnHidden(CylindersModel::WORKINGPRESS_INT, true);
	view->setColumnHidden(CylindersModel::SIZE_INT, true);
	view->setColumnHidden(CylindersModel::SENSORS, true);
	view->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate(this));
	auto tankUseDelegate = new TankUseDelegate(this);
	tankUseDelegate->setCurrentDC(get_dive_dc(&planned_dive, 0));
	view->setItemDelegateForColumn(CylindersModel::USE, tankUseDelegate);
	connect(ui.cylinderTableWidget, &TableView::addButtonClicked, plannerModel, &DivePlannerPointsModel::addCylinder_clicked);
	connect(ui.tableWidget, &TableView::addButtonClicked, plannerModel, &DivePlannerPointsModel::addDefaultStop);
	connect(cylinders, &CylindersModel::dataChanged, parent, &PlannerWidgets::repopulateGasModel);
	connect(cylinders, &CylindersModel::rowsInserted, parent, &PlannerWidgets::repopulateGasModel);
	connect(cylinders, &CylindersModel::rowsRemoved, parent, &PlannerWidgets::repopulateGasModel);
	connect(cylinders, &CylindersModel::dataChanged, plannerModel, &DivePlannerPointsModel::emitDataChanged);
	connect(cylinders, &CylindersModel::dataChanged, plannerModel, &DivePlannerPointsModel::cylinderModelEdited);
	connect(cylinders, &CylindersModel::rowsInserted, plannerModel, &DivePlannerPointsModel::cylinderModelEdited);
	connect(cylinders, &CylindersModel::rowsRemoved, plannerModel, &DivePlannerPointsModel::cylinderModelEdited);

	ui.tableWidget->setBtnToolTip(tr("Add dive data point"));
	connect(ui.startTime, &QDateEdit::timeChanged, plannerModel, &DivePlannerPointsModel::setStartTime);
	connect(ui.dateEdit, &QDateEdit::dateChanged, plannerModel, &DivePlannerPointsModel::setStartDate);
	connect(ui.ATMPressure, QOverload<int>::of(&QSpinBox::valueChanged), this, &DivePlannerWidget::atmPressureChanged);
	connect(ui.atmHeight, QOverload<int>::of(&QSpinBox::valueChanged), this, &DivePlannerWidget::heightChanged);
	connect(ui.waterType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DivePlannerWidget::waterTypeChanged);
	connect(ui.customSalinity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &DivePlannerWidget::customSalinityChanged);

	// Creating (and canceling) the plan
	replanButton = ui.buttonBox->addButton(tr("Save new"), QDialogButtonBox::ActionRole);
	connect(replanButton, &QAbstractButton::clicked, plannerModel, &DivePlannerPointsModel::saveDuplicatePlan);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, plannerModel, &DivePlannerPointsModel::savePlan);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, plannerModel, &DivePlannerPointsModel::cancelPlan);
	QShortcut *closeKey = new QShortcut(QKeySequence(Qt::Key_Escape), this);
	connect(closeKey, &QShortcut::activated, plannerModel, &DivePlannerPointsModel::cancelPlan);

	// This makes shure the spinbox gets a setMinimum(0) on it so we can't have negative time or depth.
	// Limit segments to a depth of 1000 m/3300 ft and a duration of 100 h. Setting the limit for
	// the depth will be done in settingChanged() since this depends on the chosen units.
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::RUNTIME, new SpinBoxDelegate(0, INT_MAX, 1, this));
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::DURATION, new SpinBoxDelegate(0, 6000, 1, this));
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::CCSETPOINT, new DoubleSpinBoxDelegate(0, 2, 0.01, this));

	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &DivePlannerWidget::settingsChanged);

	/* set defaults. */
	ui.ATMPressure->setValue(1013);
	ui.atmHeight->setValue(0);

	settingsChanged();

	setMinimumWidth(0);
	setMinimumHeight(0);
}

DivePlannerWidget::~DivePlannerWidget()
{
}

void DivePlannerWidget::setReplanButton(bool replan)
{
	replanButton->setVisible(replan);
}

void DivePlannerWidget::setupStartTime(QDateTime startTime)
{
	ui.startTime->setTime(startTime.time());
	ui.dateEdit->setDate(startTime.date());
}

void DivePlannerWidget::setSurfacePressure(int surface_pressure)
{
	ui.ATMPressure->setValue(surface_pressure);
}

void PlannerSettingsWidget::setDiveMode(int mode)
{
	ui.rebreathermode->setCurrentIndex(mode);
}

void DivePlannerWidget::setSalinity(int salinity)
{
	bool mapped = false;
	for (int i = 0; i < ui.waterType->count(); i++) {
		if (salinity == ui.waterType->itemData(i).toInt()) {
			mapped = true;
			ui.waterType->setCurrentIndex(i);
			break;
		}
	}

	if (!mapped) {
		/* Assign to last element "custom" in combo box */
		ui.waterType->setItemData(ui.waterType->count()-1, salinity);
		ui.waterType->setCurrentIndex(ui.waterType->count()-1);
		ui.customSalinity->setEnabled(true);
		ui.customSalinity->setValue(salinity / 10000.0);
	}
	DivePlannerPointsModel::instance()->setSalinity(salinity);
}

void DivePlannerWidget::settingsChanged()
{
	// Adopt units
	int maxDepth;
	if (get_units()->length == units::FEET) {
		ui.atmHeight->setSuffix("ft");
		ui.atmHeight->setMinimum(-300);
		ui.atmHeight->setMaximum(10000);
		maxDepth = 3300;
	} else {
		ui.atmHeight->setSuffix(("m"));
		ui.atmHeight->setMinimum(-100);
		ui.atmHeight->setMaximum(3000);
		maxDepth = 1000;
	}
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::DEPTH, new SpinBoxDelegate(0, maxDepth, 1, this));
	ui.atmHeight->blockSignals(true);
	ui.atmHeight->setValue((int) get_depth_units((int) pressure_to_altitude(DivePlannerPointsModel::instance()->getSurfacePressure()), NULL,NULL));
	ui.atmHeight->blockSignals(false);

	ui.dateEdit->setDisplayFormat(prefs.date_format);
	ui.startTime->setDisplayFormat(prefs.time_format);
}

void DivePlannerWidget::atmPressureChanged(const int pressure)
{
	DivePlannerPointsModel::instance()->setSurfacePressure(pressure);
	ui.atmHeight->blockSignals(true);
	ui.atmHeight->setValue((int) get_depth_units((int) pressure_to_altitude(pressure), NULL,NULL));
	ui.atmHeight->blockSignals(false);
}

void DivePlannerWidget::heightChanged(const int height)
{						// height is in ft or in meters
	int pressure = (int) (altitude_to_pressure(units_to_depth((double) height).mm));
	ui.ATMPressure->blockSignals(true);
	ui.ATMPressure->setValue(pressure);
	ui.ATMPressure->blockSignals(false);
	DivePlannerPointsModel::instance()->setSurfacePressure(pressure);
}

void DivePlannerWidget::waterTypeUpdateTexts()
{
	double density;
	/* Do not set text in last/custom element */
	for (int i = 0; i < ui.waterType->count()-1; i++) {
		if (ui.waterType->itemData(i) != QVariant::Invalid) {
			QString densityText = ui.waterType->itemText(i).split("(")[0].trimmed();
			density = ui.waterType->itemData(i).toInt() / 10000.0;
			densityText.append(QString(" (%L1%2)").arg(density, 0, 'f', 2).arg(tr("kg/ℓ")));
			ui.waterType->setItemText(i, densityText);
		}
	}
}

void DivePlannerWidget::waterTypeChanged(const int index)
{
	ui.customSalinity->setEnabled(index == ui.waterType->count() - 1);
	ui.customSalinity->setValue(ui.waterType->itemData(index).toInt() / 10000.0);
	DivePlannerPointsModel::instance()->setSalinity(ui.waterType->itemData(index).toInt());
}

void DivePlannerWidget::customSalinityChanged(double density)
{
	if (ui.customSalinity->isEnabled()) {
		int newSalinity = (int)(density * 10000.0);
		ui.waterType->setItemData(ui.waterType->count() - 1, newSalinity);
		DivePlannerPointsModel::instance()->setSalinity(newSalinity);
	}
}

void PlannerSettingsWidget::disableDecoElements(int mode, divemode_t rebreathermode)
{
	if (mode == RECREATIONAL) {
		ui.label_gflow->setDisabled(false);
		ui.label_gfhigh->setDisabled(false);
		ui.gflow->setDisabled(false);
		ui.gfhigh->setDisabled(false);
		ui.lastStop->setDisabled(true);
		ui.backgasBreaks->setDisabled(true);
		ui.backgasBreaks->blockSignals(true);
		ui.backgasBreaks->setChecked(false);
		ui.backgasBreaks->blockSignals(false);
		ui.bailout->setDisabled(true);
		ui.bailout->blockSignals(true);
		ui.bailout->setChecked(false);
		ui.bailout->blockSignals(false);
		ui.bottompo2->setDisabled(false);
		ui.decopo2->setDisabled(true);
		ui.safetystop->setDisabled(false);
		ui.label_reserve_gas->setDisabled(false);
		ui.reserve_gas->setDisabled(false);
		ui.label_vpmb_conservatism->setDisabled(true);
		ui.vpmb_conservatism->setDisabled(true);
		ui.switch_at_req_stop->setDisabled(true);
		ui.min_switch_duration->setDisabled(true);
		ui.surface_segment->setDisabled(true);
		ui.label_min_switch_duration->setDisabled(true);
		ui.sacfactor->setDisabled(true);
		ui.problemsolvingtime->setDisabled(true);
		ui.sacfactor->blockSignals(true);
		ui.problemsolvingtime->blockSignals(true);
		ui.sacfactor->setValue(2.0);
		ui.problemsolvingtime->setValue(0);
		ui.sacfactor->blockSignals(false);
		ui.problemsolvingtime->blockSignals(false);
		ui.display_variations->setDisabled(true);
	} else if (mode == VPMB) {
		ui.label_gflow->setDisabled(true);
		ui.label_gfhigh->setDisabled(true);
		ui.gflow->setDisabled(true);
		ui.gfhigh->setDisabled(true);
		ui.lastStop->setDisabled(false);
		if (prefs.last_stop) {
			ui.backgasBreaks->setDisabled(false);
			ui.backgasBreaks->blockSignals(true);
			ui.backgasBreaks->setChecked(prefs.doo2breaks);
			ui.backgasBreaks->blockSignals(false);
		} else {
			ui.backgasBreaks->setDisabled(true);
			ui.backgasBreaks->blockSignals(true);
			ui.backgasBreaks->setChecked(false);
			ui.backgasBreaks->blockSignals(false);
		}
		ui.bailout->setDisabled(!(rebreathermode == CCR || rebreathermode == PSCR));
		ui.bottompo2->setDisabled(false);
		ui.decopo2->setDisabled(false);
		ui.safetystop->setDisabled(true);
		ui.label_reserve_gas->setDisabled(true);
		ui.reserve_gas->setDisabled(true);
		ui.label_vpmb_conservatism->setDisabled(false);
		ui.vpmb_conservatism->setDisabled(false);
		ui.switch_at_req_stop->setDisabled(false);
		ui.min_switch_duration->setDisabled(false);
		ui.surface_segment->setDisabled(false);
		ui.label_min_switch_duration->setDisabled(false);
		ui.sacfactor->setDisabled(false);
		ui.problemsolvingtime->setDisabled(false);
		ui.sacfactor->setValue(PlannerShared::sacfactor());
		ui.problemsolvingtime->setValue(prefs.problemsolvingtime);
		ui.display_variations->setDisabled(false);
	} else if (mode == BUEHLMANN) {
		ui.label_gflow->setDisabled(false);
		ui.label_gfhigh->setDisabled(false);
		ui.gflow->setDisabled(false);
		ui.gfhigh->setDisabled(false);
		ui.lastStop->setDisabled(false);
		if (prefs.last_stop) {
			ui.backgasBreaks->setDisabled(false);
			ui.backgasBreaks->blockSignals(true);
			ui.backgasBreaks->setChecked(prefs.doo2breaks);
			ui.backgasBreaks->blockSignals(false);
		} else {
			ui.backgasBreaks->setDisabled(true);
			ui.backgasBreaks->blockSignals(true);
			ui.backgasBreaks->setChecked(false);
			ui.backgasBreaks->blockSignals(false);
		}
		ui.bailout->setDisabled(!(rebreathermode == CCR || rebreathermode == PSCR));
		ui.bottompo2->setDisabled(false);
		ui.decopo2->setDisabled(false);
		ui.safetystop->setDisabled(true);
		ui.label_reserve_gas->setDisabled(true);
		ui.reserve_gas->setDisabled(true);
		ui.label_vpmb_conservatism->setDisabled(true);
		ui.vpmb_conservatism->setDisabled(true);
		ui.switch_at_req_stop->setDisabled(false);
		ui.min_switch_duration->setDisabled(false);
		ui.surface_segment->setDisabled(false);
		ui.label_min_switch_duration->setDisabled(false);
		ui.sacfactor->setDisabled(false);
		ui.problemsolvingtime->setDisabled(false);
		ui.sacfactor->setValue(PlannerShared::sacfactor());
		ui.problemsolvingtime->setValue(prefs.problemsolvingtime);
		ui.display_variations->setDisabled(false);
	}
}

void PlannerSettingsWidget::disableBackgasBreaks(bool enabled)
{
	if (prefs.planner_deco_mode == RECREATIONAL)
		return;

	if (enabled) {
		ui.backgasBreaks->setDisabled(false);
		ui.backgasBreaks->blockSignals(true);
		ui.backgasBreaks->setChecked(prefs.doo2breaks);
		ui.backgasBreaks->blockSignals(false);
	} else {
		ui.backgasBreaks->setDisabled(true);
		ui.backgasBreaks->blockSignals(true);
		ui.backgasBreaks->setChecked(false);
		ui.backgasBreaks->blockSignals(false);
	}
}

PlannerSettingsWidget::PlannerSettingsWidget(PlannerWidgets *parent)
{
	ui.setupUi(this);

	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	plannerModel->getDiveplan().bottomsac = prefs.bottomsac;
	plannerModel->getDiveplan().decosac = prefs.decosac;

	updateUnitsUI();
	ui.lastStop->setChecked(prefs.last_stop);
	ui.verbatim_plan->setChecked(prefs.verbatim_plan);
	ui.display_duration->setChecked(prefs.display_duration);
	ui.display_runtime->setChecked(prefs.display_runtime);
	ui.display_transitions->setChecked(prefs.display_transitions);
	ui.display_variations->setChecked(prefs.display_variations);
	ui.safetystop->setChecked(prefs.safetystop);
	ui.sacfactor->setValue(PlannerShared::sacfactor());
	ui.problemsolvingtime->setValue(prefs.problemsolvingtime);
	ui.bottompo2->setValue(PlannerShared::bottompo2());
	ui.decopo2->setValue(PlannerShared::decopo2());
	ui.backgasBreaks->setChecked(prefs.doo2breaks);
	PlannerShared::set_dobailout(false);
	setBailoutVisibility(false);
	ui.o2narcotic->setChecked(prefs.o2narcotic);
	ui.drop_stone_mode->setChecked(prefs.drop_stone_mode);
	ui.switch_at_req_stop->setChecked(prefs.switch_at_req_stop);
	ui.min_switch_duration->setValue(PlannerShared::min_switch_duration());
	ui.surface_segment->setValue(PlannerShared::surface_segment());
	ui.recreational_deco->setChecked(prefs.planner_deco_mode == RECREATIONAL);
	ui.buehlmann_deco->setChecked(prefs.planner_deco_mode == BUEHLMANN);
	ui.vpmb_deco->setChecked(prefs.planner_deco_mode == VPMB);
	disableDecoElements((int) prefs.planner_deco_mode, OC);

	// should be the same order as in dive_comp_type!
	QStringList rebreather_modes = QStringList();
	for (int i = 0; i < FREEDIVE; i++)
		rebreather_modes.append(gettextFromC::tr(divemode_text_ui[i]));
	ui.rebreathermode->insertItems(0, rebreather_modes);

	connect(ui.recreational_deco, &QAbstractButton::clicked, [] { PlannerShared::set_planner_deco_mode(RECREATIONAL); });
	connect(ui.buehlmann_deco, &QAbstractButton::clicked, [] { PlannerShared::set_planner_deco_mode(BUEHLMANN); });
	connect(ui.vpmb_deco, &QAbstractButton::clicked, [] { PlannerShared::set_planner_deco_mode(VPMB); });

	connect(ui.lastStop, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setLastStop6m);
	connect(ui.lastStop, &QAbstractButton::toggled, this, &PlannerSettingsWidget::disableBackgasBreaks);
	connect(ui.verbatim_plan, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setVerbatim);
	connect(ui.display_duration, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setDisplayDuration);
	connect(ui.display_runtime, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setDisplayRuntime);
	connect(ui.display_transitions, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setDisplayTransitions);
	connect(ui.display_variations, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setDisplayVariations);
	connect(ui.safetystop, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setSafetyStop);
	connect(ui.reserve_gas, QOverload<int>::of(&QSpinBox::valueChanged), &PlannerShared::set_reserve_gas);
	connect(ui.ascRate75, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setAscrate75Display);
	connect(ui.ascRate50, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setAscrate50Display);
	connect(ui.ascRateStops, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setAscratestopsDisplay);
	connect(ui.ascRateLast6m, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setAscratelast6mDisplay);
	connect(ui.descRate, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setDescrateDisplay);
	connect(ui.drop_stone_mode, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setDropStoneMode);
	connect(ui.gfhigh, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setGFHigh);
	connect(ui.gflow, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setGFLow);
	connect(ui.vpmb_conservatism, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setVpmbConservatism);
	connect(ui.backgasBreaks, &QAbstractButton::toggled, this, &PlannerSettingsWidget::setBackgasBreaks);
	connect(ui.bailout, &QAbstractButton::toggled, &PlannerShared::set_dobailout);
	connect(ui.o2narcotic, &QAbstractButton::toggled, &PlannerShared::set_o2narcotic);
	connect(ui.switch_at_req_stop, &QAbstractButton::toggled, plannerModel, &DivePlannerPointsModel::setSwitchAtReqStop);
	connect(ui.min_switch_duration, QOverload<int>::of(&QSpinBox::valueChanged), &PlannerShared::set_min_switch_duration);
	connect(ui.surface_segment, QOverload<int>::of(&QSpinBox::valueChanged), &PlannerShared::set_surface_segment);
	connect(ui.rebreathermode, QOverload<int>::of(&QComboBox::currentIndexChanged), plannerModel, &DivePlannerPointsModel::setRebreatherMode);
	connect(ui.rebreathermode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlannerSettingsWidget::setBailoutVisibility);

	connect(ui.recreational_deco, &QAbstractButton::clicked, [this, parent] { disableDecoElements(RECREATIONAL, parent->getRebreatherMode()); });
	connect(ui.buehlmann_deco, &QAbstractButton::clicked, [this, parent] { disableDecoElements(BUEHLMANN, parent->getRebreatherMode()); });
	connect(ui.vpmb_deco, &QAbstractButton::clicked, [this, parent] { disableDecoElements(VPMB, parent->getRebreatherMode()); });

	connect(ui.sacfactor, QOverload<double>::of(&QDoubleSpinBox::valueChanged), &PlannerShared::set_sacfactor);
	connect(ui.problemsolvingtime, QOverload<int>::of(&QSpinBox::valueChanged), plannerModel, &DivePlannerPointsModel::setProblemSolvingTime);
	connect(ui.bottompo2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), &PlannerShared::set_bottompo2);
	connect(ui.decopo2, QOverload<double>::of(&QDoubleSpinBox::valueChanged), &PlannerShared::set_decopo2);
	connect(ui.bestmixEND, QOverload<int>::of(&QSpinBox::valueChanged), &PlannerShared::set_bestmixend);
	connect(ui.bottomSAC, QOverload<double>::of(&QDoubleSpinBox::valueChanged), &PlannerShared::set_bottomsac);
	connect(ui.decoStopSAC, QOverload<double>::of(&QDoubleSpinBox::valueChanged), &PlannerShared::set_decosac);
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &PlannerSettingsWidget::settingsChanged);

	settingsChanged();
	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);
	ui.vpmb_conservatism->setValue(prefs.vpmb_conservatism);

	ui.ascRate75->setKeyboardTracking(false);
	ui.ascRate50->setKeyboardTracking(false);
	ui.ascRateLast6m->setKeyboardTracking(false);
	ui.ascRateStops->setKeyboardTracking(false);
	ui.descRate->setKeyboardTracking(false);
	ui.gfhigh->setKeyboardTracking(false);
	ui.gflow->setKeyboardTracking(false);

	setMinimumWidth(0);
	setMinimumHeight(0);
}

void PlannerSettingsWidget::updateUnitsUI()
{
	DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();
	ui.ascRate75->setValue(plannerModel->ascrate75Display());
	ui.ascRate50->setValue(plannerModel->ascrate50Display());
	ui.ascRateStops->setValue(plannerModel->ascratestopsDisplay());
	ui.ascRateLast6m->setValue(plannerModel->ascratelast6mDisplay());
	ui.descRate->setValue(lrint(plannerModel->descrateDisplay()));
	ui.bestmixEND->setValue(lrint(get_depth_units(prefs.bestmixend.mm, NULL, NULL)));
}

PlannerSettingsWidget::~PlannerSettingsWidget()
{
}

void PlannerSettingsWidget::settingsChanged()
{
	QString vs;
	// don't recurse into setting the value from the ui when setting the ui from the value
	ui.bottomSAC->blockSignals(true);
	ui.decoStopSAC->blockSignals(true);
	if (get_units()->length == units::FEET) {
		vs.append(tr("ft/min"));
		ui.lastStop->setText(tr("Last stop at 20ft"));
		ui.asc50to6->setText(tr("50% avg. depth to 20ft"));
		ui.asc6toSurf->setText(tr("20ft to surface"));
		ui.bestmixEND->setSuffix(tr("ft"));
	} else {
		vs.append(tr("m/min"));
		ui.lastStop->setText(tr("Last stop at 6m"));
		ui.asc50to6->setText(tr("50% avg. depth to 6m"));
		ui.asc6toSurf->setText(tr("6m to surface"));
		ui.bestmixEND->setSuffix(tr("m"));
	}
	if (get_units()->volume == units::CUFT) {
		ui.bottomSAC->setSuffix(tr("cuft/min"));
		ui.decoStopSAC->setSuffix(tr("cuft/min"));
		ui.bottomSAC->setDecimals(2);
		ui.bottomSAC->setSingleStep(0.1);
		ui.decoStopSAC->setDecimals(2);
		ui.decoStopSAC->setSingleStep(0.1);
		ui.bottomSAC->setValue(PlannerShared::bottomsac());
		ui.decoStopSAC->setValue(PlannerShared::decosac());
	} else {
		ui.bottomSAC->setSuffix(tr("ℓ/min"));
		ui.decoStopSAC->setSuffix(tr("ℓ/min"));
		ui.bottomSAC->setDecimals(0);
		ui.bottomSAC->setSingleStep(1);
		ui.decoStopSAC->setDecimals(0);
		ui.decoStopSAC->setSingleStep(1);
		ui.bottomSAC->setValue(PlannerShared::bottomsac());
		ui.decoStopSAC->setValue(PlannerShared::decosac());
	}
	if (get_units()->pressure == units::BAR) {
		ui.reserve_gas->setSuffix(tr("bar"));
		ui.reserve_gas->setSingleStep(1);
		ui.reserve_gas->setValue(prefs.reserve_gas / 1000);
		ui.reserve_gas->setMaximum(300);
	} else {
		ui.reserve_gas->setSuffix(tr("psi"));
		ui.reserve_gas->setSingleStep(10);
		ui.reserve_gas->setMaximum(5000);
		ui.reserve_gas->setValue(lrint(mbar_to_PSI(prefs.reserve_gas)));
	}

	ui.bottomSAC->blockSignals(false);
	ui.decoStopSAC->blockSignals(false);
	updateUnitsUI();
	ui.ascRate75->setSuffix(vs);
	ui.ascRate50->setSuffix(vs);
	ui.ascRateStops->setSuffix(vs);
	ui.ascRateLast6m->setSuffix(vs);
	ui.descRate->setSuffix(vs);
}

void PlannerSettingsWidget::setBackgasBreaks(bool dobreaks)
{
	PlannerShared::set_doo2breaks(dobreaks);
}

void PlannerSettingsWidget::setBailoutVisibility(int mode)
{
	ui.bailout->setDisabled(!(mode == CCR || mode == PSCR));
	ui.sacFactor->setDisabled(mode == CCR);
}

PlannerDetails::PlannerDetails(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
#ifdef NO_PRINTING
	ui.printPlan->hide();
#endif
}

void PlannerDetails::setPlanNotes(QString plan)
{
	ui.divePlanOutput->setHtml(plan);
}

PlannerWidgets::PlannerWidgets() :
	planned_dive(alloc_dive()),
	gasModel(std::make_unique<GasSelectionModel>()),
	diveTypeModel(std::make_unique<DiveTypeSelectionModel>()),
	plannerWidget(*planned_dive, this),
	plannerSettingsWidget(this)
{
	connect(plannerDetails.printPlan(), &QPushButton::pressed, this, &PlannerWidgets::printDecoPlan);
	connect(DivePlannerPointsModel::instance(), &DivePlannerPointsModel::calculatedPlanNotes,
		&plannerDetails, &PlannerDetails::setPlanNotes);
}

PlannerWidgets::~PlannerWidgets()
{
}

struct dive *PlannerWidgets::getDive() const
{
	return planned_dive.get();
}

divemode_t PlannerWidgets::getRebreatherMode() const
{
	return planned_dive->dc.divemode;
}

void PlannerWidgets::preparePlanDive(const dive *currentDive)
{
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);
	// create a simple starting dive, using the first gas from the just copied cylinders
	DivePlannerPointsModel::instance()->createSimpleDive(planned_dive.get());

	// plan the dive in the same mode as the currently selected one
	if (currentDive) {
		plannerSettingsWidget.setDiveMode(currentDive->dc.divemode);
		plannerSettingsWidget.setBailoutVisibility(currentDive->dc.divemode);
		if (currentDive->salinity)
			plannerWidget.setSalinity(currentDive->salinity);
		else	// No salinity means salt water
			plannerWidget.setSalinity(SEAWATER_SALINITY);
	}
}

void PlannerWidgets::planDive()
{
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);

	repopulateGasModel();
	plannerWidget.setReplanButton(false);
	plannerWidget.setupStartTime(timestampToDateTime(planned_dive->when));	// This will reload the profile!
}

void PlannerWidgets::prepareReplanDive(const dive *d)
{
	copy_dive(d, planned_dive.get());
}

void PlannerWidgets::replanDive(int currentDC)
{
	DivePlannerPointsModel::instance()->setPlanMode(DivePlannerPointsModel::PLAN);
	DivePlannerPointsModel::instance()->loadFromDive(planned_dive.get(), currentDC);

	plannerWidget.setReplanButton(true);
	plannerWidget.setupStartTime(timestampToDateTime(planned_dive->when));
	if (planned_dive->surface_pressure.mbar)
		plannerWidget.setSurfacePressure(planned_dive->surface_pressure.mbar);
	if (planned_dive->salinity)
		plannerWidget.setSalinity(planned_dive->salinity);
	reset_cylinders(planned_dive.get(), true);
	DivePlannerPointsModel::instance()->cylindersModel()->updateDive(planned_dive.get(), currentDC);
}

void PlannerWidgets::repopulateGasModel()
{
	gasModel->repopulate(planned_dive.get());
}

void PlannerWidgets::printDecoPlan()
{
#ifndef NO_PRINTING
	char *disclaimer = get_planner_disclaimer_formatted();
	// Prepend a logo and a disclaimer to the plan.
	// Save the old plan so that it can be restored at the end of the function.
	QString origPlan = plannerDetails.divePlanOutput()->toHtml();
	QString diveplan = QStringLiteral("<img height=50 src=\":subsurface-icon\"> ") +
			   QString(disclaimer) + origPlan;
	free(disclaimer);

	QPrinter printer;
	QPrintDialog dialog(&printer, MainWindow::instance());
	dialog.setWindowTitle(tr("Print runtime table"));
	if (dialog.exec() != QDialog::Accepted)
		return;

	/* render the profile as a pixmap that is inserted as base64 data into a HTML <img> tag
	 * make it fit a page width defined by 2 cm margins via QTextDocument->print() (cannot be changed?)
	 * the height of the profile is 40% of the page height.
	 */
	QSizeF renderSize = printer.pageRect(QPrinter::Inch).size();
	const qreal marginsInch = 1.57480315; // = (2 x 2cm) / 2.45cm/inch
	renderSize.setWidth((renderSize.width() - marginsInch) * printer.resolution());
	renderSize.setHeight(((renderSize.height() - marginsInch) * printer.resolution()) / 2.5);

	QPixmap pixmap(renderSize.toSize());
	QPainter painter(&pixmap);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	auto profile = std::make_unique<ProfileScene>(1.0, true, false);
	profile->draw(&painter, QRect(0, 0, pixmap.width(), pixmap.height()), planned_dive.get(), 0, DivePlannerPointsModel::instance(), true);

	QByteArray byteArray;
	QBuffer buffer(&byteArray);
	pixmap.save(&buffer, "PNG");
	QString profileImage = QString("<img src=\"data:image/png;base64,") + byteArray.toBase64() + "\"/><br><br>";
	diveplan = profileImage + diveplan;

	plannerDetails.divePlanOutput()->setHtml(diveplan);
	plannerDetails.divePlanOutput()->print(&printer);
	plannerDetails.divePlanOutput()->setHtml(origPlan); // restore original plan
#endif
}
