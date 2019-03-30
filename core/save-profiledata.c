#include "core/profile.h"
#include "core/profile.h"
#include "core/dive.h"
#include "core/display.h"
#include "core/membuffer.h"
#include "core/subsurface-string.h"
#include "core/save-profiledata.h"

static void put_int(struct membuffer *b, int val)
{
	put_format(b, "\"%d\", ", val);
}

static void put_csv_string(struct membuffer *b, const char *val)
{
	put_format(b, "\"%s\", ", val);
}

static void put_double(struct membuffer *b, double val)
{
	put_format(b, "\"%f\" ", val);
}

static void put_pd(struct membuffer *b, struct plot_data *entry)
{
	if (!entry)
		return;

	put_int(b, entry->in_deco);
	put_int(b,  entry->sec);
	for (int c = 0; c < MAX_CYLINDERS; c++) {
		put_int(b, entry->pressure[c][0]);
		put_int(b, entry->pressure[c][1]);
	}
	put_int(b, entry->temperature);
	put_int(b, entry->depth);
	put_int(b, entry->ceiling);
	for (int i = 0; i < 16; i++)
		put_int(b, entry->ceilings[i]);
	for (int i = 0; i < 16; i++)
		put_int(b, entry->percentages[i]);
	put_int(b, entry->ndl);
	put_int(b, entry->tts);
	put_int(b, entry->rbt);
	put_int(b, entry->stoptime);
	put_int(b, entry->stopdepth);
	put_int(b, entry->cns);
	put_int(b, entry->smoothed);
	put_int(b, entry->sac);
	put_int(b, entry->running_sum);
	put_double(b, entry->pressures.o2);
	put_double(b, entry->pressures.n2);
	put_double(b, entry->pressures.he);
	put_int(b, entry->o2pressure.mbar);
	put_int(b, entry->o2sensor[0].mbar);
	put_int(b, entry->o2sensor[1].mbar);
	put_int(b, entry->o2sensor[2].mbar);
	put_int(b, entry->o2setpoint.mbar);
	put_int(b, entry->scr_OC_pO2.mbar);
	put_double(b, entry->mod);
	put_double(b, entry->ead);
	put_double(b, entry->end);
	put_double(b, entry->eadd);
	switch (entry->velocity) {
	case STABLE:
		put_csv_string(b, "STABLE");
		break;
	case SLOW:
		put_csv_string(b, "SLOW");
		break;
	case MODERATE:
		put_csv_string(b, "MODERATE");
		break;
	case FAST:
		put_csv_string(b, "FAST");
		break;
	case CRAZY:
		put_csv_string(b, "CRAZY");
		break;
	}
	put_int(b, entry->speed);
	put_int(b, entry->in_deco_calc);
	put_int(b, entry->ndl_calc);
	put_int(b, entry->tts_calc);
	put_int(b, entry->stoptime_calc);
	put_int(b, entry->stopdepth_calc);
	put_int(b, entry->pressure_time);
	put_int(b, entry->heartbeat);
	put_int(b, entry->bearing);
	put_double(b, entry->ambpressure);
	put_double(b, entry->gfline);
	put_double(b, entry->surface_gf);
	put_double(b, entry->density);
	put_int(b, entry->icd_warning ? 1 : 0);
}

static void put_headers(struct membuffer *b)
{
	put_csv_string(b, "in_deco");
	put_csv_string(b, "sec");
	for (int c = 0; c < MAX_CYLINDERS; c++) {
		put_format(b, "\"pressure_%d_cylinder\", ", c);
		put_format(b, "\"pressure_%d_interpolated\", ", c);
	}
	put_csv_string(b, "temperature");
	put_csv_string(b, "depth");
	put_csv_string(b, "ceiling");
	for (int i = 0; i < 16; i++)
		put_format(b, "\"ceiling_%d\", ", i);
	for (int i = 0; i < 16; i++)
		put_format(b, "\"percentage_%d\", ", i);
	put_csv_string(b, "ndl");
	put_csv_string(b, "tts");
	put_csv_string(b, "rbt");
	put_csv_string(b, "stoptime");
	put_csv_string(b, "stopdepth");
	put_csv_string(b, "cns");
	put_csv_string(b, "smoothed");
	put_csv_string(b, "sac");
	put_csv_string(b, "running_sum");
	put_csv_string(b, "pressureo2");
	put_csv_string(b, "pressuren2");
	put_csv_string(b, "pressurehe");
	put_csv_string(b, "o2pressure");
	put_csv_string(b, "o2sensor0");
	put_csv_string(b, "o2sensor1");
	put_csv_string(b, "o2sensor2");
	put_csv_string(b, "o2setpoint");
	put_csv_string(b, "scr_oc_po2");
	put_csv_string(b, "mod");
	put_csv_string(b, "ead");
	put_csv_string(b, "end");
	put_csv_string(b, "eadd");
	put_csv_string(b, "velocity");
	put_csv_string(b, "speed");
	put_csv_string(b, "in_deco_calc");
	put_csv_string(b, "ndl_calc");
	put_csv_string(b, "tts_calc");
	put_csv_string(b, "stoptime_calc");
	put_csv_string(b, "stopdepth_calc");
	put_csv_string(b, "pressure_time");
	put_csv_string(b, "heartbeat");
	put_csv_string(b, "bearing");
	put_csv_string(b, "ambpressure");
	put_csv_string(b, "gfline");
	put_csv_string(b, "surface_gf");
	put_csv_string(b, "density");
	put_csv_string(b, "icd_warning");
}

static void save_profiles_buffer(struct membuffer *b, bool select_only)
{
	int i;
	struct dive *dive;
	struct plot_info pi;
	struct deco_state *planner_deco_state = NULL;

	for_each_dive(i, dive) {
		if (select_only && !dive->selected)
			continue;
		pi = calculate_max_limits_new(dive, &dive->dc);
		create_plot_info_new(dive, &dive->dc, &pi, false, planner_deco_state);
		put_headers(b);
		put_format(b, "\n");

		for (int i = 0; i < pi.nr; i++) {
			put_pd(b, &pi.entry[i]);
			put_format(b, "\n");
		}
		put_format(b, "\n");
	}
}

int save_profiledata(const char *filename, const bool select_only)
{
	struct membuffer buf = { 0 };
	FILE *f;
	int error = 0;

	save_profiles_buffer(&buf, select_only);

	if (same_string(filename, "-")) {
		f = stdout;
	} else {
		error = -1;
		f = subsurface_fopen(filename, "w");
	}
	if (f) {
		flush_buffer(&buf, f);
		error = fclose(f);
	}
	if (error)
		report_error("Save failed (%s)", strerror(error));

	free_buffer(&buf);
	return error;
}
