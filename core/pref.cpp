// SPDX-License-Identifier: GPL-2.0
#include "pref.h"
#include "subsurface-string.h"
#include "git-access.h" // for CLOUD_HOST

struct preferences prefs, git_prefs;
struct preferences default_prefs = {
	.animation_speed = 500,
	.cloud_base_url = "https://" CLOUD_HOST_EU "/", // if we don't know any better, use the European host
#if defined(SUBSURFACE_MOBILE)
	.cloud_timeout = 10,
#else
	.cloud_timeout = 5,
#endif
	.sync_dc_time = false,
	.display_invalid_dives = false,
	.divelist_font = NULL,
	.font_size = -1,
	.mobile_scale = 1.0,
	.show_developer = true,
	.three_m_based_grid = false,
	.map_short_names = false,
	.default_cylinder = NULL,
	.include_unused_tanks = false,
	.display_default_tank_infos = true,
	.auto_recalculate_thumbnails = true,
	.extract_video_thumbnails = true,
	.extract_video_thumbnails_position = 20,		// The first fifth seems like a reasonable place
	.ffmpeg_executable = NULL,
	.defaultsetpoint = 1100,
	.default_filename = NULL,
	.default_file_behavior = LOCAL_DEFAULT_FILE,
	.o2consumption = 720,
	.pscr_ratio = 100,
	.use_default_file = true,
	.extraEnvironmentalDefault = false,
	.salinityEditDefault = false,
	.geocoding = {
		.category = { TC_NONE, TC_NONE, TC_NONE }
	},
	.date_format = NULL,
	.date_format_override = false,
	.date_format_short = NULL,
	.locale = {
		.use_system_language = true,
	},
	.time_format = NULL,
	.time_format_override = false,
	.proxy_auth = false,
	.proxy_host = NULL,
	.proxy_port = 0,
	.proxy_type = 0,
	.proxy_user = NULL,
	.proxy_pass = NULL,
	.ascratelast6m = 9000 / 60,
	.ascratestops = 9000 / 60,
	.ascrate50 = 9000 / 60,
	.ascrate75 = 9000 / 60,
	.bestmixend = { 30000 },
	.bottompo2 = 1400,
	.bottomsac = 20000,
	.decopo2 = 1600,
	.decosac = 17000,
	.descrate = 18000 / 60,
	.display_duration = true,
	.display_runtime = true,
	.display_transitions = true,
	.display_variations = false,
	.doo2breaks = false,
	.dobailout = false,
	.o2narcotic = true,
	.drop_stone_mode = false,
	.last_stop = false,
	.min_switch_duration = 60,
	.surface_segment = 0,
	.planner_deco_mode = BUEHLMANN,
	.problemsolvingtime = 4,
	.reserve_gas=40000,
	.sacfactor = 400,
	.safetystop = true,
	.switch_at_req_stop = false,
	.verbatim_plan = false,
	.calcalltissues = false,
	.calcceiling = false,
	.calcceiling3m = false,
	.calcndltts = false,
	.decoinfo = true,
	.dcceiling = true,
	.display_deco_mode = BUEHLMANN,
	.ead = false,
	.gfhigh = 75,
	.gflow = 30,
	.gf_low_at_maxdepth = false,
	.hrgraph = false,
	.mod = false,
	.modpO2 = 1.6,
	.percentagegraph = false,
	.pp_graphs = {
		.po2 = false,
		.pn2 = false,
		.phe = false,
		.po2_threshold_min = 0.16,
		.po2_threshold_max = 1.6,
		.pn2_threshold = 4.0,
		.phe_threshold = 13.0,
	},
	.redceiling = false,
	.rulergraph = false,
	.show_average_depth = true,
	.show_ccr_sensors = false,
	.show_ccr_setpoint = false,
	.show_icd = false,
	.show_pictures_in_profile = true,
	.show_sac = false,
	.show_scr_ocpo2 = false,
	.tankbar = false,
	.vpmb_conservatism = 3,
	.zoomed_plot = false,
	.infobox = true,
	.coordinates_traditional = true,
	.unit_system = METRIC,
	.units = SI_UNITS,
	.update_manager = { false, NULL, 0 }
};

/* copy a preferences block, including making copies of all included strings */
void copy_prefs(struct preferences *src, struct preferences *dest)
{
	*dest = *src;
	dest->divelist_font = copy_string(src->divelist_font);
	dest->default_filename = copy_string(src->default_filename);
	dest->default_cylinder = copy_string(src->default_cylinder);
	dest->cloud_base_url = copy_string(src->cloud_base_url);
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
