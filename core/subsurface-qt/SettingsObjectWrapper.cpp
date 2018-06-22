// SPDX-License-Identifier: GPL-2.0
#include "SettingsObjectWrapper.h"
#include <QSettings>
#include <QApplication>
#include <QFont>
#include <QDate>
#include <QNetworkProxy>

#include "core/qthelper.h"



SettingsObjectWrapper::SettingsObjectWrapper(QObject* parent):
QObject(parent),
	techDetails(new qPrefTec(this)),
	pp_gas(new qPrefGas(this)),
	facebook(new qPrefFacebook(this)),
	geocoding(new qPrefGeocoding(this)),
	proxy(new qPrefProxy(this)),
	cloud_storage(new qPrefCS(this)),
	planner_settings(new qPrefPlanner(this)),
	general_settings(new qPrefGeneral(this)),
	unit_settings(new qPrefUnits(this)),
	display_settings(new qPrefDisplay(this)),
	language_settings(new qPrefLanguage(this)),
	animation_settings(new qPrefAnimations(this)),
	location_settings(new qPrefLocation(this)),
	update_manager_settings(new qPrefUpdate(this)),
	dive_computer_settings(new qPrefDC(this))
{
}

void SettingsObjectWrapper::load()
{
	QSettings s;
	QVariant v;

	uiLanguage(NULL);
	s.beginGroup("Units");
	if (s.value("unit_system").toString() == "metric") {
		prefs.unit_system = METRIC;
		prefs.units = SI_units;
	} else if (s.value("unit_system").toString() == "imperial") {
		prefs.unit_system = IMPERIAL;
		prefs.units = IMPERIAL_units;
	} else {
		prefs.unit_system = PERSONALIZE;
		GET_UNIT("length", length, units::FEET, units::METERS);
		GET_UNIT("pressure", pressure, units::PSI, units::BAR);
		GET_UNIT("volume", volume, units::CUFT, units::LITER);
		GET_UNIT("temperature", temperature, units::FAHRENHEIT, units::CELSIUS);
		GET_UNIT("weight", weight, units::LBS, units::KG);
	}
	GET_UNIT("vertical_speed_time", vertical_speed_time, units::MINUTES, units::SECONDS);
	GET_UNIT3("duration_units", duration_units, units::MIXED, units::ALWAYS_HOURS, units::DURATION);
	GET_UNIT_BOOL("show_units_table", show_units_table);
	GET_BOOL("coordinates", coordinates_traditional);
	s.endGroup();
	s.beginGroup("TecDetails");
	GET_BOOL("po2graph", pp_graphs.po2);
	GET_BOOL("pn2graph", pp_graphs.pn2);
	GET_BOOL("phegraph", pp_graphs.phe);
	GET_DOUBLE("po2thresholdmin", pp_graphs.po2_threshold_min);
	GET_DOUBLE("po2thresholdmax", pp_graphs.po2_threshold_max);
	GET_DOUBLE("pn2threshold", pp_graphs.pn2_threshold);
	GET_DOUBLE("phethreshold", pp_graphs.phe_threshold);
	GET_BOOL("mod", mod);
	GET_DOUBLE("modpO2", modpO2);
	GET_BOOL("ead", ead);
	GET_BOOL("redceiling", redceiling);
	GET_BOOL("dcceiling", dcceiling);
	GET_BOOL("calcceiling", calcceiling);
	GET_BOOL("calcceiling3m", calcceiling3m);
	GET_BOOL("calcndltts", calcndltts);
	GET_BOOL("calcalltissues", calcalltissues);
	GET_BOOL("hrgraph", hrgraph);
	GET_BOOL("tankbar", tankbar);
	GET_BOOL("RulerBar", rulergraph);
	GET_BOOL("percentagegraph", percentagegraph);
	GET_INT("gflow", gflow);
	GET_INT("gfhigh", gfhigh);
	GET_INT("vpmb_conservatism", vpmb_conservatism);
	GET_BOOL("gf_low_at_maxdepth", gf_low_at_maxdepth);
	GET_BOOL("show_ccr_setpoint",show_ccr_setpoint);
	GET_BOOL("show_ccr_sensors",show_ccr_sensors);
	GET_BOOL("show_scr_ocpo2",show_scr_ocpo2);
	GET_BOOL("zoomed_plot", zoomed_plot);
	set_gf(prefs.gflow, prefs.gfhigh);
	set_vpmb_conservatism(prefs.vpmb_conservatism);
	GET_BOOL("show_sac", show_sac);
	GET_BOOL("display_unused_tanks", display_unused_tanks);
	GET_BOOL("show_average_depth", show_average_depth);
	GET_BOOL("show_icd", show_icd);
	GET_BOOL("show_pictures_in_profile", show_pictures_in_profile);
	prefs.display_deco_mode =  (deco_mode) s.value("display_deco_mode").toInt();
	s.endGroup();

	s.beginGroup("GeneralSettings");
	GET_TXT("default_filename", default_filename);
	GET_INT("default_file_behavior", default_file_behavior);
	if (prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		if (QString(prefs.default_filename).isEmpty())
			prefs.default_file_behavior = NO_DEFAULT_FILE;
		else
			prefs.default_file_behavior = LOCAL_DEFAULT_FILE;
	}
	GET_TXT("default_cylinder", default_cylinder);
	GET_BOOL("use_default_file", use_default_file);
	GET_INT("defaultsetpoint", defaultsetpoint);
	GET_INT("o2consumption", o2consumption);
	GET_INT("pscr_ratio", pscr_ratio);
	GET_BOOL("auto_recalculate_thumbnails", auto_recalculate_thumbnails);
	s.endGroup();

	s.beginGroup("Display");
	// get the font from the settings or our defaults
	// respect the system default font size if none is explicitly set
	QFont defaultFont = s.value("divelist_font", prefs.divelist_font).value<QFont>();
	if (IS_FP_SAME(system_divelist_default_font_size, -1.0)) {
		prefs.font_size = qApp->font().pointSizeF();
		system_divelist_default_font_size = prefs.font_size; // this way we don't save it on exit
	}
	prefs.font_size = s.value("font_size", prefs.font_size).toFloat();
	// painful effort to ignore previous default fonts on Windows - ridiculous
	QString fontName = defaultFont.toString();
	if (fontName.contains(","))
		fontName = fontName.left(fontName.indexOf(","));
	if (subsurface_ignore_font(qPrintable(fontName))) {
		defaultFont = QFont(prefs.divelist_font);
	} else {
		free((void *)prefs.divelist_font);
		prefs.divelist_font = copy_qstring(fontName);
	}
	defaultFont.setPointSizeF(prefs.font_size);
	qApp->setFont(defaultFont);
	GET_BOOL("displayinvalid", display_invalid_dives);
	s.endGroup();

	s.beginGroup("Animations");
	GET_INT("animation_speed", animation_speed);
	s.endGroup();

	s.beginGroup("Network");
	GET_INT_DEF("proxy_type", proxy_type, QNetworkProxy::DefaultProxy);
	GET_TXT("proxy_host", proxy_host);
	GET_INT("proxy_port", proxy_port);
	GET_BOOL("proxy_auth", proxy_auth);
	GET_TXT("proxy_user", proxy_user);
	GET_TXT("proxy_pass", proxy_pass);
	s.endGroup();

	s.beginGroup("CloudStorage");
	GET_TXT("email", cloud_storage_email);
	GET_TXT("email_encoded", cloud_storage_email_encoded);
#ifndef SUBSURFACE_MOBILE
	GET_BOOL("save_password_local", save_password_local);
#else
	// always save the password in Subsurface-mobile
	prefs.save_password_local = true;
#endif
	if (prefs.save_password_local) { // GET_TEXT macro is not a single statement
		GET_TXT("password", cloud_storage_password);
	}
	GET_INT("cloud_verification_status", cloud_verification_status);
	GET_BOOL("git_local_only", git_local_only);

	// creating the git url here is simply a convenience when C code wants
	// to compare against that git URL - it's always derived from the base URL
	GET_TXT("cloud_base_url", cloud_base_url);
	prefs.cloud_git_url = copy_qstring(QString(prefs.cloud_base_url) + "/git");
	s.endGroup();

	// Subsurface webservice id is stored outside of the groups
	GET_TXT("subsurface_webservice_uid", userid);

	// GeoManagement
	s.beginGroup("geocoding");

	GET_ENUM("cat0", taxonomy_category, geocoding.category[0]);
	GET_ENUM("cat1", taxonomy_category, geocoding.category[1]);
	GET_ENUM("cat2", taxonomy_category, geocoding.category[2]);
	s.endGroup();

	// GPS service time and distance thresholds
	s.beginGroup("LocationService");
	GET_INT("time_threshold", time_threshold);
	GET_INT("distance_threshold", distance_threshold);
	s.endGroup();

	s.beginGroup("Planner");
	GET_BOOL("last_stop", last_stop);
	GET_BOOL("verbatim_plan", verbatim_plan);
	GET_BOOL("display_duration", display_duration);
	GET_BOOL("display_runtime", display_runtime);
	GET_BOOL("display_transitions", display_transitions);
	GET_BOOL("display_variations", display_variations);
	GET_BOOL("safetystop", safetystop);
	GET_BOOL("doo2breaks", doo2breaks);
	GET_BOOL("switch_at_req_stop",switch_at_req_stop);
	GET_BOOL("drop_stone_mode", drop_stone_mode);

	GET_INT("reserve_gas", reserve_gas);
	GET_INT("ascrate75", ascrate75);
	GET_INT("ascrate50", ascrate50);
	GET_INT("ascratestops", ascratestops);
	GET_INT("ascratelast6m", ascratelast6m);
	GET_INT("descrate", descrate);
	GET_INT("sacfactor", sacfactor);
	GET_INT("problemsolvingtime", problemsolvingtime);
	GET_INT("bottompo2", bottompo2);
	GET_INT("decopo2", decopo2);
	GET_INT("bestmixend", bestmixend.mm);
	GET_INT("min_switch_duration", min_switch_duration);
	GET_INT("bottomsac", bottomsac);
	GET_INT("decosac", decosac);

	prefs.planner_deco_mode = deco_mode(s.value("deco_mode", default_prefs.planner_deco_mode).toInt());
	s.endGroup();

	s.beginGroup("DiveComputer");
	GET_TXT("dive_computer_vendor",dive_computer.vendor);
	GET_TXT("dive_computer_product", dive_computer.product);
	GET_TXT("dive_computer_device", dive_computer.device);
	GET_TXT("dive_computer_device_name", dive_computer.device_name);
	GET_INT("dive_computer_download_mode", dive_computer.download_mode);
	s.endGroup();

	s.beginGroup("UpdateManager");
	prefs.update_manager.dont_check_exists = s.contains("DontCheckForUpdates");
	GET_BOOL("DontCheckForUpdates", update_manager.dont_check_for_updates);
	GET_TXT("LastVersionUsed", update_manager.last_version_used);
	prefs.update_manager.next_check = copy_qstring(s.value("NextCheck").toDate().toString("dd/MM/yyyy"));
	s.endGroup();

	s.beginGroup("Language");
	GET_BOOL("UseSystemLanguage", locale.use_system_language);
	GET_TXT("UiLanguage", locale.language);
	GET_TXT("UiLangLocale", locale.lang_locale);
	GET_TXT("time_format", time_format);
	GET_TXT("date_format", date_format);
	GET_TXT("date_format_short", date_format_short);
	GET_BOOL("time_format_override", time_format_override);
	GET_BOOL("date_format_override", date_format_override);
	s.endGroup();
}

