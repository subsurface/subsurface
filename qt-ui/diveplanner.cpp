#include "diveplanner.h"
#include "graphicsview-common.h"
#include "models.h"
#include "modeldelegates.h"
#include "mainwindow.h"
#include "maintab.h"
#include "tableview.h"

#include "dive.h"
#include "divelist.h"
#include "planner.h"
#include "display.h"
#include "helpers.h"

#include <QMouseEvent>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QListView>
#include <QModelIndex>
#include <QSettings>
#include <QTableView>
#include <QColor>
#include <QShortcut>

#include <algorithm>
#include <string.h>

#define TIME_INITIAL_MAX 30

#define MAX_DEPTH M_OR_FT(150, 450)
#define MIN_DEPTH M_OR_FT(20, 60)

#define UNIT_FACTOR ((prefs.units.length == units::METERS) ? 1000.0 / 60.0 : feet_to_mm(1.0) / 60.0)

QString gasToStr(struct gasmix gas)
{
	uint o2 = (get_o2(&gas) + 5) / 10, he = (get_he(&gas) + 5) / 10;
	QString result = gasmix_is_air(&gas) ? QObject::tr("AIR") : he == 0 ? (o2 == 100 ? QObject::tr("OXYGEN") : QString("EAN%1").arg(o2, 2, 10, QChar('0'))) : QString("%1/%2").arg(o2).arg(he);
	return result;
}

QString dpGasToStr(const divedatapoint &p)
{
	return gasToStr(p.gasmix);
}

static DivePlannerPointsModel *plannerModel = DivePlannerPointsModel::instance();

bool intLessThan(int a, int b)
{
	return a <= b;
}
void DivePlannerPointsModel::removeSelectedPoints(const QVector<int> &rows)
{
	if (!rows.count())
		return;
	int firstRow = rowCount() - rows.count();
	QVector<int> v2 = rows;
	std::sort(v2.begin(), v2.end(), intLessThan);
	beginRemoveRows(QModelIndex(), firstRow, rowCount() - 1);
	for (int i = v2.count() - 1; i >= 0; i--) {
		divepoints.remove(v2[i]);
	}
	endRemoveRows();
}

void DivePlannerPointsModel::createSimpleDive()
{
	struct gasmix gas = { 0 };

	// initialize the start time in the plan
	diveplan.when = displayed_dive.when;

	if (isPlanner())
		// let's use the gas from the first cylinder
		gas = displayed_dive.cylinder[0].gasmix;

	// If we're in drop_stone_mode, don't add a first point.
	// It will be added implicit.
	if (!prefs.drop_stone_mode)
		plannerModel->addStop(M_OR_FT(15, 45), 1 * 60, &gas, 0, true);

	plannerModel->addStop(M_OR_FT(15, 45), 40 * 60, &gas, 0, true);
	if (!isPlanner()) {
		plannerModel->addStop(M_OR_FT(5, 15), 42 * 60, &gas, 0, true);
		plannerModel->addStop(M_OR_FT(5, 15), 45 * 60, &gas, 0, true);
	}
}

void DivePlannerPointsModel::setupStartTime()
{
	// if the latest dive is in the future, then start an hour after it ends
	// otherwise start an hour from now
	startTime = QDateTime::currentDateTimeUtc().addSecs(3600 + gettimezoneoffset());
	if (dive_table.nr) {
		struct dive *d = get_dive(dive_table.nr - 1);
		time_t ends = d->when + d->duration.seconds;
		time_t diff = ends - startTime.toTime_t();
		if (diff > 0) {
			startTime = startTime.addSecs(diff + 3600);
		}
	}
	emit startTimeChanged(startTime);
}

void DivePlannerPointsModel::loadFromDive(dive *d)
{
	bool oldRec = recalc;
	recalc = false;
	CylindersModel::instance()->updateDive();
	duration_t lasttime = {};
	struct gasmix gas;
	free_dps(&diveplan);
	diveplan.when = d->when;
	// is this a "new" dive where we marked manually entered samples?
	// if yes then the first sample should be marked
	// if it is we only add the manually entered samples as waypoints to the diveplan
	// otherwise we have to add all of them
	bool hasMarkedSamples = d->dc.sample[0].manually_entered;
	for (int i = 0; i < d->dc.samples - 1; i++) {
		const sample &s = d->dc.sample[i];
		if (s.time.seconds == 0 || (hasMarkedSamples && !s.manually_entered))
			continue;
		get_gas_at_time(d, &d->dc, lasttime, &gas);
		plannerModel->addStop(s.depth.mm, s.time.seconds, &gas, 0, true);
		lasttime = s.time;
	}
	recalc = oldRec;
	emitDataChanged();
}

