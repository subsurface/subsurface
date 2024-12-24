// SPDX-License-Identifier: GPL-2.0
#include "diveplannermodel.h"
#include "core/color.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/event.h"
#include "core/format.h"
#include "core/subsurface-string.h"
#include "qt-models/cylindermodel.h"
#include "core/metrics.h" // For defaultModelFont().
#include "core/planner.h"
#include "core/device.h"
#include "core/qthelper.h"
#include "core/range.h"
#include "core/sample.h"
#include "core/selection.h"
#include "core/subsurface-time.h"
#include "core/settings/qPrefDivePlanner.h"
#include "core/settings/qPrefUnit.h"
#if !defined(SUBSURFACE_TESTING)
#include "commands/command.h"
#endif // !SUBSURFACE_TESTING
#include "core/gettextfromc.h"
#include "core/deco.h"
#include <QApplication>
#include <QTextDocument>
#include <QtConcurrent>

#define VARIATIONS_IN_BACKGROUND 1

static double unit_factor()
{
	return prefs.units.length == units::METERS ? 1000.0 / 60.0
						   : feet_to_mm(1.0) / 60.0;
}

static constexpr int decotimestep = 60; // seconds

CylindersModel *DivePlannerPointsModel::cylindersModel()
{
	return &cylinders;
}

void DivePlannerPointsModel::removePoints(const std::vector<int> &rows)
{
	if (rows.empty())
		return;
	std::vector<int> v2 = rows;
	std::sort(v2.begin(), v2.end());

	for (int i = (int)v2.size() - 1; i >= 0; i--) {
		beginRemoveRows(QModelIndex(), v2[i], v2[i]);
		divepoints.erase(divepoints.begin() + v2[i]);
		endRemoveRows();
	}
}

void DivePlannerPointsModel::removeSelectedPoints(const std::vector<int> &rows)
{
	removePoints(rows);

	updateDiveProfile();
	emitDataChanged();
	cylinders.updateTrashIcon();
}

void DivePlannerPointsModel::createSimpleDive(struct dive *dIn)
{
	// clean out the dive and give it an id and the correct dc model
	d = dIn;
	dcNr = 0;
	d->clear();
	d->id = dive_getUniqID();
	d->when = QDateTime::currentMSecsSinceEpoch() / 1000L + gettimezoneoffset() + 3600;
	make_planner_dc(&d->dcs[0]);

	clear();
	removeDeco();
	setupCylinders();
	setupStartTime();

	// initialize the start time in the plan
	diveplan.when = dateTimeToTimestamp(startTime);
	d->when = diveplan.when;

	// Use gas from the first cylinder
	int cylinderid = 0;

	// If we're in drop_stone_mode, don't add a first point.
	// It will be added implicitly.
	if (!prefs.drop_stone_mode)
		addStop(m_or_ft(15, 45), 1 * 60, cylinderid, prefs.defaultsetpoint, true, UNDEF_COMP_TYPE);

	addStop(m_or_ft(15, 45), 20 * 60, 0, prefs.defaultsetpoint, true, UNDEF_COMP_TYPE);
	if (!isPlanner()) {
		addStop(m_or_ft(5, 15), 42 * 60, cylinderid, prefs.defaultsetpoint, true, UNDEF_COMP_TYPE);
		addStop(m_or_ft(5, 15), 45 * 60, cylinderid, prefs.defaultsetpoint, true, UNDEF_COMP_TYPE);
	}
	updateDiveProfile();
}

void DivePlannerPointsModel::setupStartTime()
{
	// if the latest dive is in the future, then start an hour after it ends
	// otherwise start an hour from now
	startTime = QDateTime::currentDateTimeUtc().addSecs(3600 + gettimezoneoffset());
	if (!divelog.dives.empty()) {
		time_t ends = divelog.dives.back()->endtime();
		time_t diff = ends - dateTimeToTimestamp(startTime);
		if (diff > 0)
			startTime = startTime.addSecs(diff + 3600);
	}
}

