// SPDX-License-Identifier: GPL-2.0
/* divelist.cpp */

#include "divelist.h"
#include "subsurface-string.h"
#include "deco.h"
#include "device.h"
#include "dive.h"
#include "divelog.h"
#include "divesite.h"
#include "event.h"
#include "eventtype.h"
#include "filterpreset.h"
#include "fulltext.h"
#include "interpolate.h"
#include "planner.h"
#include "qthelper.h" // for emit_reset_signal() -> should be removed
#include "range.h"
#include "gettext.h"
#include "git-access.h"
#include "selection.h"
#include "sample.h"
#include "trip.h"
#include "version.h"

#include <time.h>

void dive_table::record_dive(std::unique_ptr<dive> d)
{
	fixup_dive(*d);
	put(std::move(d));
}

void dive_table::fixup_dive(struct dive &dive) const
{
	dive.fixup_dive();
	update_cylinder_related_info(dive);
}

struct start_end_pressure {
	pressure_t start;
	pressure_t end;
};

void dive_table::force_fixup_dive(struct dive &d) const
{
	struct divecomputer *dc = &d.dcs[0];
	int old_temp = dc->watertemp.mkelvin;
	int old_mintemp = d.mintemp.mkelvin;
	int old_maxtemp = d.maxtemp.mkelvin;
	duration_t old_duration = d.duration;
	std::vector<start_end_pressure> old_pressures(d.cylinders.size());

	d.maxdepth = 0_m;
	dc->maxdepth = 0_m;
	d.watertemp = 0_K;
	dc->watertemp = 0_K;
	d.duration = 0_sec;
	d.maxtemp = 0_K;
	d.mintemp = 0_K;
	for (auto [i, cyl]: enumerated_range(d.cylinders)) {
		old_pressures[i].start = cyl.start;
		old_pressures[i].end = cyl.end;
		cyl.start = 0_bar;
		cyl.end = 0_bar;
	}

	fixup_dive(d);

	if (!d.watertemp.mkelvin)
		d.watertemp.mkelvin = old_temp;

	if (!dc->watertemp.mkelvin)
		dc->watertemp.mkelvin = old_temp;

	if (!d.maxtemp.mkelvin)
		d.maxtemp.mkelvin = old_maxtemp;

	if (!d.mintemp.mkelvin)
		d.mintemp.mkelvin = old_mintemp;

	if (!d.duration.seconds)
		d.duration = old_duration;
	for (auto [i, cyl]: enumerated_range(d.cylinders)) {
		if (!cyl.start.mbar)
			cyl.start = old_pressures[i].start;
		if (!cyl.end.mbar)
			cyl.end = old_pressures[i].end;
	}
}

// create a dive an hour from now with a default depth (15m/45ft) and duration (40 minutes)
// as a starting point for the user to edit
std::unique_ptr<dive> dive_table::default_dive()
{
	auto d = std::make_unique<dive>();
	d->when = time(nullptr) + gettimezoneoffset() + 3600;
	d->dcs[0].duration = 40_min;
	d->dcs[0].maxdepth = m_or_ft(15, 45);
	d->dcs[0].meandepth = m_or_ft(13, 39); // this creates a resonable looking safety stop
	make_manually_added_dive_dc(&d->dcs[0]);
	fake_dc(&d->dcs[0]);
	add_default_cylinder(d.get());
	fixup_dive(*d);
	return d;
}

static int active_o2(const struct dive &dive, const struct divecomputer *dc, duration_t time)
{
	struct gasmix gas = dive.get_gasmix_at_time(*dc, time);
	return get_o2(gas);
}

/* Calculate OTU for a dive - this only takes the first divecomputer into account.
   Implement the protocol in Erik Baker's document "Oxygen Toxicity Calculations". This code
   implements a third-order continuous approximation of Baker's Eq. 2 and enables OTU
   calculation for rebreathers. Baker obtained his information from:
   Comroe Jr. JH et al. (1945)  Oxygen toxicity. J. Am. Med. Assoc. 128,710-717
   Clark JM & CJ Lambertsen (1970) Pulmonary oxygen tolerance in man and derivation of pulmonary
      oxygen tolerance curves. Inst. env. Med. Report 1-70, University of Pennsylvania, Philadelphia, USA. */
