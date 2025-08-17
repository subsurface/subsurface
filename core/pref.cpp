// SPDX-License-Identifier: GPL-2.0
#include "pref.h"
#include "subsurface-string.h"
#include "git-access.h" // for CLOUD_HOST

struct preferences prefs, git_prefs, default_prefs;

preferences::preferences() :
	animation_speed(500),
	cloud_base_url("https://" CLOUD_HOST_EU "/"), // if we don't know any better, use the European host
#if defined(SUBSURFACE_MOBILE)
	cloud_timeout(10),
#else
	cloud_timeout(5),
#endif
	sync_dc_time(false),
	display_invalid_dives(false),
	font_size(-1),
	mobile_scale(1.0),
	show_developer(true),
	three_m_based_grid(false),
	map_short_names(false),
	include_unused_tanks(false),
	display_default_tank_infos(true),
	auto_recalculate_thumbnails(true),
	extract_video_thumbnails(true),
	extract_video_thumbnails_position(20),		// The first fifth seems like a reasonable place
	defaultsetpoint(1100),
	default_file_behavior(LOCAL_DEFAULT_FILE),
	o2consumption(720),
	pscr_ratio(100),
	use_default_file(true),
	extraEnvironmentalDefault(false),
	salinityEditDefault(false),
	date_format_override(false),
	time_format_override(false),
	proxy_auth(false),
	proxy_port(0),
	proxy_type(0),
	ascratelast6m(9000 / 60),
	ascratestops(9000 / 60),
	ascrate50(9000 / 60),
	ascrate75(9000 / 60),
	bestmixend(30_m),
	bottompo2(1400),
	bottomsac(20000),
	decopo2(1600),
	decosac(17000),
	descrate(18000 / 60),
	display_duration(true),
	display_runtime(true),
	display_transitions(true),
	display_variations(false),
	doo2breaks(false),
	dobailout(false),
	o2narcotic(true),
	drop_stone_mode(false),
	last_stop(false),
	min_switch_duration(60),
	surface_segment(0),
	planner_deco_mode(BUEHLMANN),
	problemsolvingtime(4),
	reserve_gas(40000),
	sacfactor(400),
	safetystop(true),
	switch_at_req_stop(false),
	verbatim_plan(false),
	calcalltissues(false),
	calcceiling(false),
	calcceiling3m(false),
	calcndltts(false),
	decoinfo(true),
	dcceiling(true),
	display_deco_mode(BUEHLMANN),
	ead(false),
	gasplot_frac(0.3),
	gfhigh(75),
	gflow(30),
	gf_low_at_maxdepth(false),
	hrgraph(false),
	mod(false),
	modpO2(1.6),
	percentagegraph(false),
	redceiling(false),
	rulergraph(false),
	show_average_depth(true),
	show_ccr_sensors(false),
	show_ccr_setpoint(false),
	show_icd(false),
	show_pictures_in_profile(true),
	show_sac(false),
	show_scr_ocpo2(false),
	tankbar(false),
	vpmb_conservatism(3),
	zoomed_plot(false),
	infobox(true),
	allowOcGasAsDiluent(false),
	coordinates_traditional(true),
	unit_system(METRIC),
	units(SI_UNITS)
{
}

preferences::~preferences() = default;

void set_git_prefs(std::string_view prefs)
{
	if (contains(prefs, "TANKBAR"))
		git_prefs.tankbar = 1;
	if (contains(prefs, "SHOW_SETPOINT"))
		git_prefs.show_ccr_setpoint = 1;
	if (contains(prefs, "SHOW_SENSORS"))
		git_prefs.show_ccr_sensors = 1;
	if (contains(prefs, "PO2_GRAPH"))
		git_prefs.pp_graphs.po2 = 1;
}
