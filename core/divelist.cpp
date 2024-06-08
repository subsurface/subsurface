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
#include "qthelper.h"
#include "range.h"
#include "gettext.h"
#include "git-access.h"
#include "selection.h"
#include "sample.h"
#include "trip.h"

void dive_table::record_dive(std::unique_ptr<dive> d)
{
	fixup_dive(d.get());
	put(std::move(d));
}

/*
 * Get "maximal" dive gas for a dive.
 * Rules:
 *  - Trimix trumps nitrox (highest He wins, O2 breaks ties)
 *  - Nitrox trumps air (even if hypoxic)
 * These are the same rules as the inter-dive sorting rules.
 */
void get_dive_gas(const struct dive *dive, int *o2_p, int *he_p, int *o2max_p)
{
	int maxo2 = -1, maxhe = -1, mino2 = 1000;

	for (auto [i, cyl]: enumerated_range(dive->cylinders)) {
		int o2 = get_o2(cyl.gasmix);
		int he = get_he(cyl.gasmix);

		if (!is_cylinder_used(dive, i))
			continue;
		if (cyl.cylinder_use == OXYGEN)
			continue;
		if (cyl.cylinder_use == NOT_USED)
			continue;
		if (o2 > maxo2)
			maxo2 = o2;
		if (o2 < mino2 && maxhe <= 0)
			mino2 = o2;
		if (he > maxhe) {
			maxhe = he;
			mino2 = o2;
		}
	}
	/* All air? Show/sort as "air"/zero */
	if ((!maxhe && maxo2 == O2_IN_AIR && mino2 == maxo2) ||
			(maxo2 == -1 && maxhe == -1 && mino2 == 1000))
		maxo2 = mino2 = 0;
	*o2_p = mino2;
	*he_p = maxhe;
	*o2max_p = maxo2;
}

int total_weight(const struct dive *dive)
{
	int total_grams = 0;

	if (dive) {
		for (auto &ws: dive->weightsystems)
			total_grams += ws.weight.grams;
	}
	return total_grams;
}

static int active_o2(const struct dive *dive, const struct divecomputer *dc, duration_t time)
{
	struct gasmix gas = get_gasmix_at_time(*dive, *dc, time);
	return get_o2(gas);
}

// Do not call on first sample as it acccesses the previous sample
static int get_sample_o2(const struct dive *dive, const struct divecomputer *dc, const struct sample &sample, const struct sample &psample)
{
	int po2i, po2f, po2;
	// Use sensor[0] if available
	if ((dc->divemode == CCR || dc->divemode == PSCR) && sample.o2sensor[0].mbar) {
		po2i = psample.o2sensor[0].mbar;
		po2f = sample.o2sensor[0].mbar;	// then use data from the first o2 sensor
		po2 = (po2f + po2i) / 2;
	} else if (sample.setpoint.mbar > 0) {
		po2 = std::min((int) sample.setpoint.mbar,
				depth_to_mbar(sample.depth.mm, dive));
	} else {
		double amb_presure = depth_to_bar(sample.depth.mm, dive);
		double pamb_pressure = depth_to_bar(psample.depth.mm , dive);
		if (dc->divemode == PSCR) {
			po2i = pscr_o2(pamb_pressure, get_gasmix_at_time(*dive, *dc, psample.time));
			po2f = pscr_o2(amb_presure, get_gasmix_at_time(*dive, *dc, sample.time));
		} else {
			int o2 = active_o2(dive, dc, psample.time);	// 	... calculate po2 from depth and FiO2.
			po2i = lrint(o2 * pamb_pressure);	// (initial) po2 at start of segment
			po2f = lrint(o2 * amb_presure);	// (final) po2 at end of segment
		}
		po2 = (po2i + po2f) / 2;
	}
	return po2;
}

/* Calculate OTU for a dive - this only takes the first divecomputer into account.
   Implement the protocol in Erik Baker's document "Oxygen Toxicity Calculations". This code
   implements a third-order continuous approximation of Baker's Eq. 2 and enables OTU
   calculation for rebreathers. Baker obtained his information from:
   Comroe Jr. JH et al. (1945)  Oxygen toxicity. J. Am. Med. Assoc. 128,710-717
   Clark JM & CJ Lambertsen (1970) Pulmonary oxygen tolerance in man and derivation of pulmonary
      oxygen tolerance curves. Inst. env. Med. Report 1-70, University of Pennsylvania, Philadelphia, USA. */
