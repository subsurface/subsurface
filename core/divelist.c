// SPDX-License-Identifier: GPL-2.0
/* divelist.c */
/* core logic for the dive list -
 * accessed through the following interfaces:
 *
 * void process_loaded_dives();
 * void process_imported_dives(bool prefer_imported);
 * dive_trip_t *dive_trip_list;
 * unsigned int amount_selected;
 * void dump_selection(void)
 * void get_dive_gas(const struct dive *dive, int *o2_p, int *he_p, int *o2low_p)
 * char *get_dive_gas_string(const struct dive *dive)
 * int total_weight(const struct dive *dive)
 * int get_divenr(const struct dive *dive)
 * int get_divesite_idx(const struct dive_site *ds)
 * int init_decompression(struct dive *dive)
 * void update_cylinder_related_info(struct dive *dive)
 * void dump_trip_list(void)
 * void insert_trip(dive_trip_t *dive_trip_p)
 * void unregister_trip(dive_trip_t *trip)
 * void free_trip(dive_trip_t *trip)
 * void remove_dive_from_trip(struct dive *dive, bool was_autogen)
 * void add_dive_to_trip(struct dive *dive, dive_trip_t *trip)
 * dive_trip_t *create_and_hookup_trip_from_dive(struct dive *dive)
 * dive_trip_t *get_dives_to_autogroup(int start, int *from, int *to, bool *allocated)
 * dive_trip_t *get_trip_for_new_dive(struct dive *new_dive, bool *allocated)
 * void autogroup_dives(void)
 * void combine_trips(struct dive_trip *trip_a, struct dive_trip *trip_b)
 * dive_trip_t *combine_trips_create(struct dive_trip *trip_a, struct dive_trip *trip_b)
 * struct dive *unregister_dive(int idx)
 * void delete_single_dive(int idx)
 * void add_dive_to_table(struct dive_table *table, int idx, struct dive *dive)
 * void add_single_dive(int idx, struct dive *dive)
 * struct dive *merge_two_dives(struct dive *a, struct dive *b)
 * void select_dive(struct dive *dive)
 * void deselect_dive(struct dive *dive)
 * void mark_divelist_changed(int changed)
 * int unsaved_changes()
 * void remove_autogen_trips()
 * bool dive_less_than(const struct dive *a, const struct dive *b)
 * bool trip_less_than(const struct dive_trip *a, const struct dive_trip *b)
 * bool dive_or_trip_less_than(struct dive_or_trip a, struct dive_or_trip b)
 * void sort_table(struct dive_table *table)
 * bool is_trip_before_after(const struct dive *dive, bool before)
 * void delete_dive_from_table(struct dive_table *table, int idx)
 * int find_next_visible_dive(timestamp_t when);
 * void clear_dive_file_data()
 * void clear_table(struct dive_table *table)
 * bool trip_is_single_day(const struct dive_trip *trip)
 * int trip_shown_dives(const struct dive_trip *trip)
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "gettext.h"
#include <assert.h>
#include <zip.h>
#include <libxslt/transform.h>

#include "dive.h"
#include "subsurface-string.h"
#include "divelist.h"
#include "display.h"
#include "planner.h"
#include "qthelper.h"
#include "git-access.h"

static bool dive_list_changed = false;

bool autogroup = false;

dive_trip_t *dive_trip_list;

unsigned int amount_selected;

// We need to stop using globals, really.
struct dive_table downloadTable;

#if DEBUG_SELECTION_TRACKING
void dump_selection(void)
{
	int i;
	struct dive *dive;

	printf("currently selected are %u dives:", amount_selected);
	for_each_dive(i, dive) {
		if (dive->selected)
			printf(" %d", i);
	}
	printf("\n");
}
#endif

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


	for (i = 0; i < MAX_CYLINDERS; i++) {
		const cylinder_t *cyl = dive->cylinder + i;
		int o2 = get_o2(cyl->gasmix);
		int he = get_he(cyl->gasmix);

		if (!is_cylinder_used(dive, i))
			continue;
		if (cylinder_none(cyl))
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
		for (i = 0; i < MAX_WEIGHTSYSTEMS; i++)
			total_grams += dive->weightsystem[i].weight.grams;
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
		if (sample->o2sensor[0].mbar) {			// if dive computer has o2 sensor(s) (CCR & PSCR) ..
			po2i = psample->o2sensor[0].mbar;
			po2f = sample->o2sensor[0].mbar;	// ... use data from the first o2 sensor
		} else {
			if (dc->divemode == CCR) {
				po2i = psample->setpoint.mbar;		// if CCR has no o2 sensors then use setpoint
				po2f = sample->setpoint.mbar;
			} else {						// For OC and rebreather without o2 sensor/setpoint
				int o2 = active_o2(dive, dc, psample->time);	// 	... calculate po2 from depth and FiO2.
				po2i = lrint(o2 * depth_to_atm(psample->depth.mm, dive));	// (initial) po2 at start of segment
				po2f = lrint(o2 * depth_to_atm(sample->depth.mm, dive));	// (final) po2 at end of segment
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
/* Table of maximum oxygen exposure durations, used in CNS calulations.
   This table shows the official NOAA maximum O2 exposure limits (in seconds) for different PO2 values. It also gives
   slope values for linear interpolation for intermediate PO2 values between the tabulated PO2 values in the 1st column.
   Top & bottom rows are inserted that are not in the NOAA table: (1) For PO2 > 1.6 the same slope value as between
   1.5 & 1.6 is used. This exptrapolation for PO2 > 1.6 likely gives an underestimate above 1.6 but is better than the
   value for PO2=1.6 (45 min). (2) The NOAA table only tabulates values for PO2 >= 0.6. Since O2-uptake occurs down to
   PO2=0.5, the same slope is used as for 0.7 > PO2 > 0.6. This gives a conservative estimate for 0.6 > PO2 > 0.5. To
   preserve the integer structure of the table, all slopes are given as slope*10: divide by 10 to get the valid slope.
   The columns below are: 
   po2 (mbar), Maximum Single Exposure (seconds), single_slope, Maximum 24 hour Exposure (seconds), 24h_slope */
