// SPDX-License-Identifier: GPL-2.0
/* divelist.c */

#include "divelist.h"
#include "subsurface-string.h"
#include "deco.h"
#include "device.h"
#include "divesite.h"
#include "dive.h"
#include "event.h"
#include "filterpreset.h"
#include "fulltext.h"
#include "interpolate.h"
#include "planner.h"
#include "qthelper.h"
#include "gettext.h"
#include "git-access.h"
#include "selection.h"
#include "sample.h"
#include "table.h"
#include "trip.h"

bool autogroup = false;

void set_autogroup(bool value)
{
	/* if we keep the UI paradigm, this needs to toggle
	 * the checkbox on the autogroup menu item */
	autogroup = value;
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
	int i;
	int maxo2 = -1, maxhe = -1, mino2 = 1000;

	for (i = 0; i < dive->cylinders.nr; i++) {
		const cylinder_t *cyl = get_cylinder(dive, i);
		int o2 = get_o2(cyl->gasmix);
		int he = get_he(cyl->gasmix);

		if (!is_cylinder_used(dive, i))
			continue;
		if (o2 > maxo2)
			maxo2 = o2;
		if (he > maxhe)
			goto newmax;
		if (he < maxhe)
			continue;
		if (o2 <= maxo2)
			continue;
	newmax:
		maxhe = he;
		mino2 = o2;
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
	int i, total_grams = 0;

	if (dive)
		for (i = 0; i < dive->weightsystems.nr; i++)
			total_grams += dive->weightsystems.weightsystems[i].weight.grams;
	return total_grams;
}

static int active_o2(const struct dive *dive, const struct divecomputer *dc, duration_t time)
{
	struct gasmix gas = get_gasmix_at_time(dive, dc, time);
	return get_o2(gas);
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
	int i;
	double otu = 0.0;
	const struct divecomputer *dc = &dive->dc;
	for (i = 1; i < dc->samples; i++) {
		int t;
		int po2i, po2f;
		double pm;
		struct sample *sample = dc->sample + i;
		struct sample *psample = sample - 1;
		t = sample->time.seconds - psample->time.seconds;
		if ((dc->divemode == CCR || dc->divemode == PSCR) && sample->o2sensor[0].mbar) {	// if dive computer has o2 sensor(s) (CCR & PSCR) ..
			po2i = psample->o2sensor[0].mbar;
			po2f = sample->o2sensor[0].mbar;	// ... use data from the first o2 sensor
		} else {
			if (dc->divemode == CCR) {
				po2i = MIN((int) psample->setpoint.mbar,
					   depth_to_mbar(psample->depth.mm, dive));		// if CCR has no o2 sensors then use setpoint
				po2f = MIN((int) sample->setpoint.mbar,
					   depth_to_mbar(sample->depth.mm, dive));
			} else {						// For OC and rebreather without o2 sensor/setpoint
				double amb_presure = depth_to_bar(sample->depth.mm, dive);
				double pamb_pressure = depth_to_bar(psample->depth.mm , dive);
				if (dc->divemode == PSCR) {
					po2i = pscr_o2(pamb_pressure, get_gasmix_at_time(dive, dc, psample->time));
					po2f = pscr_o2(amb_presure, get_gasmix_at_time(dive, dc, sample->time));
				} else {
					int o2 = active_o2(dive, dc, psample->time);	// 	... calculate po2 from depth and FiO2.
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
	int n;
	const struct divecomputer *dc = &dive->dc;
	double cns = 0.0;
	double rate;
	/* Calculate the CNS for each sample in this dive and sum them */
	for (n = 1; n < dc->samples; n++) {
		int t;
		int po2i, po2f;
		bool trueo2 = false;
		struct sample *sample = dc->sample + n;
		struct sample *psample = sample - 1;
		t = sample->time.seconds - psample->time.seconds;
		if ((dc->divemode == CCR || dc->divemode == PSCR) && sample->o2sensor[0].mbar) {			// if dive computer has o2 sensor(s) (CCR & PSCR)
			po2i = psample->o2sensor[0].mbar;
			po2f = sample->o2sensor[0].mbar;	// then use data from the first o2 sensor
			trueo2 = true;
		}
		if ((dc->divemode == CCR) && (!trueo2)) {
			po2i = MIN((int) psample->setpoint.mbar,
				   depth_to_mbar(psample->depth.mm, dive));		// if CCR has no o2 sensors then use setpoint
			po2f = MIN((int) sample->setpoint.mbar,
				   depth_to_mbar(sample->depth.mm, dive));
			trueo2 = true;
		}
		if (!trueo2) {
			double amb_presure = depth_to_bar(sample->depth.mm, dive);
			double pamb_pressure = depth_to_bar(psample->depth.mm , dive);
			if (dc->divemode == PSCR) {
				po2i = pscr_o2(pamb_pressure, get_gasmix_at_time(dive, dc, psample->time));
				po2f = pscr_o2(amb_presure, get_gasmix_at_time(dive, dc, sample->time));
			} else {
				int o2 = active_o2(dive, dc, psample->time);	// 	... calculate po2 from depth and FiO2.
				po2i = lrint(o2 * pamb_pressure);	// (initial) po2 at start of segment
				po2f = lrint(o2 * amb_presure);	// (final) po2 at end of segment
			}
		}
		int po2avg = (po2i + po2f) / 2;	// po2i now holds the mean po2 of initial and final po2 values of segment.
		/* Don't increase CNS when po2 below 500 matm */
		if (po2avg <= 500)
			continue;

		// This formula is the result of fitting two lines to the Log of the NOAA CNS table
		rate = po2i <= 1500 ? exp(-11.7853 + 0.00193873 * po2avg) : exp(-23.6349 + 0.00980829 * po2avg);
		cns += (double) t * rate * 100.0;
	}
	return cns;
}

/* this only gets called if dive->maxcns == 0 which means we know that
 * none of the divecomputers has tracked any CNS for us
 * so we calculated it "by hand" */
static int calculate_cns(struct dive *dive)
{
	int i, divenr;
	double cns = 0.0;
	timestamp_t last_starttime, last_endtime = 0;

	/* shortcut */
	if (dive->cns)
		return dive->cns;

	divenr = get_divenr(dive);
	i = divenr >= 0 ? divenr : dive_table.nr;
#if DECO_CALC_DEBUG & 2
	if (i >= 0 && i < dive_table.nr)
		printf("\n\n*** CNS for dive #%d %d\n", i, get_dive(i)->number);
	else
		printf("\n\n*** CNS for dive #%d\n", i);
#endif
	/* Look at next dive in dive list table and correct i when needed */
	while (i < dive_table.nr - 1) {
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
		if (i == divenr && i > 0)
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
		if (!pdive || pdive->when >= dive->when || dive_endtime(pdive) + 12 * 60 * 60 < last_starttime) {
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
	while (++i < dive_table.nr) {
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
		if (i == divenr) {
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
		last_endtime = dive_endtime(pdive);
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
	int i;

	for (i = 0; i < dive->cylinders.nr; i++) {
		pressure_t start, end;
		const cylinder_t *cyl = get_cylinder(dive, i);

		start = cyl->start.mbar ? cyl->start : cyl->sample_start;
		end = cyl->end.mbar ? cyl->end : cyl->sample_end;
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

		airuse += gas_volume(cyl, start) - gas_volume(cyl, end);
	}
	return airuse / 1000.0;
}

/* this only uses the first divecomputer to calculate the SAC rate */
static int calculate_sac(const struct dive *dive)
{
	const struct divecomputer *dc = &dive->dc;
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
	struct divecomputer *dc = &dive->dc;
	struct gasmix gasmix = gasmix_air;
	int i;
	const struct event *ev = NULL, *evd = NULL;
	enum divemode_t current_divemode = UNDEF_COMP_TYPE;

	if (!dc)
		return;

	for (i = 1; i < dc->samples; i++) {
		struct sample *psample = dc->sample + i - 1;
		struct sample *sample = dc->sample + i;
		int t0 = psample->time.seconds;
		int t1 = sample->time.seconds;
		int j;

		for (j = t0; j < t1; j++) {
			int depth = interpolate(psample->depth.mm, sample->depth.mm, j - t0, t1 - t0);
			gasmix = get_gasmix(dive, dc, j, &ev, gasmix);
			add_segment(ds, depth_to_bar(depth, dive), gasmix, 1, sample->setpoint.mbar,
				    get_current_divemode(&dive->dc, j, &evd, &current_divemode), dive->sac,
				    in_planner);
		}
	}
}

int get_divenr(const struct dive *dive)
{
	int i;
	const struct dive *d;
	// tempting as it may be, don't die when called with dive=NULL
	if (dive)
		for_each_dive(i, d) {
			if (d->id == dive->id) // don't compare pointers, we could be passing in a copy of the dive
				return i;
		}
	return -1;
}

static struct gasmix air = { .o2.permille = O2_IN_AIR, .he.permille = 0 };

/* take into account previous dives until there is a 48h gap between dives */
/* return last surface time before this dive or dummy value of 48h */
/* return negative surface time if dives are overlapping */
/* The place you call this function is likely the place where you want
 * to create the deco_state */
int init_decompression(struct deco_state *ds, const struct dive *dive, bool in_planner)
{
	int i, divenr = -1;
	int surface_time = 48 * 60 * 60;
	timestamp_t last_endtime = 0, last_starttime = 0;
	bool deco_init = false;
	double surface_pressure;

	if (!dive)
		return false;

	divenr = get_divenr(dive);
	i = divenr >= 0 ? divenr : dive_table.nr;
#if DECO_CALC_DEBUG & 2
	if (i >= 0 && i < dive_table.nr)
		printf("\n\n*** Init deco for dive #%d %d\n", i, get_dive(i)->number);
	else
		printf("\n\n*** Init deco for dive #%d\n", i);
#endif
	/* Look at next dive in dive list table and correct i when needed */
	while (i < dive_table.nr - 1) {
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
		if (i == divenr && i > 0)
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
		if (!pdive || pdive->when >= dive->when || dive_endtime(pdive) + 48 * 60 * 60 < last_starttime) {
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
	while (++i < dive_table.nr) {
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
		if (i == divenr) {
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
			add_segment(ds, surface_pressure, air, surface_time, 0, dive->dc.divemode, prefs.decosac, in_planner);
#if DECO_CALC_DEBUG & 2
			printf("Tissues after surface intervall of %d:%02u:\n", FRACTION(surface_time, 60));
			dump_tissues(ds);
#endif
		}

		add_dive_to_deco(ds, pdive, in_planner);

		last_starttime = pdive->when;
		last_endtime = dive_endtime(pdive);
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
		add_segment(ds, surface_pressure, air, surface_time, 0, dive->dc.divemode, prefs.decosac, in_planner);
#if DECO_CALC_DEBUG & 2
		printf("Tissues after surface intervall of %d:%02u:\n", FRACTION(surface_time, 60));
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

#define MAX_GAS_STRING 80

/* callers needs to free the string */
char *get_dive_gas_string(const struct dive *dive)
{
	int o2, he, o2max;
	char *buffer = malloc(MAX_GAS_STRING);

	if (buffer) {
		get_dive_gas(dive, &o2, &he, &o2max);
		o2 = (o2 + 5) / 10;
		he = (he + 5) / 10;
		o2max = (o2max + 5) / 10;

		if (he)
			if (o2 == o2max)
				snprintf(buffer, MAX_GAS_STRING, "%d/%d", o2, he);
			else
				snprintf(buffer, MAX_GAS_STRING, "%d/%d…%d%%", o2, he, o2max);
		else if (o2)
			if (o2 == o2max)
				snprintf(buffer, MAX_GAS_STRING, "%d%%", o2);
			else
				snprintf(buffer, MAX_GAS_STRING, "%d…%d%%", o2, o2max);
		else
			strcpy(buffer, translate("gettextFromC", "air"));
	}
	return buffer;
}

/* Like strcmp(), but don't crash on null-pointers */
static int safe_strcmp(const char *s1, const char *s2)
{
	return strcmp(s1 ? s1 : "", s2 ? s2 : "");
}

/* Compare a list of dive computers by model name */
static int comp_dc(const struct divecomputer *dc1, const struct divecomputer *dc2)
{
	int cmp;
	while (dc1 || dc2) {
		if (!dc1)
			return -1;
		if (!dc2)
			return 1;
		if ((cmp = safe_strcmp(dc1->model, dc2->model)) != 0)
			return cmp;
		dc1 = dc1->next;
		dc2 = dc2->next;
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
 * "id" is a stable, strictly increasing unique number, that
 * is handed out when a dive is added to the system.
 * We might also consider sorting by end-time and other criteria,
 * but see the caveat above (editing means rearrangement of the dives).
 */
int comp_dives(const struct dive *a, const struct dive *b)
{
	int cmp;
	if (a->when < b->when)
		return -1;
	if (a->when > b->when)
		return 1;
	if (a->divetrip != b->divetrip) {
		if (!b->divetrip)
			return -1;
		if (!a->divetrip)
			return 1;
		if (trip_date(a->divetrip) < trip_date(b->divetrip))
			return -1;
		if (trip_date(a->divetrip) > trip_date(b->divetrip))
			return 1;
	}
	if (a->number < b->number)
		return -1;
	if (a->number > b->number)
		return 1;
	if ((cmp = comp_dc(&a->dc, &b->dc)) != 0)
		return cmp;
	if (a->id < b->id)
		return -1;
	if (a->id > b->id)
		return 1;
	return 0; /* this should not happen for a != b */
}

/* Dive table functions */
static MAKE_GROW_TABLE(dive_table, struct dive *, dives)
MAKE_GET_INSERTION_INDEX(dive_table, struct dive *, dives, dive_less_than)
MAKE_ADD_TO(dive_table, struct dive *, dives)
static MAKE_REMOVE_FROM(dive_table, dives)
static MAKE_GET_IDX(dive_table, struct dive *, dives)
MAKE_SORT(dive_table, struct dive *, dives, comp_dives)
MAKE_REMOVE(dive_table, struct dive *, dive)
MAKE_CLEAR_TABLE(dive_table, dives, dive)
MAKE_MOVE_TABLE(dive_table, dives)

void insert_dive(struct dive_table *table, struct dive *d)
{
	int idx = dive_table_get_insertion_index(table, d);
	add_to_dive_table(table, idx, d);
}

/*
 * Walk the dives from the oldest dive in the given table, and see if we
 * can autogroup them. But only do this when the user selected autogrouping.
 */
static void autogroup_dives(struct dive_table *table, struct trip_table *trip_table_arg)
{
	int from, to;
	dive_trip_t *trip;
	int i, j;
	bool alloc;

	if (!autogroup)
		return;

	for (i = 0; (trip = get_dives_to_autogroup(table, i, &from, &to, &alloc)) != NULL; i = to) {
		for (j = from; j < to; ++j)
			add_dive_to_trip(table->dives[j], trip);
		/* If this was newly allocated, add trip to list */
		if (alloc)
			insert_trip(trip, trip_table_arg);
	}
	sort_trip_table(trip_table_arg);
#ifdef DEBUG_TRIP
	dump_trip_list();
#endif
}

/* Remove a dive from a dive table. This assumes that the
 * dive was already removed from any trip and deselected.
 * It simply shrinks the table and frees the trip */
void delete_dive_from_table(struct dive_table *table, int idx)
{
	free_dive(table->dives[idx]);
	remove_from_dive_table(table, idx);
}

struct dive *get_dive_from_table(int nr, const struct dive_table *dt)
{
	if (nr >= dt->nr || nr < 0)
		return NULL;
	return dt->dives[nr];
}

/* This removes a dive from the global dive table but doesn't free the
 * resources associated with the dive. The caller must removed the dive
 * from the trip-list. Returns a pointer to the unregistered dive.
 * The unregistered dive has the selection- and hidden-flags cleared. */
struct dive *unregister_dive(int idx)
{
	struct dive *dive = get_dive(idx);
	if (!dive)
		return NULL; /* this should never happen */
	/* When removing a dive from the global dive table,
	 * we also have to unregister its fulltext cache. */
	fulltext_unregister(dive);
	remove_from_dive_table(&dive_table, idx);
	if (dive->selected)
		amount_selected--;
	dive->selected = false;
	return dive;
}

/* this implements the mechanics of removing the dive from the global
 * dive table and the trip, but doesn't deal with updating dive trips, etc */
void delete_single_dive(int idx)
{
	struct dive *dive = get_dive(idx);
	if (!dive)
		return; /* this should never happen */
	if (dive->selected)
		deselect_dive(dive);
	remove_dive_from_trip(dive, &trip_table);
	unregister_dive_from_dive_site(dive);
	delete_dive_from_table(&dive_table, idx);
}

void process_loaded_dives()
{
	int i;
	struct dive *dive;

	/* Register dive computer nick names. */
	for_each_dive(i, dive)
		add_devices_of_dive(dive, &device_table);

	sort_dive_table(&dive_table);
	sort_trip_table(&trip_table);

	/* Autogroup dives if desired by user. */
	autogroup_dives(&dive_table, &trip_table);

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
static void merge_imported_dives(struct dive_table *table)
{
	int i;
	for (i = 1; i < table->nr; i++) {
		struct dive *prev = table->dives[i - 1];
		struct dive *dive = table->dives[i];
		struct dive *merged;
		struct dive_site *ds;

		/* only try to merge overlapping dives - or if one of the dives has
		 * zero duration (that might be a gps marker from the webservice) */
		if (prev->duration.seconds && dive->duration.seconds &&
		    dive_endtime(prev) < dive->when)
			continue;

		merged = try_to_merge(prev, dive, false);
		if (!merged)
			continue;

		/* Add dive to dive site; try_to_merge() does not do that! */
		ds = merged->dive_site;
		if (ds) {
			merged->dive_site = NULL;
			add_dive_to_dive_site(merged, ds);
		}
		unregister_dive_from_dive_site(prev);
		unregister_dive_from_dive_site(dive);
		unregister_dive_from_trip(prev);
		unregister_dive_from_trip(dive);

		/* Overwrite the first of the two dives and remove the second */
		free_dive(prev);
		table->dives[i - 1] = merged;
		delete_dive_from_table(table, i);

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
static bool try_to_merge_into(struct dive *dive_to_add, int idx, struct dive_table *table, bool prefer_imported,
			      /* output parameters: */
			      struct dive_table *dives_to_add, struct dive_table *dives_to_remove)
{
	struct dive *old_dive = table->dives[idx];
	struct dive *merged = try_to_merge(old_dive, dive_to_add, prefer_imported);
	if (!merged)
		return false;

	merged->divetrip = old_dive->divetrip;
	insert_dive(dives_to_remove, old_dive);
	insert_dive(dives_to_add, merged);

	return true;
}

/* Check if a dive is ranked after the last dive of the global dive list */
static bool dive_is_after_last(struct dive *d)
{
	if (dive_table.nr == 0)
		return true;
	return dive_less_than(dive_table.dives[dive_table.nr - 1], d);
}

/* Merge dives from "dives_from" into "dives_to". Overlapping dives will be merged,
 * non-overlapping dives will be moved. The results will be added to the "dives_to_add"
 * table. Dives that were merged are added to the "dives_to_remove" table.
 * Any newly added (not merged) dive will be assigned to the trip of the "trip"
 * paremeter. If "delete_from" is non-null dives will be removed from this table.
 * This function supposes that all input tables are sorted.
 * Returns true if any dive was added (not merged) that is not past the
 * last dive of the global dive list (i.e. the sequence will change).
 * The integer pointed to by "num_merged" will be increased for every
 * merged dive that is added to "dives_to_add" */
static bool merge_dive_tables(struct dive_table *dives_from, struct dive_table *delete_from,
			      struct dive_table *dives_to,
			      bool prefer_imported, struct dive_trip *trip,
			      /* output parameters: */
			      struct dive_table *dives_to_add, struct dive_table *dives_to_remove,
			      int *num_merged)
{
	int i, j;
	int last_merged_into = -1;
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
	j = 0; /* Index in dives_to */
	for (i = 0; i < dives_from->nr; i++) {
		struct dive *dive_to_add = dives_from->dives[i];

		if (delete_from)
			remove_dive(dive_to_add, delete_from);

		/* Find insertion point. */
		while (j < dives_to->nr && dive_less_than(dives_to->dives[j], dive_to_add))
			j++;

		/* Try to merge into previous dive.
		 * We are extra-careful to not merge into the same dive twice, as that
		 * would put the merged-into dive twice onto the dives-to-delete list.
		 * In principle that shouldn't happen as all dives that compare equal
		 * by is_same_dive() were already merged, and is_same_dive() should be
		 * transitive. But let's just go *completely* sure for the odd corner-case. */
		if (j > 0 && j - 1 > last_merged_into &&
		    dive_endtime(dives_to->dives[j - 1]) > dive_to_add->when) {
			if (try_to_merge_into(dive_to_add, j - 1, dives_to, prefer_imported,
					      dives_to_add, dives_to_remove)) {
				free_dive(dive_to_add);
				last_merged_into = j - 1;
				(*num_merged)++;
				continue;
			}
		}

		/* That didn't merge into the previous dive.
		 * Try to merge into next dive. */
		if (j < dives_to->nr && j > last_merged_into &&
		    dive_endtime(dive_to_add) > dives_to->dives[j]->when) {
			if (try_to_merge_into(dive_to_add, j, dives_to, prefer_imported,
					      dives_to_add, dives_to_remove)) {
				free_dive(dive_to_add);
				last_merged_into = j;
				(*num_merged)++;
				continue;
			}
		}

		/* We couldnt merge dives, simply add to list of dives to-be-added. */
		insert_dive(dives_to_add, dive_to_add);
		sequence_changed |= !dive_is_after_last(dive_to_add);
		dive_to_add->divetrip = trip;
	}

	/* we took care of all dives, clean up the import table */
	dives_from->nr = 0;

	return sequence_changed;
}

/* Merge the dives of the trip "from" and the dive_table "dives_from" into the trip "to"
 * and dive_table "dives_to". If "prefer_imported" is true, dive data of "from" takes
 * precedence */
void add_imported_dives(struct dive_table *import_table, struct trip_table *import_trip_table,
			struct dive_site_table *import_sites_table, struct device_table *import_device_table,
			int flags)
{
	int i, idx;
	struct dive_table dives_to_add = empty_dive_table;
	struct dive_table dives_to_remove = empty_dive_table;
	struct trip_table trips_to_add = empty_trip_table;
	struct dive_site_table dive_sites_to_add = empty_dive_site_table;
	struct device_table *devices_to_add = alloc_device_table();

	/* Process imported dives and generate lists of dives
	 * to-be-added and to-be-removed */
	process_imported_dives(import_table, import_trip_table, import_sites_table, import_device_table,
			       flags, &dives_to_add, &dives_to_remove, &trips_to_add,
			       &dive_sites_to_add, devices_to_add);

	/* Add new dives to trip and site to get reference count correct. */
	for (i = 0; i < dives_to_add.nr; i++) {
		struct dive *d = dives_to_add.dives[i];
		struct dive_trip *trip = d->divetrip;
		struct dive_site *site = d->dive_site;
		d->divetrip = NULL;
		d->dive_site = NULL;
		add_dive_to_trip(d, trip);
		add_dive_to_dive_site(d, site);
	}

	/* Remove old dives */
	for (i = 0; i < dives_to_remove.nr; i++) {
		idx = get_divenr(dives_to_remove.dives[i]);
		delete_single_dive(idx);
	}
	dives_to_remove.nr = 0;

	/* Add new dives */
	for (i = 0; i < dives_to_add.nr; i++)
		insert_dive(&dive_table, dives_to_add.dives[i]);
	dives_to_add.nr = 0;

	/* Add new trips */
	for (i = 0; i < trips_to_add.nr; i++)
		insert_trip(trips_to_add.trips[i], &trip_table);
	trips_to_add.nr = 0;

	/* Add new dive sites */
	for (i = 0; i < dive_sites_to_add.nr; i++)
		register_dive_site(dive_sites_to_add.dive_sites[i]);
	dive_sites_to_add.nr = 0;

	/* Add new devices */
	for (i = 0; i < nr_devices(devices_to_add); i++) {
		const struct device *dev = get_device(devices_to_add, i);
		add_to_device_table(&device_table, dev);
	}

	/* We might have deleted the old selected dive.
	 * Choose the newest dive as selected (if any) */
	current_dive = dive_table.nr > 0 ? dive_table.dives[dive_table.nr - 1] : NULL;

	free_device_table(devices_to_add);

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
bool try_to_merge_trip(struct dive_trip *trip_import, struct dive_table *import_table, bool prefer_imported,
		       /* output parameters: */
		       struct dive_table *dives_to_add, struct dive_table *dives_to_remove,
		       bool *sequence_changed, int *start_renumbering_at)
{
	int i;
	struct dive_trip *trip_old;

	for (i = 0; i < trip_table.nr; i++) {
		trip_old = trip_table.trips[i];
		if (trips_overlap(trip_import, trip_old)) {
			*sequence_changed |= merge_dive_tables(&trip_import->dives, import_table, &trip_old->dives,
							       prefer_imported, trip_old,
							       dives_to_add, dives_to_remove,
							       start_renumbering_at);
			free_trip(trip_import); /* All dives in trip have been consumed -> free */
			return true;
		}
	}

	return false;
}

/* Process imported dives: take a table of dives to be imported and
 * generate five lists:
 *	1) Dives to be added
 *	2) Dives to be removed
 *	3) Trips to be added
 *	4) Dive sites to be added
 *	5) Devices to be added
 * The dives to be added are owning (i.e. the caller is responsible
 * for freeing them).
 * The dives, trips and sites in "import_table", "import_trip_table"
 * and "import_sites_table" are consumed. On return, the tables have
 * size 0. "import_trip_table" may be NULL if all dives are not associated
 * with a trip.
 * The output tables should be empty - if not, their content
 * will be cleared!
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
void process_imported_dives(struct dive_table *import_table, struct trip_table *import_trip_table,
			    struct dive_site_table *import_sites_table, struct device_table *import_device_table,
			    int flags,
			    /* output parameters: */
			    struct dive_table *dives_to_add, struct dive_table *dives_to_remove,
			    struct trip_table *trips_to_add, struct dive_site_table *sites_to_add,
			    struct device_table *devices_to_add)
{
	int i, j, nr, start_renumbering_at = 0;
	struct dive_trip *trip_import, *new_trip;
	bool sequence_changed = false;
	bool new_dive_has_number = false;
	bool last_old_dive_is_numbered;

	/* If the caller didn't pass an import_trip_table because all
	 * dives are tripless, provide a local table. This may be
	 * necessary if the trips are autogrouped */
	struct trip_table local_trip_table = empty_trip_table;
	if (!import_trip_table)
		import_trip_table = &local_trip_table;

	/* Make sure that output parameters don't contain garbage */
	clear_dive_table(dives_to_add);
	clear_dive_table(dives_to_remove);
	clear_trip_table(trips_to_add);
	clear_dive_site_table(sites_to_add);
	clear_device_table(devices_to_add);

	/* Check if any of the new dives has a number. This will be
	 * important later to decide if we want to renumber the added
	 * dives */
	for (int i = 0; i < import_table->nr; i++) {
		if (import_table->dives[i]->number > 0) {
			new_dive_has_number = true;
			break;
		}
	}

	/* If no dives were imported, don't bother doing anything */
	if (!import_table->nr)
		return;

	/* Add only the devices that we don't know about yet. */
	for (i = 0; i < nr_devices(import_device_table); i++) {
		const struct device *dev = get_device(import_device_table, i);
		if (!device_exists(&device_table, dev))
			add_to_device_table(devices_to_add, dev);
	}

	/* check if we need a nickname for the divecomputer for newly downloaded dives;
	 * since we know they all came from the same divecomputer we just check for the
	 * first one */
	if (flags & IMPORT_IS_DOWNLOADED) {
		add_devices_of_dive(import_table->dives[0], devices_to_add);
	} else {
		/* they aren't downloaded, so record / check all new ones */
		for (i = 0; i < import_table->nr; i++)
			add_devices_of_dive(import_table->dives[i], devices_to_add);
	}

	/* Sort the table of dives to be imported and combine mergable dives */
	sort_dive_table(import_table);
	merge_imported_dives(import_table);

	/* Autogroup tripless dives if desired by user. But don't autogroup
	 * if tripless dives should be added to a new trip. */
	if (!(flags & IMPORT_ADD_TO_NEW_TRIP))
		autogroup_dives(import_table, import_trip_table);

	/* If dive sites already exist, use the existing versions. */
	for (i = 0; i  < import_sites_table->nr; i++) {
		struct dive_site *new_ds = import_sites_table->dive_sites[i];
		struct dive_site *old_ds = get_same_dive_site(new_ds);

		/* Check if it dive site is actually used by new dives. */
		for (j = 0; j < import_table->nr; j++) {
			if (import_table->dives[j]->dive_site == new_ds)
				break;
		}

		if (j == import_table->nr) {
			/* Dive site not even used - free it and go to next. */
			free_dive_site(new_ds);
			continue;
		}

		if (!old_ds) {
			/* Dive site doesn't exist. Add it to list of dive sites to be added. */
			new_ds->dives.nr = 0; /* Caller is responsible for adding dives to site */
			add_dive_site_to_table(new_ds, sites_to_add);
			continue;
		}
		/* Dive site already exists - use the old and free the new. */
		for (j = 0; j < import_table->nr; j++) {
			if (import_table->dives[j]->dive_site == new_ds)
				import_table->dives[j]->dive_site = old_ds;
		}
		free_dive_site(new_ds);
	}
	import_sites_table->nr = 0; /* All dive sites were consumed */

	/* Merge overlapping trips. Since both trip tables are sorted, we
	 * could be smarter here, but realistically not a whole lot of trips
	 * will be imported so do a simple n*m loop until someone complains.
	 */
	for (i = 0; i < import_trip_table->nr; i++) {
		trip_import = import_trip_table->trips[i];
		if ((flags & IMPORT_MERGE_ALL_TRIPS) || trip_import->autogen) {
			if (try_to_merge_trip(trip_import, import_table, flags & IMPORT_PREFER_IMPORTED, dives_to_add, dives_to_remove,
					      &sequence_changed, &start_renumbering_at))
				continue;
		}

		/* If no trip to merge-into was found, add trip as-is.
		 * First, add dives to list of dives to add */
		for (j = 0; j < trip_import->dives.nr; j++) {
			struct dive *d = trip_import->dives.dives[j];

			/* Add dive to list of dives to-be-added. */
			insert_dive(dives_to_add, d);
			sequence_changed |= !dive_is_after_last(d);

			remove_dive(d, import_table);
		}

		/* Then, add trip to list of trips to add */
		insert_trip(trip_import, trips_to_add);
		trip_import->dives.nr = 0; /* Caller is responsible for adding dives to trip */
	}
	import_trip_table->nr = 0; /* All trips were consumed */

	if ((flags & IMPORT_ADD_TO_NEW_TRIP) && import_table->nr > 0) {
		/* Create a new trip for unassigned dives, if desired. */
		new_trip = create_trip_from_dive(import_table->dives[0]);
		insert_trip(new_trip, trips_to_add);

		/* Add all remaining dives to this trip */
		for (i = 0; i < import_table->nr; i++) {
			struct dive *d = import_table->dives[i];
			d->divetrip = new_trip;
			insert_dive(dives_to_add, d);
			sequence_changed |= !dive_is_after_last(d);
		}

		import_table->nr = 0; /* All dives were consumed */
	} else if (import_table->nr > 0) {
		/* The remaining dives in import_table are those that don't belong to
		 * a trip and the caller does not want them to be associated to a
		 * new trip. Merge them into the global table. */
		sequence_changed |= merge_dive_tables(import_table, NULL, &dive_table, flags & IMPORT_PREFER_IMPORTED, NULL,
						      dives_to_add, dives_to_remove, &start_renumbering_at);
	}

	/* If new dives were only added at the end, renumber the added dives.
	 * But only if
	 *	- The last dive in the old dive table had a number itself (if there is a last dive).
	 *	- None of the new dives has a number.
	 */
	last_old_dive_is_numbered = dive_table.nr == 0 || dive_table.dives[dive_table.nr - 1]->number > 0;

	/* We counted the number of merged dives that were added to dives_to_add.
	 * Skip those. Since sequence_changed is false all added dives are *after*
	 * all merged dives. */
	if (!sequence_changed && last_old_dive_is_numbered && !new_dive_has_number) {
		nr = dive_table.nr > 0 ? dive_table.dives[dive_table.nr - 1]->number : 0;
		for (i = start_renumbering_at; i < dives_to_add->nr; i++)
			dives_to_add->dives[i]->number = ++nr;
	}
}

static struct dive *get_last_valid_dive()
{
	int i;
	for (i = dive_table.nr - 1; i >= 0; i--) {
		if (!dive_table.dives[i]->invalid)
			return dive_table.dives[i];
	}
	return NULL;
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
	if (idx < dive_table.nr)
		return 0;
	struct dive *last_dive = get_last_valid_dive();
	if (!last_dive)
		return 1;
	return last_dive->number ? last_dive->number + 1 : 0;
}

void set_dive_nr_for_current_dive()
{
	int selected_dive = get_divenr(current_dive);
	if (dive_table.nr == 1)
		current_dive->number = 1;
	else if (selected_dive == dive_table.nr - 1 && get_dive(dive_table.nr - 2)->number)
		current_dive->number = get_dive(dive_table.nr - 2)->number + 1;
}

static int min_datafile_version;

int get_min_datafile_version()
{
	return min_datafile_version;
}

void reset_min_datafile_version()
{
	min_datafile_version = 0;
}

void report_datafile_version(int version)
{
	if (min_datafile_version == 0 || min_datafile_version > version)
		min_datafile_version = version;
}

int get_dive_id_closest_to(timestamp_t when)
{
	int i;
	int nr = dive_table.nr;

	// deal with pathological cases
	if (nr == 0)
		return 0;
	else if (nr == 1)
		return dive_table.dives[0]->id;

	for (i = 0; i < nr && dive_table.dives[i]->when <= when; i++)
		; // nothing

	// again, capture the two edge cases first
	if (i == nr)
		return dive_table.dives[i - 1]->id;
	else if (i == 0)
		return dive_table.dives[0]->id;

	if (when - dive_table.dives[i - 1]->when < dive_table.dives[i]->when - when)
		return dive_table.dives[i - 1]->id;
	else
		return dive_table.dives[i]->id;
}

void clear_dive_file_data()
{
	fulltext_unregister_all();
	select_single_dive(NULL);	// This is propagate up to the UI and clears all the information.

	while (dive_table.nr)
		delete_single_dive(0);
	current_dive = NULL;
	while (dive_site_table.nr)
		delete_dive_site(get_dive_site(0, &dive_site_table), &dive_site_table);
	if (trip_table.nr != 0) {
		fprintf(stderr, "Warning: trip table not empty in clear_dive_file_data()!\n");
		trip_table.nr = 0;
	}

	clear_dive(&displayed_dive);
	clear_device_table(&device_table);
	clear_events();
	clear_filter_presets();

	reset_min_datafile_version();
	clear_git_id();

	reset_tank_info_table(&tank_info_table);

	/* Inform frontend of reset data. This should reset all the models. */
	emit_reset_signal();
}

bool dive_less_than(const struct dive *a, const struct dive *b)
{
	return comp_dives(a, b) < 0;
}

/* When comparing a dive to a trip, use the first dive of the trip. */
static int comp_dive_to_trip(struct dive *a, struct dive_trip *b)
{
	/* This should never happen, nevertheless don't crash on trips
	 * with no (or worse a negative number of) dives. */
	if (!b || b->dives.nr <= 0)
		return -1;
	return comp_dives(a, b->dives.dives[0]);
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
		return comp_dives(a.dive, b.dive);
	if (a.trip && b.trip)
		return comp_trips(a.trip, b.trip);
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
	int i;
	timestamp_t prev_end;

	/* find previous dive. might want to use a binary search. */
	for (i = dive_table.nr - 1; i >= 0; --i) {
		if (dive_table.dives[i]->when < when)
			break;
	}
	if (i < 0)
		return -1;

	prev_end = dive_endtime(dive_table.dives[i]);
	if (prev_end > when)
		return 0;
	return when - prev_end;
}

/* Find visible dive close to given date. First search towards older,
 * then newer dives. */
struct dive *find_next_visible_dive(timestamp_t when)
{
	int i, j;

	if (!dive_table.nr)
		return NULL;

	/* we might want to use binary search here */
	for (i = 0; i < dive_table.nr; i++) {
		if (when <= get_dive(i)->when)
			break;
	}

	for (j = i - 1; j > 0; j--) {
		if (!get_dive(j)->hidden_by_filter)
			return get_dive(j);
	}

	for (j = i; j < dive_table.nr; j++) {
		if (!get_dive(j)->hidden_by_filter)
			return get_dive(j);
	}

	return NULL;
}
