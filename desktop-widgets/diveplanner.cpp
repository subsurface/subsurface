// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/diveplanner.h"
#include "desktop-widgets/modeldelegates.h"
#include "desktop-widgets/mainwindow.h"
#include "core/planner.h"
#include "core/qthelper.h"
#include "core/units.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/gettextfromc.h"
#include "backend-shared/plannershared.h"

#include "qt-models/cylindermodel.h"
#include "qt-models/models.h"
#include "profile-widget/profilewidget2.h"
#include "qt-models/diveplannermodel.h"

#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QShortcut>

#define TIME_INITIAL_MAX 30

#define MAX_DEPTH M_OR_FT(150, 450)
#define MIN_DEPTH M_OR_FT(20, 60)

static DivePlannerPointsModel* plannerModel = DivePlannerPointsModel::instance();

DiveHandler::DiveHandler() : QGraphicsEllipseItem()
{
	setRect(-5, -5, 10, 10);
	setFlags(ItemIgnoresTransformations | ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
	setBrush(Qt::white);
	setZValue(2);
	t.start();
}

int DiveHandler::parentIndex()
{
	ProfileWidget2 *view = qobject_cast<ProfileWidget2 *>(scene()->views().first());
	return view->handles.indexOf(this);
}

void DiveHandler::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	QMenu m;
	// Don't have a gas selection for the last point
	emit released();
	QModelIndex index = plannerModel->index(parentIndex(), DivePlannerPointsModel::GAS);
	if (index.sibling(index.row() + 1, index.column()).isValid()) {
		GasSelectionModel *model = GasSelectionModel::instance();
		model->repopulate();
		int rowCount = model->rowCount();
		for (int i = 0; i < rowCount; i++) {
			QAction *action = new QAction(&m);
			action->setText(model->data(model->index(i, 0), Qt::DisplayRole).toString());
			action->setData(i);
			connect(action, SIGNAL(triggered(bool)), this, SLOT(changeGas()));
			m.addAction(action);
		}
	}
	// don't allow removing the last point
	if (plannerModel->rowCount() > 1) {
		m.addSeparator();
		m.addAction(gettextFromC::tr("Remove this point"), this, &DiveHandler::selfRemove);
		m.exec(event->screenPos());
	}
}

void DiveHandler::selfRemove()
{
	setSelected(true);
	ProfileWidget2 *view = qobject_cast<ProfileWidget2 *>(scene()->views().first());
	view->keyDeleteAction();
}

void DiveHandler::changeGas()
{
	QAction *action = qobject_cast<QAction *>(sender());
	QModelIndex index = plannerModel->index(parentIndex(), DivePlannerPointsModel::GAS);
	plannerModel->gasChange(index.sibling(index.row() + 1, index.column()), action->data().toInt());
}

void DiveHandler::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (t.elapsed() < 40)
		return;
	t.start();

	ProfileWidget2 *view = qobject_cast<ProfileWidget2*>(scene()->views().first());
	if(view->isPointOutOfBoundaries(event->scenePos()))
		return;

	QGraphicsEllipseItem::mouseMoveEvent(event);
	emit moved();
}

void DiveHandler::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mousePressEvent(event);
	emit clicked();
}

void DiveHandler::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mouseReleaseEvent(event);
	emit released();
}

