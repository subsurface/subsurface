// SPDX-License-Identifier: GPL-2.0
#include "pref.h"
#include "subsurface-string.h"

struct preferences prefs, git_prefs;
struct preferences default_prefs = {
	.cloud_base_url = "https://cloud.subsurface-divelog.org/",
	.units = SI_UNITS,
	.unit_system = METRIC,
	.coordinates_traditional = true,
	.pp_graphs = {
		.po2 = false,
		.pn2 = false,
		.phe = false,
		.po2_threshold_min = 0.16,
		.po2_threshold_max = 1.6,
		.pn2_threshold = 4.0,
		.phe_threshold = 13.0,
	},
	.mod = false,
	.modpO2 = 1.6,
	.ead = false,
	.hrgraph = false,
	.percentagegraph = false,
	.dcceiling = true,
	.redceiling = false,
	.calcceiling = false,
	.calcceiling3m = false,
	.calcndltts = false,
	.decoinfo = true,
	.gflow = 30,
	.gfhigh = 75,
	.animation_speed = 500,
	.gf_low_at_maxdepth = false,
	.show_ccr_setpoint = false,
	.show_ccr_sensors = false,
	.show_scr_ocpo2 = false,
	.font_size = -1,
	.mobile_scale = 1.0,
	.display_invalid_dives = false,
	.show_sac = false,
	.display_unused_tanks = false,
	.display_default_tank_infos = true,
	.show_average_depth = true,
	.show_icd = false,
	.ascrate75 = 9000 / 60,
	.ascrate50 = 9000 / 60,
	.ascratestops = 9000 / 60,
	.ascratelast6m = 9000 / 60,
	.descrate = 18000 / 60,
	.sacfactor = 400,
	.problemsolvingtime = 4,
	.bottompo2 = 1400,
	.decopo2 = 1600,
	.bestmixend.mm = 30000,
	.doo2breaks = false,
	.dobailout = false,
	.drop_stone_mode = false,
	.switch_at_req_stop = false,
	.min_switch_duration = 60,
	.surface_segment = 0,
	.last_stop = false,
	.verbatim_plan = false,
	.display_runtime = true,
	.display_duration = true,
	.display_transitions = true,
	.display_variations = false,
	.o2narcotic = true,
	.safetystop = true,
	.bottomsac = 20000,
	.decosac = 17000,
	.reserve_gas=40000,
	.o2consumption = 720,
	.pscr_ratio = 100,
	.show_pictures_in_profile = true,
	.tankbar = false,
	.defaultsetpoint = 1100,
	.geocoding = {
		.category = { 0 }
	},
	.locale = {
		.use_system_language = true,
	},
	.planner_deco_mode = BUEHLMANN,
	.vpmb_conservatism = 3,
	.distance_threshold = 100,
	.time_threshold = 300,
#if defined(SUBSURFACE_MOBILE)
	.cloud_timeout = 10,
#else
	.cloud_timeout = 5,
#endif
	.auto_recalculate_thumbnails = true,
	.extract_video_thumbnails = true,
	.extract_video_thumbnails_position = 20,		// The first fifth seems like a reasonable place
};

/* copy a preferences block, including making copies of all included strings */
void copy_prefs(struct preferences *src, struct preferences *dest)
{
	*dest = *src;
	dest->divelist_font = copy_string(src->divelist_font);
	dest->default_filename = copy_string(src->default_filename);
	dest->default_cylinder = copy_string(src->default_cylinder);
	dest->cloud_base_url = copy_string(src->cloud_base_url);
	dest->cloud_git_url = copy_string(src->cloud_git_url);
	dest->proxy_host = copy_string(src->proxy_host);
	dest->proxy_user = copy_string(src->proxy_user);
	dest->proxy_pass = copy_string(src->proxy_pass);
	dest->time_format = copy_string(src->time_format);
	dest->date_format = copy_string(src->date_format);
	dest->date_format_short = copy_string(src->date_format_short);
	dest->cloud_storage_password = copy_string(src->cloud_storage_password);
	dest->cloud_storage_email = copy_string(src->cloud_storage_email);
	dest->cloud_storage_email_encoded = copy_string(src->cloud_storage_email_encoded);
	dest->ffmpeg_executable = copy_string(src->ffmpeg_executable);
}

/*
 * Free strduped prefs before exit.
 *
 * These are not real leaks but they plug the holes found by eg.
 * valgrind so you can find the real leaks.
 */
void free_prefs(void)
{
	// nop
}