void DivePlannerPointsModel::loadFromDive(dive *dIn, int dcNrIn)
{
	d = dIn;
	dcNr = dcNrIn;

	depth_t depthsum;
	int samplecount = 0;
	o2pressure_t last_sp;
	struct divecomputer *dc = d->get_dc(dcNr);
	cylinders.updateDive(d, dcNr);
	duration_t lasttime;
	duration_t lastrecordedtime;
	duration_t newtime;

	clear();
	removeDeco();
	diveplan.dp.clear();

	diveplan.when = d->when;
	// is this a "new" dive where we marked manually entered samples?
	// if yes then the first sample should be marked
	// if it is we only add the manually entered samples as waypoints to the diveplan
	// otherwise we have to add all of them

	bool hasMarkedSamples = false;

	if (!dc->samples.empty())
		hasMarkedSamples = dc->samples[0].manually_entered;
	else
		fake_dc(dc);

	// if this dive has more than 100 samples (so it is probably a logged dive),
	// average samples so we end up with a total of 100 samples.
	int plansamples = std::min(static_cast<int>(dc->samples.size()), 100);
	int j = 0;
	int cylinderid = 0;

	gasmix_loop loop_gas(*d, *dc);
	divemode_loop loop_mode(*dc);
	for (int i = 0; i < plansamples - 1; i++) {
		if (dc->last_manual_time.seconds && dc->last_manual_time.seconds > 120 && lasttime.seconds >= dc->last_manual_time.seconds)
			break;
		while (j * plansamples <= i * static_cast<int>(dc->samples.size())) {
			const sample &s = dc->samples[j];
			if (s.time.seconds != 0 && (!hasMarkedSamples || s.manually_entered)) {
				depthsum += s.depth;
				if (j > 0)
					last_sp = dc->samples[j-1].setpoint;
				++samplecount;
				newtime = s.time;
			}
			j++;
		}
		if (samplecount) {
			cylinderid = get_cylinderid_at_time(d, dc, lasttime);
			duration_t nexttime = newtime;
			++nexttime.seconds;
			if (newtime.seconds - lastrecordedtime.seconds > 10 || cylinderid == get_cylinderid_at_time(d, dc, nexttime)) {
				if (newtime.seconds == lastrecordedtime.seconds)
					newtime.seconds += 10;

				[[maybe_unused]] auto [current_divemode, _cylinder_index, _gasmix] = get_dive_status_at(*d, *dc, newtime.seconds - 1, &loop_mode, &loop_gas);
				addStop(depthsum / samplecount, newtime.seconds, cylinderid, last_sp.mbar, true, current_divemode);
				lastrecordedtime = newtime;
			}
			lasttime = newtime;
			depthsum = 0_m;
			samplecount = 0;
		}
	}
	// make sure we get the last point right so the duration is correct
	[[maybe_unused]] auto [current_divemode, _cylinder_index, _gasmix] = get_dive_status_at(*d, *dc, dc->duration.seconds, &loop_mode, &loop_gas);
	if (!hasMarkedSamples && !dc->last_manual_time.seconds)
		addStop(0_m, dc->duration.seconds, cylinderid, last_sp.mbar, true, current_divemode);
	preserved_until = d->duration;

	updateDiveProfile();
	emitDataChanged();
}

// copy the tanks from the current dive, or the default cylinder
// or an unknown cylinder
// setup the cylinder widget accordingly
void DivePlannerPointsModel::setupCylinders()
{
	d->cylinders.clear();
	if (mode == PLAN && current_dive) {
		// take the displayed cylinders from the selected dive as starting point
		copy_used_cylinders(current_dive, d, !prefs.include_unused_tanks);
		reset_cylinders(d, true);

		if (!d->cylinders.empty()) {
			cylinders.updateDive(d, dcNr);
			return;		// We have at least one cylinder
		}
	}

	add_default_cylinder(d);
	cylinders.updateDive(d, dcNr);
}

// Update the dive's maximum depth.  Returns true if max. depth changed
bool DivePlannerPointsModel::updateMaxDepth()
{
	int prevMaxDepth = d->maxdepth.mm;
	d->maxdepth = 0_m;
	for (int i = 0; i < rowCount(); i++) {
		divedatapoint p = at(i);
		if (p.depth.mm > d->maxdepth.mm)
			d->maxdepth.mm = p.depth.mm;
	}
	return d->maxdepth.mm != prevMaxDepth;
}

void DivePlannerPointsModel::removeDeco()
{
	std::vector<int> computedPoints;
	for (int i = 0; i < rowCount(); i++) {
		if (!at(i).entered)
			computedPoints.push_back(i);
	}
	removePoints(computedPoints);
}

void DivePlannerPointsModel::addCylinder_clicked()
{
	cylinders.add();
}

void DivePlannerPointsModel::setPlanMode(Mode m)
{
	mode = m;
	// the planner may reset our GF settings that are used to show deco
	// reset them to what's in the preferences
	if (m != PLAN) {
		set_gf(prefs.gflow, prefs.gfhigh);
		set_vpmb_conservatism(prefs.vpmb_conservatism);
	}
}

bool DivePlannerPointsModel::isPlanner() const
{
	return mode == PLAN;
}

int DivePlannerPointsModel::columnCount(const QModelIndex&) const
{
	return COLUMNS; // to disable CCSETPOINT subtract one
}

static divemode_t get_local_divemode(struct dive *d, int dcNr, const divedatapoint &p)
{
	divemode_t divemode;
	switch (d->get_dc(dcNr)->divemode) {
	case OC:
	default:
		divemode = OC;

		break;
	case CCR:
		divemode = d->get_cylinder(p.cylinderid)->cylinder_use == DILUENT ? CCR : OC;
		if (prefs.allowOcGasAsDiluent && d->get_cylinder(p.cylinderid)->cylinder_use == OC_GAS && p.divemode == CCR)
			divemode = CCR;

		break;
	case PSCR:
		divemode = p.divemode == PSCR ? PSCR : OC;

		break;
	}

	return divemode;
}