DivePlannerWidget::DivePlannerWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	ui.setupUi(this);
	ui.dateEdit->setDisplayFormat(prefs.date_format);
	ui.tableWidget->setTitle(tr("Dive planner points"));
	ui.tableWidget->setModel(plannerModel);
	connect(ui.tableWidget, &TableView::itemClicked, plannerModel, &DivePlannerPointsModel::remove);
	plannerModel->setRecalc(true);
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::GAS, new AirTypesDelegate(this));
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::DIVEMODE, new DiveTypesDelegate(this));
	ui.cylinderTableWidget->setTitle(tr("Available gases"));
	ui.cylinderTableWidget->setBtnToolTip(tr("Add cylinder"));
	ui.cylinderTableWidget->setModel(CylindersModel::instance());
	connect(ui.cylinderTableWidget, &TableView::itemClicked, CylindersModel::instance(), &CylindersModel::remove);
	ui.waterType->setItemData(0, FRESHWATER_SALINITY);
	ui.waterType->setItemData(1, SEAWATER_SALINITY);
	ui.waterType->setItemData(2, EN13319_SALINITY);
	waterTypeUpdateTexts();

	QTableView *view = ui.cylinderTableWidget->view();
	view->setColumnHidden(CylindersModel::START, true);
	view->setColumnHidden(CylindersModel::END, true);
	view->setColumnHidden(CylindersModel::DEPTH, false);
	view->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate(this));
	connect(ui.cylinderTableWidget, SIGNAL(addButtonClicked()), plannerModel, SLOT(addCylinder_clicked()));
	connect(ui.tableWidget, SIGNAL(addButtonClicked()), plannerModel, SLOT(addStop()));

	connect(CylindersModel::instance(), SIGNAL(dataChanged(QModelIndex, QModelIndex)),
		GasSelectionModel::instance(), SLOT(repopulate()));
	connect(CylindersModel::instance(), SIGNAL(rowsInserted(QModelIndex, int, int)),
		GasSelectionModel::instance(), SLOT(repopulate()));
	connect(CylindersModel::instance(), SIGNAL(rowsRemoved(QModelIndex, int, int)),
		GasSelectionModel::instance(), SLOT(repopulate()));
	connect(CylindersModel::instance(), SIGNAL(dataChanged(QModelIndex, QModelIndex)),
		plannerModel, SLOT(emitDataChanged()));
	connect(CylindersModel::instance(), SIGNAL(dataChanged(QModelIndex, QModelIndex)),
		plannerModel, SIGNAL(cylinderModelEdited()));
	connect(CylindersModel::instance(), SIGNAL(rowsInserted(QModelIndex, int, int)),
		plannerModel, SIGNAL(cylinderModelEdited()));
	connect(CylindersModel::instance(), SIGNAL(rowsRemoved(QModelIndex, int, int)),
		plannerModel, SIGNAL(cylinderModelEdited()));
	connect(plannerModel, SIGNAL(calculatedPlanNotes()), MainWindow::instance(), SLOT(setPlanNotes()));


	ui.tableWidget->setBtnToolTip(tr("Add dive data point"));
	connect(ui.startTime, SIGNAL(timeChanged(QTime)), plannerModel, SLOT(setStartTime(QTime)));
	connect(ui.dateEdit, SIGNAL(dateChanged(QDate)), plannerModel, SLOT(setStartDate(QDate)));
	connect(ui.ATMPressure, SIGNAL(valueChanged(int)), this, SLOT(atmPressureChanged(int)));
	connect(ui.atmHeight, SIGNAL(valueChanged(int)), this, SLOT(heightChanged(int)));
	connect(ui.waterType, SIGNAL(currentIndexChanged(int)), this, SLOT(waterTypeChanged(int)));
	connect(ui.customSalinity, SIGNAL(valueChanged(double)), this, SLOT(customSalinityChanged(double)));
	connect(plannerModel, SIGNAL(startTimeChanged(QDateTime)), this, SLOT(setupStartTime(QDateTime)));

	// Creating (and canceling) the plan
	replanButton = ui.buttonBox->addButton(tr("Save new"), QDialogButtonBox::ActionRole);
	connect(replanButton, SIGNAL(clicked()), plannerModel, SLOT(saveDuplicatePlan()));
	connect(ui.buttonBox, SIGNAL(accepted()), plannerModel, SLOT(savePlan()));
	connect(ui.buttonBox, SIGNAL(rejected()), plannerModel, SLOT(cancelPlan()));
	QShortcut *closeKey = new QShortcut(QKeySequence(Qt::Key_Escape), this);
	connect(closeKey, SIGNAL(activated()), plannerModel, SLOT(cancelPlan()));

	// This makes shure the spinbox gets a setMinimum(0) on it so we can't have negative time or depth.
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::DEPTH, new SpinBoxDelegate(0, INT_MAX, 1, this));
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::RUNTIME, new SpinBoxDelegate(0, INT_MAX, 1, this));
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::DURATION, new SpinBoxDelegate(0, INT_MAX, 1, this));
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::CCSETPOINT, new DoubleSpinBoxDelegate(0, 2, 0.1, this));

	/* set defaults. */
	ui.ATMPressure->setValue(1013);
	ui.atmHeight->setValue(0);

	setMinimumWidth(0);
	setMinimumHeight(0);
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
	plannerModel->setSalinity(salinity);
}

