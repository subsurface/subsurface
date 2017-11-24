// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/diveplanner.h"
#include "desktop-widgets/modeldelegates.h"
#include "desktop-widgets/mainwindow.h"
#include "core/planner.h"
#include "core/helpers.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"

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

#define UNIT_FACTOR ((prefs.units.length == units::METERS) ? 1000.0 / 60.0 : feet_to_mm(1.0) / 60.0)

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
		m.addAction(QObject::tr("Remove this point"), this, SLOT(selfRemove()));
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
	plannerModel->setRecalc(true);
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::GAS, new AirTypesDelegate(this));
	ui.cylinderTableWidget->setTitle(tr("Available gases"));
	ui.cylinderTableWidget->setBtnToolTip(tr("Add cylinder"));
	ui.cylinderTableWidget->setModel(CylindersModel::instance());
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
	connect(ui.salinity, SIGNAL(valueChanged(double)), this, SLOT(salinityChanged(double)));
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
	ui.salinity->setValue(salinity / 10000.0);
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
	ui.atmHeight->setValue((int) get_depth_units((int) (log(1013.0 / plannerModel->getSurfacePressure()) * 7800000), NULL,NULL));
	ui.atmHeight->blockSignals(false);
}

void DivePlannerWidget::atmPressureChanged(const int pressure)
{
	plannerModel->setSurfacePressure(pressure);
	ui.atmHeight->blockSignals(true);
	ui.atmHeight->setValue((int) get_depth_units((int) (log(1013.0 / pressure) * 7800000), NULL,NULL));
	ui.atmHeight->blockSignals(false);
}

void DivePlannerWidget::heightChanged(const int height)
{
	int pressure = (int) (1013.0 * exp(- (double) units_to_depth((double) height).mm / 7800000.0));
	ui.ATMPressure->blockSignals(true);
	ui.ATMPressure->setValue(pressure);
	ui.ATMPressure->blockSignals(false);
	plannerModel->setSurfacePressure(pressure);
}

void DivePlannerWidget::salinityChanged(const double salinity)
{
	/* Salinity is expressed in weight in grams per 10l */
	plannerModel->setSalinity(lrint(10000 * salinity));
}

void PlannerSettingsWidget::bottomSacChanged(const double bottomSac)
{
	plannerModel->setBottomSac(bottomSac);
}