QVariant DivePlannerPointsModel::data(const QModelIndex &index, int role) const
{
	const divedatapoint p = divepoints.at(index.row());
	bool isInappropriateCylinder = !is_cylinder_use_appropriate(*d->get_dc(dcNr), *d->get_cylinder(p.cylinderid), false);
	divemode_t divemode = get_local_divemode(d, dcNr, p);
	if (role == Qt::DisplayRole || role == Qt::EditRole) {

		switch (index.column()) {
		case CCSETPOINT:
			return (divemode == CCR) ? (double)(p.setpoint / 1000.0) : QVariant();
		case DEPTH:
			return int_cast<int>(get_depth_units(p.depth, NULL, NULL));
		case RUNTIME:
			return p.time / 60;
		case DURATION:
			if (index.row())
				return (p.time - divepoints.at(index.row() - 1).time) / 60;
			else
				return p.time / 60;
		case DIVEMODE:
			return gettextFromC::tr(divemode_text_ui[divemode]);
		case GAS:
			return get_dive_gas(d, dcNr, p.cylinderid);
		}
	} else if (role == Qt::DecorationRole) {
		switch (index.column()) {
		case REMOVE:
			if (rowCount() > 1)
				return p.entered ? trashIcon() : QVariant();
			else
				return trashForbiddenIcon();
		}
	} else if (role == Qt::SizeHintRole) {
		switch (index.column()) {
		case REMOVE:
			if (rowCount() > 1)
				return p.entered ? trashIcon().size() : QVariant();
			else
				return trashForbiddenIcon().size();
		}
	} else if (role == Qt::FontRole) {
		QFont font = defaultModelFont();

		font.setBold(!p.entered);

		font.setItalic(isInappropriateCylinder);

		return font;
	} else if (role == Qt::BackgroundRole) {
		switch (index.column()) {
		case GAS:
			if (isInappropriateCylinder)
				return REDORANGE1_HIGH_TRANS;

			break;
		case CCSETPOINT:
			if (divemode != CCR)
				return MED_GRAY_HIGH_TRANS;

			break;
		}
	}

	return QVariant();
}

bool DivePlannerPointsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	int i, shift;
	if (role == Qt::EditRole) {
		divedatapoint &p = divepoints[index.row()];
		switch (index.column()) {
		case DEPTH: {
			int depth = value.toInt();
			if (depth >= 0) {
				p.depth = units_to_depth(depth);
				if (updateMaxDepth())
					cylinders.updateBestMixes();
			}
			break;
		}
		case RUNTIME: {
			int secs = value.toInt() * 60;
			i = index.row();
			int duration = secs;
			if (i)
				duration -= divepoints[i-1].time;
			// Make sure segments have a minimal duration
			if (duration <= 0)
				secs += 10 - duration;
			p.time = secs;
			while (++i < divepoints.size())
				if (divepoints[i].time < divepoints[i - 1].time + 10)
					divepoints[i].time = divepoints[i - 1].time + 10;
			break;
		}
		case DURATION: {
			int secs = value.toInt() * 60;
			if (secs < 0)
				secs = 10;
			i = index.row();
			if (i)
				shift = divepoints[i].time - divepoints[i - 1].time - secs;
			else
				shift = divepoints[i].time - secs;
			while (i < divepoints.size())
				divepoints[i++].time -= shift;
			break;
		}
		case CCSETPOINT: {
			bool ok;
			int po2 = static_cast<int>(round(value.toFloat(&ok) * 100) * 10);

			if (ok)
				p.setpoint = std::max(po2, 160);

			break;
		}
		case GAS:
			if (value.toInt() >= 0)
				p.cylinderid = value.toInt();
			/* Did we change the start (dp 0) cylinder to another cylinderid than 0? */
			if (value.toInt() > 0 && index.row() == 0)
				cylinders.moveAtFirst(value.toInt());
			cylinders.updateTrashIcon();
			break;
		case DIVEMODE:
			if (value.toInt() < FREEDIVE) {
				p.divemode = (enum divemode_t) value.toInt();
			}
			break;
		}
		editStop(index.row(), p);
	}
	return QAbstractItemModel::setData(index, value, role);
}

void DivePlannerPointsModel::gasChange(const QModelIndex &index, int newcylinderid)
{
	int i = index.row(), oldcylinderid = divepoints[i].cylinderid;
	while (i < rowCount() && oldcylinderid == divepoints[i].cylinderid)
		divepoints[i++].cylinderid = newcylinderid;
	emitDataChanged();
}

void DivePlannerPointsModel::cylinderRenumber(int mapping[])
{
	for (int i = 0; i < rowCount(); i++) {
		if (mapping[divepoints[i].cylinderid] >= 0)
			divepoints[i].cylinderid = mapping[divepoints[i].cylinderid];
	}
	emitDataChanged();
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
			return tr("Setpoint");
		case DIVEMODE:
			return tr("Dive mode");
		}
	} else if (role == Qt::FontRole) {
		return defaultModelFont();
	}
	return QVariant();
}

Qt::ItemFlags DivePlannerPointsModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return QAbstractItemModel::flags(index);

	if (index.column() == REMOVE)
		return Qt::ItemIsEnabled;

	const divedatapoint p = divepoints.at(index.row());
	switch (index.column()) {
	case REMOVE:
		return QAbstractItemModel::flags(index);
	case CCSETPOINT:
		if (get_local_divemode(d, dcNr, p) != CCR)
			return QAbstractItemModel::flags(index) & ~Qt::ItemIsEditable & ~Qt::ItemIsEnabled;

		break;
	case DIVEMODE:
		if (!((d->get_dc(dcNr)->divemode == CCR && prefs.allowOcGasAsDiluent && d->get_cylinder(p.cylinderid)->cylinder_use == OC_GAS) || d->get_dc(dcNr)->divemode == PSCR))
			return QAbstractItemModel::flags(index) & ~Qt::ItemIsEditable & ~Qt::ItemIsEnabled;

		break;
	}

	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

int DivePlannerPointsModel::rowCount(const QModelIndex&) const
{
	return divepoints.count();
}

