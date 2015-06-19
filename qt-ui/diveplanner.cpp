#include "diveplanner.h"
#include "modeldelegates.h"
#include "mainwindow.h"
#include "planner.h"
#include "helpers.h"
#include "cylindermodel.h"
#include "models.h"
#include "profile/profilewidget2.h"
#include "diveplannermodel.h"

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
	GasSelectionModel *model = GasSelectionModel::instance();
	model->repopulate();
	int rowCount = model->rowCount();
	for (int i = 0; i < rowCount; i++) {
		QAction *action = new QAction(&m);
		action->setText(model->data(model->index(i, 0), Qt::DisplayRole).toString());
		connect(action, SIGNAL(triggered(bool)), this, SLOT(changeGas()));
		m.addAction(action);
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
	plannerModel->gaschange(index.sibling(index.row() + 1, index.column()), action->text());
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
	ui.dateEdit->setDisplayFormat(getDateFormat());
	ui.tableWidget->setTitle(tr("Dive planner points"));
	ui.tableWidget->setModel(plannerModel);
	plannerModel->setRecalc(true);
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::GAS, new AirTypesDelegate(this));
	ui.cylinderTableWidget->setTitle(tr("Available gases"));
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

void DivePlannerWidget::settingsChanged()
{
	// Adopt units
	if (get_units()->length == units::FEET) {
		ui.atmHeight->setSuffix("ft");
	} else {
		ui.atmHeight->setSuffix(("m"));
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
	int pressure = (int) (1013.0 * exp(- (double) units_to_depth((double) height) / 7800000.0));
	ui.ATMPressure->blockSignals(true);
	ui.ATMPressure->setValue(pressure);
	ui.ATMPressure->blockSignals(false);
	plannerModel->setSurfacePressure(pressure);
}

void DivePlannerWidget::salinityChanged(const double salinity)
{
	/* Salinity is expressed in weight in grams per 10l */
	plannerModel->setSalinity(10000 * salinity);
}

void PlannerSettingsWidget::bottomSacChanged(const double bottomSac)
{
	plannerModel->setBottomSac(bottomSac);
}

void PlannerSettingsWidget::decoSacChanged(const double decosac)
{
	plannerModel->setDecoSac(decosac);
}

void PlannerSettingsWidget::disableDecoElements(bool value)
{
	ui.lastStop->setDisabled(value);
	ui.backgasBreaks->setDisabled(value);
	ui.bottompo2->setDisabled(value);
	ui.decopo2->setDisabled(value);
	ui.reserve_gas->setDisabled(!value);
}

void DivePlannerWidget::printDecoPlan()
{
	MainWindow::instance()->printPlan();
}

PlannerSettingsWidget::PlannerSettingsWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	ui.setupUi(this);

	QSettings s;
	QStringList rebreater_modes;
	s.beginGroup("Planner");
	prefs.last_stop = s.value("last_stop", prefs.last_stop).toBool();
	prefs.verbatim_plan = s.value("verbatim_plan", prefs.verbatim_plan).toBool();
	prefs.display_duration = s.value("display_duration", prefs.display_duration).toBool();
	prefs.display_runtime = s.value("display_runtime", prefs.display_runtime).toBool();
	prefs.display_transitions = s.value("display_transitions", prefs.display_transitions).toBool();
	prefs.recreational_mode = s.value("recreational_mode", prefs.recreational_mode).toBool();
	prefs.safetystop = s.value("safetystop", prefs.safetystop).toBool();
	prefs.reserve_gas = s.value("reserve_gas", prefs.reserve_gas).toInt();
	prefs.ascrate75 = s.value("ascrate75", prefs.ascrate75).toInt();
	prefs.ascrate50 = s.value("ascrate50", prefs.ascrate50).toInt();
	prefs.ascratestops = s.value("ascratestops", prefs.ascratestops).toInt();
	prefs.ascratelast6m = s.value("ascratelast6m", prefs.ascratelast6m).toInt();
	prefs.descrate = s.value("descrate", prefs.descrate).toInt();
	prefs.bottompo2 = s.value("bottompo2", prefs.bottompo2).toInt();
	prefs.decopo2 = s.value("decopo2", prefs.decopo2).toInt();
	prefs.doo2breaks = s.value("doo2breaks", prefs.doo2breaks).toBool();
	prefs.min_switch_duration = s.value("min_switch_duration", prefs.min_switch_duration).toInt();
	prefs.drop_stone_mode = s.value("drop_stone_mode", prefs.drop_stone_mode).toBool();
	prefs.bottomsac = s.value("bottomsac", prefs.bottomsac).toInt();
	prefs.decosac = s.value("decosac", prefs.decosac).toInt();
	plannerModel->getDiveplan().bottomsac = prefs.bottomsac;
	plannerModel->getDiveplan().decosac = prefs.decosac;
	s.endGroup();

	updateUnitsUI();
	ui.lastStop->setChecked(prefs.last_stop);
	ui.verbatim_plan->setChecked(prefs.verbatim_plan);
	ui.display_duration->setChecked(prefs.display_duration);
	ui.display_runtime->setChecked(prefs.display_runtime);
	ui.display_transitions->setChecked(prefs.display_transitions);
	ui.recreational_mode->setChecked(prefs.recreational_mode);
	ui.safetystop->setChecked(prefs.safetystop);
	ui.reserve_gas->setValue(prefs.reserve_gas / 1000);
	ui.bottompo2->setValue(prefs.bottompo2 / 1000.0);
	ui.decopo2->setValue(prefs.decopo2 / 1000.0);
	ui.backgasBreaks->setChecked(prefs.doo2breaks);
	ui.drop_stone_mode->setChecked(prefs.drop_stone_mode);
	ui.min_switch_duration->setValue(prefs.min_switch_duration / 60);
	// should be the same order as in dive_comp_type!
	rebreater_modes << tr("Open circuit") << tr("CCR") << tr("pSCR");
	ui.rebreathermode->insertItems(0, rebreater_modes);

	connect(ui.lastStop, SIGNAL(toggled(bool)), plannerModel, SLOT(setLastStop6m(bool)));
	connect(ui.verbatim_plan, SIGNAL(toggled(bool)), plannerModel, SLOT(setVerbatim(bool)));
	connect(ui.display_duration, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayDuration(bool)));
	connect(ui.display_runtime, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayRuntime(bool)));
	connect(ui.display_transitions, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayTransitions(bool)));
	connect(ui.safetystop, SIGNAL(toggled(bool)), plannerModel, SLOT(setSafetyStop(bool)));
	connect(ui.recreational_mode, SIGNAL(toggled(bool)), plannerModel, SLOT(setRecreationalMode(bool)));
	connect(ui.reserve_gas, SIGNAL(valueChanged(int)), plannerModel, SLOT(setReserveGas(int)));
	connect(ui.ascRate75, SIGNAL(valueChanged(int)), this, SLOT(setAscRate75(int)));
	connect(ui.ascRate75, SIGNAL(valueChanged(int)), plannerModel, SLOT(emitDataChanged()));
	connect(ui.ascRate50, SIGNAL(valueChanged(int)), this, SLOT(setAscRate50(int)));
	connect(ui.ascRate50, SIGNAL(valueChanged(int)), plannerModel, SLOT(emitDataChanged()));
	connect(ui.ascRateStops, SIGNAL(valueChanged(int)), this, SLOT(setAscRateStops(int)));
	connect(ui.ascRateStops, SIGNAL(valueChanged(int)), plannerModel, SLOT(emitDataChanged()));
	connect(ui.ascRateLast6m, SIGNAL(valueChanged(int)), this, SLOT(setAscRateLast6m(int)));
	connect(ui.ascRateLast6m, SIGNAL(valueChanged(int)), plannerModel, SLOT(emitDataChanged()));
	connect(ui.descRate, SIGNAL(valueChanged(int)), this, SLOT(setDescRate(int)));
	connect(ui.descRate, SIGNAL(valueChanged(int)), plannerModel, SLOT(emitDataChanged()));
	connect(ui.bottompo2, SIGNAL(valueChanged(double)), this, SLOT(setBottomPo2(double)));
	connect(ui.decopo2, SIGNAL(valueChanged(double)), this, SLOT(setDecoPo2(double)));
	connect(ui.drop_stone_mode, SIGNAL(toggled(bool)), plannerModel, SLOT(setDropStoneMode(bool)));
	connect(ui.bottomSAC, SIGNAL(valueChanged(double)), this, SLOT(bottomSacChanged(double)));
	connect(ui.decoStopSAC, SIGNAL(valueChanged(double)), this, SLOT(decoSacChanged(double)));
	connect(ui.gfhigh, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFHigh(int)));
	connect(ui.gflow, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFLow(int)));
	connect(ui.gfhigh, SIGNAL(editingFinished()), plannerModel, SLOT(triggerGFHigh()));
	connect(ui.gflow, SIGNAL(editingFinished()), plannerModel, SLOT(triggerGFLow()));
	connect(ui.backgasBreaks, SIGNAL(toggled(bool)), this, SLOT(setBackgasBreaks(bool)));
	connect(ui.min_switch_duration, SIGNAL(valueChanged(int)), plannerModel, SLOT(setMinSwitchDuration(int)));
	connect(ui.rebreathermode, SIGNAL(currentIndexChanged(int)), plannerModel, SLOT(setRebreatherMode(int)));
	connect(plannerModel, SIGNAL(recreationChanged(bool)), this, SLOT(disableDecoElements(bool)));

	settingsChanged();
	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);

	setMinimumWidth(0);
	setMinimumHeight(0);
}