int const cns_table[][5] = {
	{ 1600,  45 * 60, 456, 150 * 60, 180 },
	{ 1550,  83 * 60, 456, 165 * 60, 180 },
	{ 1500, 120 * 60, 444, 180 * 60, 180 },
	{ 1450, 135 * 60, 180, 180 * 60,  00 },
	{ 1400, 150 * 60, 180, 180 * 60,  00 },
	{ 1350, 165 * 60, 180, 195 * 60, 180 },
	{ 1300, 180 * 60, 180, 210 * 60, 180 },
	{ 1250, 195 * 60, 180, 225 * 60, 180 },
	{ 1200, 210 * 60, 180, 240 * 60, 180 },
	{ 1100, 240 * 60, 180, 270 * 60, 180 },
	{ 1000, 300 * 60, 360, 300 * 60, 180 },
	{ 900,  360 * 60, 360, 360 * 60, 360 },
	{ 800,  450 * 60, 540, 450 * 60, 540 },
	{ 700,  570 * 60, 720, 570 * 60, 720 },
	{ 600,  720 * 60, 900, 720 * 60, 900 },
	{ 500,  870 * 60, 900, 870 * 60, 900 }
};

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
	size_t j;
	const struct divecomputer *dc = &dive->dc;
	double cns = 0.0;
	/* Calculate the CNS for each sample in this dive and sum them */
	for (n = 1; n < dc->samples; n++) {
		int t;
		int po2i, po2f;
		bool trueo2 = false;
		struct sample *sample = dc->sample + n;
		struct sample *psample = sample - 1;
		t = sample->time.seconds - psample->time.seconds;
		if (sample->o2sensor[0].mbar) {			// if dive computer has o2 sensor(s) (CCR & PSCR)
			po2i = psample->o2sensor[0].mbar;
			po2f = sample->o2sensor[0].mbar;	// then use data from the first o2 sensor
			trueo2 = true;
		}
		if ((dc->divemode == CCR) && (!trueo2)) {
			po2i = psample->setpoint.mbar;		// if CCR has no o2 sensors then use setpoint
			po2f = sample->setpoint.mbar;
			trueo2 = true;
		}
		if (!trueo2) {
			int o2 = active_o2(dive, dc, psample->time);			// For OC and rebreather without o2 sensor:
			po2i = lrint(o2 * depth_to_atm(psample->depth.mm, dive));	// (initial) po2 at start of segment
			po2f = lrint(o2 * depth_to_atm(sample->depth.mm, dive));	// (final) po2 at end of segment
		}
		po2i = (po2i + po2f) / 2;	// po2i now holds the mean po2 of initial and final po2 values of segment.
		/* Don't increase CNS when po2 below 500 matm */
		if (po2i <= 500)
			continue;
		/* Find the table-row for calculating the maximum exposure at this PO2 */
		for (j = 1; j < sizeof(cns_table) / (sizeof(int) * NO_COLUMNS); j++)
			if (po2i > cns_table[j][PO2VAL])
				break;
		/* Increment CNS with simple linear interpolation: 100 * time / (single-exposure-time + delta-PO2 * single-slope) */
		cns += (double)t / ((double)cns_table[j][SINGLE_EXP] - ((double)po2i - (double)cns_table[j][PO2VAL]) * (double)cns_table[j][SINGLE_SLOPE] / 10.0) * 100;
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

	for (i = 0; i < MAX_CYLINDERS; i++) {
		pressure_t start, end;
		const cylinder_t *cyl = dive->cylinder + i;

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
static void add_dive_to_deco(struct deco_state *ds, struct dive *dive)
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
				get_current_divemode(&dive->dc, j, &evd, &current_divemode), dive->sac);
		}
	}
}