DivePlannerPointsModel::DivePlannerPointsModel(QObject *parent) : QAbstractTableModel(parent),
	d(nullptr),
	cylinders(true),
	mode(NOTHING)
{
	startTime.setTimeSpec(Qt::UTC);
	// use a Qt-connection to send the variations text across thread boundary (in case we
	// are calculating the variations in a background thread).
	connect(this, &DivePlannerPointsModel::variationsComputed, this, &DivePlannerPointsModel::computeVariationsDone);
}

DivePlannerPointsModel *DivePlannerPointsModel::instance()
{
	static DivePlannerPointsModel self;
	return &self;
}

void DivePlannerPointsModel::emitDataChanged()
{
	updateDiveProfile();
	emit dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, COLUMNS - 1));
}

void DivePlannerPointsModel::setBottomSac(double sac)
{
// mobile delivers the same value as desktop when using
// units:METERS
// however when using units:CUFT mobile deliver 0-300 which
// are really 0.00 - 3.00 so start be correcting that
#ifdef SUBSURFACE_MOBILE
	if (qPrefUnits::volume() == units::CUFT)
		sac /= 100; // cuft without decimals (0 - 300)
#endif
	diveplan.bottomsac = units_to_sac(sac);
	qPrefDivePlanner::set_bottomsac(diveplan.bottomsac);
	emitDataChanged();
}

void DivePlannerPointsModel::setDecoSac(double sac)
{
// mobile delivers the same value as desktop when using
// units:METERS
// however when using units:CUFT mobile deliver 0-300 which
// are really 0.00 - 3.00 so start be correcting that
#ifdef SUBSURFACE_MOBILE
	if (qPrefUnits::volume() == units::CUFT)
		sac /= 100; // cuft without decimals (0 - 300)
#endif
	diveplan.decosac = units_to_sac(sac);
	qPrefDivePlanner::set_decosac(diveplan.decosac);
	emitDataChanged();
}

void DivePlannerPointsModel::setSacFactor(double factor)
{
// sacfactor is normal x.y (one decimal), however mobile
// delivers 0 - 100 so adjust that to 0.0 - 10.0, to have
// the same value as desktop
#ifdef SUBSURFACE_MOBILE
	factor /= 10.0;
#endif
	qPrefDivePlanner::set_sacfactor((int) round(factor * 100));
	emitDataChanged();
}

void DivePlannerPointsModel::setProblemSolvingTime(int minutes)
{
	qPrefDivePlanner::set_problemsolvingtime(minutes);
	emitDataChanged();
}

void DivePlannerPointsModel::setGFHigh(const int gfhigh)
{
	if (diveplan.gfhigh != gfhigh) {
		diveplan.gfhigh = gfhigh;
		emitDataChanged();
	}
}

int DivePlannerPointsModel::gfHigh() const
{
	return diveplan.gfhigh;
}

void DivePlannerPointsModel::setGFLow(const int gflow)
{
	if (diveplan.gflow != gflow) {
		diveplan.gflow = gflow;
		emitDataChanged();
	}
}

int DivePlannerPointsModel::gfLow() const
{
	return diveplan.gflow;
}

void DivePlannerPointsModel::cylindersChanged()
{
	if (!d)
		return;

	cylinders.updateDive(d, dcNr);

	emitDataChanged();
	cylinders.emitDataChanged();
}

void DivePlannerPointsModel::setVpmbConservatism(int level)
{
	if (diveplan.vpmb_conservatism != level) {
		diveplan.vpmb_conservatism = level;
		emitDataChanged();
	}
}

void DivePlannerPointsModel::setSurfacePressure(pressure_t pressure)
{
	diveplan.surface_pressure = pressure;
	emitDataChanged();
}

void DivePlannerPointsModel::setSalinity(int salinity)
{
	diveplan.salinity = salinity;
	emitDataChanged();
}

pressure_t DivePlannerPointsModel::getSurfacePressure() const
{
	return diveplan.surface_pressure;
}

void DivePlannerPointsModel::setLastStop6m(bool value)
{
	qPrefDivePlanner::set_last_stop(value);
	emitDataChanged();
}

void DivePlannerPointsModel::setAscrate75Display(int rate)
{
	qPrefDivePlanner::set_ascrate75(lrint(rate * unit_factor()));
	emitDataChanged();
}
int DivePlannerPointsModel::ascrate75Display() const
{
	return lrint((float)prefs.ascrate75 / unit_factor());
}

void DivePlannerPointsModel::setAscrate50Display(int rate)
{
	qPrefDivePlanner::set_ascrate50(lrint(rate * unit_factor()));
	emitDataChanged();
}
int DivePlannerPointsModel::ascrate50Display() const
{
	return lrint((float)prefs.ascrate50 / unit_factor());
}

void DivePlannerPointsModel::setAscratestopsDisplay(int rate)
{
	qPrefDivePlanner::set_ascratestops(lrint(rate * unit_factor()));
	emitDataChanged();
}

int DivePlannerPointsModel::ascratestopsDisplay() const
{
	return lrint((float)prefs.ascratestops / unit_factor());
}

void DivePlannerPointsModel::setAscratelast6mDisplay(int rate)
{
	qPrefDivePlanner::set_ascratelast6m(lrint(rate * unit_factor()));
	emitDataChanged();
}
int DivePlannerPointsModel::ascratelast6mDisplay() const
{
	return lrint((float)prefs.ascratelast6m / unit_factor());
}

