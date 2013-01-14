#include <string.h>

#include "dive.h"

static void set_bool_conf(char *name, gboolean value, gboolean def)
{
	if (value == def) {
		subsurface_unset_conf(name);
		return;
	}
	subsurface_set_conf_bool(name, value);
}
#define __SAVE_BOOLEAN(name, field, value) \
	set_bool_conf(name, prefs.field == value, default_prefs.field == value)
#define SAVE_UNIT(name, field, value) __SAVE_BOOLEAN(name, units.field, value)
#define SAVE_BOOL(name, field) __SAVE_BOOLEAN(name, field, TRUE)

static void set_string_conf(char *name, const char *value, const char *def)
{
	if (!strcmp(value, def)) {
		subsurface_unset_conf(name);
		return;
	}
	subsurface_set_conf(name, value);
}
#define SAVE_STRING(name, field) set_string_conf(name, prefs.field, default_prefs.field)

/* We don't really save doubles */
static void save_double_conf(char *name, double _val, double _def)
{
	int val = rint(_val * 100), neg, len;
	int def = rint(_def * 100);
	char string[16];

	if (val == def) {
		subsurface_unset_conf(name);
		return;
	}
	neg = 0;
	if (val < 0) {
		neg = 1;
		val = -val;
	}
	len = snprintf(string, sizeof(string), "%s%d.%02d",
		neg ? "-" : "", val / 100, val % 100);

	/* Save with 0-2 decimals */
	if (string[len-1] == '0') {
		len--;
		if (string[len-1] == '0')
			len -= 2;	/* Remove decimal point too */
		string[len] = 0;
	}

	subsurface_set_conf(name, string);
}

#define SAVE_DOUBLE(name, field) save_double_conf(name, prefs.field, default_prefs.field)
#define SAVE_PERCENT(name, field) save_double_conf(name, prefs.field*100, default_prefs.field*100)

void save_preferences(void)
{
	SAVE_UNIT("feet", length, FEET);
	SAVE_UNIT("psi", pressure, PSI);
	SAVE_UNIT("cuft", volume, CUFT);
	SAVE_UNIT("fahrenheit", temperature, FAHRENHEIT);
	SAVE_UNIT("lbs", weight, LBS);

	SAVE_BOOL("TEMPERATURE", visible_cols.temperature);
	SAVE_BOOL("TOTALWEIGHT", visible_cols.totalweight);
	SAVE_BOOL("SUIT", visible_cols.suit);
	SAVE_BOOL("CYLINDER", visible_cols.cylinder);
	SAVE_BOOL("NITROX", visible_cols.nitrox);
	SAVE_BOOL("SAC", visible_cols.sac);
	SAVE_BOOL("OTU", visible_cols.otu);
	SAVE_BOOL("MAXCNS", visible_cols.maxcns);

	SAVE_STRING("divelist_font", divelist_font);

	SAVE_BOOL("po2graph", pp_graphs.po2);
	SAVE_BOOL("pn2graph", pp_graphs.pn2);
	SAVE_BOOL("phegraph", pp_graphs.phe);

	SAVE_DOUBLE("po2threshold", pp_graphs.po2_threshold);
	SAVE_DOUBLE("pn2threshold", pp_graphs.pn2_threshold);
	SAVE_DOUBLE("phethreshold", pp_graphs.phe_threshold);

	SAVE_BOOL("mod", mod);
	SAVE_DOUBLE("modppO2", mod_ppO2);
	SAVE_BOOL("ead", ead);
	SAVE_BOOL("redceiling", profile_red_ceiling);
	SAVE_BOOL("calcceiling", profile_calc_ceiling);
	SAVE_BOOL("calcceiling3m", calc_ceiling_3m_incr);

	SAVE_PERCENT("gflow", gflow);
	SAVE_PERCENT("gfhigh", gfhigh);

	SAVE_STRING("default_filename", default_filename);

	/* Flush the changes out to the system */
	subsurface_flush_conf();
}

static gboolean get_bool(char *name, gboolean def)
{
	int val = subsurface_get_conf_bool(name);
	if (val < 0)
		return def;
	return val;
}
#define GET_UNIT(name, field, f, t) \
	prefs.units.field = get_bool(name, default_prefs.units.field) ? (t) : (f)
#define GET_BOOL(name, field) \
	prefs.field = get_bool(name, default_prefs.field)


void load_preferences(void)
{
	const char *conf_value;

	GET_UNIT("feet", length, METERS, FEET);
	GET_UNIT("psi", pressure, BAR, PSI);
	GET_UNIT("cuft", volume, LITER, CUFT);
	GET_UNIT("fahrenheit", temperature, CELSIUS, FAHRENHEIT);
	GET_UNIT("lbs", weight, KG, LBS);

	/* an unset key is 'default' */
	GET_BOOL("CYLINDER", visible_cols.cylinder);
	GET_BOOL("TEMPERATURE", visible_cols.temperature);
	GET_BOOL("TOTALWEIGHT", visible_cols.totalweight);
	GET_BOOL("SUIT", visible_cols.suit);
	GET_BOOL("NITROX", visible_cols.nitrox);
	GET_BOOL("OTU", visible_cols.otu);
	GET_BOOL("MAXCNS", visible_cols.maxcns);
	GET_BOOL("SAC", visible_cols.sac);
	GET_BOOL("po2graph", pp_graphs.po2);
	GET_BOOL("pn2graph", pp_graphs.pn2);
	GET_BOOL("phegraph", pp_graphs.phe);

	conf_value = subsurface_get_conf("po2threshold");
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.pp_graphs.po2_threshold);
		free((void *)conf_value);
	}
	conf_value = subsurface_get_conf("pn2threshold");
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.pp_graphs.pn2_threshold);
		free((void *)conf_value);
	}
	conf_value = subsurface_get_conf("phethreshold");
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.pp_graphs.phe_threshold);
		free((void *)conf_value);
	}
	GET_BOOL("mod", mod);
	conf_value = subsurface_get_conf("modppO2");
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.mod_ppO2);
		free((void *)conf_value);
	}
	GET_BOOL("ead", ead);
	GET_BOOL("redceiling", profile_red_ceiling);
	GET_BOOL("calcceiling", profile_calc_ceiling);
	GET_BOOL("calcceiling3m", calc_ceiling_3m_incr);
	conf_value = subsurface_get_conf("gflow");
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.gflow);
		prefs.gflow /= 100.0;
		free((void *)conf_value);
	}
	conf_value = subsurface_get_conf("gfhigh");
	if (conf_value) {
		sscanf(conf_value, "%lf", &prefs.gfhigh);
		prefs.gfhigh /= 100.0;
		free((void *)conf_value);
	}
	set_gf(prefs.gflow, prefs.gfhigh);

	conf_value = subsurface_get_conf("divelist_font");
	if (conf_value)
		prefs.divelist_font = conf_value;


	conf_value = subsurface_get_conf("default_filename");
	if (conf_value)
		prefs.default_filename = conf_value;
}