// copy the tanks from the current dive, or the default cylinder
// or an unknown cylinder
// setup the cylinder widget accordingly
void DivePlannerPointsModel::setupCylinders()
{
	if (mode == PLAN && current_dive) {
		// take the used cylinders from the selected dive as starting point
		CylindersModel::instance()->copyFromDive(current_dive);
		copy_cylinders(current_dive, &displayed_dive, true);
		reset_cylinders(&displayed_dive, true);
		return;
	}
	if (!same_string(prefs.default_cylinder, "")) {
		fill_default_cylinder(&displayed_dive.cylinder[0]);
	} else {
		// roughly an AL80
		displayed_dive.cylinder[0].type.description = strdup(tr("unknown").toUtf8().constData());
		displayed_dive.cylinder[0].type.size.mliter = 11100;
		displayed_dive.cylinder[0].type.workingpressure.mbar = 207000;
	}
	reset_cylinders(&displayed_dive, false);
	CylindersModel::instance()->copyFromDive(&displayed_dive);
}

QStringList &DivePlannerPointsModel::getGasList()
{
	static QStringList list;
	list.clear();
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &displayed_dive.cylinder[i];
		if (cylinder_nodata(cyl))
			break;
		list.push_back(gasToStr(cyl->gasmix));
	}
	return list;
}

void DivePlannerPointsModel::removeDeco()
{
	bool oldrec = setRecalc(false);
	QVector<int> computedPoints;
	for (int i = 0; i < plannerModel->rowCount(); i++)
		if (!plannerModel->at(i).entered)
			computedPoints.push_back(i);
	removeSelectedPoints(computedPoints);
	setRecalc(oldrec);
}

#if 0
void DivePlannerGraphics::drawProfile()
{
	// Code ported to the new profile is deleted. This part that I left here
	// is because I didn't fully understood the reason of the magic with
	// the plannerModel.
	bool oldRecalc = plannerModel->setRecalc(false);
	plannerModel->removeDeco();
	// Here we plotted the old planner profile. why there's the magic with the plannerModel here?
	plannerModel->setRecalc(oldRecalc);
	plannerModel->deleteTemporaryPlan();
}
#endif

DiveHandler::DiveHandler() : QGraphicsEllipseItem()
{
	setRect(-5, -5, 10, 10);
	setFlags(ItemIgnoresTransformations | ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
	setBrush(Qt::white);
	setZValue(2);
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
	if (DivePlannerPointsModel::instance()->rowCount() > 1) {
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
	plannerModel->setData(index, action->text());
}

void DiveHandler::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
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
	ui.tableWidget->setModel(DivePlannerPointsModel::instance());
	DivePlannerPointsModel::instance()->setRecalc(true);
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::GAS, new AirTypesDelegate(this));
	ui.cylinderTableWidget->setTitle(tr("Available gases"));
	ui.cylinderTableWidget->setModel(CylindersModel::instance());
	QTableView *view = ui.cylinderTableWidget->view();
	view->setColumnHidden(CylindersModel::START, true);
	view->setColumnHidden(CylindersModel::END, true);
	view->setColumnHidden(CylindersModel::DEPTH, false);
	view->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate(this));
	connect(ui.cylinderTableWidget, SIGNAL(addButtonClicked()), DivePlannerPointsModel::instance(), SLOT(addCylinder_clicked()));
	connect(ui.tableWidget, SIGNAL(addButtonClicked()), DivePlannerPointsModel::instance(), SLOT(addStop()));

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

	ui.tableWidget->setBtnToolTip(tr("Add dive data point"));
	connect(ui.startTime, SIGNAL(timeChanged(QTime)), plannerModel, SLOT(setStartTime(QTime)));
	connect(ui.dateEdit, SIGNAL(dateChanged(QDate)), plannerModel, SLOT(setStartDate(QDate)));
	connect(ui.ATMPressure, SIGNAL(valueChanged(int)), this, SLOT(atmPressureChanged(int)));
	connect(ui.atmHeight, SIGNAL(valueChanged(int)), this, SLOT(heightChanged(int)));
	connect(ui.salinity, SIGNAL(valueChanged(double)), this, SLOT(salinityChanged(double)));
	connect(DivePlannerPointsModel::instance(), SIGNAL(startTimeChanged(QDateTime)), this, SLOT(setupStartTime(QDateTime)));

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