void DivePlannerPointsModel::setDescrateDisplay(int rate)
{
	qPrefDivePlanner::set_descrate(lrint(rate * unit_factor()));
	emitDataChanged();
}
int DivePlannerPointsModel::descrateDisplay() const
{
	return lrint((float)prefs.descrate / unit_factor());
}

void DivePlannerPointsModel::setVerbatim(bool value)
{
	qPrefDivePlanner::set_verbatim_plan(value);
	emitDataChanged();
}

void DivePlannerPointsModel::setDisplayRuntime(bool value)
{
	qPrefDivePlanner::set_display_runtime(value);
	emitDataChanged();
}

void DivePlannerPointsModel::setDisplayDuration(bool value)
{
	qPrefDivePlanner::set_display_duration(value);
	emitDataChanged();
}

void DivePlannerPointsModel::setDisplayTransitions(bool value)
{
	qPrefDivePlanner::set_display_transitions(value);
	emitDataChanged();
}

void DivePlannerPointsModel::setDisplayVariations(bool value)
{
	qPrefDivePlanner::set_display_variations(value);
	emitDataChanged();
}

void DivePlannerPointsModel::setDecoMode(int mode)
{
	qPrefDivePlanner::set_planner_deco_mode(deco_mode(mode));
	emit recreationChanged(mode == int(prefs.planner_deco_mode));
	emitDataChanged();
}

void DivePlannerPointsModel::setSafetyStop(bool value)
{
	qPrefDivePlanner::set_safetystop(value);
	emitDataChanged();
}

void DivePlannerPointsModel::setReserveGas(int reserve)
{
	if (prefs.units.pressure == units::BAR)
		qPrefDivePlanner::set_reserve_gas(reserve * 1000);
	else
		qPrefDivePlanner::set_reserve_gas(psi_to_mbar(reserve));
	emitDataChanged();
}

void DivePlannerPointsModel::setDropStoneMode(bool value)
{
	qPrefDivePlanner::set_drop_stone_mode(value);
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
		p.time = p.depth.mm / prefs.descrate;
		divepoints.push_front(p);
		endInsertRows();
	}
	emitDataChanged();
}

void DivePlannerPointsModel::setSwitchAtReqStop(bool value)
{
	qPrefDivePlanner::set_switch_at_req_stop(value);
	emitDataChanged();
}

void DivePlannerPointsModel::setMinSwitchDuration(int duration)
{
	qPrefDivePlanner::set_min_switch_duration(duration * 60);
	emitDataChanged();
}

void DivePlannerPointsModel::setSurfaceSegment(int duration)
{
	qPrefDivePlanner::set_surface_segment(duration * 60);
	emitDataChanged();
}

void DivePlannerPointsModel::setStartDate(const QDate &date)
{
	startTime.setDate(date);
	diveplan.when = dateTimeToTimestamp(startTime);
	d->when = diveplan.when;
	emitDataChanged();
}

void DivePlannerPointsModel::setStartTime(const QTime &t)
{
	startTime.setTime(t);
	diveplan.when = dateTimeToTimestamp(startTime);
	d->when = diveplan.when;
	emitDataChanged();
}

bool divePointsLessThan(const divedatapoint &p1, const divedatapoint &p2)
{
	return p1.time < p2.time;
}

int DivePlannerPointsModel::lastEnteredPoint() const
{
	for (int i = divepoints.count() - 1; i >= 0; i--)
		if (divepoints.at(i).entered)
			return i;
	return -1;
}

void DivePlannerPointsModel::addDefaultStop()
{
	removeDeco();
	addStop(0_m, 0, -1, prefs.defaultsetpoint, true, UNDEF_COMP_TYPE);
}

void DivePlannerPointsModel::addStop(depth_t depth, int seconds)
{
	removeDeco();
	addStop(depth, seconds, -1, prefs.defaultsetpoint, true, UNDEF_COMP_TYPE);
	updateDiveProfile();
}

// cylinderid_in == -1 means same gas as before.
// divemode == UNDEF_COMP_TYPE means determine from previous point.
int DivePlannerPointsModel::addStop(depth_t depth, int seconds, int cylinderid_in, int ccpoint, bool entered, enum divemode_t divemode)
{
	int cylinderid = 0;
	bool usePrevious = false;
	if (cylinderid_in >= 0)
		cylinderid = cylinderid_in;
	else
		usePrevious = true;

	int row = divepoints.count();
	if (seconds == 0 && depth.mm == 0) {
		if (row == 0) {
			depth = m_or_ft(5, 15); // 5m / 15ft
			seconds = 600;		     // 10 min
			// Default to the first cylinder
			cylinderid = 0;
		} else {
			/* this is only possible if the user clicked on the 'plus' sign on the DivePoints Table */
			const divedatapoint t = divepoints.at(lastEnteredPoint());
			depth = t.depth;
			seconds = t.time + 600; // 10 minutes.
			cylinderid = t.cylinderid;
			ccpoint = t.setpoint;
		}
	}

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
			cylinderid = divepoints.at(row).cylinderid;
			if (divemode == UNDEF_COMP_TYPE)
				divemode = divepoints.at(row).divemode;
			ccpoint = divepoints.at(row).setpoint;
		} else if (row > 0) {
			cylinderid = divepoints.at(row - 1).cylinderid;
			if (divemode == UNDEF_COMP_TYPE)
				divemode = divepoints.at(row - 1).divemode;
			ccpoint = divepoints.at(row -1).setpoint;
		}
	}
	if (divemode == UNDEF_COMP_TYPE)
		divemode = d->get_dc(dcNr)->divemode;

	// add the new stop
	beginInsertRows(QModelIndex(), row, row);
	divedatapoint point(seconds, depth, cylinderid, ccpoint, entered);
	point.divemode = divemode;
	divepoints.insert(divepoints.begin() + row, point);
	endInsertRows();
	return row;
}