static int get_idx_in_table(const struct dive_table *table, const struct dive *dive)
{
	for (int i = 0; i < table->nr; ++i) {
		if (table->dives[i] == dive)
			return i;
	}
	return -1;
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

int get_divesite_idx(const struct dive_site *ds)
{
	int i;
	const struct dive_site *d;
	// tempting as it may be, don't die when called with dive=NULL
	if (ds)
		for_each_dive_site(i, d) {
			if (d->uuid == ds->uuid) // don't compare pointers, we could be passing in a copy of the dive
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
int init_decompression(struct deco_state *ds, struct dive *dive)
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
			clear_deco(ds, surface_pressure);
			deco_init = true;
#if DECO_CALC_DEBUG & 2
			printf("Tissues after init:\n");
			dump_tissues(ds);
#endif
		}
		else {
			surface_time = pdive->when - last_endtime;
			if (surface_time < 0) {
#if DECO_CALC_DEBUG & 2
				printf("Exit because surface intervall is %d\n", surface_time);
#endif
				return surface_time;
			}
			add_segment(ds, surface_pressure, air, surface_time, 0, dive->dc.divemode, prefs.decosac);
#if DECO_CALC_DEBUG & 2
			printf("Tissues after surface intervall of %d:%02u:\n", FRACTION(surface_time, 60));
			dump_tissues(ds);
#endif
		}

		add_dive_to_deco(ds, pdive);

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
		clear_deco(ds, surface_pressure);
#if DECO_CALC_DEBUG & 2
		printf("Tissues after no previous dive, surface time set to 48h:\n");
		dump_tissues(ds);
#endif
	}
	else {
		surface_time = dive->when - last_endtime;
		if (surface_time < 0) {
#if DECO_CALC_DEBUG & 2
			printf("Exit because surface intervall is %d\n", surface_time);
#endif
			return surface_time;
		}
		add_segment(ds, surface_pressure, air, surface_time, 0, dive->dc.divemode, prefs.decosac);
#if DECO_CALC_DEBUG & 2
		printf("Tissues after surface intervall of %d:%02u:\n", FRACTION(surface_time, 60));
		dump_tissues(ds);
#endif
	}

	// I do not dare to remove this call. We don't need the result but it might have side effects. Bummer.
	tissue_tolerance_calc(ds, dive, surface_pressure);
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
#define UTF8_ELLIPSIS "\xE2\x80\xA6"

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
				snprintf(buffer, MAX_GAS_STRING, "%d/%d" UTF8_ELLIPSIS "%d%%", o2, he, o2max);
		else if (o2)
			if (o2 == o2max)
				snprintf(buffer, MAX_GAS_STRING, "%d%%", o2);
			else
				snprintf(buffer, MAX_GAS_STRING, "%d" UTF8_ELLIPSIS "%d%%", o2, o2max);
		else
			strcpy(buffer, translate("gettextFromC", "air"));
	}
	return buffer;
}

/*
 * helper functions for dive_trip handling
 */
#ifdef DEBUG_TRIP
void dump_trip_list(void)
{
	dive_trip_t *trip;
	int i = 0;
	timestamp_t last_time = 0;

	for (trip = dive_trip_list; trip; trip = trip->next) {
		struct tm tm;
		utc_mkdate(trip_date(trip), &tm);
		if (trip->when < last_time)
			printf("\n\ndive_trip_list OUT OF ORDER!!!\n\n\n");
		printf("%s trip %d to \"%s\" on %04u-%02u-%02u %02u:%02u:%02u (%d dives - %p)\n",
		       trip->autogen ? "autogen " : "",
		       ++i, trip->location,
		       tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		       trip->dives.nr, trip);
		last_time = trip->when;
	}
	printf("-----\n");
}
#endif

/* insert the trip into the dive_trip_list */
void insert_trip(dive_trip_t *dive_trip)
{
	dive_trip_t **p = &dive_trip_list;
	dive_trip_t *trip;

	/* Walk the dive trip list looking for the right location.. */
	while ((trip = *p) != NULL && trip_less_than(trip, dive_trip))
		p = &trip->next;

	dive_trip->next = trip;
	*p = dive_trip;
#ifdef DEBUG_TRIP
	dump_trip_list();
#endif
}

/* free resources associated with a trip structure */
void free_trip(dive_trip_t *trip)
{
	if (trip) {
		free(trip->location);
		free(trip->notes);
		free(trip);
	}
}

/* remove trip from the trip-list, but don't free its memory.
 * caller takes ownership of the trip. */
void unregister_trip(dive_trip_t *trip)
{
	dive_trip_t **p, *tmp;

	assert(!trip->dives.nr);

	/* Remove the trip from the list of trips */
	p = &dive_trip_list;
	while ((tmp = *p) != NULL) {
		if (tmp == trip) {
			*p = trip->next;
			break;
		}
		p = &tmp->next;
	}
}

static void delete_trip(dive_trip_t *trip)
{
	unregister_trip(trip);
	free_trip(trip);
}


timestamp_t trip_date(const struct dive_trip *trip)
{
	if (!trip || trip->dives.nr == 0)
		return 0;
	return trip->dives.dives[0]->when;
}

