#include "diveplanner.h"
#include "graphicsview-common.h"
#include "models.h"
#include "modeldelegates.h"
#include "mainwindow.h"
#include "maintab.h"
#include "tableview.h"

#include "../dive.h"
#include "../divelist.h"
#include "../planner.h"
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

#include <algorithm>

#define TIME_INITIAL_MAX 30

#define MAX_DEPTH M_OR_FT(150, 450)
#define MIN_DEPTH M_OR_FT(20, 60)

QString gasToStr(const int o2Permille, const int hePermille)
{
	uint o2 = (o2Permille + 5) / 10, he = (hePermille + 5) / 10;
	QString result = is_air(o2Permille, hePermille) ? QObject::tr("AIR") : he == 0 ? QString("EAN%1").arg(o2, 2, 10, QChar('0')) : QString("%1/%2").arg(o2).arg(he);
	return result;
}

QString dpGasToStr(const divedatapoint &p)
{
	return gasToStr(p.o2, p.he);
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

void DivePlannerPointsModel::createSimpleDive(bool planner)
{
	//	plannerModel->addStop(0, 0, O2_IN_AIR, 0, 0);
	plannerModel->addStop(M_OR_FT(15, 45), 1 * 60, O2_IN_AIR, 0, 0, true);
	plannerModel->addStop(M_OR_FT(15, 45), 40 * 60, O2_IN_AIR, 0, 0, true);
	if (!planner) {
		plannerModel->addStop(M_OR_FT(5, 15), 42 * 60, O2_IN_AIR, 0, 0, true);
		plannerModel->addStop(M_OR_FT(5, 15), 45 * 60, O2_IN_AIR, 0, 0, true);
	}
}

void DivePlannerPointsModel::loadFromDive(dive *d)
{
	// We need to make a copy, because as soon as the model is modified, it will
	// remove all samples from the dive.
	memcpy(&backupDive, d, sizeof(struct dive));
	copy_samples(d, &backupDive);
	copy_events(d, &backupDive);
	copy_cylinders(d, stagingDive); // this way the correct cylinder data is shown
	CylindersModel::instance()->setDive(stagingDive);
	int lasttime = 0;
	// we start with the first gas and see if it was changed
	int o2 = get_o2(&backupDive.cylinder[0].gasmix);
	int he = get_he(&backupDive.cylinder[0].gasmix);
	for (int i = 0; i < backupDive.dc.samples - 1; i++) {
		const sample &s = backupDive.dc.sample[i];
		if (s.time.seconds == 0)
			continue;
		get_gas_from_events(&backupDive.dc, lasttime, &o2, &he);
		plannerModel->addStop(s.depth.mm, s.time.seconds, o2, he, 0, true);
		lasttime = s.time.seconds;
	}
}

void DivePlannerPointsModel::restoreBackupDive()
{
	memcpy(current_dive, &backupDive, sizeof(struct dive));
}

void DivePlannerPointsModel::copyCylinders(dive *d)
{
	copy_cylinders(stagingDive, d);
}

void DivePlannerPointsModel::copyCylindersFrom(dive *d)
{
	copy_cylinders(d, stagingDive);
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
			list.push_back(gasToStr(get_o2(&cyl->gasmix), get_he(&cyl->gasmix)));
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
	connect(ui.gflow, SIGNAL(valueChanged(int)), plannerModel, SLOT(setGFLow(int)));
	connect(ui.lastStop, SIGNAL(toggled(bool)), plannerModel, SLOT(setLastStop6m(bool)));

	// Creating the plan
	connect(ui.buttonBox, SIGNAL(accepted()), plannerModel, SLOT(createPlan()));
	connect(ui.buttonBox, SIGNAL(rejected()), plannerModel, SLOT(cancelPlan()));
	connect(plannerModel, SIGNAL(planCreated()), MainWindow::instance(), SLOT(showProfile()));
	connect(plannerModel, SIGNAL(planCreated()), MainWindow::instance(), SLOT(refreshDisplay()));
	connect(plannerModel, SIGNAL(planCanceled()), MainWindow::instance(), SLOT(showProfile()));

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
	ui.gflow->setValue(prefs.gflow);
	ui.gfhigh->setValue(prefs.gfhigh);
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

void DivePlannerPointsModel::setPlanMode(Mode m)
{
	mode = m;
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
	if (role == Qt::DisplayRole) {
		divedatapoint p = divepoints.at(index.row());
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
			return QIcon(":trash");
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
	int o2 = 0;
	int he = 0;
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
			if (index.row())
				p.time = value.toInt() * 60 + divepoints[index.row() - 1].time;
			else
				p.time = value.toInt() * 60;
			break;
		case CCSETPOINT: {
			int po2 = 0;
			QByteArray gasv = value.toByteArray();
			if (validate_po2(gasv.data(), &po2))
				p.po2 = po2;
		} break;
		case GAS:
			QByteArray gasv = value.toByteArray();
			if (validate_gas(gasv.data(), &o2, &he)) {
				p.o2 = o2;
				p.he = he;
			}
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
	if (index.column() != DURATION)
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
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setGFLow(const int ghflow)
{
	diveplan.gflow = ghflow;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
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

void DivePlannerPointsModel::setStartTime(const QTime &t)
{
	diveplan.when = (t.msec() + QDateTime::currentMSecsSinceEpoch()) / 1000 - gettimezoneoffset();
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

bool divePointsLessThan(const divedatapoint &p1, const divedatapoint &p2)
{
	return p1.time <= p2.time;
}

bool DivePlannerPointsModel::addGas(int o2, int he)
{
	struct gasmix mix;

	mix.o2.permille = o2;
	mix.he.permille = he;
	sanitize_gasmix(&mix);

	if (is_air(o2, he))
		o2 = 0;

	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &stagingDive->cylinder[i];
		if (cylinder_nodata(cyl)) {
			fill_default_cylinder(cyl);
			cyl->gasmix.o2.permille = o2;
			cyl->gasmix.he.permille = he;
			sanitize_gasmix(&cyl->gasmix);
			/* The depth to change to that gas is given by the depth where its pO2 is 1.6 bar.
			 * The user should be able to change this depth manually. */
			pressure_t modppO2;
			modppO2.mbar = 1600;
			cyl->depth = gas_mod(&cyl->gasmix, modppO2);
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

int DivePlannerPointsModel::addStop(int milimeters, int seconds, int o2, int he, int ccpoint, bool entered)
{
	if (recalcQ())
		removeDeco();

	int row = divepoints.count();
	if (seconds == 0 && milimeters == 0 && row != 0) {
		/* this is only possible if the user clicked on the 'plus' sign on the DivePoints Table */
		const divedatapoint t = divepoints.at(lastEnteredPoint());
		milimeters = t.depth;
		seconds = t.time + 600; // 10 minutes.
		o2 = t.o2;
		he = t.he;
		ccpoint = t.po2;
	} else if (seconds == 0 && milimeters == 0 && row == 0) {
		milimeters = M_OR_FT(5, 15); // 5m / 15ft
		seconds = 600;		     // 10 min
		//Default to the first defined gas, if we got one.
		cylinder_t *cyl = &stagingDive->cylinder[0];
		if (cyl) {
			o2 = get_o2(&cyl->gasmix);
			he = get_he(&cyl->gasmix);
		}
	}
	if (o2 != -1)
		if (!addGas(o2, he))
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
	if (o2 == -1) {
		if (row > 0) {
			o2 = divepoints.at(row - 1).o2;
			he = divepoints.at(row - 1).he;
		} else {
			// when we add a first data point we need to make sure that there is a
			// tank for it to use;
			// first check to the right, then to the left, but if there's nothing,
			// we simply default to AIR
			if (row < divepoints.count()) {
				o2 = divepoints.at(row).o2;
				he = divepoints.at(row).he;
			} else {
				o2 = O2_IN_AIR;
				if (!addGas(o2, 0))
					qDebug("addGas failed"); // FIXME add error propagation
			}
		}
	}

	// add the new stop
	beginInsertRows(QModelIndex(), row, row);
	divedatapoint point;
	point.depth = milimeters;
	point.time = seconds;
	point.o2 = o2;
	point.he = he;
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
	clear();
	emit planCanceled();
	if (mode != ADD)
		free(stagingDive);
	setPlanMode(NOTHING);
	stagingDive = NULL;
	diveplan.dp = NULL;
	CylindersModel::instance()->setDive(current_dive);
	CylindersModel::instance()->update();
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

bool DivePlannerPointsModel::tankInUse(int o2, int he)
{
	for (int j = 0; j < rowCount(); j++) {
		divedatapoint &p = divepoints[j];
		if (p.time == 0) // special entries that hold the available gases
			continue;
		if (!p.entered) // removing deco gases is ok
			continue;
		if ((p.o2 == o2 && p.he == he) ||
		    (is_air(p.o2, p.he) && is_air(o2, he)))
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
					int o2 = oldGases.at(i).first;
					int he = oldGases.at(i).second;
					if ((p.o2 == o2 && p.he == he) ||
					    (is_air(p.o2, p.he) && (is_air(o2, he) || (o2 == 0 && he == 0)))) {
						p.o2 = gases.at(i).first;
						p.he = gases.at(i).second;
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
	if (mode == ADD) {
		stagingDive = current_dive;
	} else {
		if (!stagingDive)
			stagingDive = alloc_dive();
		memset(stagingDive->cylinder, 0, MAX_CYLINDERS * sizeof(cylinder_t));
	}
	CylindersModel::instance()->setDive(stagingDive);
	if (rowCount() > 0) {
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		divepoints.clear();
		endRemoveRows();
	}
	CylindersModel::instance()->clear();
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
			plan_add_segment(&diveplan, deltaT, p.depth, p.o2, p.he, p.po2, true);
	}
	char *cache = NULL;
	tempDive = NULL;
	struct divedatapoint *dp = NULL;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = &stagingDive->cylinder[i];
		if (cyl->depth.mm) {
			dp = create_dp(0, cyl->depth.mm, get_o2(&cyl->gasmix), get_he(&cyl->gasmix), 0);
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
	if (plannerModel->recalcQ())
		plan(&diveplan, &cache, &tempDive, isPlanner());
	if (mode == ADD || mode == PLAN) {
		// copy the samples and events, but don't overwrite the cylinders
		copy_samples(tempDive, current_dive);
		copy_events(tempDive, current_dive);
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

	if (!diveplan.dp)
		return cancelPlan();

	bool oldRecalc = plannerModel->setRecalc(false);
	removeDeco();
	createTemporaryPlan();
	plannerModel->setRecalc(oldRecalc);

	//TODO: C-based function here?
	plan(&diveplan, &cache, &tempDive, isPlanner());
	copy_cylinders(stagingDive, tempDive);
	int mean[MAX_CYLINDERS], duration[MAX_CYLINDERS];
	per_cylinder_mean_depth(tempDive, select_dc(tempDive), mean, duration);
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		cylinder_t *cyl = tempDive->cylinder + i;
		if (cylinder_none(cyl))
			continue;
		// FIXME: The epic assumption that all the cylinders after the first is deco
		int sac = i ? diveplan.decosac : diveplan.bottomsac;
		cyl->start.mbar = cyl->type.workingpressure.mbar;
		if (cyl->type.size.mliter) {
			int consumption = ((depth_to_mbar(mean[i], tempDive) * duration[i] / 60) * sac) / (cyl->type.size.mliter / 1000);
			cyl->end.mbar = cyl->start.mbar - consumption;
		} else {
			// Cylinders without a proper size are easily emptied.
			// Don't attempt to to calculate the infinite pressure drop.
			cyl->end.mbar = 0;
		}
	}
	mark_divelist_changed(true);

	// Remove and clean the diveplan, so we don't delete
	// the dive by mistake.
	diveplan.dp = NULL;
	clear();
	planCreated();
	setPlanMode(NOTHING);
	free(stagingDive);
	stagingDive = NULL;
	oldRecalc = plannerModel->setRecalc(false);
	CylindersModel::instance()->setDive(current_dive);
	CylindersModel::instance()->update();
	plannerModel->setRecalc(oldRecalc);
}