static int calculate_otu(const struct dive &dive)
{
	double otu = 0.0;
	const struct divecomputer &dc = dive.dcs[0];
	gasmix_loop loop_gas(dive, dc);
	divemode_loop loop_mode(dc);
	for (auto [psample, sample]: pairwise_range(dc.samples)) {
		int po2i, po2f;
		double pm;
		int t = (sample.time - psample.time).seconds;
		depth_t depth = sample.depth;
		depth_t pdepth = psample.depth;
		// if there is sensor data use sensor[0]
		if ((dc.divemode == CCR || dc.divemode == PSCR) && psample.o2sensor[0].mbar) {
			po2i = psample.o2sensor[0].mbar;
			if (sample.o2sensor[0].mbar)
				po2f = sample.o2sensor[0].mbar;
			else
				po2f = po2i;	// ... use data from the first o2 sensor
		} else {
			[[maybe_unused]] auto [divemode, _cylinder_index, gasmix] = get_dive_status_at(dive, dc, psample.time.seconds, &loop_mode, &loop_gas);
			if (divemode == CCR) {
				po2i = std::min((int) psample.setpoint.mbar,
						 dive.depth_to_mbar(pdepth));
				po2f = std::min((int) psample.setpoint.mbar,
						 dive.depth_to_mbar(depth));
			} else {						// For OC and rebreather without o2 sensor/setpoint
				double amb_presure = dive.depth_to_bar(depth);
				double pamb_pressure = dive.depth_to_bar(pdepth);
				if (dc.divemode == PSCR) {
					po2i = pscr_o2(pamb_pressure, dive.get_gasmix_at_time(dc, psample.time));
					po2f = pscr_o2(amb_presure, dive.get_gasmix_at_time(dc, sample.time));
				} else {
					int o2 = active_o2(dive, &dc, psample.time);	//	... calculate po2 from depth and FiO2.
					po2i = lrint(o2 * pamb_pressure);	// (initial) po2 at start of segment
					po2f = lrint(o2 * amb_presure);	// (final) po2 at end of segment
				}
			}
		}
		if ((po2i > 500) || (po2f > 500)) {			// If PO2 in segment is above 500 mbar then calculate otu
			if (po2i <= 500) {				// For descent segment with po2i <= 500 mbar ..
				t = t * (po2f - 500) / (po2f - po2i);	// .. only consider part with PO2 > 500 mbar
				po2i = 501;				// Mostly important for the dive planner with long segments
			} else {
				if (po2f <= 500){
					t = t * (po2i - 500) / (po2i - po2f);	// For ascent segment with po2f <= 500 mbar ..
					po2f = 501;				// .. only consider part with PO2 > 500 mbar
				}
			}
			pm = (po2f + po2i)/1000.0 - 1.0;
			// This is a 3rd order continuous approximation of Baker's eq. 2, therefore Baker's eq. 1 is not used:
			otu += t / 60.0 * pow(pm, 5.0/6.0) * (1.0 - 5.0 * (po2f - po2i) * (po2f - po2i) / 216000000.0 / (pm * pm));
		}
	}
	return lrint(otu);
}

/* Calculate the CNS for a single dive  - this only takes the first divecomputer into account.
   The CNS contributions are summed for dive segments defined by samples. The maximum O2 exposure duration for each
   segment is calculated based on the mean depth of the two samples (start & end) that define each segment. The CNS
   contribution of each segment is found by dividing the time duration of the segment by its maximum exposure duration.
   The contributions of all segments of the dive are summed to get the total CNS% value. This is a partial implementation
   of the proposals in Erik Baker's document "Oxygen Toxicity Calculations" using fixed-depth calculations for the mean
   po2 for each segment. Empirical testing showed that, for large changes in depth, the cns calculation for the mean po2
   value is extremely close, if not identical to the additive calculations for 0.1 bar increments in po2 from the start
   to the end of the segment, assuming a constant rate of change in po2 (i.e. depth) with time. */
static double calculate_cns_dive(const struct dive &dive)
{
	const struct divecomputer dc = dive.dcs[0];
	double cns = 0.0;
	double rate;
	gasmix_loop loop_gas(dive, dc);
	divemode_loop loop_mode(dc);
	/* Calculate the CNS for each sample in this dive and sum them */
	for (auto [psample, sample]: pairwise_range(dc.samples)) {
		int t = (sample.time - psample.time).seconds;
		int po2i, po2f, po2;
		// Use sensor[0] if available
		depth_t depth = sample.depth;
		depth_t pdepth = psample.depth;
		[[maybe_unused]] auto [divemode, _cylinder_index, _gasmix] = get_dive_status_at(dive, dc, psample.time.seconds, &loop_mode, &loop_gas);
		if ((dc.divemode == CCR || dc.divemode == PSCR) && psample.o2sensor[0].mbar) {
			po2i = psample.o2sensor[0].mbar;
			if (sample.o2sensor[0].mbar)
				po2f = sample.o2sensor[0].mbar;
			else
				po2f = po2i;	// then use data from the first o2 sensor
			po2 = (po2f + po2i) / 2;
		} else if (divemode == CCR) {
			po2 = std::min((int) psample.setpoint.mbar,
					dive.depth_to_mbar(pdepth));
		} else {
			double amb_presure = dive.depth_to_bar(depth);
			double pamb_pressure = dive.depth_to_bar(pdepth);
			if (dc.divemode == PSCR) {
				po2i = pscr_o2(pamb_pressure, dive.get_gasmix_at_time(dc, psample.time));
				po2f = pscr_o2(amb_presure, dive.get_gasmix_at_time(dc, sample.time));
			} else {
				int o2 = active_o2(dive, &dc, psample.time);	//	... calculate po2 from depth and FiO2.
				po2i = lrint(o2 * pamb_pressure);	// (initial) po2 at start of segment
				po2f = lrint(o2 * amb_presure);	// (final) po2 at end of segment
			}
			po2 = (po2i + po2f) / 2;
		}

		/* Don't increase CNS when po2 below 500 matm */
		if (po2 <= 500)
			continue;

		// This formula is the result of fitting two lines to the Log of the NOAA CNS table
		rate = po2 <= 1500 ? exp(-11.7853 + 0.00193873 * po2) : exp(-23.6349 + 0.00980829 * po2);
		cns += (double) t * rate * 100.0;
	}
	return cns;
}

/* this only gets called if dive.maxcns == 0 which means we know that
 * none of the divecomputers has tracked any CNS for us
 * so we calculated it "by hand" */