void PlannerSettingsWidget::updateUnitsUI()
{
	ui.ascRate75->setValue(rint(prefs.ascrate75 / UNIT_FACTOR));
	ui.ascRate50->setValue(rint(prefs.ascrate50 / UNIT_FACTOR));
	ui.ascRateStops->setValue(rint(prefs.ascratestops / UNIT_FACTOR));
	ui.ascRateLast6m->setValue(rint(prefs.ascratelast6m / UNIT_FACTOR));
	ui.descRate->setValue(rint(prefs.descrate / UNIT_FACTOR));
}

PlannerSettingsWidget::~PlannerSettingsWidget()
{
	QSettings s;
	s.beginGroup("Planner");
	s.setValue("last_stop", prefs.last_stop);
	s.setValue("verbatim_plan", prefs.verbatim_plan);
	s.setValue("display_duration", prefs.display_duration);
	s.setValue("display_runtime", prefs.display_runtime);
	s.setValue("display_transitions", prefs.display_transitions);
	s.setValue("recreational_mode", prefs.recreational_mode);
	s.setValue("safetystop", prefs.safetystop);
	s.setValue("reserve_gas", prefs.reserve_gas);
	s.setValue("ascrate75", prefs.ascrate75);
	s.setValue("ascrate50", prefs.ascrate50);
	s.setValue("ascratestops", prefs.ascratestops);
	s.setValue("ascratelast6m", prefs.ascratelast6m);
	s.setValue("descrate", prefs.descrate);
	s.setValue("bottompo2", prefs.bottompo2);
	s.setValue("decopo2", prefs.decopo2);
	s.setValue("doo2breaks", prefs.doo2breaks);
	s.setValue("drop_stone_mode", prefs.drop_stone_mode);
	s.setValue("min_switch_duration", prefs.min_switch_duration);
	s.setValue("bottomsac", prefs.bottomsac);
	s.setValue("decosac", prefs.decosac);
	s.endGroup();
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
	} else {
		vs.append(tr("m/min"));
		ui.lastStop->setText(tr("Last stop at 6m"));
		ui.asc50to6->setText(tr("50% avg. depth to 6m"));
		ui.asc6toSurf->setText(tr("6m to surface"));
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
	ui.bottomSAC->blockSignals(false);
	ui.decoStopSAC->blockSignals(false);
	updateUnitsUI();
	ui.ascRate75->setSuffix(vs);
	ui.ascRate50->setSuffix(vs);
	ui.ascRateStops->setSuffix(vs);
	ui.ascRateLast6m->setSuffix(vs);
	ui.descRate->setSuffix(vs);
}

