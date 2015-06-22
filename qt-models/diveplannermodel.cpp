#include "diveplannermodel.h"
#include "dive.h"
#include "helpers.h"
#include "cylindermodel.h"
#include "planner.h"
#include "models.h"

/* TODO: Port this to CleanerTableModel to remove a bit of boilerplate and
 * use the signal warningMessage() to communicate errors to the MainWindow.
 */
void DivePlannerPointsModel::removeSelectedPoints(const QVector<int> &rows)
{
	if (!rows.count())
		return;
	int firstRow = rowCount() - rows.count();
	QVector<int> v2 = rows;
	std::sort(v2.begin(), v2.end());

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
		addStop(M_OR_FT(15, 45), 1 * 60, &gas, 0, true);

	addStop(M_OR_FT(15, 45), 20 * 60, &gas, 0, true);
	if (!isPlanner()) {
		addStop(M_OR_FT(5, 15), 42 * 60, &gas, 0, true);
		addStop(M_OR_FT(5, 15), 45 * 60, &gas, 0, true);
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
	int depthsum = 0;
	int samplecount = 0;
	bool oldRec = recalc;
	recalc = false;
	CylindersModel::instance()->updateDive();
	duration_t lasttime = {};
	duration_t newtime = {};
	struct gasmix gas;
	free_dps(&diveplan);
	diveplan.when = d->when;
	// is this a "new" dive where we marked manually entered samples?
	// if yes then the first sample should be marked
	// if it is we only add the manually entered samples as waypoints to the diveplan
	// otherwise we have to add all of them
	bool hasMarkedSamples = d->dc.sample[0].manually_entered;
	// if this dive has more than 100 samples (so it is probably a logged dive),
	// average samples so we end up with a total of 100 samples.
	int plansamples = d->dc.samples <= 100 ? d->dc.samples : 100;
	int j = 0;
	for (int i = 0; i < plansamples - 1; i++) {
		while (j * plansamples <= i * d->dc.samples) {
			const sample &s = d->dc.sample[j];
			if (s.time.seconds != 0 && (!hasMarkedSamples || s.manually_entered)) {
				depthsum += s.depth.mm;
				++samplecount;
				newtime = s.time;
			}
			j++;
		}
		if (samplecount) {
			get_gas_at_time(d, &d->dc, lasttime, &gas);
			addStop(depthsum / samplecount, newtime.seconds, &gas, 0, true);
			lasttime = newtime;
			depthsum = 0;
			samplecount = 0;
		}
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
		// take the displayed cylinders from the selected dive as starting point
		CylindersModel::instance()->copyFromDive(current_dive);
		copy_cylinders(current_dive, &displayed_dive, !prefs.display_unused_tanks);
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
		list.push_back(get_gas_string(cyl->gasmix));
	}
	return list;
}

void DivePlannerPointsModel::removeDeco()
{
	bool oldrec = setRecalc(false);
	QVector<int> computedPoints;
	for (int i = 0; i < rowCount(); i++)
		if (!at(i).entered)
			computedPoints.push_back(i);
	removeSelectedPoints(computedPoints);
	setRecalc(oldrec);
}

void DivePlannerPointsModel::addCylinder_clicked()
{
	CylindersModel::instance()->add();
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
			return get_divepoint_gas_string(p);
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

void DivePlannerPointsModel::gaschange(const QModelIndex &index, QString newgas)
{
	int i = index.row();
	gasmix oldgas = divepoints[i].gasmix;
	gasmix gas = { 0 };
	if (!validate_gas(newgas.toUtf8().data(), &gas))
		return;
	while (i < rowCount() && gasmix_distance(&oldgas, &divepoints[i].gasmix) == 0)
		divepoints[i++].gasmix = gas;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
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
	recalc(false),
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
		emitDataChanged();
	}
}

void DivePlannerPointsModel::setGFLow(const int ghflow)
{
	tempGFLow = ghflow;
	triggerGFLow();
}

void DivePlannerPointsModel::setRebreatherMode(int mode)
{
	int i;
	displayed_dive.dc.divemode = (dive_comp_type) mode;
	for (i=0; i < rowCount(); i++)
		divepoints[i].setpoint = mode == CCR ? prefs.defaultsetpoint : 0;
	emitDataChanged();
}

void DivePlannerPointsModel::triggerGFLow()
{
	if (diveplan.gflow != tempGFLow) {
		diveplan.gflow = tempGFLow;
		emitDataChanged();
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
	prefs.last_stop = value;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setVerbatim(bool value)
{
	set_verbatim(value);
	prefs.verbatim_plan = value;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDisplayRuntime(bool value)
{
	set_display_runtime(value);
	prefs.display_runtime = value;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDisplayDuration(bool value)
{
	set_display_duration(value);
	prefs.display_duration = value;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setDisplayTransitions(bool value)
{
	set_display_transitions(value);
	prefs.display_transitions = value;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setRecreationalMode(bool value)
{
	prefs.recreational_mode = value;
	emit recreationChanged(value);
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS -1));
}

void DivePlannerPointsModel::setSafetyStop(bool value)
{
	prefs.safetystop = value;
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS -1));
}

void DivePlannerPointsModel::setReserveGas(int reserve)
{
	prefs.reserve_gas = reserve * 1000;
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

void DivePlannerPointsModel::setMinSwitchDuration(int duration)
{
	prefs.min_switch_duration = duration * 60;
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
			if (displayed_dive.dc.divemode == PSCR)
				modpO2.mbar = prefs.decopo2 + (1000 - get_o2(&mix)) * SURFACE_PRESSURE *
						prefs.o2consumption / prefs.decosac / prefs.pscr_ratio;
			else
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
	/*
	 * When moving divepoints rigorously, we might end up with index
	 * out of range, thus returning the last one instead.
	 */
	if (row >= divepoints.count())
		return;
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
	/*
	 * When moving divepoints rigorously, we might end up with index
	 * out of range, thus returning the last one instead.
	 */
	if (row >= divepoints.count())
		return divepoints.at(divepoints.count() - 1);
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

/* TODO: this seems so wrong.
 * We can't do this here if we plan to use QML on mobile
 * as mobile has no ControlModifier.
 * The correct thing to do is to create a new method
 * remove method that will pass the first and last index of the
 * removed rows, and remove those in a go.
 */
//	if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
//		beginRemoveRows(QModelIndex(), index.row(), rows - 1);
//		for (i = rows - 1; i >= index.row(); i--)
//			divepoints.remove(i);
//	} else {
		beginRemoveRows(QModelIndex(), index.row(), index.row());
		divepoints.remove(index.row());
//	}
	endRemoveRows();
}

struct diveplan &DivePlannerPointsModel::getDiveplan()
{
	return diveplan;
}

void DivePlannerPointsModel::cancelPlan()
{
	/* TODO:
	 * This check shouldn't be here - this is the interface responsability.
	 * as soon as the interface thinks that it could cancel the plan, this should be
	 * called.
	 */

	/*
	if (mode == PLAN && rowCount()) {
		if (QMessageBox::warning(MainWindow::instance(), TITLE_OR_TEXT(tr("Discard the plan?"),
												 tr("You are about to discard your plan.")),
					 QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Discard) != QMessageBox::Discard) {
			return;
		}
	}
	*/

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
		if (gasmix_distance(&p.gasmix, &gasmix) < 100)
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
					if (gasmix_distance(&gas, &p.gasmix) < 100) {
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
	if (recalcQ() && !diveplan_empty(&diveplan)) {
		plan(&diveplan, &cache, isPlanner(), false);
		emit calculatedPlanNotes();
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
	bool oldRecalc = setRecalc(false);
	removeDeco();
	createTemporaryPlan();
	setRecalc(oldRecalc);

	//TODO: C-based function here?
	bool did_deco = plan(&diveplan, &cache, isPlanner(), true);
	free(cache);
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
			QString oldnotes(current_dive->notes);
			if (oldnotes.indexOf(QString(disclaimer)) >= 0)
				oldnotes.truncate(oldnotes.indexOf(QString(disclaimer)));
			if (did_deco)
				oldnotes.append(displayed_dive.notes);
			displayed_dive.notes = strdup(oldnotes.toUtf8().data());
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