int dive_table::calculate_cns(struct dive &dive) const
{
	double cns = 0.0;
	timestamp_t last_starttime, last_endtime = 0;

	/* shortcut */
	if (dive.cns)
		return dive.cns;

	size_t divenr = get_idx(&dive);
	int nr_dives = static_cast<int>(size());
	int i = divenr != std::string::npos ? static_cast<int>(divenr)
					    : nr_dives;
#if DECO_CALC_DEBUG & 2
	if (static_cast<size_t>(i) < size())
		printf("\n\n*** CNS for dive #%d %d\n", i, ()[i]->number);
	else
		printf("\n\n*** CNS for dive #%d\n", i);
#endif
	/* Look at next dive in dive list table and correct i when needed */
	while (i < nr_dives - 1) {
		if ((*this)[i]->when > dive.when)
			break;
		i++;
	}
	/* Look at previous dive in dive list table and correct i when needed */
	while (i > 0) {
		if ((*this)[i - 1]->when < dive.when)
			break;
		i--;
	}
#if DECO_CALC_DEBUG & 2
	printf("Dive number corrected to #%d\n", i);
#endif
	last_starttime = dive.when;
	/* Walk backwards to check previous dives - how far do we need to go back? */
	while (i--) {
		if (static_cast<size_t>(i) == divenr && i > 0)
			i--;
#if DECO_CALC_DEBUG & 2
		printf("Check if dive #%d %d has to be considered as prev dive: ", i, (*this)[i]->number);
#endif
		const struct dive &pdive = *(*this)[i];
		/* we don't want to mix dives from different trips as we keep looking
		 * for how far back we need to go */
		if (dive.divetrip && pdive.divetrip != dive.divetrip) {
#if DECO_CALC_DEBUG & 2
			printf("No - other dive trip\n");
#endif
			continue;
		}
		if (pdive.when >= dive.when || pdive.endtime() + 12 * 60 * 60 < last_starttime) {
#if DECO_CALC_DEBUG & 2
			printf("No\n");
#endif
			break;
		}
		last_starttime = pdive.when;
#if DECO_CALC_DEBUG & 2
		printf("Yes\n");
#endif
	}
	/* Walk forward and add dives and surface intervals to CNS */
	while (++i < nr_dives) {
#if DECO_CALC_DEBUG & 2
		printf("Check if dive #%d %d will be really added to CNS calc: ", i, (*this)[i]->number);
#endif
		const struct dive &pdive = *(*this)[i];
		/* again skip dives from different trips */
		if (dive.divetrip && dive.divetrip != pdive.divetrip) {
#if DECO_CALC_DEBUG & 2
			printf("No - other dive trip\n");
#endif
			continue;
		}
		/* Don't add future dives */
		if (pdive.when >= dive.when) {
#if DECO_CALC_DEBUG & 2
			printf("No - future or same dive\n");
#endif
			break;
		}
		/* Don't add the copy of the dive itself */
		if (static_cast<size_t>(i) == divenr) {
#if DECO_CALC_DEBUG & 2
			printf("No - copy of dive\n");
#endif
			continue;
		}
#if DECO_CALC_DEBUG & 2
		printf("Yes\n");
#endif

		/* CNS reduced with 90min halftime during surface interval */
		if (last_endtime)
			cns /= pow(2, (pdive.when - last_endtime) / (90.0 * 60.0));
#if DECO_CALC_DEBUG & 2
		printf("CNS after surface interval: %f\n", cns);
#endif

		cns += calculate_cns_dive(pdive);
#if DECO_CALC_DEBUG & 2
		printf("CNS after previous dive: %f\n", cns);
#endif

		last_starttime = pdive.when;
		last_endtime = pdive.endtime();
	}

	/* CNS reduced with 90min halftime during surface interval */
	if (last_endtime)
		cns /= pow(2, (dive.when - last_endtime) / (90.0 * 60.0));
#if DECO_CALC_DEBUG & 2
	printf("CNS after last surface interval: %f\n", cns);
#endif

	cns += calculate_cns_dive(dive);
#if DECO_CALC_DEBUG & 2
	printf("CNS after dive: %f\n", cns);
#endif

	/* save calculated cns in dive struct */
	dive.cns = lrint(cns);
	return dive.cns;
}

/*
 * Return air usage (in liters).
 */
static double calculate_airuse(const struct dive &dive)
{
	int airuse = 0;

	// SAC for a CCR dive does not make sense.
	if (dive.dcs[0].divemode == CCR)
		return 0.0;

	for (auto [i, cyl]: enumerated_range(dive.cylinders)) {
		pressure_t start, end;

		start = cyl.start.mbar ? cyl.start : cyl.sample_start;
		end = cyl.end.mbar ? cyl.end : cyl.sample_end;
		if (!end.mbar || start.mbar <= end.mbar) {
			// If a cylinder is used but we do not have info on amout of gas used
			// better not pretend we know the total gas use.
			// Eventually, logic should be fixed to compute average depth and total time
			// for those segments where cylinders with known pressure drop are breathed from.
			if (dive.is_cylinder_used(i))
				return 0.0;
			else
				continue;
		}

		// TODO: implement subtraction for units.h types
		airuse += cyl.gas_volume(start).mliter - cyl.gas_volume(end).mliter;
	}
	return airuse / 1000.0;
}

/* this only uses the first divecomputer to calculate the SAC rate */
static int calculate_sac(const struct dive &dive)
{
	const struct divecomputer *dc = &dive.dcs[0];
	double airuse, pressure, sac;
	int duration;

	airuse = calculate_airuse(dive);
	if (!airuse)
		return 0;

	duration = dc->duration.seconds;
	if (!duration)
		return 0;

	if (!dc->meandepth.mm)
		return 0;

	/* Mean pressure in ATM (SAC calculations are in atm*l/min) */
	pressure = dive.depth_to_atm(dc->meandepth);
	sac = airuse / pressure * 60 / duration;

	/* milliliters per minute.. */
	return lrint(sac * 1000);
}