static int calculate_otu(const struct dive *dive)
{
	double otu = 0.0;
	const struct divecomputer *dc = &dive->dcs[0];
	for (auto [psample, sample]: pairwise_range(dc->samples)) {
		int t;
		int po2i, po2f;
		double pm;
		t = sample.time.seconds - psample.time.seconds;
		// if there is sensor data use sensor[0]
		if ((dc->divemode == CCR || dc->divemode == PSCR) && sample.o2sensor[0].mbar) {
			po2i = psample.o2sensor[0].mbar;
			po2f = sample.o2sensor[0].mbar;	// ... use data from the first o2 sensor
		} else {
			if (sample.setpoint.mbar > 0) {
				po2f = std::min((int) sample.setpoint.mbar,
						 depth_to_mbar(sample.depth.mm, dive));
				if (psample.setpoint.mbar > 0)
					po2i = std::min((int) psample.setpoint.mbar,
							 depth_to_mbar(psample.depth.mm, dive));
				else
					po2i = po2f;
			} else {						// For OC and rebreather without o2 sensor/setpoint
				double amb_presure = depth_to_bar(sample.depth.mm, dive);
				double pamb_pressure = depth_to_bar(psample.depth.mm , dive);
				if (dc->divemode == PSCR) {
					po2i = pscr_o2(pamb_pressure, get_gasmix_at_time(*dive, *dc, psample.time));
					po2f = pscr_o2(amb_presure, get_gasmix_at_time(*dive, *dc, sample.time));
				} else {
					int o2 = active_o2(dive, dc, psample.time);	// 	... calculate po2 from depth and FiO2.
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
static double calculate_cns_dive(const struct dive *dive)
{
	const struct divecomputer *dc = &dive->dcs[0];
	double cns = 0.0;
	double rate;
	/* Calculate the CNS for each sample in this dive and sum them */
	for (auto [psample, sample]: pairwise_range(dc->samples)) {
		int t = sample.time.seconds - psample.time.seconds;
		int po2 = get_sample_o2(dive, dc, sample, psample);
		/* Don't increase CNS when po2 below 500 matm */
		if (po2 <= 500)
			continue;

		// This formula is the result of fitting two lines to the Log of the NOAA CNS table
		rate = po2 <= 1500 ? exp(-11.7853 + 0.00193873 * po2) : exp(-23.6349 + 0.00980829 * po2);
		cns += (double) t * rate * 100.0;
	}
	return cns;
}

/* this only gets called if dive->maxcns == 0 which means we know that
 * none of the divecomputers has tracked any CNS for us
 * so we calculated it "by hand" */
static int calculate_cns(struct dive *dive)
{
	double cns = 0.0;
	timestamp_t last_starttime, last_endtime = 0;

	/* shortcut */
	if (dive->cns)
		return dive->cns;

	size_t divenr = divelog.dives.get_idx(dive);
	int i = divenr != std::string::npos ? static_cast<int>(divenr)
					    : static_cast<int>(divelog.dives.size());
	int nr_dives = static_cast<int>(divelog.dives.size());
#if DECO_CALC_DEBUG & 2
	if (static_cast<size_t>(i) < divelog.table->size())
		printf("\n\n*** CNS for dive #%d %d\n", i, (*divelog.table)[i]->number);
	else
		printf("\n\n*** CNS for dive #%d\n", i);
#endif
	/* Look at next dive in dive list table and correct i when needed */
	while (i < nr_dives - 1) {
		struct dive *pdive = get_dive(i);
		if (!pdive || pdive->when > dive->when)
			break;
		i++;
	}
	/* Look at previous dive in dive list table and correct i when needed */
	while (i > 0) {
		struct dive *pdive = get_dive(i - 1);
		if (!pdive || pdive->when < dive->when)
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
		printf("Check if dive #%d %d has to be considered as prev dive: ", i, get_dive(i)->number);
#endif
		struct dive *pdive = get_dive(i);
		/* we don't want to mix dives from different trips as we keep looking
		 * for how far back we need to go */
		if (dive->divetrip && pdive->divetrip != dive->divetrip) {
#if DECO_CALC_DEBUG & 2
			printf("No - other dive trip\n");
#endif
			continue;
		}
		if (!pdive || pdive->when >= dive->when || pdive->endtime() + 12 * 60 * 60 < last_starttime) {
#if DECO_CALC_DEBUG & 2
			printf("No\n");
#endif
			break;
		}
		last_starttime = pdive->when;
#if DECO_CALC_DEBUG & 2
		printf("Yes\n");
#endif
	}
	/* Walk forward and add dives and surface intervals to CNS */
	while (++i < nr_dives) {
#if DECO_CALC_DEBUG & 2
		printf("Check if dive #%d %d will be really added to CNS calc: ", i, get_dive(i)->number);
#endif
		struct dive *pdive = get_dive(i);
		/* again skip dives from different trips */
		if (dive->divetrip && dive->divetrip != pdive->divetrip) {
#if DECO_CALC_DEBUG & 2
			printf("No - other dive trip\n");
#endif
			continue;
		}
		/* Don't add future dives */
		if (pdive->when >= dive->when) {
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
			cns /= pow(2, (pdive->when - last_endtime) / (90.0 * 60.0));
#if DECO_CALC_DEBUG & 2
		printf("CNS after surface interval: %f\n", cns);
#endif

		cns += calculate_cns_dive(pdive);
#if DECO_CALC_DEBUG & 2
		printf("CNS after previous dive: %f\n", cns);
#endif

		last_starttime = pdive->when;
		last_endtime = pdive->endtime();
	}

	/* CNS reduced with 90min halftime during surface interval */
	if (last_endtime)
		cns /= pow(2, (dive->when - last_endtime) / (90.0 * 60.0));
#if DECO_CALC_DEBUG & 2
	printf("CNS after last surface interval: %f\n", cns);
#endif

	cns += calculate_cns_dive(dive);
#if DECO_CALC_DEBUG & 2
	printf("CNS after dive: %f\n", cns);
#endif

	/* save calculated cns in dive struct */
	dive->cns = lrint(cns);
	return dive->cns;
}
/*
 * Return air usage (in liters).
 */
static double calculate_airuse(const struct dive *dive)
{
	int airuse = 0;

	// SAC for a CCR dive does not make sense.
	if (dive->dcs[0].divemode == CCR)
		return 0.0;

	for (auto [i, cyl]: enumerated_range(dive->cylinders)) {
		pressure_t start, end;

		start = cyl.start.mbar ? cyl.start : cyl.sample_start;
		end = cyl.end.mbar ? cyl.end : cyl.sample_end;
		if (!end.mbar || start.mbar <= end.mbar) {
			// If a cylinder is used but we do not have info on amout of gas used
			// better not pretend we know the total gas use.
			// Eventually, logic should be fixed to compute average depth and total time
			// for those segments where cylinders with known pressure drop are breathed from.
			if (is_cylinder_used(dive, i))
				return 0.0;
			else
				continue;
		}

		airuse += gas_volume(&cyl, start) - gas_volume(&cyl, end);
	}
	return airuse / 1000.0;
}

/* this only uses the first divecomputer to calculate the SAC rate */
static int calculate_sac(const struct dive *dive)
{
	const struct divecomputer *dc = &dive->dcs[0];
	double airuse, pressure, sac;
	int duration, meandepth;

	airuse = calculate_airuse(dive);
	if (!airuse)
		return 0;

	duration = dc->duration.seconds;
	if (!duration)
		return 0;

	meandepth = dc->meandepth.mm;
	if (!meandepth)
		return 0;

	/* Mean pressure in ATM (SAC calculations are in atm*l/min) */
	pressure = depth_to_atm(meandepth, dive);
	sac = airuse / pressure * 60 / duration;

	/* milliliters per minute.. */
	return lrint(sac * 1000);
}

/* for now we do this based on the first divecomputer */
static void add_dive_to_deco(struct deco_state *ds, struct dive *dive, bool in_planner)
{
	struct divecomputer *dc = &dive->dcs[0];

	gasmix_loop loop(*dive, dive->dcs[0]);
	divemode_loop loop_d(dive->dcs[0]);
	for (auto [psample, sample]: pairwise_range(dc->samples)) {
		int t0 = psample.time.seconds;
		int t1 = sample.time.seconds;
		int j;

		for (j = t0; j < t1; j++) {
			int depth = interpolate(psample.depth.mm, sample.depth.mm, j - t0, t1 - t0);
			auto gasmix = loop.next(j);
			add_segment(ds, depth_to_bar(depth, dive), gasmix, 1, sample.setpoint.mbar,
				    loop_d.next(j), dive->sac,
				    in_planner);
		}
	}
}

/* take into account previous dives until there is a 48h gap between dives */
/* return last surface time before this dive or dummy value of 48h */
/* return negative surface time if dives are overlapping */
/* The place you call this function is likely the place where you want
 * to create the deco_state */
int init_decompression(struct deco_state *ds, const struct dive *dive, bool in_planner)
{
	int surface_time = 48 * 60 * 60;
	timestamp_t last_endtime = 0, last_starttime = 0;
	bool deco_init = false;
	double surface_pressure;

	if (!dive)
		return false;

	int nr_dives = static_cast<int>(divelog.dives.size());
	size_t divenr = divelog.dives.get_idx(dive);
	int i = divenr != std::string::npos ? static_cast<int>(divenr)
					    : static_cast<int>(divelog.dives.size());
#if DECO_CALC_DEBUG & 2
	if (i < dive_table.nr)
		printf("\n\n*** Init deco for dive #%d %d\n", i, get_dive(i)->number);
	else
		printf("\n\n*** Init deco for dive #%d\n", i);
#endif
	/* Look at next dive in dive list table and correct i when needed */
	while (i + 1 < nr_dives) {
		struct dive *pdive = get_dive(i);
		if (!pdive || pdive->when > dive->when)
			break;
		i++;
	}
	/* Look at previous dive in dive list table and correct i when needed */
	while (i > 0) {
		struct dive *pdive = get_dive(i - 1);
		if (!pdive || pdive->when < dive->when)
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
		printf("Check if dive #%d %d has to be considered as prev dive: ", i, get_dive(i)->number);
#endif
		struct dive *pdive = get_dive(i);
		/* we don't want to mix dives from different trips as we keep looking
		 * for how far back we need to go */
		if (dive->divetrip && pdive->divetrip != dive->divetrip) {
#if DECO_CALC_DEBUG & 2
			printf("No - other dive trip\n");
#endif
			continue;
		}
		if (!pdive || pdive->when >= dive->when || pdive->endtime() + 48 * 60 * 60 < last_starttime) {
#if DECO_CALC_DEBUG & 2
			printf("No\n");
#endif
			break;
		}
		last_starttime = pdive->when;
#if DECO_CALC_DEBUG & 2
		printf("Yes\n");
#endif
	}
	/* Walk forward an add dives and surface intervals to deco */
	while (++i < nr_dives) {
#if DECO_CALC_DEBUG & 2
		printf("Check if dive #%d %d will be really added to deco calc: ", i, get_dive(i)->number);
#endif
		struct dive *pdive = get_dive(i);
		/* again skip dives from different trips */
		if (dive->divetrip && dive->divetrip != pdive->divetrip) {
#if DECO_CALC_DEBUG & 2
			printf("No - other dive trip\n");
#endif
			continue;
		}
		/* Don't add future dives */
		if (pdive->when >= dive->when) {
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

		surface_pressure = get_surface_pressure_in_mbar(pdive, true) / 1000.0;
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
			surface_time = pdive->when - last_endtime;
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

		last_starttime = pdive->when;
		last_endtime = pdive->endtime();
		clear_vpmb_state(ds);
#if DECO_CALC_DEBUG & 2
		printf("Tissues after added dive #%d:\n", pdive->number);
		dump_tissues(ds);
#endif
	}

	surface_pressure = get_surface_pressure_in_mbar(dive, true) / 1000.0;
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

void update_cylinder_related_info(struct dive *dive)
{
	if (dive != NULL) {
		dive->sac = calculate_sac(dive);
		dive->otu = calculate_otu(dive);
		if (dive->maxcns == 0)
			dive->maxcns = calculate_cns(dive);
	}
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

/*
 * Walk the dives from the oldest dive in the given table, and see if we
 * can autogroup them. But only do this when the user selected autogrouping.
 */
static void autogroup_dives(struct dive_table &table, struct trip_table &trip_table)
{
	if (!divelog.autogroup)
		return;

	for (auto &entry: get_dives_to_autogroup(table)) {
		for (auto it = table.begin() + entry.from; it != table.begin() + entry.to; ++it)
			entry.trip->add_dive(it->get());
		/* If this was newly allocated, add trip to list */
		if (entry.created_trip)
			trip_table.put(std::move(entry.created_trip));
	}
	trip_table.sort(); // Necessary?
#ifdef DEBUG_TRIP
	dump_trip_list();
#endif
}

/* This removes a dive from the global dive table but doesn't free the
 * resources associated with the dive. The caller must removed the dive
 * from the trip-list. Returns a pointer to the unregistered dive.
 * The unregistered dive has the selection- and hidden-flags cleared. */
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
 */
struct dive *register_dive(std::unique_ptr<dive> d)
{
	// When we add dives, we start in hidden-by-filter status. Once all
	// dives have been added, their status will be updated.
	d->hidden_by_filter = true;

	fulltext_register(d.get());				// Register the dive's fulltext cache
	invalidate_dive_cache(d.get());				// Ensure that dive is written in git_save()
	auto [res, idx] = divelog.dives.put(std::move(d));

	return res;
}

void process_loaded_dives()
{
	divelog.dives.sort();
	divelog.trips->sort();

	/* Autogroup dives if desired by user. */
	autogroup_dives(divelog.dives, *divelog.trips);

	fulltext_populate();

	/* Inform frontend of reset data. This should reset all the models. */
	emit_reset_signal();

	/* Now that everything is settled, select the newest dive. */
	select_newest_visible_dive();
}

/*
 * Merge subsequent dives in a table, if mergeable. This assumes
 * that the dives are neither selected, not part of a trip, as
 * is the case of freshly imported dives.
 */
static void merge_imported_dives(struct dive_table &table)
{
	for (size_t i = 1; i < table.size(); i++) {
		auto &prev = table[i - 1];
		auto &dive = table[i];

		/* only try to merge overlapping dives - or if one of the dives has
		 * zero duration (that might be a gps marker from the webservice) */
		if (prev->duration.seconds && dive->duration.seconds &&
		    prev->endtime() < dive->when)
			continue;

		auto merged = try_to_merge(*prev, *dive, false);
		if (!merged)
			continue;

		/* Add dive to dive site; try_to_merge() does not do that! */
		struct dive_site *ds = merged->dive_site;
		if (ds) {
			merged->dive_site = NULL;
			ds->add_dive(merged.get());
		}
		unregister_dive_from_dive_site(prev.get());
		unregister_dive_from_dive_site(dive.get());
		unregister_dive_from_trip(prev.get());
		unregister_dive_from_trip(dive.get());

		/* Overwrite the first of the two dives and remove the second */
		table[i - 1] = std::move(merged);
		table.erase(table.begin() + i);

		/* Redo the new 'i'th dive */
		i--;
	}
}

/*
 * Try to merge a new dive into the dive at position idx. Return
 * true on success. On success, the old dive will be added to the
 * dives_to_remove table and the merged dive to the dives_to_add
 * table. On failure everything stays unchanged.
 * If "prefer_imported" is true, use data of the new dive.
 */
static bool try_to_merge_into(struct dive &dive_to_add, struct dive *old_dive, bool prefer_imported,
			      /* output parameters: */
			      struct dive_table &dives_to_add, struct std::vector<dive *> &dives_to_remove)
{
	auto merged = try_to_merge(*old_dive, dive_to_add, prefer_imported);
	if (!merged)
		return false;

	merged->divetrip = old_dive->divetrip;
	range_insert_sorted(dives_to_remove, old_dive, comp_dives_ptr);
	dives_to_add.put(std::move(merged));

	return true;
}

/* Check if a dive is ranked after the last dive of the global dive list */
static bool dive_is_after_last(const struct dive &d)
{
	if (divelog.dives.empty())
		return true;
	return dive_less_than(*divelog.dives.back(), d);
}

/* Merge dives from "dives_from", owned by "delete" into the owned by "dives_to".
 * Overlapping dives will be merged, non-overlapping dives will be moved. The results
 * will be added to the "dives_to_add" table. Dives that were merged are added to
 * the "dives_to_remove" table. Any newly added (not merged) dive will be assigned
 * to the trip of the "trip" paremeter. If "delete_from" is non-null dives will be
 * removed from this table.
 * This function supposes that all input tables are sorted.
 * Returns true if any dive was added (not merged) that is not past the
 * last dive of the global dive list (i.e. the sequence will change).
 * The integer referenced by "num_merged" will be increased for every
 * merged dive that is added to "dives_to_add" */
static bool merge_dive_tables(const std::vector<dive *> &dives_from, struct dive_table &delete_from,
			      const std::vector<dive *> &dives_to,
			      bool prefer_imported, struct dive_trip *trip,
			      /* output parameters: */
			      struct dive_table &dives_to_add, struct std::vector<dive *> &dives_to_remove,
			      int &num_merged)
{
	bool sequence_changed = false;

	/* Merge newly imported dives into the dive table.
	 * Since both lists (old and new) are sorted, we can step
	 * through them concurrently and locate the insertions points.
	 * Once found, check if the new dive can be merged in the
	 * previous or next dive.
	 * Note that this doesn't consider pathological cases such as:
	 *  - New dive "connects" two old dives (turn three into one).
	 *  - New dive can not be merged into adjacent but some further dive.
	 */
	size_t j = 0; /* Index in dives_to */
	size_t last_merged_into = std::string::npos;
	for (dive *add: dives_from) {
		/* This gets an owning pointer to the dive to add and removes it from
		 * the delete_from table. If the dive is not explicitly stored, it will
		 * be automatically deleting when ending the loop iteration */
		auto [dive_to_add, idx] = delete_from.pull(add);
		if (!dive_to_add) {
			report_info("merging unknown dives!");
			continue;
		}

		/* Find insertion point. */
		while (j < dives_to.size() && dive_less_than(*dives_to[j], *dive_to_add))
			j++;

		/* Try to merge into previous dive.
		 * We are extra-careful to not merge into the same dive twice, as that
		 * would put the merged-into dive twice onto the dives-to-delete list.
		 * In principle that shouldn't happen as all dives that compare equal
		 * by is_same_dive() were already merged, and is_same_dive() should be
		 * transitive. But let's just go *completely* sure for the odd corner-case. */
		if (j > 0 && (last_merged_into == std::string::npos || j > last_merged_into + 1) &&
		    dives_to[j - 1]->endtime() > dive_to_add->when) {
			if (try_to_merge_into(*dive_to_add, dives_to[j - 1], prefer_imported,
					      dives_to_add, dives_to_remove)) {
				last_merged_into = j - 1;
				num_merged++;
				continue;
			}
		}

		/* That didn't merge into the previous dive.
		 * Try to merge into next dive. */
		if (j < dives_to.size() && (last_merged_into == std::string::npos || j > last_merged_into) &&
		    dive_to_add->endtime() > dives_to[j]->when) {
			if (try_to_merge_into(*dive_to_add, dives_to[j], prefer_imported,
					      dives_to_add, dives_to_remove)) {
				last_merged_into = j;
				num_merged++;
				continue;
			}
		}

		sequence_changed |= !dive_is_after_last(*dive_to_add);
		dives_to_add.put(std::move(dive_to_add));
	}

	return sequence_changed;
}

/* Merge the dives of the trip "from" and the dive_table "dives_from" into the trip "to"
 * and dive_table "dives_to". If "prefer_imported" is true, dive data of "from" takes
 * precedence */
void add_imported_dives(struct divelog &import_log, int flags)
{
	/* Process imported dives and generate lists of dives
	 * to-be-added and to-be-removed */
	auto [dives_to_add, dives_to_remove, trips_to_add, dive_sites_to_add, devices_to_add] =
		process_imported_dives(import_log, flags);

	/* Start by deselecting all dives, so that we don't end up with an invalid selection */
	select_single_dive(NULL);

	/* Add new dives to trip and site to get reference count correct. */
	for (auto &d: dives_to_add) {
		struct dive_trip *trip = d->divetrip;
		struct dive_site *site = d->dive_site;
		d->divetrip = NULL;
		d->dive_site = NULL;
		trip->add_dive(d.get());
		if (site)
			site->add_dive(d.get());
	}

	/* Remove old dives */
	divelog.delete_multiple_dives(dives_to_remove);

	/* Add new dives */
	for (auto &d: dives_to_add)
		divelog.dives.put(std::move(d));
	dives_to_add.clear();

	/* Add new trips */
	for (auto &trip: trips_to_add)
		divelog.trips->put(std::move(trip));
	trips_to_add.clear();

	/* Add new dive sites */
	for (auto &ds: dive_sites_to_add)
		divelog.sites.register_site(std::move(ds));

	/* Add new devices */
	for (auto &dev: devices_to_add)
		add_to_device_table(divelog.devices, dev);

	/* We might have deleted the old selected dive.
	 * Choose the newest dive as selected (if any) */
	current_dive = !divelog.dives.empty() ? divelog.dives.back().get() : nullptr;

	/* Inform frontend of reset data. This should reset all the models. */
	emit_reset_signal();
}

/* Helper function for process_imported_dives():
 * Try to merge a trip into one of the existing trips.
 * The bool pointed to by "sequence_changed" is set to true, if the sequence of
 * the existing dives changes.
 * The int pointed to by "start_renumbering_at" keeps track of the first dive
 * to be renumbered in the dives_to_add table.
 * For other parameters see process_imported_dives()
 * Returns true if trip was merged. In this case, the trip will be
 * freed.
 */
static bool try_to_merge_trip(dive_trip &trip_import, struct dive_table &import_table, bool prefer_imported,
			      /* output parameters: */
			      struct dive_table &dives_to_add, std::vector<dive *> &dives_to_remove,
			      bool &sequence_changed, int &start_renumbering_at)
{
	for (auto &trip_old: *divelog.trips) {
		if (trips_overlap(trip_import, *trip_old)) {
			sequence_changed |= merge_dive_tables(trip_import.dives, import_table, trip_old->dives,
							       prefer_imported, trip_old.get(),
							       dives_to_add, dives_to_remove,
							       start_renumbering_at);
			/* we took care of all dives of the trip, clean up the table */
			trip_import.dives.clear();
			return true;
		}
	}

	return false;
}

// Helper function to convert a table of owned dives into a table of non-owning pointers.
// Used to merge *all* dives of a log into a different table.
static std::vector<dive *> dive_table_to_non_owning(const dive_table &dives)
{
	std::vector<dive *> res;
	res.reserve(dives.size());
	for (auto &d: dives)
		res.push_back(d.get());
	return res;
}

/* Process imported dives: take a table of dives to be imported and
 * generate five lists:
 *	1) Dives to be added		(newly created, owned)
 *	2) Dives to be removed		(old, non-owned, references global divelog)
 *	3) Trips to be added		(newly created, owned)
 *	4) Dive sites to be added	(newly created, owned)
 *	5) Devices to be added		(newly created, owned)
 * The dives, trips and sites in import_log are consumed.
 * On return, the tables have * size 0.
 *
 * Note: The new dives will have their divetrip- and divesites-fields
 * set, but will *not* be part of the trip and site. The caller has to
 * add them to the trip and site.
 *
 * The lists are generated by merging dives if possible. This is
 * performed trip-wise. Finer control on merging is provided by
 * the "flags" parameter:
 * - If IMPORT_PREFER_IMPORTED is set, data of the new dives are
 *   prioritized on merging.
 * - If IMPORT_MERGE_ALL_TRIPS is set, all overlapping trips will
 *   be merged, not only non-autogenerated trips.
 * - If IMPORT_IS_DOWNLOADED is true, only the divecomputer of the
 *   first dive will be considered, as it is assumed that all dives
 *   come from the same computer.
 * - If IMPORT_ADD_TO_NEW_TRIP is true, dives that are not assigned
 *   to a trip will be added to a newly generated trip.
 */
process_imported_dives_result process_imported_dives(struct divelog &import_log, int flags)
{
	int start_renumbering_at = 0;
	bool sequence_changed = false;
	bool last_old_dive_is_numbered;

	process_imported_dives_result res;

	/* If no dives were imported, don't bother doing anything */
	if (import_log.dives.empty())
		return res;

	/* Check if any of the new dives has a number. This will be
	 * important later to decide if we want to renumber the added
	 * dives */
	bool new_dive_has_number = std::any_of(import_log.dives.begin(), import_log.dives.end(),
					       [](auto &d) { return d->number > 0; });

	/* Add only the devices that we don't know about yet. */
	for (auto &dev: import_log.devices) {
		if (!device_exists(divelog.devices, dev))
			add_to_device_table(res.devices_to_add, dev);
	}

	/* Sort the table of dives to be imported and combine mergable dives */
	import_log.dives.sort();
	merge_imported_dives(import_log.dives);

	/* Autogroup tripless dives if desired by user. But don't autogroup
	 * if tripless dives should be added to a new trip. */
	if (!(flags & IMPORT_ADD_TO_NEW_TRIP))
		autogroup_dives(import_log.dives, *import_log.trips);

	/* If dive sites already exist, use the existing versions. */
	for (auto &new_ds: import_log.sites) {
		/* Check if it dive site is actually used by new dives. */
		if (std::none_of(import_log.dives.begin(), import_log.dives.end(), [ds=new_ds.get()]
				 (auto &d) { return d->dive_site == ds; }))
			continue;

		struct dive_site *old_ds = divelog.sites.get_same(*new_ds);
		if (!old_ds) {
			/* Dive site doesn't exist. Add it to list of dive sites to be added. */
			new_ds->dives.clear(); /* Caller is responsible for adding dives to site */
			res.sites_to_add.put(std::move(new_ds));
		} else {
			/* Dive site already exists - use the old one. */
			for (auto &d: import_log.dives) {
				if (d->dive_site == new_ds.get())
					d->dive_site = old_ds;
			}
		}
	}
	import_log.sites.clear();

	/* Merge overlapping trips. Since both trip tables are sorted, we
	 * could be smarter here, but realistically not a whole lot of trips
	 * will be imported so do a simple n*m loop until someone complains.
	 */
	for (auto &trip_import: *import_log.trips) {
		if ((flags & IMPORT_MERGE_ALL_TRIPS) || trip_import->autogen) {
			if (try_to_merge_trip(*trip_import, import_log.dives, flags & IMPORT_PREFER_IMPORTED,
					      res.dives_to_add, res.dives_to_remove,
					      sequence_changed, start_renumbering_at))
				continue;
		}

		/* If no trip to merge-into was found, add trip as-is.
		 * First, add dives to list of dives to add */
		for (struct dive *d: trip_import->dives) {
			/* Add dive to list of dives to-be-added. */
			auto [owned, idx] = import_log.dives.pull(d);
			if (!owned)
				continue;
			sequence_changed |= !dive_is_after_last(*owned);
			res.dives_to_add.put(std::move(owned));
		}

		trip_import->dives.clear(); /* Caller is responsible for adding dives to trip */

		/* Finally, add trip to list of trips to add */
		res.trips_to_add.put(std::move(trip_import));
	}
	import_log.trips->clear(); /* All trips were consumed */

	if ((flags & IMPORT_ADD_TO_NEW_TRIP) && !import_log.dives.empty()) {
		/* Create a new trip for unassigned dives, if desired. */
		auto [new_trip, idx] = res.trips_to_add.put(
			create_trip_from_dive(import_log.dives.front().get())
		);

		/* Add all remaining dives to this trip */
		for (auto &d: import_log.dives) {
			sequence_changed |= !dive_is_after_last(*d);
			d->divetrip = new_trip;
			res.dives_to_add.put(std::move(d));
		}

		import_log.dives.clear(); /* All dives were consumed */
	} else if (!import_log.dives.empty()) {
		/* The remaining dives in import_log.dives are those that don't belong to
		 * a trip and the caller does not want them to be associated to a
		 * new trip. Merge them into the global table. */
		sequence_changed |= merge_dive_tables(dive_table_to_non_owning(import_log.dives),
						      import_log.dives,
						      dive_table_to_non_owning(divelog.dives),
						      flags & IMPORT_PREFER_IMPORTED, NULL,
						      res.dives_to_add, res.dives_to_remove, start_renumbering_at);
	}

	/* If new dives were only added at the end, renumber the added dives.
	 * But only if
	 *	- The last dive in the old dive table had a number itself (if there is a last dive).
	 *	- None of the new dives has a number.
	 */
	last_old_dive_is_numbered = divelog.dives.empty() || divelog.dives.back()->number > 0;

	/* We counted the number of merged dives that were added to dives_to_add.
	 * Skip those. Since sequence_changed is false all added dives are *after*
	 * all merged dives. */
	if (!sequence_changed && last_old_dive_is_numbered && !new_dive_has_number) {
		int nr = !divelog.dives.empty() ? divelog.dives.back()->number : 0;
		for (auto it = res.dives_to_add.begin() + start_renumbering_at; it < res.dives_to_add.end(); ++it)
			(*it)->number = ++nr;
	}

	return res;
}

static struct dive *get_last_valid_dive()
{
	auto it = std::find_if(divelog.dives.rbegin(), divelog.dives.rend(),
			      [](auto &d) { return !d->invalid; });
	return it != divelog.dives.rend() ? it->get() : nullptr;
}

/* return the number a dive gets when inserted at the given index.
 * this function is supposed to be called *before* a dive was added.
 * this returns:
 * 	- 1 for an empty log
 * 	- last_nr+1 for addition at end of log (if last dive had a number)
 * 	- 0 for all other cases
 */
int get_dive_nr_at_idx(int idx)
{
	if (static_cast<size_t>(idx) < divelog.dives.size())
		return 0;
	struct dive *last_dive = get_last_valid_dive();
	if (!last_dive)
		return 1;
	return last_dive->number ? last_dive->number + 1 : 0;
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

static int min_datafile_version;

int get_min_datafile_version()
{
	return min_datafile_version;
}

void report_datafile_version(int version)
{
	if (min_datafile_version == 0 || min_datafile_version > version)
		min_datafile_version = version;
}

void clear_dive_file_data()
{
	fulltext_unregister_all();
	select_single_dive(NULL);	// This is propagated up to the UI and clears all the information.

	current_dive = NULL;
	divelog.clear();

	clear_event_types();

	min_datafile_version = 0;
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
timestamp_t get_surface_interval(timestamp_t when)
{
	timestamp_t prev_end;

	/* find previous dive. might want to use a binary search. */
	auto it = std::find_if(divelog.dives.rbegin(), divelog.dives.rend(),
			       [when] (auto &d) { return d->when < when; });
	if (it == divelog.dives.rend())
		return -1;

	prev_end = (*it)->endtime();
	if (prev_end > when)
		return 0;
	return when - prev_end;
}

/* Find visible dive close to given date. First search towards older,
 * then newer dives. */
struct dive *find_next_visible_dive(timestamp_t when)
{
	if (divelog.dives.empty())
		return nullptr;

	/* we might want to use binary search here */
	auto it = std::find_if(divelog.dives.begin(), divelog.dives.end(),
			       [when] (auto &d) { return d->when <= when; });

	for (auto it2 = it; it2 != divelog.dives.begin(); --it2) {
		if (!(*std::prev(it2))->hidden_by_filter)
			return it2->get();
	}

	for (auto it2 = it; it2 != divelog.dives.end(); ++it2) {
		if (!(*it2)->hidden_by_filter)
			return it2->get();
	}

	return nullptr;
}

bool has_dive(unsigned int deviceid, unsigned int diveid)
{
	return std::any_of(divelog.dives.begin(), divelog.dives.end(), [deviceid,diveid] (auto &d) {
			return std::any_of(d->dcs.begin(), d->dcs.end(), [deviceid,diveid] (auto &dc) {
				return dc.deviceid == deviceid && dc.diveid == diveid;
			});
	       });
}