void DivePlannerPointsModel::editStop(int row, divedatapoint newData)
{
	if (row < 0 || row >= divepoints.count())
		return;

	// Refuse to move to 0, since that has special meaning.
	if (newData.time <= 0)
		return;

	/*
	 * When moving divepoints rigorously, we might end up with index
	 * out of range, thus returning the last one instead.
	 */
	int old_first_cylid = divepoints[0].cylinderid;

	// Refuse creation of two points with the same time stamp.
	// Note: "time" is moved in the positive direction to avoid
	// time becoming zero or, worse, negative.
	while (std::any_of(divepoints.begin(), divepoints.begin() + row,
			[t = newData.time] (const divedatapoint &data)
			{ return data.time == t; }))
		newData.time += 10;
	while (std::any_of(divepoints.begin() + row + 1, divepoints.end(),
			[t = newData.time] (const divedatapoint &data)
			{ return data.time == t; }))
		newData.time += 10;

	// Is it ok to change data first and then move the rows?
	divepoints[row] = newData;

	// If the time changed, the item might have to be moved. Oh joy.
	int newRow = row;
	while (newRow + 1 < divepoints.count() && divepoints[newRow + 1].time < divepoints[row].time)
		++newRow;
	if (newRow != row) {
		++newRow; // Move one past item with smaller time stamp
	} else {
		// If we didn't move forward, try moving backwards
		while (newRow > 0 && divepoints[newRow - 1].time > divepoints[row].time)
			--newRow;
	}

	if (newRow != row && newRow != row + 1) {
		beginMoveRows(QModelIndex(), row, row, QModelIndex(), newRow);
		move_in_range(divepoints, row, row + 1, newRow);
		endMoveRows();

		// Account for moving the row backwards in the array.
		row = newRow > row ? newRow - 1 : newRow;
	}

	if (updateMaxDepth())
		cylinders.updateBestMixes();
	if (divepoints[0].cylinderid != old_first_cylid)
		cylinders.moveAtFirst(divepoints[0].cylinderid);

	updateDiveProfile();
	emit dataChanged(createIndex(row, 0), createIndex(row, COLUMNS - 1));
}

divedatapoint DivePlannerPointsModel::at(int row) const
{
	/*
	 * When moving divepoints rigorously, we might end up with index
	 * out of range, thus returning the last one instead.
	 */
	if (row >= divepoints.count())
		return divepoints.at(divepoints.count() - 1);
	return divepoints.at(row);
}

void DivePlannerPointsModel::removeControlPressed(const QModelIndex &index)
{
	// Never delete all points.
	int rows = rowCount();
	if (index.column() != REMOVE || index.row() <= 0 || index.row() >= rows)
		return;

	int old_first_cylid = divepoints[0].cylinderid;

	preserved_until.seconds = divepoints.at(index.row()).time;
	beginRemoveRows(QModelIndex(), index.row(), rows - 1);
	divepoints.erase(divepoints.begin() + index.row(), divepoints.end());
	endRemoveRows();

	cylinders.updateTrashIcon();
	if (divepoints[0].cylinderid != old_first_cylid)
		cylinders.moveAtFirst(divepoints[0].cylinderid);

	updateDiveProfile();
	emitDataChanged();
}

void DivePlannerPointsModel::remove(const QModelIndex &index)
{
/* TODO: this seems so wrong.
 * We can't do this here if we plan to use QML on mobile
 * as mobile has no ControlModifier.
 * The correct thing to do is to create a new method
 * remove method that will pass the first and last index of the
 * removed rows, and remove those in a go.
 */
	if (QApplication::keyboardModifiers() & Qt::ControlModifier)
		return removeControlPressed(index);

	// Refuse deleting the last point.
	int rows = rowCount();
	if (index.column() != REMOVE || index.row() < 0 || index.row() >= rows || rows <= 1)
		return;

	divedatapoint dp = at(index.row());
	if (!dp.entered)
		return;

	int old_first_cylid = divepoints[0].cylinderid;

	if (index.row() == rows)
		preserved_until.seconds = divepoints.at(rows - 1).time;
	beginRemoveRows(QModelIndex(), index.row(), index.row());
	divepoints.remove(index.row());
	endRemoveRows();

	cylinders.updateTrashIcon();
	if (divepoints[0].cylinderid != old_first_cylid)
		cylinders.moveAtFirst(divepoints[0].cylinderid);

	updateDiveProfile();
	emitDataChanged();
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
	diveplan.dp.clear();

	emit planCanceled();
}

DivePlannerPointsModel::Mode DivePlannerPointsModel::currentMode() const
{
	return mode;
}

bool DivePlannerPointsModel::tankInUse(int cylinderid) const
{
	for (int j = 0; j < rowCount(); j++) {
		const divedatapoint &p = divepoints[j];
		if (p.time == 0) // special entries that hold the available gases
			continue;
		if (!p.entered) // removing deco gases is ok
			continue;
		if (p.cylinderid == cylinderid) // tank is in use
			return true;
	}
	return false;
}

void DivePlannerPointsModel::clear()
{
	cylinders.clear();
	preserved_until = 0_sec;
	beginResetModel();
	divepoints.clear();
	endResetModel();
}