void PlannerSettingsWidget::decoSacChanged(const double decosac)
{
	plannerModel->setDecoSac(decosac);
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
		ui.bottompo2->setDisabled(false);
		ui.decopo2->setDisabled(true);
		ui.safetystop->setDisabled(false);
		ui.label_reserve_gas->setDisabled(false);
		ui.reserve_gas->setDisabled(false);
		ui.label_vpmb_conservatism->setDisabled(true);
		ui.vpmb_conservatism->setDisabled(true);
		ui.switch_at_req_stop->setDisabled(true);
		ui.min_switch_duration->setDisabled(true);
		ui.label_min_switch_duration->setDisabled(true);
		ui.sacfactor->setDisabled(true);
		ui.problemsolvingtime->setDisabled(true);
		ui.sacfactor->blockSignals(true);
		ui.problemsolvingtime->blockSignals(true);
		ui.sacfactor->setValue(2.0);
		ui.problemsolvingtime->setValue(0);
		ui.sacfactor->blockSignals(false);
		ui.problemsolvingtime->blockSignals(false);
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
		ui.bottompo2->setDisabled(false);
		ui.decopo2->setDisabled(false);
		ui.safetystop->setDisabled(true);
		ui.label_reserve_gas->setDisabled(true);
		ui.reserve_gas->setDisabled(true);
		ui.label_vpmb_conservatism->setDisabled(false);
		ui.vpmb_conservatism->setDisabled(false);
		ui.switch_at_req_stop->setDisabled(false);
		ui.min_switch_duration->setDisabled(false);
		ui.label_min_switch_duration->setDisabled(false);
		ui.sacfactor->setDisabled(false);
		ui.problemsolvingtime->setDisabled(false);
		ui.sacfactor->setValue(prefs.sacfactor / 100.0);
		ui.problemsolvingtime->setValue(prefs.problemsolvingtime);
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
		ui.bottompo2->setDisabled(false);
		ui.decopo2->setDisabled(false);
		ui.safetystop->setDisabled(true);
		ui.label_reserve_gas->setDisabled(true);
		ui.reserve_gas->setDisabled(true);
		ui.label_vpmb_conservatism->setDisabled(true);
		ui.vpmb_conservatism->setDisabled(true);
		ui.switch_at_req_stop->setDisabled(false);
		ui.min_switch_duration->setDisabled(false);
		ui.label_min_switch_duration->setDisabled(false);
		ui.sacfactor->setDisabled(false);
		ui.problemsolvingtime->setDisabled(false);
		ui.sacfactor->setValue(prefs.sacfactor / 100.0);
		ui.problemsolvingtime->setValue(prefs.problemsolvingtime);
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
	QStringList rebreather_modes;

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
	ui.sacfactor->setValue(prefs.sacfactor / 100.0);
	ui.problemsolvingtime->setValue(prefs.problemsolvingtime);
	ui.bottompo2->setValue(prefs.bottompo2 / 1000.0);
	ui.decopo2->setValue(prefs.decopo2 / 1000.0);
	ui.backgasBreaks->setChecked(prefs.doo2breaks);
	ui.drop_stone_mode->setChecked(prefs.drop_stone_mode);
	ui.switch_at_req_stop->setChecked(prefs.switch_at_req_stop);
	ui.min_switch_duration->setValue(prefs.min_switch_duration / 60);
	ui.recreational_deco->setChecked(prefs.planner_deco_mode == RECREATIONAL);
	ui.buehlmann_deco->setChecked(prefs.planner_deco_mode == BUEHLMANN);
	ui.vpmb_deco->setChecked(prefs.planner_deco_mode == VPMB);
	disableDecoElements((int) prefs.planner_deco_mode);

	// should be the same order as in dive_comp_type!
	rebreather_modes << tr("Open circuit") << tr("CCR") << tr("pSCR");
	ui.rebreathermode->insertItems(0, rebreather_modes);

	modeMapper = new QSignalMapper(this);
	connect(modeMapper, SIGNAL(mapped(int)) , plannerModel, SLOT(setDecoMode(int)));
	modeMapper->setMapping(ui.recreational_deco, int(RECREATIONAL));
	modeMapper->setMapping(ui.buehlmann_deco, int(BUEHLMANN));
	modeMapper->setMapping(ui.vpmb_deco, int(VPMB));

	connect(ui.recreational_deco, SIGNAL(clicked()), modeMapper, SLOT(map()));
	connect(ui.buehlmann_deco, SIGNAL(clicked()), modeMapper, SLOT(map()));
	connect(ui.vpmb_deco, SIGNAL(clicked()), modeMapper, SLOT(map()));

	connect(ui.lastStop, SIGNAL(toggled(bool)), plannerModel, SLOT(setLastStop6m(bool)));
	connect(ui.lastStop, SIGNAL(toggled(bool)), this, SLOT(disableBackgasBreaks(bool)));
	connect(ui.verbatim_plan, SIGNAL(toggled(bool)), plannerModel, SLOT(setVerbatim(bool)));
	connect(ui.display_duration, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayDuration(bool)));
	connect(ui.display_runtime, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayRuntime(bool)));
	connect(ui.display_transitions, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayTransitions(bool)));
	connect(ui.display_variations, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayVariations(bool)));
	connect(ui.safetystop, SIGNAL(toggled(bool)), plannerModel, SLOT(setSafetyStop(bool)));
	connect(ui.reserve_gas, SIGNAL(valueChanged(int)), plannerModel, SLOT(setReserveGas(int)));
	connect(ui.ascRate75, SIGNAL(valueChanged(int)), plannerModel, SLOT(setAscrate75(int)));
	connect(ui.ascRate50, SIGNAL(valueChanged(int)), plannerModel, SLOT(setAscrate50(int)));
	connect(ui.ascRateStops, SIGNAL(valueChanged(int)), plannerModel, SLOT(setAscratestops(int)));
	connect(ui.ascRateLast6m, SIGNAL(valueChanged(int)), plannerModel, SLOT(setAscratelast6m(int)));
	connect(ui.descRate, SIGNAL(valueChanged(int)), plannerModel, SLOT(setDescrate(int)));
	connect(ui.drop_stone_mode, SIGNAL(toggled(bool)), plannerModel, SLOT(setDropStoneMode(bool)));
	connect(ui.gfhigh, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFHigh(int)));
	connect(ui.gflow, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFLow(int)));
	connect(ui.gfhigh, SIGNAL(editingFinished()), plannerModel, SLOT(triggerGFHigh()));
	connect(ui.gflow, SIGNAL(editingFinished()), plannerModel, SLOT(triggerGFLow()));
	connect(ui.vpmb_conservatism, SIGNAL(valueChanged(int)), plannerModel, SLOT(setVpmbConservatism(int)));
	connect(ui.backgasBreaks, SIGNAL(toggled(bool)), this, SLOT(setBackgasBreaks(bool)));
	connect(ui.switch_at_req_stop, SIGNAL(toggled(bool)), plannerModel, SLOT(setSwitchAtReqStop(bool)));
	connect(ui.min_switch_duration, SIGNAL(valueChanged(int)), plannerModel, SLOT(setMinSwitchDuration(int)));
	connect(ui.rebreathermode, SIGNAL(currentIndexChanged(int)), plannerModel, SLOT(setRebreatherMode(int)));

	connect(ui.bottompo2, SIGNAL(valueChanged(double)), CylindersModel::instance(), SLOT(updateBestMixes()));
	connect(ui.bestmixEND, SIGNAL(valueChanged(int)), CylindersModel::instance(), SLOT(updateBestMixes()));

	connect(modeMapper, SIGNAL(mapped(int)), this, SLOT(disableDecoElements(int)));
	connect(ui.ascRate75, SIGNAL(valueChanged(int)), this, SLOT(setAscrate75(int)));
	connect(ui.ascRate50, SIGNAL(valueChanged(int)), this, SLOT(setAscrate50(int)));
	connect(ui.descRate, SIGNAL(valueChanged(int)), this, SLOT(setDescrate(int)));
	connect(ui.ascRateStops, SIGNAL(valueChanged(int)), this, SLOT(setAscratestops(int)));
	connect(ui.ascRateLast6m, SIGNAL(valueChanged(int)), this, SLOT(setAscratelast6m(int)));
	connect(ui.sacfactor, SIGNAL(valueChanged(double)), this, SLOT(sacFactorChanged(double)));
	connect(ui.problemsolvingtime, SIGNAL(valueChanged(int)), this, SLOT(problemSolvingTimeChanged(int)));
	connect(ui.bottompo2, SIGNAL(valueChanged(double)), this, SLOT(setBottomPo2(double)));
	connect(ui.decopo2, SIGNAL(valueChanged(double)), this, SLOT(setDecoPo2(double)));
	connect(ui.bestmixEND, SIGNAL(valueChanged(int)), this, SLOT(setBestmixEND(int)));
	connect(ui.bottomSAC, SIGNAL(valueChanged(double)), this, SLOT(bottomSacChanged(double)));
	connect(ui.decoStopSAC, SIGNAL(valueChanged(double)), this, SLOT(decoSacChanged(double)));

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
	ui.ascRate75->setValue(lrint(prefs.ascrate75 / UNIT_FACTOR));
	ui.ascRate50->setValue(lrint(prefs.ascrate50 / UNIT_FACTOR));
	ui.ascRateStops->setValue(lrint(prefs.ascratestops / UNIT_FACTOR));
	ui.ascRateLast6m->setValue(lrint(prefs.ascratelast6m / UNIT_FACTOR));
	ui.descRate->setValue(lrint(prefs.descrate / UNIT_FACTOR));
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
		ui.bottomSAC->setValue((double) prefs.bottomsac / 1000.0);
		ui.decoStopSAC->setValue((double) prefs.decosac / 1000.0);
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