/* for now we do this based on the first divecomputer */
static void add_dive_to_deco(struct deco_state *ds, const struct dive &dive, bool in_planner)
{
	const struct divecomputer &dc = dive.dcs[0];

	gasmix_loop loop_gas(dive, dc);
	divemode_loop loop_mode(dc);
	for (auto [psample, sample]: pairwise_range(dc.samples)) {
		int t0 = psample.time.seconds;
		int t1 = sample.time.seconds;
		int j;

		for (j = t0; j < t1; j++) {
			depth_t depth = interpolate(psample.depth, sample.depth, j - t0, t1 - t0);
			[[maybe_unused]] auto [divemode, _cylinder_index, gasmix] = get_dive_status_at(dive, dc, j, &loop_mode, &loop_gas);
			add_segment(ds, dive.depth_to_bar(depth), *gasmix, 1, sample.setpoint.mbar,
				    divemode, dive.sac,
				    in_planner);
		}
	}
}

/* take into account previous dives until there is a 48h gap between dives */
/* return last surface time before this dive or dummy value of 48h */
/* return negative surface time if dives are overlapping */
/* The place you call this function is likely the place where you want
 * to create the deco_state */
int dive_table::init_decompression(struct deco_state *ds, const struct dive *dive, bool in_planner) const
{
	int surface_time = 48 * 60 * 60;
	timestamp_t last_endtime = 0, last_starttime = 0;
	bool deco_init = false;
	double surface_pressure;

	if (!dive)
		return false;

	int nr_dives = static_cast<int>(size());
	size_t divenr = get_idx(dive);
	int i = divenr != std::string::npos ? static_cast<int>(divenr)
					    : nr_dives;
#if DECO_CALC_DEBUG & 2
	if (i < dive_table.nr)
		printf("\n\n*** Init deco for dive #%d %d\n", i, (*this)[i]->number);
	else
		printf("\n\n*** Init deco for dive #%d\n", i);
#endif
	/* Look at next dive in dive list table and correct i when needed */
	while (i + 1 < nr_dives) {
		if ((*this)[i]->when > dive->when)
			break;
		i++;
	}
	/* Look at previous dive in dive list table and correct i when needed */
	while (i > 0) {
		if ((*this)[i - 1]->when < dive->when)
			break;
		i--;
	}
#if DECO_CALC_DEBUG & 2
	printf("Dive number corrected to #%d\n", i);
#endif
	last_starttime = dive->when;
	/* Walk backwards to check previous dives - how far do we need to go back? */
	while (i--) {
		if (static_cast<size_t>(i) == divenr && i > 0)
			i--;
#if DECO_CALC_DEBUG & 2
		printf("Check if dive #%d %d has to be considered as prev dive: ", i, (*this)[i]->number);
#endif
		const struct dive &pdive = *(*this)[i];
		/* we don't want to mix dives from different trips as we keep looking
		 * for how far back we need to go */
		if (dive->divetrip && pdive.divetrip != dive->divetrip) {
#if DECO_CALC_DEBUG & 2
			printf("No - other dive trip\n");
#endif
			continue;
		}
		if (pdive.when >= dive->when || pdive.endtime() + 48 * 60 * 60 < last_starttime) {
#if DECO_CALC_DEBUG & 2
			printf("No\n");
#endif
			break;
		}
		last_starttime = pdive.when;
#if DECO_CALC_DEBUG & 2
		printf("Yes\n");
#endif
	}
	/* Walk forward an add dives and surface intervals to deco */
	while (++i < nr_dives) {
#if DECO_CALC_DEBUG & 2
		printf("Check if dive #%d %d will be really added to deco calc: ", i, (*this)[i]->number);
#endif
		const struct dive &pdive = *(*this)[i];
		/* again skip dives from different trips */
		if (dive->divetrip && dive->divetrip != pdive.divetrip) {
#if DECO_CALC_DEBUG & 2
			printf("No - other dive trip\n");
#endif
			continue;
		}
		/* Don't add future dives */
		if (pdive.when >= dive->when) {
#if DECO_CALC_DEBUG & 2
			printf("No - future or same dive\n");
#endif
			break;
		}
		/* Don't add the copy of the dive itself */
		if (static_cast<size_t>(i) == divenr) {
#if DECO_CALC_DEBUG & 2
			printf("No - copy of dive\n");
#endif
			continue;
		}
#if DECO_CALC_DEBUG & 2
		printf("Yes\n");
#endif

		surface_pressure = pdive.get_surface_pressure().mbar / 1000.0;
		/* Is it the first dive we add? */
		if (!deco_init) {
#if DECO_CALC_DEBUG & 2
			printf("Init deco\n");
#endif
			clear_deco(ds, surface_pressure, in_planner);
			deco_init = true;
#if DECO_CALC_DEBUG & 2
			printf("Tissues after init:\n");
			dump_tissues(ds);
#endif
		} else {
			surface_time = pdive.when - last_endtime;
			if (surface_time < 0) {
#if DECO_CALC_DEBUG & 2
				printf("Exit because surface intervall is %d\n", surface_time);
#endif
				return surface_time;
			}
			add_segment(ds, surface_pressure, gasmix_air, surface_time, 0, OC, prefs.decosac, in_planner);
#if DECO_CALC_DEBUG & 2
			printf("Tissues after surface intervall of %d:%02u:\n", FRACTION_TUPLE(surface_time, 60));
			dump_tissues(ds);
#endif
		}

		add_dive_to_deco(ds, pdive, in_planner);

		last_starttime = pdive.when;
		last_endtime = pdive.endtime();
		clear_vpmb_state(ds);
#if DECO_CALC_DEBUG & 2
		printf("Tissues after added dive #%d:\n", pdive.number);
		dump_tissues(ds);
#endif
	}

	surface_pressure = dive->get_surface_pressure().mbar / 1000.0;
	/* We don't have had a previous dive at all? */
	if (!deco_init) {
#if DECO_CALC_DEBUG & 2
		printf("Init deco\n");
#endif
		clear_deco(ds, surface_pressure, in_planner);
#if DECO_CALC_DEBUG & 2
		printf("Tissues after no previous dive, surface time set to 48h:\n");
		dump_tissues(ds);
#endif
	} else {
		surface_time = dive->when - last_endtime;
		if (surface_time < 0) {
#if DECO_CALC_DEBUG & 2
			printf("Exit because surface intervall is %d\n", surface_time);
#endif
			return surface_time;
		}
		add_segment(ds, surface_pressure, gasmix_air, surface_time, 0, OC, prefs.decosac, in_planner);
#if DECO_CALC_DEBUG & 2
		printf("Tissues after surface intervall of %d:%02u:\n", FRACTION_TUPLE(surface_time, 60));
		dump_tissues(ds);
#endif
	}

	// I do not dare to remove this call. We don't need the result but it might have side effects. Bummer.
	tissue_tolerance_calc(ds, dive, surface_pressure, in_planner);
	return surface_time;
}