void DivePlannerWidget::settingsChanged()
{
	// Adopt units
	if (get_units()->length == units::FEET) {
		ui.atmHeight->setSuffix("ft");
		ui.atmHeight->setMinimum(-300);
		ui.atmHeight->setMaximum(10000);
	} else {
		ui.atmHeight->setSuffix(("m"));
		ui.atmHeight->setMinimum(-100);
		ui.atmHeight->setMaximum(3000);
	}
	ui.atmHeight->blockSignals(true);
	ui.atmHeight->setValue((int) get_depth_units((int) pressure_to_altitude(plannerModel->getSurfacePressure()), NULL,NULL));
	ui.atmHeight->blockSignals(false);
}

void DivePlannerWidget::atmPressureChanged(const int pressure)
{
	plannerModel->setSurfacePressure(pressure);
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
	plannerModel->setSurfacePressure(pressure);
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
	plannerModel->setSalinity(ui.waterType->itemData(index).toInt());
}

void DivePlannerWidget::customSalinityChanged(double density)
{
	if (ui.customSalinity->isEnabled()) {
		int newSalinity = (int)(density * 10000.0);
		ui.waterType->setItemData(ui.waterType->count() - 1, newSalinity);
		plannerModel->setSalinity(newSalinity);
	}
}

void PlannerSettingsWidget::disableDecoElements(int mode)
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
	}
	else if (mode == VPMB) {
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
		ui.bailout->setDisabled(!(displayed_dive.dc.divemode == CCR || displayed_dive.dc.divemode == PSCR));
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
		ui.sacfactor->setValue(plannerShared::sacfactor());
		ui.problemsolvingtime->setValue(prefs.problemsolvingtime);
		ui.display_variations->setDisabled(false);
	}
	else if (mode == BUEHLMANN) {
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
		ui.bailout->setDisabled(!(displayed_dive.dc.divemode == CCR || displayed_dive.dc.divemode == PSCR));
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
		ui.sacfactor->setValue(plannerShared::sacfactor());
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

void DivePlannerWidget::printDecoPlan()
{
	MainWindow::instance()->printPlan();
}

PlannerSettingsWidget::PlannerSettingsWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	ui.setupUi(this);

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
	ui.sacfactor->setValue(plannerShared::sacfactor());
	ui.problemsolvingtime->setValue(prefs.problemsolvingtime);
	ui.bottompo2->setValue(plannerShared::bottompo2());
	ui.decopo2->setValue(plannerShared::decopo2());
	ui.backgasBreaks->setChecked(prefs.doo2breaks);
	plannerShared::set_dobailout(false);
	setBailoutVisibility(false);
	ui.o2narcotic->setChecked(prefs.o2narcotic);
	ui.drop_stone_mode->setChecked(prefs.drop_stone_mode);
	ui.switch_at_req_stop->setChecked(prefs.switch_at_req_stop);
	ui.min_switch_duration->setValue(plannerShared::min_switch_duration());
	ui.surface_segment->setValue(prefs.surface_segment / 60);
	ui.recreational_deco->setChecked(prefs.planner_deco_mode == RECREATIONAL);
	ui.buehlmann_deco->setChecked(prefs.planner_deco_mode == BUEHLMANN);
	ui.vpmb_deco->setChecked(prefs.planner_deco_mode == VPMB);
	disableDecoElements((int) prefs.planner_deco_mode);

	// should be the same order as in dive_comp_type!
	QStringList rebreather_modes = QStringList();
	for (int i = 0; i < FREEDIVE; i++)
		rebreather_modes.append(gettextFromC::tr(divemode_text_ui[i]));
	ui.rebreathermode->insertItems(0, rebreather_modes);

	connect(ui.recreational_deco, &QAbstractButton::clicked, [=] { plannerShared::set_planner_deco_mode(RECREATIONAL); });
	connect(ui.buehlmann_deco, &QAbstractButton::clicked, [=] { plannerShared::set_planner_deco_mode(BUEHLMANN); });
	connect(ui.vpmb_deco, &QAbstractButton::clicked, [=] { plannerShared::set_planner_deco_mode(VPMB); });

	connect(ui.lastStop, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_last_stop(bool)));
	connect(ui.lastStop, SIGNAL(toggled(bool)), this, SLOT(disableBackgasBreaks(bool)));
	connect(ui.verbatim_plan, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_verbatim_plan(bool)));
	connect(ui.display_duration, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_display_duration(bool)));
	connect(ui.display_runtime, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_display_runtime(bool)));
	connect(ui.display_transitions, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_display_transitions(bool)));
	connect(ui.display_variations, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_display_variations(bool)));
	connect(ui.safetystop, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_safetystop(bool)));
	connect(ui.reserve_gas, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_reserve_gas(int)));
	connect(ui.ascRate75, SIGNAL(valueChanged(int)), plannerModel, SLOT(setAscrate75(int)));
	connect(ui.ascRate50, SIGNAL(valueChanged(int)), plannerModel, SLOT(setAscrate50(int)));
	connect(ui.ascRateStops, SIGNAL(valueChanged(int)), plannerModel, SLOT(setAscratestops(int)));
	connect(ui.ascRateLast6m, SIGNAL(valueChanged(int)), plannerModel, SLOT(setAscratelast6m(int)));
	connect(ui.descRate, SIGNAL(valueChanged(int)), plannerModel, SLOT(setDescrate(int)));
	connect(ui.drop_stone_mode, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_drop_stone_mode(bool)));
	connect(ui.gfhigh, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_gfhigh(int)));
	connect(ui.gflow, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_gflow(int)));
	connect(ui.vpmb_conservatism, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_vpmb_conservatism(int)));
	connect(ui.backgasBreaks, SIGNAL(toggled(bool)), this, SLOT(setBackgasBreaks(bool)));
	connect(ui.bailout, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_dobailout(bool)));
	connect(ui.o2narcotic, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_o2narcotic(bool)));
	connect(ui.switch_at_req_stop, SIGNAL(toggled(bool)), plannerShared::instance(), SLOT(set_switch_at_req_stop(bool)));
	connect(ui.min_switch_duration, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_min_switch_duration(int)));
	connect(ui.surface_segment, SIGNAL(valueChanged(int)), plannerModel, SLOT(setSurfaceSegment(int)));
	connect(ui.rebreathermode, SIGNAL(currentIndexChanged(int)), plannerModel, SLOT(setRebreatherMode(int)));
	connect(ui.rebreathermode, SIGNAL(currentIndexChanged(int)), this, SLOT(setBailoutVisibility(int)));

	connect(ui.recreational_deco, &QAbstractButton::clicked, [=] { disableDecoElements(RECREATIONAL); });
	connect(ui.buehlmann_deco, &QAbstractButton::clicked, [=] { disableDecoElements(BUEHLMANN); });
	connect(ui.vpmb_deco, &QAbstractButton::clicked, [=] { disableDecoElements(VPMB); });

	connect(ui.ascRate75, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_ascrate75(int)));
	connect(ui.ascRate50, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_ascrate50(int)));
	connect(ui.descRate, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_descrate(int)));
	connect(ui.ascRateStops, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_ascratestops(int)));
	connect(ui.ascRateLast6m, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_ascratelast6m(int)));
	connect(ui.sacfactor, SIGNAL(valueChanged(double)), plannerShared::instance(), SLOT(set_sacfactor(double)));
	connect(ui.problemsolvingtime, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_problemsolvingtime(int)));
	connect(ui.bottompo2, SIGNAL(valueChanged(double)), plannerShared::instance(), SLOT(set_bottompo2(double)));
	connect(ui.decopo2, SIGNAL(valueChanged(double)), plannerShared::instance(), SLOT(set_decopo2(double)));
	connect(ui.bestmixEND, SIGNAL(valueChanged(int)), plannerShared::instance(), SLOT(set_bestmixend(int)));
	connect(ui.bottomSAC, SIGNAL(valueChanged(double)), plannerShared::instance(), SLOT(set_bottomsac(double)));
	connect(ui.decoStopSAC, SIGNAL(valueChanged(double)), plannerShared::instance(), SLOT(set_decosac(double)));

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
	ui.ascRate75->setValue(plannerShared::ascrate75());
	ui.ascRate50->setValue(plannerShared::ascrate50());
	ui.ascRateStops->setValue(plannerShared::ascratestops());
	ui.ascRateLast6m->setValue(plannerShared::ascratelast6m());
	ui.descRate->setValue(lrint(plannerShared::descrate()));
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
	if(get_units()->volume == units::CUFT) {
		ui.bottomSAC->setSuffix(tr("cuft/min"));
		ui.decoStopSAC->setSuffix(tr("cuft/min"));
		ui.bottomSAC->setDecimals(2);
		ui.bottomSAC->setSingleStep(0.1);
		ui.decoStopSAC->setDecimals(2);
		ui.decoStopSAC->setSingleStep(0.1);
		ui.bottomSAC->setValue(ml_to_cuft(prefs.bottomsac));
		ui.decoStopSAC->setValue(ml_to_cuft(prefs.decosac));
	} else {
		ui.bottomSAC->setSuffix(tr("ℓ/min"));
		ui.decoStopSAC->setSuffix(tr("ℓ/min"));
		ui.bottomSAC->setDecimals(0);
		ui.bottomSAC->setSingleStep(1);
		ui.decoStopSAC->setDecimals(0);
		ui.decoStopSAC->setSingleStep(1);
		ui.bottomSAC->setValue(plannerShared::bottomsac());
		ui.decoStopSAC->setValue(plannerShared::decosac());
	}
	if(get_units()->pressure == units::BAR) {
		ui.reserve_gas->setSuffix(tr("bar"));
		ui.reserve_gas->setSingleStep(1);
		ui.reserve_gas->setValue(prefs.reserve_gas / 1000);
		ui.reserve_gas->setMaximum(300);
	} else {
		ui.reserve_gas->setSuffix(tr("psi"));
		ui.reserve_gas->setSingleStep(10);
		ui.reserve_gas->setMaximum(5000);
		ui.reserve_gas->setValue(mbar_to_PSI(prefs.reserve_gas));
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

void PlannerSettingsWidget::printDecoPlan()
{
}

void PlannerSettingsWidget::setBackgasBreaks(bool dobreaks)
{
	plannerShared::set_doo2breaks(dobreaks);
}

void PlannerSettingsWidget::setBailoutVisibility(int mode)
{
		ui.bailout->setDisabled(!(mode == CCR || mode == PSCR));
}

PlannerDetails::PlannerDetails(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
}