/* check if we have a trip right before / after this dive */
bool is_trip_before_after(const struct dive *dive, bool before)
{
	int idx = get_idx_by_uniq_id(dive->id);
	if (before) {
		if (idx > 0 && get_dive(idx - 1)->divetrip)
			return true;
	} else {
		if (idx < dive_table.nr - 1 && get_dive(idx + 1)->divetrip)
			return true;
	}
	return false;
}

struct dive *first_selected_dive()
{
	int idx;
	struct dive *d;

	for_each_dive (idx, d) {
		if (d->selected)
			return d;
	}
	return NULL;
}

struct dive *last_selected_dive()
{
	int idx;
	struct dive *d, *ret = NULL;

	for_each_dive (idx, d) {
		if (d->selected)
			ret = d;
	}
	return ret;
}

static void unregister_dive_from_table(struct dive_table *table, int idx)
{
	int i;
	for (i = idx; i < table->nr - 1; i++)
		table->dives[i] = table->dives[i + 1];
	table->dives[--table->nr] = NULL;
}

/* remove a dive from the trip it's associated to, but don't delete the
 * trip if this was the last dive in the trip. the caller is responsible
 * for removing the trip, if the trip->dives.nr went to 0.
 */
struct dive_trip *unregister_dive_from_trip(struct dive *dive, short was_autogen)
{
	dive_trip_t *trip = dive->divetrip;
	int idx;

	if (!trip)
		return NULL;

	idx = get_idx_in_table(&trip->dives, dive);
	if (idx >= 0)
		unregister_dive_from_table(&trip->dives, idx);
	dive->divetrip = NULL;
	return trip;
}

void remove_dive_from_trip(struct dive *dive, short was_autogen)
{
	struct dive_trip *trip = unregister_dive_from_trip(dive, was_autogen);
	if (trip && trip->dives.nr == 0)
		delete_trip(trip);
}

/* Add dive to a trip. Caller is responsible for removing dive
 * from trip beforehand. */
void add_dive_to_trip(struct dive *dive, dive_trip_t *trip)
{
	if (dive->divetrip == trip)
		return;
	if (dive->divetrip)
		fprintf(stderr, "Warning: adding dive to trip that has trip set\n");
	add_dive_to_table(&trip->dives, -1, dive);
	dive->divetrip = trip;
}

dive_trip_t *alloc_trip(void)
{
	return calloc(1, sizeof(dive_trip_t));
}

dive_trip_t *create_trip_from_dive(struct dive *dive)
{
	dive_trip_t *trip;

	trip = alloc_trip();
	trip->location = copy_string(get_dive_location(dive));

	return trip;
}

dive_trip_t *create_and_hookup_trip_from_dive(struct dive *dive)
{
	dive_trip_t *dive_trip = alloc_trip();

	dive_trip = create_trip_from_dive(dive);
	insert_trip(dive_trip);

	add_dive_to_trip(dive, dive_trip);
	return dive_trip;
}

/*
 * Find a trip a new dive should be autogrouped with. If no such trips
 * exist, allocate a new trip. The bool "*allocated" is set to true
 * if a new trip was allocated.
 */
dive_trip_t *get_trip_for_new_dive(struct dive *new_dive, bool *allocated)
{
	struct dive *d;
	dive_trip_t *trip;
	int i;

	/* Find dive that is within TRIP_THRESHOLD of current dive */
	for_each_dive(i, d) {
		/* Check if we're past the range of possible dives */
		if (d->when >= new_dive->when + TRIP_THRESHOLD)
			break;

		if (d->when + TRIP_THRESHOLD >= new_dive->when && d->divetrip) {
			/* Found a dive with trip in the range */
			*allocated = false;
			return d->divetrip;
		}
	}

	/* Didn't find a trip -> allocate a new one */
	trip = create_trip_from_dive(new_dive);
	trip->autogen = true;
	*allocated = true;
	return trip;
}

/*
 * Collect dives for auto-grouping. Pass in first dive which should be checked.
 * Returns range of dives that should be autogrouped and trip it should be
 * associated to. If the returned trip was newly allocated, the last bool
 * is set to true. Caller still has to register it in the system. Note
 * whereas this looks complicated - it is needed by the undo-system, which
 * manually injects the new trips. If there are no dives to be autogrouped,
 * return NULL.
 */