void dive_table::update_cylinder_related_info(struct dive &dive) const
{
	dive.sac = calculate_sac(dive);
	dive.otu = calculate_otu(dive);
	if (dive.maxcns == 0)
		dive.maxcns = calculate_cns(dive);
}

/* Compare list of dive computers by model name */
static int comp_dc(const struct dive *d1, const struct dive *d2)
{
	auto it1 = d1->dcs.begin();
	auto it2 = d2->dcs.begin();
	while (it1 != d1->dcs.end() || it2 != d2->dcs.end()) {
		if (it1 == d1->dcs.end())
			return -1;
		if (it2 == d2->dcs.end())
			return 1;
		int cmp = it1->model.compare(it2->model);
		if (cmp != 0)
			return cmp;
		++it1;
		++it2;
	}
	return 0;
}

/* This function defines the sort ordering of dives. The core
 * and the UI models should use the same sort function, which
 * should be stable. This is not crucial at the moment, as the
 * indices in core and UI are independent, but ultimately we
 * probably want to unify the models.
 * After editing a key used in this sort-function, the order of
 * the dives must be re-astablished.
 * Currently, this does a lexicographic sort on the
 * (start-time, trip-time, number, id) tuple.
 * trip-time is defined such that dives that do not belong to
 * a trip are sorted *after* dives that do. Thus, in the default
 * chronologically-descending sort order, they are shown *before*.
 * "id" is a stable, strictly increasing unique number, which
 * is generated when a dive is added to the system.
 * We might also consider sorting by end-time and other criteria,
 * but see the caveat above (editing means reordering of the dives).
 */
int comp_dives(const struct dive &a, const struct dive &b)
{
	int cmp;
	if (&a == &b)
		return 0;	/* reflexivity */
	if (a.when < b.when)
		return -1;
	if (a.when > b.when)
		return 1;
	if (a.divetrip != b.divetrip) {
		if (!b.divetrip)
			return -1;
		if (!a.divetrip)
			return 1;
		if (a.divetrip->date() < b.divetrip->date())
			return -1;
		if (a.divetrip->date() > b.divetrip->date())
			return 1;
	}
	if (a.number < b.number)
		return -1;
	if (a.number > b.number)
		return 1;
	if ((cmp = comp_dc(&a, &b)) != 0)
		return cmp;
	if (a.id < b.id)
		return -1;
	if (a.id > b.id)
		return 1;
	return &a < &b ? -1 : 1; /* give up. */
}

int comp_dives_ptr(const struct dive *a, const struct dive *b)
{
	return comp_dives(*a, *b);
}

/* This removes a dive from the global dive table but doesn't free the
 * resources associated with the dive. The caller must removed the dive
 * from the trip-list. Returns a pointer to the unregistered dive.
 * The unregistered dive has the selection- and hidden-flags cleared.
 * TODO: This makes me unhappy, as it touches global state, viz.
 * selection and fulltext. */
std::unique_ptr<dive> dive_table::unregister_dive(int idx)
{
	if (idx < 0 || static_cast<size_t>(idx) >= size())
		return {}; /* this should never happen */

	auto dive = pull_at(idx);

	/* When removing a dive from the global dive table,
	 * we also have to unregister its fulltext cache. */
	fulltext_unregister(dive.get());
	if (dive->selected)
		amount_selected--;
	dive->selected = false;
	return dive;
}

/* Add a dive to the global dive table.
 * Index it in the fulltext cache and make sure that it is written
 * in git_save().
 * TODO: This makes me unhappy, as it touches global state, viz.
 * selection and fulltext. */
struct dive *dive_table::register_dive(std::unique_ptr<dive> d)
{
	// When we add dives, we start in hidden-by-filter status. Once all
	// dives have been added, their status will be updated.
	d->hidden_by_filter = true;

	fulltext_register(d.get());			// Register the dive's fulltext cache
	d->invalidate_cache();				// Ensure that dive is written in git_save()
	auto [res, idx] = put(std::move(d));

	return res;
}

/* return the number a dive gets when inserted at the given index.
 * this function is supposed to be called *before* a dive was added.
 * this returns:
 * 	- 1 for an empty log
 * 	- last_nr+1 for addition at end of log (if last dive had a number)
 * 	- 0 for all other cases
 */
int dive_table::get_dive_nr_at_idx(int idx) const
{
	if (static_cast<size_t>(idx) < size())
		return 0;
	auto it = std::find_if(rbegin(), rend(), [](auto &d) { return !d->invalid; });
	if (it == rend())
		return 1;
	return (*it)->number ? (*it)->number + 1 : 0;
}