void DivePlannerPointsModel::addCylinder_clicked()
{
	CylindersModel::instance()->add();
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

void DivePlannerWidget::printDecoPlan()
{
	MainWindow::instance()->printPlan();
}

PlannerSettingsWidget::PlannerSettingsWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	ui.setupUi(this);

	QSettings s;
	s.beginGroup("Planner");
	prefs.ascrate75 = s.value("ascrate75", prefs.ascrate75).toInt();
	prefs.ascrate50 = s.value("ascrate50", prefs.ascrate50).toInt();
	prefs.ascratestops = s.value("ascratestops", prefs.ascratestops).toInt();
	prefs.ascratelast6m = s.value("ascratelast6m", prefs.ascratelast6m).toInt();
	prefs.descrate = s.value("descrate", prefs.descrate).toInt();
	prefs.bottompo2 = s.value("bottompo2", prefs.bottompo2).toInt();
	prefs.decopo2 = s.value("decopo2", prefs.decopo2).toInt();
	prefs.doo2breaks = s.value("doo2breaks", prefs.doo2breaks).toBool();
	prefs.drop_stone_mode = s.value("drop_stone_mode", prefs.drop_stone_mode).toBool();
	prefs.bottomsac = s.value("bottomsac", prefs.bottomsac).toInt();
	prefs.decosac = s.value("decosac", prefs.decosac).toInt();
	plannerModel->getDiveplan().bottomsac = prefs.bottomsac;
	plannerModel->getDiveplan().decosac = prefs.decosac;
	s.endGroup();

	updateUnitsUI();
	ui.bottompo2->setValue(prefs.bottompo2 / 1000.0);
	ui.decopo2->setValue(prefs.decopo2 / 1000.0);
	ui.backgasBreaks->setChecked(prefs.doo2breaks);
	ui.drop_stone_mode->setChecked(prefs.drop_stone_mode);

	connect(ui.lastStop, SIGNAL(toggled(bool)), plannerModel, SLOT(setLastStop6m(bool)));
	connect(ui.verbatim_plan, SIGNAL(toggled(bool)), plannerModel, SLOT(setVerbatim(bool)));
	connect(ui.display_duration, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayDuration(bool)));
	connect(ui.display_runtime, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayRuntime(bool)));
	connect(ui.display_transitions, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayTransitions(bool)));
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
	s.setValue("ascrate75", prefs.ascrate75);
	s.setValue("ascrate50", prefs.ascrate50);
	s.setValue("ascratestops", prefs.ascratestops);
	s.setValue("ascratelast6m", prefs.ascratelast6m);
	s.setValue("descrate", prefs.descrate);
	s.setValue("bottompo2", prefs.bottompo2);
	s.setValue("decopo2", prefs.decopo2);
	s.setValue("doo2breaks", prefs.doo2breaks);
	s.setValue("drop_stone_mode", prefs.drop_stone_mode);
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


void DivePlannerPointsModel::setPlanMode(Mode m)
{
	mode = m;
	// the planner may reset our GF settings that are used to show deco
	// reset them to what's in the preferences
	if (m != PLAN)
		set_gf(prefs.gflow, prefs.gfhigh, prefs.gf_low_at_maxdepth);
}

bool DivePlannerPointsModel::isPlanner()
{
	return mode == PLAN;
}

/* When the planner adds deco stops to the model, adding those should not trigger a new deco calculation.
 * We thus start the planner only when recalc is true. */

bool DivePlannerPointsModel::setRecalc(bool rec)
{
	bool old = recalc;
	recalc = rec;
	return old;
}

bool DivePlannerPointsModel::recalcQ()
{
	return recalc;
}

int DivePlannerPointsModel::columnCount(const QModelIndex &parent) const
{
	return COLUMNS; // to disable CCSETPOINT subtract one
}