dive_trip_t *get_dives_to_autogroup(int start, int *from, int *to, bool *allocated)
{
	int i;
	struct dive *dive, *lastdive = NULL;

	/* Find first dive that should be merged and remember any previous
	 * dive that could be merged into.
	 */
	for (i = start; (dive = get_dive(i)) != NULL; i++) {
		dive_trip_t *trip;

		if (dive->divetrip) {
			lastdive = dive;
			continue;
		}

		/* Only consider dives that have not been explicitly removed from
		 * a dive trip by the user.  */
		if (dive->notrip) {
			lastdive = NULL;
			continue;
		}

		/* We found a dive, let's see if we have to allocate a new trip */
		if (!lastdive || dive->when >= lastdive->when + TRIP_THRESHOLD) {
			/* allocate new trip */
			trip = create_trip_from_dive(dive);
			trip->autogen = true;
			*allocated = true;
		} else {
			/* use trip of previous dive */
			trip = lastdive->divetrip;
			*allocated = false;
		}

		// Now, find all dives that will be added to this trip
		lastdive = dive;
		*from = i;
		for (*to = *from + 1; (dive = get_dive(*to)) != NULL; (*to)++) {
			if (dive->divetrip || dive->notrip ||
			    dive->when >= lastdive->when + TRIP_THRESHOLD)
				break;
			if (get_dive_location(dive) && !trip->location)
				trip->location = copy_string(get_dive_location(dive));
			lastdive = dive;
		}
		return trip;
	}

	/* Did not find anyhting - mark as end */
	return NULL;
}

/*
 * Walk the dives from the oldest dive, and see if we can autogroup them.
 * But only do this when the user selected autogrouping.
 */
void autogroup_dives(void)
{
	int from, to;
	dive_trip_t *trip;
	int i, j;
	bool alloc;

	if (!autogroup)
		return;

	for (i = 0; (trip = get_dives_to_autogroup(i, &from, &to, &alloc)) != NULL; i = to) {
		/* If this was newly allocated, add trip to list */
		if (alloc)
			insert_trip(trip);
		for (j = from; j < to; ++j)
			add_dive_to_trip(get_dive(j), trip);
	}
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
	unregister_dive_from_table(table, idx);
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
	unregister_dive_from_table(&dive_table, idx);
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
	remove_dive_from_trip(dive, false);
	delete_dive_from_table(&dive_table, idx);
}

struct dive **grow_dive_table(struct dive_table *table)
{
	int nr = table->nr, allocated = table->allocated;
	struct dive **dives = table->dives;

	if (nr >= allocated) {
		allocated = (nr + 32) * 3 / 2;
		dives = realloc(dives, allocated * sizeof(struct dive *));
		if (!dives)
			exit(1);
		table->dives = dives;
		table->allocated = allocated;
	}
	return dives;
}

/* get the index where we want to insert the dive so that everything stays
 * ordered according to dive_less_than() */
int dive_get_insertion_index(struct dive_table *table, struct dive *dive)
{
	/* we might want to use binary search here */
	for (int i = 0; i < table->nr; i++) {
		if (dive_less_than(dive, table->dives[i]))
			return i;
	}
	return table->nr;
}

/* add a dive at the given index to a dive table. if the index is negative,
 * the dive will be added according to dive_less_than() order */
void add_dive_to_table(struct dive_table *table, int idx, struct dive *dive)
{
	int i;
	if (idx < 0)
		idx = dive_get_insertion_index(table, dive);
	grow_dive_table(table);
	table->nr++;

	for (i = idx; i < table->nr; i++) {
		struct dive *tmp = table->dives[i];
		table->dives[i] = dive;
		dive = tmp;
	}
}

/* add a dive at the given index in the global dive table and keep track
 * of the number of selected dives. if the index is negative, the dive will
 * be added according to dive_less_than() order */
void add_single_dive(int idx, struct dive *dive)
{
	add_dive_to_table(&dive_table, idx, dive);
	if (dive->selected)
		amount_selected++;
}

bool consecutive_selected()
{
	struct dive *d;
	int i;
	bool consecutive = true;
	bool firstfound = false;
	bool lastfound = false;

	if (amount_selected == 0 || amount_selected == 1)
		return true;

	for_each_dive(i, d) {
		if (d->selected) {
			if (!firstfound)
				firstfound = true;
			else if (lastfound)
				consecutive = false;
		} else if (firstfound) {
			lastfound = true;
		}
	}
	return consecutive;
}

/*
 * Merge two dives. 'a' is always before 'b' in the dive list
 * (and thus in time).
 */
struct dive *merge_two_dives(struct dive *a, struct dive *b)
{
	struct dive *res;
	int i, j, nr, nrdiff;
	int id;

	if (!a || !b)
		return NULL;

	id = a->id;
	i = get_divenr(a);
	j = get_divenr(b);
	if (i < 0 || j < 0)
		// something is wrong with those dives. Bail
		return NULL;
	res = merge_dives(a, b, b->when - a->when, false, NULL);
	if (!res)
		return NULL;

	/*
	 * If 'a' and 'b' were numbered, and in proper order,
	 * then the resulting dive will get the first number,
	 * and the subsequent dives will be renumbered by the
	 * difference.
	 *
	 * So if you had a dive list  1 3 6 7 8, and you
	 * merge 1 and 3, the resulting numbered list will
	 * be 1 4 5 6, because we assume that there were
	 * some missing dives (originally dives 4 and 5),
	 * that now will still be missing (dives 2 and 3
	 * in the renumbered world).
	 *
	 * Obviously the normal case is that everything is
	 * consecutive, and the difference will be 1, so the
	 * above example is not supposed to be normal.
	 */
	nrdiff = 0;
	nr = a->number;
	if (a->number && b->number > a->number) {
		res->number = nr;
		nrdiff = b->number - nr;
	}