/* lookup of trip in main trip_table based on its id */
dive *dive_table::get_by_uniq_id(int id) const
{
	auto it = std::find_if(begin(), end(), [id](auto &d) { return d->id == id; });
#ifdef DEBUG
	if (it == end()) {
		report_info("Invalid id %x passed to get_dive_by_diveid, try to fix the code", id);
		exit(1);
	}
#endif
	return it != end() ? it->get() : nullptr;
}

void clear_dive_file_data()
{
	fulltext_unregister_all();
	select_single_dive(NULL);	// This is propagated up to the UI and clears all the information.

	current_dive = NULL;
	divelog.clear();

	clear_event_types();

	clear_min_datafile_version();
	clear_git_id();

	reset_tank_info_table(tank_info_table);

	/* Inform frontend of reset data. This should reset all the models. */
	emit_reset_signal();
}

bool dive_less_than(const struct dive &a, const struct dive &b)
{
	return comp_dives(a, b) < 0;
}

bool dive_less_than_ptr(const struct dive *a, const struct dive *b)
{
	return comp_dives(*a, *b) < 0;
}

/* When comparing a dive to a trip, use the first dive of the trip. */
static int comp_dive_to_trip(struct dive *a, struct dive_trip *b)
{
	/* This should never happen, nevertheless don't crash on trips
	 * with no (or worse a negative number of) dives. */
	if (!b || b->dives.empty())
		return -1;
	return comp_dives(*a, *b->dives[0]);
}

static int comp_dive_or_trip(struct dive_or_trip a, struct dive_or_trip b)
{
	/* we should only be called with both a and b having exactly one of
	 * dive or trip not NULL. But in an abundance of caution, make sure
	 * we still give a consistent answer even when called with invalid
	 * arguments, as otherwise we might be hunting down crashes at a later
	 * time...
	 */
	if (!a.dive && !a.trip && !b.dive && !b.trip)
		return 0;
	if (!a.dive && !a.trip)
		return -1;
	if (!b.dive && !b.trip)
		return 1;
	if (a.dive && b.dive)
		return comp_dives(*a.dive, *b.dive);
	if (a.trip && b.trip)
		return comp_trips(*a.trip, *b.trip);
	if (a.dive)
		return comp_dive_to_trip(a.dive, b.trip);
	else
		return -comp_dive_to_trip(b.dive, a.trip);
}

bool dive_or_trip_less_than(struct dive_or_trip a, struct dive_or_trip b)
{
	return comp_dive_or_trip(a, b) < 0;
}

/*
 * Calculate surface interval for dive starting at "when". Currently, we
 * might display dives which are not yet in the divelist, therefore the
 * input parameter is a timestamp.
 * If the given dive starts during a different dive, the surface interval
 * is 0. If we can't determine a surface interval (first dive), <0 is
 * returned. This does *not* consider pathological cases such as dives
 * that happened inside other dives. The interval will always be calculated
 * with respect to the dive that started previously.
 */
timestamp_t dive_table::get_surface_interval(timestamp_t when) const
{
	/* find previous dive. might want to use a binary search. */
	auto it = std::find_if(rbegin(), rend(),
			       [when] (auto &d) { return d->when < when; });
	if (it == rend())
		return -1;

	timestamp_t prev_end = (*it)->endtime();
	if (prev_end > when)
		return 0;
	return when - prev_end;
}

/* Find visible dive close to given date. First search towards older,
 * then newer dives. */
struct dive *dive_table::find_next_visible_dive(timestamp_t when)
{
	/* we might want to use binary search here */
	auto it = std::find_if(begin(), end(),
			       [when] (auto &d) { return d->when <= when; });

	for (auto it2 = it; it2 != begin(); --it2) {
		if (!(*std::prev(it2))->hidden_by_filter)
			return it2->get();
	}

	for (auto it2 = it; it2 != end(); ++it2) {
		if (!(*it2)->hidden_by_filter)
			return it2->get();
	}

	return nullptr;
}

bool dive_table::has_dive(unsigned int deviceid, unsigned int diveid) const
{
	return std::any_of(begin(), end(), [deviceid,diveid] (auto &d) {
			return std::any_of(d->dcs.begin(), d->dcs.end(), [deviceid,diveid] (auto &dc) {
				return dc.deviceid == deviceid && dc.diveid == diveid;
			});
	       });
}

/*
 * This splits the dive src by dive computer. The first output dive has all
 * dive computers except num, the second only dive computer num.
 * The dives will not be associated with a trip.
 * On error, both output parameters are set to NULL.
 */
std::array<std::unique_ptr<dive>, 2> dive_table::split_divecomputer(const struct dive &src, int num) const
{
	if (num < 0 || src.dcs.size() < 2 || static_cast<size_t>(num) >= src.dcs.size())
		return {};

	// Copy the dive with full divecomputer list
	auto out1 = std::make_unique<dive>(src);

	// Remove all DCs, stash them and copy the dive again.
	// Then, we have to dives without DCs and a list of DCs.
	std::vector<divecomputer> dcs;
	std::swap(out1->dcs, dcs);
	auto out2 = std::make_unique<dive>(*out1);

	// Give the dives new unique ids and remove them from the trip.
	out1->id = dive_getUniqID();
	out2->id = dive_getUniqID();
	out1->divetrip = out2->divetrip = NULL;

	// Now copy the divecomputers
	out1->dcs.reserve(src.dcs.size() - 1);
	for (auto [idx, dc]: enumerated_range(dcs)) {
		auto &dcs = idx == num ? out2->dcs : out1->dcs;
		dcs.push_back(std::move(dc));
	}

	// Recalculate gas data, etc.
	fixup_dive(*out1);
	fixup_dive(*out2);

	return { std::move(out1), std::move(out2) };
}

