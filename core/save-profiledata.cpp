#include "core/save-profiledata.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/profile.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "core/format.h"
#include "core/membuffer.h"
#include "core/subsurface-string.h"
#include "core/version.h"
#include <errno.h>

static void put_int(struct membuffer *b, int val)
{
	put_format(b, "\"%d\", ", val);
}

static void put_int_with_nl(struct membuffer *b, int val)
{
	put_format(b, "\"%d\"\n", val);
}

static void put_csv_string(struct membuffer *b, const char *val)
{
	put_format(b, "\"%s\", ", val);
}

static void put_csv_string_with_nl(struct membuffer *b, const char *val)
{
	put_format(b, "\"%s\"\n", val);
}

static void put_double(struct membuffer *b, double val)
{
	put_format(b, "\"%f\", ", val);
}

static std::string video_time(int secs)
{
	int hours = secs / 3600;
	secs -= hours * 3600;
	int mins = secs / 60;
	secs -= mins * 60;
	return format_string_std("%d:%02d:%02d.000,", hours, mins, secs);
}

static void put_pd(struct membuffer *b, const struct plot_info &pi, int idx)
{
	const struct plot_data &entry = pi.entry[idx];

	put_int(b, entry.in_deco);
	put_int(b, entry.sec);
	for (int c = 0; c < pi.nr_cylinders; c++) {
		put_int(b, get_plot_sensor_pressure(pi, idx, c));
		put_int(b, get_plot_interpolated_pressure(pi, idx, c));
	}
	put_int(b, entry.temperature);
	put_int(b, entry.depth);
	put_int(b, entry.ceiling);
	for (int i = 0; i < 16; i++)
		put_int(b, entry.ceilings[i]);
	for (int i = 0; i < 16; i++)
		put_int(b, entry.percentages[i]);
	put_int(b, entry.ndl);
	put_int(b, entry.tts);
	put_int(b, entry.rbt);
	put_int(b, entry.stoptime);
	put_int(b, entry.stopdepth);
	put_int(b, entry.cns);
	put_int(b, entry.smoothed);
	put_int(b, entry.sac);
	put_int(b, entry.running_sum);
	put_double(b, entry.pressures.o2);
	put_double(b, entry.pressures.n2);
	put_double(b, entry.pressures.he);
	put_int(b, entry.o2pressure.mbar);
	for (int i = 0; i < MAX_O2_SENSORS; i++)
		put_int(b, entry.o2sensor[i].mbar);
	put_int(b, entry.o2setpoint.mbar);
	put_int(b, entry.scr_OC_pO2.mbar);
	put_int(b, entry.mod);
	put_int(b, entry.ead);
	put_int(b, entry.end);
	put_int(b, entry.eadd);
	switch (entry.velocity) {
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
	put_int(b, entry.speed);
	put_int(b, entry.in_deco_calc);
	put_int(b, entry.ndl_calc);
	put_int(b, entry.tts_calc);
	put_int(b, entry.stoptime_calc);
	put_int(b, entry.stopdepth_calc);
	put_int(b, entry.pressure_time);
	put_int(b, entry.heartbeat);
	put_int(b, entry.bearing);
	put_double(b, entry.ambpressure);
	put_double(b, entry.gfline);
	put_double(b, entry.surface_gf);
	put_double(b, entry.density);
	put_int_with_nl(b, entry.icd_warning ? 1 : 0);
}

static void put_headers(struct membuffer *b, int nr_cylinders)
{
	put_csv_string(b, "in_deco");
	put_csv_string(b, "sec");
	for (int c = 0; c < nr_cylinders; c++) {
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
	put_csv_string_with_nl(b, "icd_warning");
}

static std::string format_st_event(const plot_data &entry, const plot_data &next_entry, int offset, int length)
{
	double value;
	int decimals;
	const char *unit;

	if (entry.sec < offset || entry.sec > offset + length)
		return {};

	std::string res = "Dialogue: 0,";
	res += video_time(entry.sec - offset);
	res += video_time(next_entry.sec - offset < length ? next_entry.sec - offset : length);
	res += "Default,,0,0,0,,";
	res += format_string_std("%d:%02d ", FRACTION_TUPLE(entry.sec, 60));
	value = get_depth_units(entry.depth, &decimals, &unit);
	res += format_string_std("D=%02.2f %s ", value, unit);
	if (entry.temperature) {
		value = get_temp_units(entry.temperature, &unit);
		res += format_string_std("T=%.1f%s ", value, unit);
	}
	// Only show NDL if it is not essentially infinite, show TTS for mandatory stops.
	if (entry.ndl_calc < 3600) {
		if (entry.ndl_calc > 0)
			res += format_string_std("NDL=%d:%02d ", FRACTION_TUPLE(entry.ndl_calc, 60));
		else
			if (entry.tts_calc > 0)
				res += format_string_std("TTS=%d:%02d ", FRACTION_TUPLE(entry.tts_calc, 60));
	}
	if (entry.surface_gf > 0.0) {
		res += format_string_std("sGF=%.1f%% ", entry.surface_gf);
	}
	res += "\n";
	return res;
}

static void save_profiles_buffer(struct membuffer *b, bool select_only)
{
	struct deco_state *planner_deco_state = NULL;

	for(auto &dive: divelog.dives) {
		if (select_only && !dive->selected)
			continue;
		plot_info pi = create_plot_info_new(dive.get(), &dive->dcs[0], planner_deco_state);
		put_headers(b, pi.nr_cylinders);

		for (int i = 0; i < pi.nr; i++)
			put_pd(b, pi, i);
		put_format(b, "\n");
	}
}

std::string save_subtitles_buffer(struct dive *dive, int offset, int length)
{
	struct deco_state *planner_deco_state = NULL;

	plot_info pi = create_plot_info_new(dive, &dive->dcs[0], planner_deco_state);

	std::string res;
	res += "[Script Info]\n";
	res += "; Script generated by Subsurface " + std::string(subsurface_canonical_version()) + "%s\n";
	res += "ScriptType: v4.00+\nPlayResX: 384\nPlayResY: 288\n\n";
	res += "[V4+ Styles]\nFormat: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n";
	res += "Style: Default,Arial,12,&Hffffff,&Hffffff,&H0,&H0,0,0,0,0,100,100,0,0,1,1,0,7,10,10,10,0\n\n";
	res += "[Events]\nFormat: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n";

	for (int i = 0; i + 1 < pi.nr; i++)
		res += format_st_event(pi.entry[i], pi.entry[i + 1], offset, length);
	res += "\n";
	return res;
}

int save_profiledata(const char *filename, bool select_only)
{
	membuffer buf;
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
		report_error("Save failed (%s)", strerror(errno));

	return error;
}