	add_single_dive(i, res);
	delete_single_dive(i + 1);
	delete_single_dive(j);
	// now make sure that we keep the id of the first dive.
	// why?
	// because this way one of the previously selected ids is still around
	res->id = id;

	// renumber dives from merged one in advance by difference between
	// merged dives numbers. Do not renumber if actual number is zero.
	for (; j < dive_table.nr; j++) {
		struct dive *dive = dive_table.dives[j];
		int newnr;

		if (!dive->number)
			continue;
		newnr = dive->number - nrdiff;

		/*
		 * Don't renumber stuff that isn't in order!
		 *
		 * So if the new dive number isn't larger than the
		 * previous dive number, just stop here.
		 */
		if (newnr <= nr)
			break;
		dive->number = newnr;
		nr = newnr;
	}

	mark_divelist_changed(true);
	return res;
}

void select_dive(struct dive *dive)
{
	if (!dive)
		return;
	if (!dive->selected) {
		dive->selected = 1;
		amount_selected++;
	}
	current_dive = dive;
}

void deselect_dive(struct dive *dive)
{
	int idx;
	if (dive && dive->selected) {
		dive->selected = 0;
		if (amount_selected)
			amount_selected--;
		if (current_dive == dive && amount_selected > 0) {
			/* pick a different dive as selected */
			int selected_dive = idx = get_divenr(dive);
			while (--selected_dive >= 0) {
				dive = get_dive(selected_dive);
				if (dive && dive->selected) {
					current_dive = dive;
					return;
				}
			}
			selected_dive = idx;
			while (++selected_dive < dive_table.nr) {
				dive = get_dive(selected_dive);
				if (dive && dive->selected) {
					current_dive = dive;
					return;
				}
			}
		}
		current_dive = NULL;
	}
}

void deselect_dives_in_trip(struct dive_trip *trip)
{
	if (!trip)
		return;
	for (int i = 0; i < trip->dives.nr; ++i)
		deselect_dive(trip->dives.dives[i]);
}

void select_dives_in_trip(struct dive_trip *trip)
{
	struct dive *dive;
	if (!trip)
		return;
	for (int i = 0; i < trip->dives.nr; ++i) {
		dive = trip->dives.dives[i];
		if (!dive->hidden_by_filter)
			select_dive(dive);
	}
}

void filter_dive(struct dive *d, bool shown)
{
	if (!d)
		return;
	d->hidden_by_filter = !shown;
	if (!shown && d->selected)
		deselect_dive(d);
}


/* This only gets called with non-NULL trips.
 * It does not combine notes or location, just picks the first one
 * (or the second one if the first one is empty */
void combine_trips(struct dive_trip *trip_a, struct dive_trip *trip_b)
{
	if (empty_string(trip_a->location) && trip_b->location) {
		free(trip_a->location);
		trip_a->location = strdup(trip_b->location);
	}
	if (empty_string(trip_a->notes) && trip_b->notes) {
		free(trip_a->notes);
		trip_a->notes = strdup(trip_b->notes);
	}
	/* this also removes the dives from trip_b and eventually
	 * calls delete_trip(trip_b) when the last dive has been moved */
	while (trip_b->dives.nr > 0)
		add_dive_to_trip(trip_b->dives.dives[0], trip_a);
}

/* Out of two strings, copy the string that is not empty (if any). */
static char *copy_non_empty_string(const char *a, const char *b)
{
	return copy_string(empty_string(b) ? a : b);
}

/* Combine trips new. This combines two trips, generating a
 * new trip. To support undo, we have to preserve the old trips. */
dive_trip_t *combine_trips_create(struct dive_trip *trip_a, struct dive_trip *trip_b)
{
	dive_trip_t *trip;

	trip = alloc_trip();
	trip->location = copy_non_empty_string(trip_a->location, trip_b->location);
	trip->notes = copy_non_empty_string(trip_a->notes, trip_b->notes);

	return trip;
}

void mark_divelist_changed(bool changed)
{
	if (dive_list_changed == changed)
		return;
	dive_list_changed = changed;
	updateWindowTitle();
}

int unsaved_changes()
{
	return dive_list_changed;
}

void remove_autogen_trips()
{
	int i;
	struct dive *dive;

	for_each_dive(i, dive) {
		dive_trip_t *trip = dive->divetrip;

		if (trip && trip->autogen)
			remove_dive_from_trip(dive, true);
	}
}

/*
 * When adding dives to the dive table, we try to renumber
 * the new dives based on any old dives in the dive table.
 *
 * But we only do it if:
 *
 *  - there are no dives in the dive table
 *
 *  OR
 *
 *  - the last dive in the old dive table was numbered
 *
 *  - all the new dives are strictly at the end (so the
 *    "last dive" is at the same location in the dive table
 *    after re-sorting the dives.
 *
 *  - none of the new dives have any numbers
 *
 * This catches the common case of importing new dives from
 * a dive computer, and gives them proper numbers based on
 * your old dive list. But it tries to be very conservative
 * and not give numbers if there is *any* question about
 * what the numbers should be - in which case you need to do
 * a manual re-numbering.
 */
