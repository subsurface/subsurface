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

QString gasToStr(struct gasmix gas)
{
	uint o2 = (gas.o2.permille + 5) / 10, he = (gas.he.permille + 5) / 10;
	QString result = gasmix_is_air(&gas) ? QObject::tr("AIR") : he == 0 ? QString("EAN%1").arg(o2, 2, 10, QChar('0')) : QString("%1/%2").arg(o2).arg(he);
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
	if (isPlanner())
		// let's use the gas from the first cylinder
		gas = stagingDive->cylinder[0].gasmix;

	plannerModel->addStop(M_OR_FT(15, 45), 1 * 60, &gas, 0, true);
	plannerModel->addStop(M_OR_FT(15, 45), 40 * 60, &gas, 0, true);
	if (!isPlanner()) {
		plannerModel->addStop(M_OR_FT(5, 15), 42 * 60, &gas, 0, true);
		plannerModel->addStop(M_OR_FT(5, 15), 45 * 60, &gas, 0, true);
	}
}

void DivePlannerPointsModel::loadFromDive(dive *d)
{
	// We need to make a copy, because as soon as the model is modified, it will
	// remove all samples from the dive.
	memcpy(&backupDive, d, sizeof(struct dive));
	copy_samples(d, &backupDive);
	copy_events(d, &backupDive);
	copy_cylinders(d, stagingDive, false); // this way the correct cylinder data is shown
	CylindersModel::instance()->setDive(stagingDive);
	int lasttime = 0;
	// we start with the first gas and see if it was changed
	struct gasmix gas = backupDive.cylinder[0].gasmix;
	for (int i = 0; i < backupDive.dc.samples - 1; i++) {
		const sample &s = backupDive.dc.sample[i];
		if (s.time.seconds == 0)
			continue;
		get_gas_from_events(&backupDive.dc, lasttime, &gas);
		plannerModel->addStop(s.depth.mm, s.time.seconds, &gas, 0, true);
		lasttime = s.time.seconds;
	}
}

void DivePlannerPointsModel::restoreBackupDive()
{
	memcpy(current_dive, &backupDive, sizeof(struct dive));
}

void DivePlannerPointsModel::copyCylinders(dive *d)
{
	copy_cylinders(stagingDive, d, false);
}

// copy the tanks from the current dive, or the default cylinder
// or an unknown cylinder
// setup the cylinder widget accordingly
void DivePlannerPointsModel::setupCylinders()
{
	if (!stagingDive)
		return;

	if (stagingDive != current_dive) {
		// we are planning a dive
		if (current_dive) {
			// take the used cylinders from the selected dive as starting point
			CylindersModel::instance()->copyFromDive(current_dive);
			copy_cylinders(current_dive, stagingDive, true);
			reset_cylinders(stagingDive);
			return;
		} else {
			if (!same_string(prefs.default_cylinder, "")) {
				fill_default_cylinder(&stagingDive->cylinder[0]);
			} else {
				// roughly an AL80
				stagingDive->cylinder[0].type.description = strdup(tr("unknown").toUtf8().constData());
				stagingDive->cylinder[0].type.size.mliter = 11100;
				stagingDive->cylinder[0].type.workingpressure.mbar = 207000;
			}
		}
	}
	reset_cylinders(stagingDive);
	CylindersModel::instance()->copyFromDive(stagingDive);
}

QStringList &DivePlannerPointsModel::getGasList()
{
	struct dive *activeDive = isPlanner() ? stagingDive : current_dive;
	static QStringList list;
	list.clear();
	if (!activeDive) {
		list.push_back(tr("AIR"));
	} else {
		for (int i = 0; i < MAX_CYLINDERS; i++) {
			cylinder_t *cyl = &activeDive->cylinder[i];
			if (cylinder_nodata(cyl))
				break;
			list.push_back(gasToStr(cyl->gasmix));
		}
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
	m.addSeparator();
	m.addAction(QObject::tr("Remove this Point"), this, SLOT(selfRemove()));
	m.exec(event->screenPos());
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

DivePlannerWidget::DivePlannerWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	ui.setupUi(this);
	ui.tableWidget->setTitle(tr("Dive Planner Points"));
	ui.tableWidget->setModel(DivePlannerPointsModel::instance());
	DivePlannerPointsModel::instance()->setRecalc(true);
	ui.tableWidget->view()->setItemDelegateForColumn(DivePlannerPointsModel::GAS, new AirTypesDelegate(this));
	ui.cylinderTableWidget->setTitle(tr("Available Gases"));
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

	ui.tableWidget->setBtnToolTip(tr("add dive data point"));
	connect(ui.startTime, SIGNAL(timeChanged(QTime)), plannerModel, SLOT(setStartTime(QTime)));
	connect(ui.ATMPressure, SIGNAL(textChanged(QString)), this, SLOT(atmPressureChanged(QString)));
	connect(ui.bottomSAC, SIGNAL(textChanged(QString)), this, SLOT(bottomSacChanged(QString)));
	connect(ui.decoStopSAC, SIGNAL(textChanged(QString)), this, SLOT(decoSacChanged(QString)));
	connect(ui.gfhigh, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFHigh(int)));
	connect(ui.gfhigh, SIGNAL(editingFinished()), plannerModel, SLOT(emitDataChanged()));
	connect(ui.gflow, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFLow(int)));
	connect(ui.gflow, SIGNAL(editingFinished()), plannerModel, SLOT(emitDataChanged()));
	connect(ui.lastStop, SIGNAL(toggled(bool)), plannerModel, SLOT(setLastStop6m(bool)));
	connect(ui.verbatim_plan, SIGNAL(toggled(bool)), plannerModel, SLOT(setVerbatim(bool)));
	connect(ui.display_duration, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayDuration(bool)));
	connect(ui.display_runtime, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayRuntime(bool)));
	connect(ui.display_transitions, SIGNAL(toggled(bool)), plannerModel, SLOT(setDisplayTransitions(bool)));
	connect(ui.printPlan, SIGNAL(pressed()), this, SLOT(printDecoPlan()));

	// Creating (and canceling) the plan
	connect(ui.buttonBox, SIGNAL(accepted()), plannerModel, SLOT(createPlan()));
	connect(ui.buttonBox, SIGNAL(rejected()), plannerModel, SLOT(cancelPlan()));
	QShortcut *closeKey = new QShortcut(QKeySequence(Qt::Key_Escape), this);
	connect(closeKey, SIGNAL(activated()), plannerModel, SLOT(cancelPlan()));

	/* set defaults. */
	ui.startTime->setTime(QTime(1, 0));
	ui.ATMPressure->setText("1013");
	ui.bottomSAC->setText("20");
	ui.decoStopSAC->setText("17");
	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);

	setMinimumWidth(0);
	setMinimumHeight(0);
}