/*
 * Split a dive that has a surface interval from samples 'a' to 'b'
 * into two dives, but don't add them to the log yet.
 * Returns the nr of the old dive or <0 on failure.
 * Moreover, on failure both output dives are set to NULL.
 * On success, the newly allocated dives are returned in out1 and out2.
 */
std::array<std::unique_ptr<dive>, 2> dive_table::split_dive_at(const struct dive &dive, int a, int b) const
{
	size_t nr = get_idx(&dive);

	/* if we can't find the dive in the dive list, don't bother */
	if (nr == std::string::npos)
		return {};

	bool is_last_dive = size() == nr + 1;

	/* Splitting should leave at least 3 samples per dive */
	if (a < 3 || static_cast<size_t>(b + 4) > dive.dcs[0].samples.size())
		return {};

	/* We're not trying to be efficient here.. */
	auto d1 = std::make_unique<struct dive>(dive);
	auto d2 = std::make_unique<struct dive>(dive);
	d1->id = dive_getUniqID();
	d2->id = dive_getUniqID();
	d1->divetrip = d2->divetrip = nullptr;

	/* now unselect the first first segment so we don't keep all
	 * dives selected by mistake. But do keep the second one selected
	 * so the algorithm keeps splitting the dive further */
	d1->selected = false;

	struct divecomputer &dc1 = d1->dcs[0];
	struct divecomputer &dc2 = d2->dcs[0];
	/*
	 * Cut off the samples of d1 at the beginning
	 * of the interval.
	 */
	dc1.samples.resize(a);

	/* And get rid of the 'b' first samples of d2 */
	dc2.samples.erase(dc2.samples.begin(), dc2.samples.begin() + b);

	/* Now the secondary dive computers */
	int32_t t = dc2.samples[0].time.seconds;
	for (auto it1 = d1->dcs.begin() + 1; it1 != d1->dcs.end(); ++it1) {
		auto it = std::find_if(it1->samples.begin(), it1->samples.end(),
				       [t](auto &sample) { return sample.time.seconds >= t; });
		it1->samples.erase(it, it1->samples.end());
	}
	for (auto it2 = d2->dcs.begin() + 1; it2 != d2->dcs.end(); ++it2) {
		auto it = std::find_if(it2->samples.begin(), it2->samples.end(),
				       [t](auto &sample) { return sample.time.seconds >= t; });
		it2->samples.erase(it2->samples.begin(), it);
	}

	/*
	 * This is where we cut off events from d1,
	 * and shift everything in d2
	 */
	d2->when += t;
	auto it1 = d1->dcs.begin();
	auto it2 = d2->dcs.begin();
	while (it1 != d1->dcs.end() && it2 != d2->dcs.end()) {
		it2->when += t;
		for (auto &sample: it2->samples)
			sample.time.seconds -= t;

		/* Remove the events past 't' from d1 */
		auto it = std::lower_bound(it1->events.begin(), it1->events.end(), t,
					   [] (struct event &ev, int t)
					   { return ev.time.seconds < t; });
		it1->events.erase(it, it1->events.end());

		/* Remove the events before 't' from d2, and shift the rest */
		it = std::lower_bound(it2->events.begin(), it2->events.end(), t,
				      [] (struct event &ev, int t)
				      { return ev.time.seconds < t; });
		it2->events.erase(it2->events.begin(), it);
		for (auto &ev: it2->events)
			ev.time.seconds -= t;

		++it1;
		++it2;
	}

	force_fixup_dive(*d1);
	force_fixup_dive(*d2);

	/*
	 * Was the dive numbered? If it was the last dive, then we'll
	 * increment the dive number for the tail part that we split off.
	 * Otherwise the tail is unnumbered.
	 */
	if (d2->number && is_last_dive)
		d2->number++;
	else
		d2->number = 0;

	return { std::move(d1), std::move(d2) };
}

/* in freedive mode we split for as little as 10 seconds on the surface,
 * otherwise we use a minute */
static bool should_split(const struct divecomputer *dc, int t1, int t2)
{
	int threshold = dc->divemode == FREEDIVE ? 10 : 60;

	return t2 - t1 >= threshold;
}

/*
 * Try to split a dive into multiple dives at a surface interval point.
 *
 * NOTE! We will split when there is at least one surface event that has
 * non-surface events on both sides.
 *
 * The surface interval points are determined using the first dive computer.
 *
 * In other words, this is a (simplified) reversal of the dive merging.
 */
std::array<std::unique_ptr<dive>, 2> dive_table::split_dive(const struct dive &dive) const
{
	const struct divecomputer *dc = &dive.dcs[0];
	bool at_surface = true;
	if (dc->samples.empty())
		return {};
	auto surface_start = dc->samples.begin();
	for (auto it = dc->samples.begin() + 1; it != dc->samples.end(); ++it) {
		bool surface_sample = it->depth.mm < SURFACE_THRESHOLD;

		/*
		 * We care about the transition from and to depth 0,
		 * not about the depth staying similar.
		 */
		if (at_surface == surface_sample)
			continue;
		at_surface = surface_sample;

		// Did it become surface after having been non-surface? We found the start
		if (at_surface) {
			surface_start = it;
			continue;
		}

		// Going down again? We want at least a minute from
		// the surface start.
		if (surface_start == dc->samples.begin())
			continue;
		if (!should_split(dc, surface_start->time.seconds, std::prev(it)->time.seconds))
			continue;

		return split_dive_at(dive, surface_start - dc->samples.begin(), it - dc->samples.begin() - 1);
	}
	return {};
}