void PlannerSettingsWidget::atmPressureChanged(const QString &pressure)
{
}

void PlannerSettingsWidget::printDecoPlan()
{
}

void PlannerSettingsWidget::setAscRate75(int rate)
{
	prefs.ascrate75 = rate * UNIT_FACTOR;
}

void PlannerSettingsWidget::setAscRate50(int rate)
{
	prefs.ascrate50 = rate * UNIT_FACTOR;
}

void PlannerSettingsWidget::setAscRateStops(int rate)
{
	prefs.ascratestops = rate * UNIT_FACTOR;
}

void PlannerSettingsWidget::setAscRateLast6m(int rate)
{
	prefs.ascratelast6m = rate * UNIT_FACTOR;
}

void PlannerSettingsWidget::setDescRate(int rate)
{
	prefs.descrate = rate * UNIT_FACTOR;
}

void PlannerSettingsWidget::setBottomPo2(double po2)
{
	prefs.bottompo2 = (int) (po2 * 1000.0);
}

void PlannerSettingsWidget::setDecoPo2(double po2)
{
	prefs.decopo2 = (int) (po2 * 1000.0);
}

void PlannerSettingsWidget::setBackgasBreaks(bool dobreaks)
{
	prefs.doo2breaks = dobreaks;
	plannerModel->emitDataChanged();
}

PlannerDetails::PlannerDetails(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
}