void DivePlannerPointsModel::createTemporaryPlan()
{
	// Get the user-input and calculate the dive info
	diveplan.dp.clear();

	for (auto [i, cyl]: enumerated_range(d->cylinders)) {
		if (cyl.depth.mm && cyl.cylinder_use == OC_GAS)
			plan_add_segment(diveplan, 0, cyl.depth, i, 0, false, OC);
	}

	int lastIndex = -1;
	for (int i = 0; i < rowCount(); i++) {
		const divedatapoint p = at(i);
		divemode_t divemode = get_local_divemode(d, dcNr, p);
		int deltaT = lastIndex != -1 ? p.time - at(lastIndex).time : p.time;
		lastIndex = i;
		if (i == 0 && mode == PLAN && prefs.drop_stone_mode) {
			/* Okay, we add a first segment where we go down to depth */
			plan_add_segment(diveplan, p.depth.mm / prefs.descrate, p.depth, p.cylinderid, divemode == CCR ? p.setpoint : 0, true, divemode);
			deltaT -= p.depth.mm / prefs.descrate;
		}
		if (p.entered)
			plan_add_segment(diveplan, deltaT, p.depth, p.cylinderid, divemode == CCR ? p.setpoint : 0, true, divemode);
	}

#if DEBUG_PLAN
	dump_plan(diveplan);
#endif
}

static bool shouldComputeVariations()
{
	return prefs.display_variations && decoMode(true) != RECREATIONAL;
}

void DivePlannerPointsModel::updateDiveProfile()
{
	if (!d)
		return;
	createTemporaryPlan();
	if (diveplan.is_empty())
		return;

	// For calculating variations, we need a copy of the plan. We have to copy _before_
	// calling plan(), because that adds deco stops.
	bool computeVariations = isPlanner() && shouldComputeVariations();
	struct diveplan plan_copy;
	if (computeVariations)
		plan_copy = diveplan;

	deco_state_cache cache;
	struct deco_state plan_deco_state;

	plan(&plan_deco_state, diveplan, d, dcNr, decotimestep, cache, isPlanner(), false);
	updateMaxDepth();

	if (computeVariations) {
#ifdef VARIATIONS_IN_BACKGROUND
		QtConcurrent::run([this, plan = std::move(plan_copy), deco = plan_deco_state] ()
				  { this->computeVariations(std::move(plan), deco); });
#else
		computeVariations(std::move(plan_copy), plan_deco_state);
#endif
		final_deco_state = plan_deco_state;
	}
	emit calculatedPlanNotes(QString::fromStdString(d->notes));

#if DEBUG_PLAN
	save_dive(stderr, *d);
	dump_plan(&diveplan);
#endif
}

void DivePlannerPointsModel::deleteTemporaryPlan()
{
	diveplan.dp.clear();
}

void DivePlannerPointsModel::savePlan()
{
	createPlan(false);
}

void DivePlannerPointsModel::saveDuplicatePlan()
{
	createPlan(true);
}

int DivePlannerPointsModel::analyzeVariations(const std::vector<decostop> &min, const std::vector<decostop> &mid, const std::vector<decostop> &max, const char *unit)
{
	auto sum_time = [](int time, const decostop &ds) { return ds.time + time; };
	int minsum = std::accumulate(min.begin(), min.end(), 0, sum_time);
	int midsum = std::accumulate(mid.begin(), mid.end(), 0, sum_time);
	int maxsum = std::accumulate(max.begin(), max.end(), 0, sum_time);
	int leftsum = midsum - minsum;
	int rightsum = maxsum - midsum;

#ifdef DEBUG_STOPVAR
	printf("Total + %d:%02d/%s +- %d s/%s\n\n", FRACTION_TUPLE((leftsum + rightsum) / 2, 60), unit,
						       (rightsum - leftsum) / 2, unit);
#else
	Q_UNUSED(unit)
#endif
	return (leftsum + rightsum) / 2;
}

// Return reference to second to last element.
// Caller is responsible for checking that there are at least two elements.
template <typename T>
auto &second_to_last(T &v)
{
	return *std::prev(std::prev(v.end()));
}