void SettingsObjectWrapper::sync()
{
	QSettings s;
	s.beginGroup("Planner");
	s.setValue("last_stop", prefs.last_stop);
	s.setValue("verbatim_plan", prefs.verbatim_plan);
	s.setValue("display_duration", prefs.display_duration);
	s.setValue("display_runtime", prefs.display_runtime);
	s.setValue("display_transitions", prefs.display_transitions);
	s.setValue("display_variations", prefs.display_variations);
	s.setValue("safetystop", prefs.safetystop);
	s.setValue("reserve_gas", prefs.reserve_gas);
	s.setValue("ascrate75", prefs.ascrate75);
	s.setValue("ascrate50", prefs.ascrate50);
	s.setValue("ascratestops", prefs.ascratestops);
	s.setValue("ascratelast6m", prefs.ascratelast6m);
	s.setValue("descrate", prefs.descrate);
	s.setValue("sacfactor", prefs.sacfactor);
	s.setValue("problemsolvingtime", prefs.problemsolvingtime);
	s.setValue("bottompo2", prefs.bottompo2);
	s.setValue("decopo2", prefs.decopo2);
	s.setValue("bestmixend", prefs.bestmixend.mm);
	s.setValue("doo2breaks", prefs.doo2breaks);
	s.setValue("drop_stone_mode", prefs.drop_stone_mode);
	s.setValue("switch_at_req_stop", prefs.switch_at_req_stop);
	s.setValue("min_switch_duration", prefs.min_switch_duration);
	s.setValue("bottomsac", prefs.bottomsac);
	s.setValue("decosac", prefs.decosac);
	s.setValue("deco_mode", int(prefs.planner_deco_mode));
	s.endGroup();
}

SettingsObjectWrapper* SettingsObjectWrapper::instance()
{
	static SettingsObjectWrapper settings;
	return &settings;
}