std::array<std::unique_ptr<dive>, 2> dive_table::split_dive_at_time(const struct dive &dive, duration_t time) const
{
	auto it = std::find_if(dive.dcs[0].samples.begin(), dive.dcs[0].samples.end(),
			       [time](auto &sample) { return sample.time.seconds >= time.seconds; });
	if (it == dive.dcs[0].samples.end())
		return {};
	size_t idx = it - dive.dcs[0].samples.begin();
	if (idx < 1)
		return {};
	return split_dive_at(dive, static_cast<int>(idx), static_cast<int>(idx - 1));
}

/*
 * Pick a trip for a dive
 */
static struct dive_trip *get_preferred_trip(const struct dive &a, const struct dive &b)
{
	dive_trip *atrip, *btrip;

	/* If only one dive has a trip, choose that */
	atrip = a.divetrip;
	btrip = b.divetrip;
	if (!atrip)
		return btrip;
	if (!btrip)
		return atrip;

	/* Both dives have a trip - prefer the non-autogenerated one */
	if (atrip->autogen && !btrip->autogen)
		return btrip;
	if (!atrip->autogen && btrip->autogen)
		return atrip;

	/* Otherwise, look at the trip data and pick the "better" one */
	if (atrip->location.empty())
		return btrip;
	if (btrip->location.empty())
		return atrip;
	if (atrip->notes.empty())
		return btrip;
	if (btrip->notes.empty())
		return atrip;

	/*
	 * Ok, so both have location and notes.
	 * Pick the earlier one.
	 */
	if (a.when < b.when)
		return atrip;
	return btrip;
}

/*
 * Merging two dives can be subtle, because there's two different ways
 * of merging:
 *
 * (a) two distinctly _different_ dives that have the same dive computer
 *     are merged into one longer dive, because the user asked for it
 *     in the divelist.
 *
 *     Because this case is with the same dive computer, we *know* the
 *     two must have a different start time, and "offset" is the relative
 *     time difference between the two.
 *
 * (b) two different dive computers that we might want to merge into
 *     one single dive with multiple dive computers.
 *
 *     This is the "try_to_merge()" case, which will have offset == 0,
 *     even if the dive times might be different.
 *
 * If new dives are merged into the dive table, dive a is supposed to
 * be the old dive and dive b is supposed to be the newly imported
 * dive. If the flag "prefer_downloaded" is set, data of the latter
 * will take priority over the former.
 *
 * The dive site is set, but the caller still has to add it to the
 * divelog's dive site manually.
 *
 */
std::unique_ptr<dive> dive_table::merge_two_dives(const struct dive &a_in, const struct dive &b_in, int offset, bool prefer_downloaded) const
{
	const dive *a = &a_in;
	const dive *b = &b_in;
	if (is_dc_planner(&a->dcs[0]))
		std::swap(a, b);

	auto d = dive::create_merged_dive(*a, *b, offset, prefer_downloaded);

	/* The CNS values will be recalculated from the sample in fixup_dive() */
	d->cns = d->maxcns = 0;

	/* Unselect the new dive if the original dive was selected. */
	d->selected = false;

	/* we take the first dive site, unless it's empty */
	d->dive_site = a->dive_site && !a->dive_site->is_empty() ? a->dive_site : b->dive_site;

	fixup_dive(*d);

	return d;
}

merge_result dive_table::merge_dives(const std::vector<dive *> &dives) const
{
	merge_result res;

	// We don't support merging of less than two dives, but
	// let's try to treat this gracefully.
	if (dives.empty())
		return res;
	if (dives.size() == 1) {
		res.dive = std::make_unique<dive>(*dives[0]);
		return res;
	}

	auto d = merge_two_dives(*dives[0], *dives[1], dives[1]->when - dives[0]->when, false);
	d->divetrip = get_preferred_trip(*dives[0], *dives[1]);

	for (size_t i = 2; i < dives.size(); ++i) {
		auto d2 = divelog.dives.merge_two_dives(*d, *dives[i], dives[i]->when - d->when, false);
		d2->divetrip = get_preferred_trip(*d, *dives[i]);
		d = std::move(d2);
	}

	// If the new dive site has no gps location, try to find the first dive with a gps location
	if (d->dive_site && !d->dive_site->has_gps_location()) {
		auto it = std::find_if(dives.begin(), dives.end(), [](const dive *d)
				       { return d->dive_site && d->dive_site->has_gps_location(); } );
		if (it != dives.end())
			res.set_location = (*it)->dive_site->location;
	}
	res.dive = std::move(d);
	return res;
}

/*
 * This could do a lot more merging. Right now it really only
 * merges almost exact duplicates - something that happens easily
 * with overlapping dive downloads.
 *
 * If new dives are merged into the dive table, dive a is supposed to
 * be the old dive and dive b is supposed to be the newly imported
 * dive. If the flag "prefer_downloaded" is set, data of the latter
 * will take priority over the former.
 *
 * Attn: The dive_site parameter of the dive will be set, but the caller
 * still has to register the dive in the dive site!
 */
struct std::unique_ptr<dive> dive_table::try_to_merge(const struct dive &a, const struct dive &b, bool prefer_downloaded) const
{
	if (!a.likely_same(b))
		return {};

	return merge_two_dives(a, b, 0, prefer_downloaded);
}

/* Clone a dive and delete given dive computer */
std::unique_ptr<dive> dive_table::clone_delete_divecomputer(const struct dive &d, int dc_number)
{
	/* copy the dive */
	auto res = std::make_unique<dive>(d);

	/* make a new unique id, since we still can't handle two equal ids */
	res->id = dive_getUniqID();

	if (res->dcs.size() <= 1)
		return res;

	if (dc_number < 0 || static_cast<size_t>(dc_number) >= res->dcs.size())
		return res;

	res->dcs.erase(res->dcs.begin() + dc_number);

	force_fixup_dive(*res);

	return res;
}