void DivePlannerWidget::settingsChanged()
{
	// right now there's nothing special we do when settings change
}

void DivePlannerPointsModel::addCylinder_clicked()
{
	CylindersModel::instance()->add();
}

void DivePlannerWidget::atmPressureChanged(const QString &pressure)
{
	plannerModel->setSurfacePressure(pressure.toInt());
}

void DivePlannerWidget::bottomSacChanged(const QString &bottomSac)
{
	plannerModel->setBottomSac(bottomSac.toInt());
}

void DivePlannerWidget::decoSacChanged(const QString &decosac)
{
	plannerModel->setDecoSac(decosac.toInt());
}

void DivePlannerWidget::printDecoPlan()
{
	MainWindow::instance()->printPlan();
}

void DivePlannerPointsModel::setPlanMode(Mode m)
{
	mode = m;
	if (m == NOTHING)
		stagingDive = NULL;
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
	return COLUMNS;
}

QVariant DivePlannerPointsModel::data(const QModelIndex &index, int role) const
{
	divedatapoint p = divepoints.at(index.row());
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		switch (index.column()) {
		case CCSETPOINT:
			return (double)p.po2 / 1000;
		case DEPTH:
			return rint(get_depth_units(p.depth, NULL, NULL));
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
			return p.entered ? QIcon(":trash") : QVariant();
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
				p.po2 = po2;
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
			return tr("Final Depth");
		case RUNTIME:
			return tr("Run time");
		case DURATION:
			return tr("Duration");
		case GAS:
			return tr("Used Gas");
		case CCSETPOINT:
			return tr("CC Set Point");
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

DivePlannerPointsModel::DivePlannerPointsModel(QObject *parent) : QAbstractTableModel(parent), mode(NOTHING), tempDive(NULL), stagingDive(NULL)
{
	memset(&diveplan, 0, sizeof(diveplan));
	memset(&backupDive, 0, sizeof(backupDive));
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

void DivePlannerPointsModel::setBottomSac(int sac)
{
	diveplan.bottomsac = sac * 1000;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDecoSac(int sac)
{
	diveplan.decosac = sac * 1000;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setGFHigh(const int gfhigh)
{
	diveplan.gfhigh = gfhigh;
}

void DivePlannerPointsModel::setGFLow(const int ghflow)
{
	diveplan.gflow = ghflow;
}

void DivePlannerPointsModel::setSurfacePressure(int pressure)
{
	diveplan.surface_pressure = pressure;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
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

void DivePlannerPointsModel::setStartTime(const QTime &t)
{
	diveplan.when = (t.msec() + QDateTime::currentMSecsSinceEpoch()) / 1000 - gettimezoneoffset();
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
		cylinder_t *cyl = &stagingDive->cylinder[i];
		if (cylinder_nodata(cyl)) {
			fill_default_cylinder(cyl);
			cyl->gasmix = mix;
			/* The depth to change to that gas is given by the depth where its pO2 is 1.6 bar.
			 * The user should be able to change this depth manually. */
			pressure_t modppO2;
			modppO2.mbar = 1600;
			cyl->depth = gas_mod(&mix, modppO2);
			CylindersModel::instance()->setDive(stagingDive);
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
		ccpoint = t.po2;
	} else if (seconds == 0 && milimeters == 0 && row == 0) {
		milimeters = M_OR_FT(5, 15); // 5m / 15ft
		seconds = 600;		     // 10 min
		//Default to the first defined gas, if we got one.
		cylinder_t *cyl = &stagingDive->cylinder[0];
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
	if (usePrevious) {
		if (row > 0) {
			gas = divepoints.at(row - 1).gasmix;
		} else {
			// when we add a first data point we need to make sure that there is a
			// tank for it to use;
			// first check to the right, then to the left, but if there's nothing,
			// we simply default to AIR
			if (row < divepoints.count()) {
				gas = divepoints.at(row).gasmix;
			} else {
				if (!addGas(air))
					qDebug("addGas failed"); // FIXME add error propagation
			}
		}
	}

	// add the new stop
	beginInsertRows(QModelIndex(), row, row);
	divedatapoint point;
	point.depth = milimeters;
	point.time = seconds;
	point.gasmix = gas;
	point.po2 = ccpoint;
	point.entered = entered;
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
	if (index.column() != REMOVE)
		return;

	divedatapoint dp = at(index.row());
	if (!dp.entered)
		return;

	beginRemoveRows(QModelIndex(), index.row(), index.row());
	divepoints.remove(index.row());
	endRemoveRows();
}

struct diveplan &DivePlannerPointsModel::getDiveplan()
{
	return diveplan;
}

void DivePlannerPointsModel::cancelPlan()
{
	if (mode == PLAN && rowCount()) {
		if (QMessageBox::warning(MainWindow::instance(), TITLE_OR_TEXT(tr("Discard the Plan?"),
									       tr("You are about to discard your plan.")),
					 QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard) != QMessageBox::Discard) {
			return;
		}
	}

	if (mode != ADD) // for ADD stagingDive points at current_dive
		free(stagingDive);
	stagingDive = NULL; // always reset the stagingDive to NULL
	setPlanMode(NOTHING);
	diveplan.dp = NULL;
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
	oldGases = collectGases(stagingDive);
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
	QVector<QPair<int, int> > gases = collectGases(stagingDive);
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
	Q_ASSERT(stagingDive == 0);
	if (mode == ADD) {
		stagingDive = current_dive;
	} else {
		stagingDive = alloc_dive();
	}
	bool oldRecalc = setRecalc(false);
	CylindersModel::instance()->setDive(stagingDive);
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
	// This needs to be done in the following steps:
	// Get the user-input and calculate the dive info
	// Not sure if this is the place to create the diveplan...
	// We just start with a surface node at time = 0
	if (!stagingDive)
		return;
	//TODO: this thingy looks like it could be a good C-based function
	diveplan.dp = NULL;
	int lastIndex = -1;
	for (int i = 0; i < rowCount(); i++) {
		divedatapoint p = at(i);
		int deltaT = lastIndex != -1 ? p.time - at(lastIndex).time : p.time;
		lastIndex = i;
		if (p.entered)
			plan_add_segment(&diveplan, deltaT, p.depth, p.gasmix, p.po2, true);
	}
	char *cache = NULL;
	tempDive = NULL;
	struct divedatapoint *dp = NULL;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &stagingDive->cylinder[i];
		if (cyl->depth.mm) {
			dp = create_dp(0, cyl->depth.mm, cyl->gasmix, 0);
			if (diveplan.dp) {
				dp->next = diveplan.dp->next;
				diveplan.dp->next = dp;
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
		plan(&diveplan, &cache, &tempDive, stagingDive, isPlanner(), false);
		MainWindow::instance()->setPlanNotes(tempDive->notes);
		if (mode == ADD || mode == PLAN) {
			// copy the samples and events, but don't overwrite the cylinders
			copy_samples(tempDive, current_dive);
			copy_events(tempDive, current_dive);
			copy_cylinders(tempDive, current_dive, false);
		}
	}
	// throw away the cache
	free(cache);
#if DEBUG_PLAN
	dump_plan(&diveplan);
#endif
}

void DivePlannerPointsModel::deleteTemporaryPlan()
{
	deleteTemporaryPlan(diveplan.dp);
	diveplan.dp = NULL;
	delete_single_dive(get_divenr(tempDive));
	tempDive = NULL;
}

void DivePlannerPointsModel::deleteTemporaryPlan(struct divedatapoint *dp)
{
	if (!dp)
		return;

	deleteTemporaryPlan(dp->next);
	free(dp);
}

void DivePlannerPointsModel::createPlan()
{
	// Ok, so, here the diveplan creates a dive,
	// puts it on the dive list, and we need to remember
	// to not delete it later. mumble. ;p
	char *cache = NULL;
	tempDive = NULL;

	bool oldRecalc = plannerModel->setRecalc(false);
	removeDeco();
	createTemporaryPlan();
	plannerModel->setRecalc(oldRecalc);

	//TODO: C-based function here?
	plan(&diveplan, &cache, &tempDive, stagingDive, isPlanner(), true);
	record_dive(tempDive);
	mark_divelist_changed(true);

	// Remove and clean the diveplan, so we don't delete
	// the dive by mistake.
	diveplan.dp = NULL;
	setPlanMode(NOTHING);
	free(stagingDive);
	stagingDive = NULL;
	planCreated();
}