static void try_to_renumber(int preexisting)
{
	int i, nr;
	struct dive *last = get_dive(preexisting - 1);

	/*
	 * If there was a last dive, but it didn't have
	 * a number, give up.
	 */
	if (last && !last->number)
		return;

	/*
	 * If any of the new dives already had a number,
	 * we'll have to do a manual renumbering.
	 */
	for (i = preexisting; i < dive_table.nr; i++) {
		struct dive *dive = get_dive(i);
		if (dive->number)
			return;
	}

	/*
	 * Ok, renumber..
	 */
	if (last)
		nr = last->number;
	else
		nr = 0;
	for (i = preexisting; i < dive_table.nr; i++) {
		struct dive *dive = get_dive(i);
		dive->number = ++nr;
	}
}

void process_loaded_dives()
{
	int i;
	struct dive *dive;

	/* Register dive computer nick names */
	for_each_dive(i, dive)
		set_dc_nickname(dive);

	sort_table(&dive_table);
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

		/* only try to merge overlapping dives - or if one of the dives has
		 * zero duration (that might be a gps marker from the webservice) */
		if (prev->duration.seconds && dive->duration.seconds &&
		    dive_endtime(prev) < dive->when)
			continue;

		merged = try_to_merge(prev, dive, false);
		if (!merged)
			continue;

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
 * true on success. On success, the dive to add and the old dive
 * will be deleted. On failure, they are untouched.
 * If "prefer_imported" is true, use data of the new dive.
 */
static bool try_to_merge_into(struct dive *dive_to_add, int idx, bool prefer_imported)
{
	struct dive *old_dive = dive_table.dives[idx];
	struct dive_trip *trip = old_dive->divetrip;
	struct dive *merged = try_to_merge(old_dive, dive_to_add, prefer_imported);
	if (!merged)
		return false;

	merged->id = old_dive->id;
	merged->selected = old_dive->selected;
	dive_table.dives[idx] = merged;
	if (trip)
		remove_dive_from_trip(old_dive, false);
	free_dive(old_dive);
	remove_dive_from_trip(dive_to_add, false);
	free_dive(dive_to_add);

	return true;
}

/*
 * Add imported dive to global dive table. Overlapping dives will
 * be merged if possible. If prefer_imported is true, data of the
 * new dives are prioritized in such a case.
 * If downloaded is true, only the divecomputer of the first dive
 * will be considered, as it is assumed that all dives come from
 * the same computer.
 * Note: the dives in import_table are consumed! On return import_table
 * has size 0.
 */
void process_imported_dives(struct dive_table *import_table, bool prefer_imported, bool downloaded)
{
	int i, j;
	int preexisting;
	bool sequence_changed = false;

	/* If no dives were imported, don't bother doing anything */
	if (!import_table->nr)
		return;

	/* check if we need a nickname for the divecomputer for newly downloaded dives;
	 * since we know they all came from the same divecomputer we just check for the
	 * first one */
	if (downloaded)
		set_dc_nickname(import_table->dives[0]);
	else
		/* they aren't downloaded, so record / check all new ones */
		for (i = 0; i < import_table->nr; i++)
			set_dc_nickname(import_table->dives[i]);

	/* Sort the table of dives to be imported and combine mergable dives */
	sort_table(import_table);
	merge_imported_dives(import_table);

	/* Merge newly imported dives into the dive table.
	 * Since both lists (old and new) are sorted, we can step
	 * through them concurrently and locate the insertions points.
	 * Once found, check if the new dive can be merged in the
	 * previous or next dive.
	 * Note that this doesn't consider pathological cases such as:
	 *  - New dive "connects" two old dives (turn three into one).
	 *  - New dive can not be merged into adjacent but some further dive.
	 */
	j = 0; /* Index in old dives */
	preexisting = dive_table.nr; /* Remember old size for renumbering */
	for (i = 0; i < import_table->nr; i++) {
		struct dive *dive_to_add = import_table->dives[i];

		/* Find insertion point. */
		while (j < dive_table.nr && dive_less_than(dive_table.dives[j], dive_to_add))
			j++;

		/* Try to merge into previous dive. */
		if (j > 0 && dive_endtime(dive_table.dives[j - 1]) > dive_to_add->when) {
			if (try_to_merge_into(dive_to_add, j - 1, prefer_imported))
				continue;
		}

		/* That didn't merge into the previous dive. If we're
		 * at the end of the dive table, quit the loop and add
		 * all new dives at the end. */
		if (j >= dive_table.nr)
			break;

		/* Try to merge into next dive. */
		if (dive_endtime(dive_to_add) > dive_table.dives[j]->when) {
			if (try_to_merge_into(dive_to_add, j, prefer_imported))
				continue;
		}

		/* We couldnt merge dives, add at the given position. */
		add_single_dive(j, dive_to_add);
		j++;
		sequence_changed = true;
	}

	/* If there are still dives to add, add them at the end of the dive table. */
	for ( ; i <  import_table->nr; i++)
		add_single_dive(dive_table.nr, import_table->dives[i]);

	/* we took care of all dives, clean up the import table */
	import_table->nr = 0;

	/* If the sequence wasn't changed, renumber */
	if (!sequence_changed)
		try_to_renumber(preexisting);

	/* We might have deleted the old selected dive.
	 * Choose the newest dive as selected (if any) */
	current_dive = dive_table.nr > 0 ? dive_table.dives[dive_table.nr - 1] : NULL;
	mark_divelist_changed(true);
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
	if (dive_table.nr == 0)
		return 1;
	if (idx >= dive_table.nr) {
		struct dive *last_dive = get_dive(dive_table.nr - 1);
		return last_dive->number ? last_dive->number + 1 : 0;
	}
	return 0;
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
	while (dive_table.nr)
		delete_single_dive(0);
	while (dive_site_table.nr)
		delete_dive_site(get_dive_site(0));

	clear_dive(&displayed_dive);

	reset_min_datafile_version();
	saved_git_id = "";
}

/*
 * Clear a dive_table
 */
void clear_table(struct dive_table *table)
{
	for (int i = 0; i < table->nr; i++)
		free_dive(table->dives[i]);
	table->nr = 0;
}

/* This function defines the sort ordering of dives. The core
 * and the UI models should use the same sort function, which
 * should be stable. This is not crucial at the moment, as the
 * indices in core and UI are independent, but ultimately we
 * probably want to unify the models.
 * After editing a key used in this sort-function, the order of
 * the dives must be re-astablished.
 * Currently, this does a lexicographic sort on the
 * (start-time, trip-time, id) tuple.
 * trip-time is defined such that dives that do not belong to
 * a trip are sorted *after* dives that do. Thus, in the default
 * chronologically-descending sort order, they are shown *before*.
 * "id" is a stable, strictly increasing unique number, that
 * is handed out when a dive is added to the system.
 * We might also consider sorting by end-time and other criteria,
 * but see the caveat above (editing means rearrangement of the dives).
 */
static int comp_dives(const struct dive *a, const struct dive *b)
{
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
	if (a->id < b->id)
		return -1;
	if (a->id > b->id)
		return 1;
	return 0; /* this should not happen for a != b */
}

bool dive_less_than(const struct dive *a, const struct dive *b)
{
	return comp_dives(a, b) < 0;
}

/* Trips are compared according to the first dive in the trip. */
static int comp_trips(const struct dive_trip *a, const struct dive_trip *b)
{
	/* This should never happen, nevertheless don't crash on trips
	 * with no (or worse a negative number of) dives. */
	if (a->dives.nr <= 0)
		return b->dives.nr <= 0 ? 0 : -1;
	if (b->dives.nr <= 0)
		return 1;
	return comp_dives(a->dives.dives[0], b->dives.dives[0]);
}

bool trip_less_than(const struct dive_trip *a, const struct dive_trip *b)
{
	return comp_trips(a, b) < 0;
}

/* When comparing a dive to a trip, use the first dive of the trip. */
static int comp_dive_to_trip(struct dive *a, struct dive_trip *b)
{
	/* This should never happen, nevertheless don't crash on trips
	 * with no (or worse a negative number of) dives. */
	if (b->dives.nr <= 0)
		return -1;
	return comp_dives(a, b->dives.dives[0]);
}

static int comp_dive_or_trip(struct dive_or_trip a, struct dive_or_trip b)
{
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

static int sortfn(const void *_a, const void *_b)
{
	const struct dive *a = (const struct dive *)*(const void **)_a;
	const struct dive *b = (const struct dive *)*(const void **)_b;
	return comp_dives(a, b);
}

void sort_table(struct dive_table *table)
{
	qsort(table->dives, table->nr, sizeof(struct dive *), sortfn);
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

static bool is_same_day(timestamp_t trip_when, timestamp_t dive_when)
{
	static timestamp_t twhen = (timestamp_t) 0;
	static struct tm tmt;
	struct tm tmd;

	utc_mkdate(dive_when, &tmd);

	if (twhen != trip_when) {
		twhen = trip_when;
		utc_mkdate(twhen, &tmt);
	}

	return (tmd.tm_mday == tmt.tm_mday) && (tmd.tm_mon == tmt.tm_mon) && (tmd.tm_year == tmt.tm_year);
}

bool trip_is_single_day(const struct dive_trip *trip)
{
	if (trip->dives.nr <= 1)
		return true;
	return is_same_day(trip->dives.dives[0]->when,
			   trip->dives.dives[trip->dives.nr - 1]->when);
}

int trip_shown_dives(const struct dive_trip *trip)
{
	int res = 0;
	for (int i = 0; i < trip->dives.nr; ++i) {
		if (!trip->dives.dives[i]->hidden_by_filter)
			res++;
	}
	return res;
}