void DivePlannerPointsModel::computeVariations(struct diveplan original_plan, struct deco_state ds)
{
	// nothing to do unless there's an original plan
	if (original_plan.dp.empty())
		return;

	auto dive = std::make_unique<struct dive>();
	copy_dive(d, dive.get());
	deco_state_cache cache, save;
	struct diveplan plan_copy;

	int my_instance = ++instanceCounter;
	save.cache(&ds);

	duration_t delta_time = 1_min;
	QString time_units = tr("min");
	depth_t delta_depth;
	QString depth_units;

	if (prefs.units.length == units::METERS) {
		delta_depth = 1_m;
		depth_units = tr("m");
	} else {
		delta_depth = 1_ft;
		depth_units = tr("ft");
	}

	plan_copy = original_plan;
	if (plan_copy.dp.size() < 2)
		return;
	if (my_instance != instanceCounter)
		return;
	auto original = plan(&ds, plan_copy, dive.get(), dcNr, 1, cache, true, false);
	save.restore(&ds, false);

	plan_copy = original_plan;
	second_to_last(plan_copy.dp).depth.mm += delta_depth.mm;
	plan_copy.dp.back().depth.mm += delta_depth.mm;
	if (my_instance != instanceCounter)
		return;
	auto deeper = plan(&ds, plan_copy, dive.get(), dcNr, 1, cache, true, false);
	save.restore(&ds, false);

	plan_copy = original_plan;
	second_to_last(plan_copy.dp).depth.mm -= delta_depth.mm;
	plan_copy.dp.back().depth.mm -= delta_depth.mm;
	if (my_instance != instanceCounter)
		return;
	auto shallower = plan(&ds, plan_copy, dive.get(), dcNr, 1, cache, true, false);
	save.restore(&ds, false);

	plan_copy = original_plan;
	plan_copy.dp.back().time += delta_time.seconds;
	if (my_instance != instanceCounter)
		return;
	auto longer = plan(&ds, plan_copy, dive.get(), dcNr, 1, cache, true, false);
	save.restore(&ds, false);

	plan_copy = original_plan;
	plan_copy.dp.back().time -= delta_time.seconds;
	if (my_instance != instanceCounter)
		return;
	auto shorter = plan(&ds, plan_copy, dive.get(), dcNr, 1, cache, true, false);
	save.restore(&ds, false);

	std::string buf = format_string_std(", %s: %c %d:%02d /%s %c %d:%02d /min", qPrintable(tr("Stop times")),
		SIGNED_FRAC_TRIPLET(analyzeVariations(shallower, original, deeper, qPrintable(depth_units)), 60), qPrintable(depth_units),
		SIGNED_FRAC_TRIPLET(analyzeVariations(shorter, original, longer, qPrintable(time_units)), 60));

	// By using a signal, we can transport the variations to the main thread.
	emit variationsComputed(QString::fromStdString(buf));
#ifdef DEBUG_STOPVAR
	printf("\n\n");
#endif
}

void DivePlannerPointsModel::computeVariationsDone(QString variations)
{
	QString notes = QString::fromStdString(d->notes);
	notes = notes.replace("VARIATIONS", variations);
	d->notes = notes.toStdString();
	emit calculatedPlanNotes(notes);
}

static void addDive(dive *d, bool autogroup, bool newNumber)
{
	// Create a new dive and clear out the old one.
	auto new_d = std::make_unique<dive>();
	std::swap(*d, *new_d);
#if !defined(SUBSURFACE_TESTING)
	Command::addDive(std::move(new_d), autogroup, newNumber);
#endif // !SUBSURFACE_TESTING
}

void DivePlannerPointsModel::createPlan(bool saveAsNew)
{
	// Ok, so, here the diveplan creates a dive
	deco_state_cache cache;
	removeDeco();
	createTemporaryPlan();

	// For calculating variations, we need a copy of the plan. We have to copy _before_
	// calling plan(), because that adds deco stops.
	struct diveplan plan_copy;
	if (shouldComputeVariations())
		plan_copy = diveplan;

	plan(&ds_after_previous_dives, diveplan, d, dcNr, decotimestep, cache, isPlanner(), true);

	if (shouldComputeVariations())
		computeVariations(std::move(plan_copy), ds_after_previous_dives);

	// Fixup planner notes.
	if (current_dive && d->id == current_dive->id) {
		// Try to identify old planner output and remove only this part
		// Treat user provided text as plain text.
		QTextDocument notesDocument;
		notesDocument.setHtml(QString::fromStdString(current_dive->notes));
		QString oldnotes(notesDocument.toPlainText());
		QString disclaimer = get_planner_disclaimer();
		int disclaimerMid = disclaimer.indexOf("%s");
		QString disclaimerBegin, disclaimerEnd;
		if (disclaimerMid >= 0) {
			disclaimerBegin = disclaimer.left(disclaimerMid);
			disclaimerEnd = disclaimer.mid(disclaimerMid + 2);
		} else {
			disclaimerBegin = std::move(disclaimer);
		}
		int disclaimerPositionStart = oldnotes.indexOf(disclaimerBegin);
		if (disclaimerPositionStart >= 0) {
			if (oldnotes.indexOf(disclaimerEnd, disclaimerPositionStart) >= 0) {
				// We found a disclaimer according to the current locale.
				// Remove the disclaimer and anything after the disclaimer, because
				// that's supposedly the old planner notes.
				oldnotes = oldnotes.left(disclaimerPositionStart);
			}
		}
		// Deal with line breaks
		oldnotes.replace("\n", "<br>");
		oldnotes.append(QString::fromStdString(d->notes));
		d->notes = oldnotes.toStdString();
		// If we save as new create a copy of the dive here
	}

	setPlanMode(NOTHING);

	// Now, add or modify the dive.
	if (!current_dive || d->id != current_dive->id) {
		// we were planning a new dive, not re-planning an existing one
		d->divetrip = nullptr; // Should not be necessary, just in case!
		addDive(d, divelog.autogroup, true);
	} else {
		copy_events_until(current_dive, d, dcNr, preserved_until.seconds);
		if (saveAsNew) {
			// we were planning an old dive and save as a new dive
			d->id = dive_getUniqID(); // Things will break horribly if we create dives with the same id.
			addDive(d, false, false);
		} else {
			// we were planning an old dive and rewrite the plan
#if !defined(SUBSURFACE_TESTING)
			Command::replanDive(d);
#endif // !SUBSURFACE_TESTING
		}
	}

	// Remove and clean the diveplan, so we don't delete
	// the dive by mistake.
	diveplan.dp.clear();

	planCreated(); // This signal will exit the UI from planner state.
}