void PlannerSettingsWidget::setAscrate75(int rate)
{
	SettingsObjectWrapper::instance()->planner_settings->setAscrate75(lrint(rate * UNIT_FACTOR));
}

void PlannerSettingsWidget::setAscrate50(int rate)
{
	SettingsObjectWrapper::instance()->planner_settings->setAscrate50(lrint(rate * UNIT_FACTOR));
}

void PlannerSettingsWidget::setAscratestops(int rate)
{
	SettingsObjectWrapper::instance()->planner_settings->setAscratestops(lrint(rate * UNIT_FACTOR));
}

void PlannerSettingsWidget::setAscratelast6m(int rate)
{
	SettingsObjectWrapper::instance()->planner_settings->setAscratelast6m(lrint(rate * UNIT_FACTOR));
}

void PlannerSettingsWidget::setDescrate(int rate)
{
	SettingsObjectWrapper::instance()->planner_settings->setDescrate(lrint(rate * UNIT_FACTOR));
}

void PlannerSettingsWidget::sacFactorChanged(const double factor)
{
	plannerModel->setSacFactor(factor);
}

void PlannerSettingsWidget::problemSolvingTimeChanged(const int minutes)
{
	plannerModel->setProblemSolvingTime(minutes);
}

void PlannerSettingsWidget::setBottomPo2(double po2)
{
	SettingsObjectWrapper::instance()->planner_settings->setBottompo2((int) (po2 * 1000.0));
}

void PlannerSettingsWidget::setDecoPo2(double po2)
{
	pressure_t olddecopo2;
	olddecopo2.mbar = prefs.decopo2;
	SettingsObjectWrapper::instance()->planner_settings->setDecopo2((int) (po2 * 1000.0));
	CylindersModel::instance()->updateDecoDepths(olddecopo2);
}

void PlannerSettingsWidget::setBestmixEND(int depth)
{
	SettingsObjectWrapper::instance()->planner_settings->setBestmixend(units_to_depth(depth).mm);
}

void PlannerSettingsWidget::setBackgasBreaks(bool dobreaks)
{
	SettingsObjectWrapper::instance()->planner_settings->setDoo2breaks(dobreaks);
	plannerModel->emitDataChanged();
}

PlannerDetails::PlannerDetails(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
}