QVariant DivePlannerPointsModel::data(const QModelIndex &index, int role) const
{
	divedatapoint p = divepoints.at(index.row());
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
		case CCSETPOINT:
			return (double)p.setpoint / 1000;
		case DEPTH:
			return (int) rint(get_depth_units(p.depth, NULL, NULL));
		case RUNTIME:
			return p.time / 60;
		case DURATION:
			if (index.row())
				return (p.time - divepoints.at(index.row() - 1).time) / 60;
			else
				return p.time / 60;
		case GAS:
			return dpGasToStr(p);
		}
	} else if (role == Qt::DecorationRole) {
		switch (index.column()) {
		case REMOVE:
			if (rowCount() > 1)
				return p.entered ? trashIcon() : QVariant();
		}
	} else if (role == Qt::SizeHintRole) {
		switch (index.column()) {
		case REMOVE:
			if (rowCount() > 1)
				return p.entered ? trashIcon().size() : QVariant();
		}
	} else if (role == Qt::FontRole) {
		if (divepoints.at(index.row()).entered) {
			return defaultModelFont();
		} else {
			QFont font = defaultModelFont();
			font.setBold(true);
			return font;
		}
	}
	return QVariant();
}

bool DivePlannerPointsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	struct gasmix gas = { 0 };
	int i, shift;
	if (role == Qt::EditRole) {
		divedatapoint &p = divepoints[index.row()];
		switch (index.column()) {
		case DEPTH:
			if (value.toInt() >= 0)
				p.depth = units_to_depth(value.toInt());
			break;
		case RUNTIME:
			p.time = value.toInt() * 60;
			break;
		case DURATION:
			i = index.row();
			if (i)
				shift = divepoints[i].time - divepoints[i - 1].time - value.toInt() * 60;
			else
				shift = divepoints[i].time - value.toInt() * 60;
			while (i < divepoints.size())
				divepoints[i++].time -= shift;
			break;
		case CCSETPOINT: {
			int po2 = 0;
			QByteArray gasv = value.toByteArray();
			if (validate_po2(gasv.data(), &po2))
				p.setpoint = po2;
		} break;
		case GAS:
			QByteArray gasv = value.toByteArray();
			if (validate_gas(gasv.data(), &gas))
				p.gasmix = gas;
			break;
		}
		editStop(index.row(), p);
	}
	return QAbstractItemModel::setData(index, value, role);
}

QVariant DivePlannerPointsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		switch (section) {
		case DEPTH:
			return tr("Final depth");
		case RUNTIME:
			return tr("Run time");
		case DURATION:
			return tr("Duration");
		case GAS:
			return tr("Used gas");
		case CCSETPOINT:
			return tr("CC set point");
		}
	} else if (role == Qt::FontRole) {
		return defaultModelFont();
	}
	return QVariant();
}

Qt::ItemFlags DivePlannerPointsModel::flags(const QModelIndex &index) const
{
	if (index.column() != REMOVE)
		return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
	else
		return QAbstractItemModel::flags(index);
}

int DivePlannerPointsModel::rowCount(const QModelIndex &parent) const
{
	return divepoints.count();
}

DivePlannerPointsModel::DivePlannerPointsModel(QObject *parent) : QAbstractTableModel(parent),
	mode(NOTHING),
	tempGFHigh(100),
	tempGFLow(100)
{
	memset(&diveplan, 0, sizeof(diveplan));
}

DivePlannerPointsModel *DivePlannerPointsModel::instance()
{
	static QScopedPointer<DivePlannerPointsModel> self(new DivePlannerPointsModel());
	return self.data();
}

void DivePlannerPointsModel::emitDataChanged()
{
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setBottomSac(double sac)
{
	diveplan.bottomsac = units_to_sac(sac);
	prefs.bottomsac = diveplan.bottomsac;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDecoSac(double sac)
{
	diveplan.decosac = units_to_sac(sac);
	prefs.decosac = diveplan.decosac;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setGFHigh(const int gfhigh)
{
	tempGFHigh = gfhigh;
	// GFHigh <= 34 can cause infinite deco at 6m - don't trigger a recalculation
	// for smaller GFHigh unless the user explicitly leaves the field
	if (tempGFHigh > 34)
		triggerGFHigh();
}

void DivePlannerPointsModel::triggerGFHigh()
{
	if (diveplan.gfhigh != tempGFHigh) {
		diveplan.gfhigh = tempGFHigh;
		plannerModel->emitDataChanged();
	}
}

void DivePlannerPointsModel::setGFLow(const int ghflow)
{
	tempGFLow = ghflow;
	triggerGFLow();
}

void DivePlannerPointsModel::triggerGFLow()
{
	if (diveplan.gflow != tempGFLow) {
		diveplan.gflow = tempGFLow;
		plannerModel->emitDataChanged();
	}
}

void DivePlannerPointsModel::setSurfacePressure(int pressure)
{
	diveplan.surface_pressure = pressure;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setSalinity(int salinity)
{
	diveplan.salinity = salinity;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

int DivePlannerPointsModel::getSurfacePressure()
{
	return diveplan.surface_pressure;
}

void DivePlannerPointsModel::setLastStop6m(bool value)
{
	set_last_stop(value);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setVerbatim(bool value)
{
	set_verbatim(value);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDisplayRuntime(bool value)
{
	set_display_runtime(value);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDisplayDuration(bool value)
{
	set_display_duration(value);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDisplayTransitions(bool value)
{
	set_display_transitions(value);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDropStoneMode(bool value)
{
	prefs.drop_stone_mode = value;
	if (prefs.drop_stone_mode) {
	/* Remove the first entry if we enable drop_stone_mode */
		if (rowCount() >= 2) {
			beginRemoveRows(QModelIndex(), 0, 0);
			divepoints.remove(0);
			endRemoveRows();
		}
	} else {
		/* Add a first entry if we disable drop_stone_mode */
		beginInsertRows(QModelIndex(), 0, 0);
		/* Copy the first current point */
		divedatapoint p = divepoints.at(0);
		p.time = p.depth / prefs.descrate;
		divepoints.push_front(p);
		endInsertRows();
	}
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setStartDate(const QDate &date)
{
	startTime.setDate(date);
	diveplan.when = startTime.toTime_t();
	displayed_dive.when = diveplan.when;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setStartTime(const QTime &t)
{
	startTime.setTime(t);
	diveplan.when = startTime.toTime_t();
	displayed_dive.when = diveplan.when;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

bool divePointsLessThan(const divedatapoint &p1, const divedatapoint &p2)
{
	return p1.time <= p2.time;
}

bool DivePlannerPointsModel::addGas(struct gasmix mix)
{
	sanitize_gasmix(&mix);

	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &displayed_dive.cylinder[i];
		if (cylinder_nodata(cyl)) {
			fill_default_cylinder(cyl);
			cyl->gasmix = mix;
			/* The depth to change to that gas is given by the depth where its pO₂ is 1.6 bar.
			 * The user should be able to change this depth manually. */
			pressure_t modpO2;
			modpO2.mbar = prefs.decopo2;
			cyl->depth = gas_mod(&mix, modpO2, M_OR_FT(3,10));




			// FIXME -- need to get rid of stagingDIve
			// the following now uses displayed_dive !!!!



			CylindersModel::instance()->updateDive();
			return true;
		}
		if (!gasmix_distance(&cyl->gasmix, &mix))
			return true;
	}
	qDebug("too many gases");
	return false;
}

int DivePlannerPointsModel::lastEnteredPoint()
{
	for (int i = divepoints.count() - 1; i >= 0; i--)
		if (divepoints.at(i).entered)
			return i;
	return -1;
}

int DivePlannerPointsModel::addStop(int milimeters, int seconds, gasmix *gas_in, int ccpoint, bool entered)
{
	struct gasmix air = { 0 };
	struct gasmix gas = { 0 };
	bool usePrevious = false;
	if (gas_in)
		gas = *gas_in;
	else
		usePrevious = true;
	if (recalcQ())
		removeDeco();

	int row = divepoints.count();
	if (seconds == 0 && milimeters == 0 && row != 0) {
		/* this is only possible if the user clicked on the 'plus' sign on the DivePoints Table */
		const divedatapoint t = divepoints.at(lastEnteredPoint());
		milimeters = t.depth;
		seconds = t.time + 600; // 10 minutes.
		gas = t.gasmix;
		ccpoint = t.setpoint;
	} else if (seconds == 0 && milimeters == 0 && row == 0) {
		milimeters = M_OR_FT(5, 15); // 5m / 15ft
		seconds = 600;		     // 10 min
		//Default to the first defined gas, if we got one.
		cylinder_t *cyl = &displayed_dive.cylinder[0];
		if (cyl)
			gas = cyl->gasmix;
	}
	if (!usePrevious)
		if (!addGas(gas))
			qDebug("addGas failed"); // FIXME add error propagation

	// check if there's already a new stop before this one:
	for (int i = 0; i < row; i++) {
		const divedatapoint &dp = divepoints.at(i);
		if (dp.time == seconds) {
			row = i;
			beginRemoveRows(QModelIndex(), row, row);
			divepoints.remove(row);
			endRemoveRows();
			break;
		}
		if (dp.time > seconds) {
			row = i;
			break;
		}
	}
	// Previous, actually means next as we are typically subdiving a segment and the gas for
	// the segment is determined by the waypoint at the end.
	if (usePrevious) {
		if (row  < divepoints.count()) {
			gas = divepoints.at(row).gasmix;
		} else if (row > 0) {
			gas = divepoints.at(row - 1).gasmix;
		} else {
			if (!addGas(air))
				qDebug("addGas failed"); // FIXME add error propagation

		}
	}

	// add the new stop
	beginInsertRows(QModelIndex(), row, row);
	divedatapoint point;
	point.depth = milimeters;
	point.time = seconds;
	point.gasmix = gas;
	point.setpoint = ccpoint;
	point.entered = entered;
	point.next = NULL;
	divepoints.append(point);
	std::sort(divepoints.begin(), divepoints.end(), divePointsLessThan);
	endInsertRows();
	return row;
}

void DivePlannerPointsModel::editStop(int row, divedatapoint newData)
{
	divepoints[row] = newData;
	std::sort(divepoints.begin(), divepoints.end(), divePointsLessThan);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

int DivePlannerPointsModel::size()
{
	return divepoints.size();
}

divedatapoint DivePlannerPointsModel::at(int row)
{
	return divepoints.at(row);
}

void DivePlannerPointsModel::remove(const QModelIndex &index)
{
	int i;
	int rows = rowCount();
	if (index.column() != REMOVE || rowCount() == 1)
		return;

	divedatapoint dp = at(index.row());
	if (!dp.entered)
		return;

	if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
		beginRemoveRows(QModelIndex(), index.row(), rows - 1);
		for (i = rows - 1; i >= index.row(); i--)
			divepoints.remove(i);
	} else {
		beginRemoveRows(QModelIndex(), index.row(), index.row());
		divepoints.remove(index.row());
	}
	endRemoveRows();
}

struct diveplan &DivePlannerPointsModel::getDiveplan()
{
	return diveplan;
}

void DivePlannerPointsModel::cancelPlan()
{
	if (mode == PLAN && rowCount()) {
		if (QMessageBox::warning(MainWindow::instance(), TITLE_OR_TEXT(tr("Discard the plan?"),
									       tr("You are about to discard your plan.")),
					 QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard) != QMessageBox::Discard) {
			return;
		}
	}
	setPlanMode(NOTHING);
	free_dps(&diveplan);

	emit planCanceled();
}

DivePlannerPointsModel::Mode DivePlannerPointsModel::currentMode() const
{
	return mode;
}

QVector<QPair<int, int> > DivePlannerPointsModel::collectGases(struct dive *d)
{
	QVector<QPair<int, int> > l;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &d->cylinder[i];
		if (!cylinder_nodata(cyl))
			l.push_back(qMakePair(get_o2(&cyl->gasmix), get_he(&cyl->gasmix)));
	}
	return l;
}
void DivePlannerPointsModel::rememberTanks()
{
	oldGases = collectGases(&displayed_dive);
}

bool DivePlannerPointsModel::tankInUse(struct gasmix gasmix)
{
	for (int j = 0; j < rowCount(); j++) {
		divedatapoint &p = divepoints[j];
		if (p.time == 0) // special entries that hold the available gases
			continue;
		if (!p.entered) // removing deco gases is ok
			continue;
		if (gasmix_distance(&p.gasmix, &gasmix) < 200)
			return true;
	}
	return false;
}

void DivePlannerPointsModel::tanksUpdated()
{
	// we don't know exactly what changed - what we care about is
	// "did a gas change on us". So we look through the diveplan to
	// see if there is a gas that is now missing and if there is, we
	// replace it with the matching new gas.
	QVector<QPair<int, int> > gases = collectGases(&displayed_dive);
	if (gases.count() == oldGases.count()) {
		// either nothing relevant changed, or exactly ONE gasmix changed
		for (int i = 0; i < gases.count(); i++) {
			if (gases.at(i) != oldGases.at(i)) {
				if (oldGases.count(oldGases.at(i)) > 1) {
					// we had this gas more than once, so don't
					// change segments that used this gas as it still exists
					break;
				}
				for (int j = 0; j < rowCount(); j++) {
					divedatapoint &p = divepoints[j];
					struct gasmix gas;
					gas.o2.permille = oldGases.at(i).first;
					gas.he.permille = oldGases.at(i).second;
					if (gasmix_distance(&gas, &p.gasmix) < 200) {
						p.gasmix.o2.permille = gases.at(i).first;
						p.gasmix.he.permille = gases.at(i).second;
					}
				}
				break;
			}
		}
	}
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::clear()
{
	bool oldRecalc = setRecalc(false);

	CylindersModel::instance()->updateDive();
	if (rowCount() > 0) {
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		divepoints.clear();
		endRemoveRows();
	}
	CylindersModel::instance()->clear();
	setRecalc(oldRecalc);
}

void DivePlannerPointsModel::createTemporaryPlan()
{
	// Get the user-input and calculate the dive info
	free_dps(&diveplan);
	int lastIndex = -1;
	for (int i = 0; i < rowCount(); i++) {
		divedatapoint p = at(i);
		int deltaT = lastIndex != -1 ? p.time - at(lastIndex).time : p.time;
		lastIndex = i;
		if (i == 0 && prefs.drop_stone_mode) {
			/* Okay, we add a fist segment where we go down to depth */
			plan_add_segment(&diveplan, p.depth / prefs.descrate, p.depth, p.gasmix, p.setpoint, true);
			deltaT -= p.depth / prefs.descrate;
		}
		if (p.entered)
			plan_add_segment(&diveplan, deltaT, p.depth, p.gasmix, p.setpoint, true);
	}

	// what does the cache do???
	char *cache = NULL;
	struct divedatapoint *dp = NULL;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &displayed_dive.cylinder[i];
		if (cyl->depth.mm) {
			dp = create_dp(0, cyl->depth.mm, cyl->gasmix, 0);
			if (diveplan.dp) {
				dp->next = diveplan.dp;
				diveplan.dp = dp;
			} else {
				dp->next = NULL;
				diveplan.dp = dp;
			}
		}
	}
#if DEBUG_PLAN
	dump_plan(&diveplan);
#endif
	if (plannerModel->recalcQ() && !diveplan_empty(&diveplan)) {
		plan(&diveplan, &cache, isPlanner(), false);
		MainWindow::instance()->setPlanNotes(displayed_dive.notes);
	}
	// throw away the cache
	free(cache);
#if DEBUG_PLAN
	save_dive(stderr, &displayed_dive);
	dump_plan(&diveplan);
#endif
}

void DivePlannerPointsModel::deleteTemporaryPlan()
{
	free_dps(&diveplan);
}

void DivePlannerPointsModel::savePlan()
{
	createPlan(false);
}

void DivePlannerPointsModel::saveDuplicatePlan()
{
	createPlan(true);
}

void DivePlannerPointsModel::createPlan(bool replanCopy)
{
	// Ok, so, here the diveplan creates a dive
	char *cache = NULL;
	bool oldRecalc = plannerModel->setRecalc(false);
	removeDeco();
	createTemporaryPlan();
	plannerModel->setRecalc(oldRecalc);

	//TODO: C-based function here?
	plan(&diveplan, &cache, isPlanner(), true);
	if (!current_dive || displayed_dive.id != current_dive->id) {
		// we were planning a new dive, not re-planning an existing on
		record_dive(clone_dive(&displayed_dive));
	} else if (current_dive && displayed_dive.id == current_dive->id) {
		// we are replanning a dive - make sure changes are reflected
		// correctly in the dive structure and copy it back into the dive table
		displayed_dive.maxdepth.mm = 0;
		displayed_dive.dc.maxdepth.mm = 0;
		fixup_dive(&displayed_dive);
		if (replanCopy) {
			struct dive *copy = alloc_dive();
			copy_dive(current_dive, copy);
			copy->id = 0;
			copy->divetrip = NULL;
			if (current_dive->divetrip)
				add_dive_to_trip(copy, current_dive->divetrip);
			record_dive(copy);
		}
		copy_dive(&displayed_dive, current_dive);
	}
	mark_divelist_changed(true);

	// Remove and clean the diveplan, so we don't delete
	// the dive by mistake.
	free_dps(&diveplan);
	setPlanMode(NOTHING);
	planCreated();
}
