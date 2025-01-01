// SPDX-License-Identifier: GPL-2.0
/*
 * Scubapro's LogTRAK is a Java based program running on Windows and MacOS.
 * It seems to be a development on older TravelTRAK software, and shares with
 * it the exportable .asd binary file format.
 * More recent versions of LogTRAK seem to support downloading data from
 * older IR based devices (Galileo, etc), not just Bluetooth BLE devices.
 *
 * Dive log data are kept (only valid for Windows) in a directory structure
 * on the user folder like this:
 * - user directory	-|- .jtrak	-|- DB_V4	-|- jtrak.properties
 *					 |		 |- jtrak.script
 *					 |- raw_data	-|(empty)
 *					 |- DBok.asd
 *					 |- logtrak.log
 *
 * For us the interesting file is the one with the .script extension.
 *
 * LogTRAK uses HSQLdb under the hood, and fortunately, stores all the data in
 * the .script file; being this file an script to build on the fly the in-memory
 * HSQLDB database each time the software is run.
 *
 * The .script file is, thus, a plain text file containing a serie of HSQLDB
 * commands which contain themselves the full dive info (including full raw DC
 * data in plain ascii text), and some other LogTRAK produced information.
 *
 * LogTRAK (in my opinion) is a very limited software, even difficult to name it
 * a true divelog. It doesn't support manual dives insertion or addition; most
 * of the information it supports comes from the DC device and very (very, very)
 * limited info addition is supported (not even buddies).
 *
 * It can import data from SmartTrak via .asd files, although this should be
 * avoided as a big amount of data is lost in  the process. From a Subsurface
 * point of view, SmartTrak import is always better than LogTrak import as, for
 * a concerned diver, first one can store much more data.
 */

#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <vector>

#include "gettext.h"
#include "dive.h"
#include "divelist.h"
#include "file.h"
#include "device.h"
#include "subsurface-string.h"
#include "libdivecomputer.h"
#include "divesite.h"
#include "locale.h"
#include "errorhelper.h"
#include "divelog.h"
#include "tag.h"
#include "format.h"

/*
 * Defined in import-asd.cpp and shared here.
 */
extern "C" dc_descriptor_t *get_data_descriptor(int data_model, dc_family_t data_fam);

struct T_BOTTLE
{
	std::string equipment_id;
	int tank_vol;		// mililiters
	int tanknum;		// tank number in dive
	int startp;		// start pressure, bar
	int endp;		// end pressure, bar
	int o2_mix;		// O2 percent
	int he_mix;		// HE percent
	std::string tankmat;	// tank material
};

struct T_EQUIPMENT
{
	std::string equipment_id;
	std::string suit;
	float weight = 0;
};

struct T_LOCATION
{
	std::string loc_id;
	std::string loc_name;
};

struct T_SITE
{
	std::string site_id;
	std::string site_name;
	float GPSx;
	float GPSy;
	std::string comment;
	struct T_LOCATION site_loc;
};

std::vector<T_BOTTLE> bottle_db;
std::vector<T_EQUIPMENT> equipment_db;
std::vector<T_LOCATION> location_db;
std::vector<T_SITE> site_db;
std::string db_version;

/*
 * This macro seems weird in the negative part of condition. But it's meant to manage
 * ascii chars ranging 0..9 and a..z, no others.  Thus a string "0a1f" becomes
 * 0x00 0x0a 0x01 0x0f.
 */
#define ASCII_CHR_TO_BYTE(_char) ((_char < 58) ? _char - 48: _char - 87)

/*
 * Returns a sigle line string from the full buffer.
 */
static std::string get_single_line(std::string &begin)
{
	std::string line;

	auto ss = std::stringstream{begin};
	std::getline(ss, line, '\n');

	return line;
}

/*
 * Most strings in LogTrak are sigle-quoted. This function removes initial and
 * ending single quotes and returns a clean string.
 */
static std::string lt_remove_quotes(const std::string &input)
{
	std::string tmp = input;

	if (input.front() == 0x27)
		tmp.erase(tmp.begin());

	if (tmp.back() == 0x27)
		tmp.pop_back();

	return tmp;
}

/*
 * We can catch some strings "NULL" or some other weird strings coming from
 * SmartTrak,  "???" or "---" (default strings in mandatory fields).
 * Check the string. If true, the caller must remove its content.
 */
static bool is_null(const std::string &input) {
	return (input == "NULL" || input == "???" || input == "---");
}

/*
 * LogTrak scapes single quotes with another quote.
 * Remove one of them if we find two in a string.
 */
static std::string trim_quotes(const std::string &in) {
	std::string tmp = in;

	if (in.empty())
		return in;
	size_t s = tmp.find("''");
	while (s != std::string::npos) {
		tmp.erase(s, 1);
		s = tmp.find("''", s);
	}
	return tmp;
}

/*
 * Utility to convert an UCN syntax string \\uxxyy in another one
 * containing an 1, 2 or 3 bytes utf-8 char.
 * AFAIK LogTrak only supports 2 byte unicode, so we don't
 * expect integers bigger than 0xFFFF.
 */
static std::string u_str_to_utf8(const std::string &in)
{
	if (in.empty())
		return in;

	std::string tmp;
	int c = (ASCII_CHR_TO_BYTE(in[2]) << 12) + (ASCII_CHR_TO_BYTE(in[3]) << 8) + (ASCII_CHR_TO_BYTE(in[4]) << 4) + ASCII_CHR_TO_BYTE(in[5]);

	if (c <= 0x7F) {
		tmp.push_back(static_cast<char>(c));
	}
	else if (c <= 0x7FF) {
		tmp.push_back(static_cast<char>((c >> 6) | 0xC0));
		tmp.push_back(static_cast<char>((c & 0x3F) | 0x80));
	} else {
		tmp.push_back(static_cast<char>((c >> 12) | 0xE0));
		tmp.push_back(static_cast<char>(((c >> 6) & 0x3F) | 0x80));
		tmp.push_back(static_cast<char>((c & 0x3F) | 0x80));
	}
	return tmp;
}

/*
 * Parse a string containing unicode chars in
 * UCN syntax "\\u000a" and convert them to utf-8.
 */
static std::string to_utf8(const std::string &in)
{
	if (in.empty())
		return in;

	std::string tmp = in;
	std::size_t pos = tmp.find("\\u");
	while (pos != std::string::npos) {
		std::string u_str = u_str_to_utf8(tmp.substr(pos, 6));
		tmp.replace(pos, 6, u_str);
		pos = tmp.find("\\u", pos + u_str.length());
	}
	return tmp;
}

/*
 * Process a string removing single quotes, escaped quotes, catching "NULL"
 * and converting unicode characters to utf-8 if any.
 */
static std::string process(std::string &input)
{
	if (input.empty())
		return input;

	if (is_null(input)) {
		input.clear();
		return input;
	}
	input = lt_remove_quotes(input);
	input = to_utf8(input);
	return trim_quotes(input);
}

/*
 * A class to manage LogTrak strings.
 * The constructor just "cleans" the input string and converts it to UTF8
 * using previosusly defined functions.
 */
class Lt_String {
	std::string str;

public:
	Lt_String() = default;
	Lt_String(std::string input);
	Lt_String(const char *in);
	~Lt_String();
	const std::string& string() { return str; }
};

Lt_String::Lt_String(std::string in)
{
	this->str = process(in);
}

Lt_String::Lt_String(const char *in)
{
	std::string tmp(in);
	this->str = process(tmp);
}

Lt_String::~Lt_String()
{
	str.clear();
}

/*
 * LogTrak, like SmartTrak, stores the whole data downloaded from the DC.
 * It is stored as a plain ascii sequence of chars (e.g. "a5a50eff..."); this
 * function process an string in such format  and returns a buffer with bytes.
 */
extern "C" bool lt_convert_profile(unsigned char *input, unsigned char *output)
{
	unsigned char *runner = input;
	int i = 0;

	if (!runner || !*runner)
		return false;

	while (runner && *runner) {
		output[i] = ( ASCII_CHR_TO_BYTE(runner[0]) << 4 ) + ASCII_CHR_TO_BYTE(runner[1]);
		i++;
		runner += 2;
	}
	return true;
}

/*
 * Extract an ascii text string of unknown format from a logtrak db line.
 * String must be pointed to its first char.
 * Places the string on the passed variable ref and returns a pointer to the
 * following text to parse.
 */

static std::string get_lt_string(const std::string &input, Lt_String &output)
{
	if (input.empty()){
		return input;
	}
	std::size_t pos = input.find("',");
	if (pos == std::string::npos)
		pos = input.find("')");
	if (pos == std::string::npos){
		return input;
	}
	Lt_String tmp(input.substr(0, pos));
	output = tmp;
	return input.substr(pos + 2);
}

/*
 * Load auxiliary tables data to our vectors. Main T_DIVE table will
 * be parsed in logtrak_import() function.
 */
static void lt_auxiliary_parser(const std::string &buffer)
{
	std::size_t pos = buffer.find("INSERT INTO ");
	std::string runner = buffer.substr(pos);
	while (pos < std::string::npos) {
		std::string line = get_single_line(runner);
		std::size_t lpos = line.find(" VALUES");
		std::string ltable = line.substr(12, lpos - 12);
		lpos = line.find('(');
		line = line.substr(lpos + 1);
		if (ltable == "T_BOTTLE") {
			char *tankmat, *eq_ref;
			int tankvol = 0, o2mix = 0, startp = 0, endp = 0, hemix = 0, tanknum = 0;
			std::sscanf(line.c_str(), "%*[0-9],%d,%d,%d,%d,%m[a-zA-Z0-9-_'],%*d,%*[0-9a-zA-Z'],%d,%*d,%d,%m[0-9],", &tankvol, &o2mix, &startp, &endp, &tankmat, &hemix, &tanknum, &eq_ref);
			Lt_String tmp(tankmat);
			bottle_db.push_back({eq_ref, tankvol, tanknum, startp, endp, o2mix, hemix, tmp.string()});
			free(tankmat);
			free(eq_ref);
		} else if (ltable == "T_EQUIPMENT") {
			Lt_String suit;
			char *eq_ref;
			float weight_s = 0;
			std::sscanf(line.c_str(), "%m[0-9],%*[0-9a-zA-Z'],%*[0-9a-zA-Z' \\],%f", &eq_ref, &weight_s);
			lpos = line.find(",'");
			line = line.substr(lpos + 2);
			get_lt_string(line, suit);
			equipment_db.push_back({eq_ref, suit.string(), weight_s});
			free(eq_ref);
		} else if (ltable == "T_LOCATION") {
			char *id;
			Lt_String name;
			std::sscanf(line.c_str(), "%m[0-9],", &id);
			lpos = line.find(",'");
			line = line.substr(lpos + 2);
			get_lt_string(line, name);
			std::string loc_id(id);
			location_db.push_back({loc_id, name.string()});
			free(id);
		} else if (ltable == "T_SITE") {
			char *id = NULL, *locid = NULL;
			float GPSx = 0, GPSy = 0;
			Lt_String name, comm;
			std::sscanf(line.c_str(), "%m[0-9],", &id);
			lpos = line.find(",'");
			line = line.substr(lpos + 2);
			line = get_lt_string(line, name);
			std::sscanf(line.c_str(),"%f,%f,", &GPSx, &GPSy);
			lpos = line.find(",'");
			line = line.substr(lpos + 2);
			line = get_lt_string(line, comm);
			std::sscanf(line.c_str(), "%*m[a-zA-Z0-9],%m[0-9])", &locid);
			std::string loc_id(locid);
			int i = 0;
			while (location_db[i].loc_id != loc_id)
				i++;
			site_db.push_back({id, name.string(), GPSx, GPSy, comm.string(), {loc_id, location_db[i].loc_name}});
			free(id);
			free(locid);
		} else if (ltable == "TABLE_DBVERSION") {
			Lt_String db_ver;
			lpos = line.find(",'");
			line = line.substr(lpos + 2);
			get_lt_string(line, db_ver);
			db_version = db_ver.string();
		}
		runner = runner.substr(2);
		pos = runner.find("INSERT INTO ");
		if (pos < std::string::npos)
			runner = runner.substr(pos);
	}
}

/*
 * Build tank info for a given dive.
 * AFAIK there is no chance in LogTrak to manually add tanks, so there should
 * be no difference between DC data and divelog data, just the complementary
 * data (e.g. tank volume, tank material ...) not in DC.
 */
static void lt_get_tank_info(char *eq_id, struct dive *ltd)
{
	std::string eqid(eq_id);

	auto it = std::find_if(bottle_db.begin(), bottle_db.end(), [eqid](const T_BOTTLE &bottle) {
		return bottle.equipment_id == eqid;
	});

	const T_BOTTLE &bottle = *it;
	int tanknum = bottle.tanknum - 1;
	if (bottle.tank_vol)
		ltd->get_or_create_cylinder(tanknum)->type.size.mliter = bottle.tank_vol;
	// Always prefer data from DC over data from Logtrak
	if (bottle.startp && !ltd->get_or_create_cylinder(tanknum)->start.mbar)
		ltd->get_or_create_cylinder(tanknum)->start.mbar = bottle.startp * 1000;
	if (bottle.endp && !ltd->get_or_create_cylinder(tanknum)->end.mbar)
		ltd->get_or_create_cylinder(tanknum)->end.mbar = bottle.endp * 1000;
	if (bottle.o2_mix && !ltd->get_or_create_cylinder(tanknum)->gasmix.o2.permille)
		ltd->get_or_create_cylinder(tanknum)->gasmix.o2.permille = bottle.o2_mix * 10;
	if (bottle.he_mix && !ltd->get_or_create_cylinder(tanknum)->gasmix.he.permille)
		ltd->get_or_create_cylinder(tanknum)->gasmix.he.permille = bottle.he_mix * 10;
	if (!bottle.tankmat.empty())
		ltd->get_or_create_cylinder(tanknum)->type.description = bottle.tankmat;
}

/*
 * Build a site for a given dive.
 * Check if it exist, to avoid duplicities.
 */
static void lt_build_dive_site(const char *site_id, struct divelog *log, struct dive_site **lt_site)
{
	/* Abort if we don't have a site_id */
	if (empty_string(site_id)) {
		lt_site = NULL;
		return;
	}

	auto it = std::find_if(site_db.begin(), site_db.end(), [site_id](const T_SITE &site) {
		return site.site_id == site_id;
	});

	if (it == site_db.end()) {
		*lt_site = nullptr;
		return;
	}

	// LogTrak enforces location/site structure, but lazy user may set just one,
	// resulting in the same name for both, location and site. Ensure we just use
	// one in this case.
	const T_SITE &site_data = *it;
	std::string built_name = !site_data.site_name.empty() && site_data.site_name != site_data.site_loc.loc_name ? site_data.site_loc.loc_name + ", " + site_data.site_name : site_data.site_loc.loc_name;
	// build gps position
	location_t gps_loc = create_location(site_data.GPSx, site_data.GPSy);

	// build the dive site structure
	struct dive_site *site = log->sites.get_by_name(built_name);
	if (!site) {
		site = has_location(&gps_loc) ? log->sites.create(built_name, gps_loc) : log->sites.create(built_name);
	}
	if (!site_data.comment.empty())
		site->notes = site_data.comment;

	*lt_site = site;
}

/*
 * Main function.
 * Runs along recived buffer importing T_DIVE data to subsurface's dives.
 * WARNING!! LogTrak supports more than a divelog in a single db, so we may end up
 * with a mixed divelog.
 * Input: a std::string buffer produced by readfile() an a dive_table list.
 * Output: Luckily adds LogTrak dives to the dive_table list.
 * Returns: Integer (0 as default or negative values on error).
 */
int logtrak_import(const std::string &mem, struct divelog *log)
{
	double ltd_temp = 0, ltd_mintemp = 0;
	int ltd_max_depth = 0, dive_count = 0;
	std::string tmpstr;
	std::size_t pos;

	setlocale(LC_NUMERIC, "POSIX");
	setlocale(LC_CTYPE, "");

	// Set auxiliary DBs
	lt_auxiliary_parser(mem);

	pos = mem.find("INSERT INTO T_DIVE ");
	std::string runner = mem.substr(pos);

	while (pos < std::string::npos) {
		char *ltd_id = NULL, *ltd_type = NULL, *ltd_weather = NULL, *ltd_ref_eq = NULL, *ltd_ref_site = NULL,
		     *ltd_dc = NULL, *ltd_dc_id = NULL, *ltd_dive = NULL, *ltd_dc_soft = NULL, *ltd_gf_low = NULL,
		     *ltd_gf_high = NULL, *ltd_log_id = NULL, *ltd_airtemp = NULL;
		auto lt_dive = std::make_unique<dive>();
		auto devdata = std::make_unique<device_data_t>();
		devdata->log = log;
		dive_count++;
		Lt_String ltd_notes;
		int rc;

		tmpstr=get_single_line(runner);
		// break the loop if we can't get a line or empty, something went wrong
		if (tmpstr.empty()) {
			report_error("[LogTrak import] Couldn't get any dive. Check the selected file.");
			return -1;
		}
		lt_dive->number = dive_count;
		pos = tmpstr.find('(');
		tmpstr = tmpstr.substr(pos + 1);
		// Read up to time zone. Discard this one.
		std::sscanf(tmpstr.c_str(), "%m[0-9],'%m[a-zA-Z0-9]',%lf,'%m[a-zA-Z0-9]',%d,%d,]",\
			    &ltd_id, &ltd_type, &ltd_temp, &ltd_weather, &lt_dive->visibility, &lt_dive->rating);

		if (ltd_temp)
			lt_dive->watertemp.mkelvin = C_to_mkelvin(ltd_temp);
		if (!empty_string(ltd_weather)) {
			taglist_add_tag(lt_dive->tags, ltd_weather);
			free(ltd_weather);
		}

		// Move past time zone
		for (int i = 0; i < 3; i++){
			pos = tmpstr.find("',") + 2;
			tmpstr = tmpstr.substr(pos);
		}
		// The notes string will be manually parsed as it can contain every printable caracter
		// and even non printable in unicode format; e.g. "\n" will show as \u000a. The notes
		// string is 256 char long, at most.
		tmpstr = get_lt_string(tmpstr, ltd_notes);
		lt_dive->notes = ltd_notes.string().c_str();
		// Continue with sscanf. Send useless trends to devnull
		// There are, at least two different versions of DB with different order
		if (db_version != "1.3.5")
			rc = std::sscanf(tmpstr.c_str(),"%m[0-9A-Z],%m[0-9A-Z],%*m[-0-9A-Z],%m[0-9A-Z],%m[0-9A-Z],%m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%d,%*m[-0-9A-Z],%lf,%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[-0-9A-Z],%m[0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%m[0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%m[0-9A-Z],%m[0-9A-Z],%m[0-9A-Z]",&ltd_ref_eq, &ltd_ref_site, &ltd_dc, &ltd_dc_id, &ltd_dive, &ltd_max_depth, &ltd_mintemp, &ltd_airtemp, &ltd_dc_soft, &ltd_gf_low, &ltd_gf_high, &ltd_log_id);
		else
			rc = std::sscanf(tmpstr.c_str(),"%m[0-9A-Z],%m[0-9A-Z],%*m[-0-9A-Z],%m[0-9A-Z],%m[0-9A-Z],%m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%d,%*m[-0-9A-Z],%lf,%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%m[0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[-0-9A-Z],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%*m[-0-9A-Z],%*m[-0-9A-Z],%m[0-9A-Z],%*m[0-9A-Za-z'],%*m[0-9A-Za-z'],%m[0-9A-Z],%m[0-9A-Z],%m[0-9A-Z],",&ltd_ref_eq, &ltd_ref_site, &ltd_dc, &ltd_dc_id, &ltd_dive, &ltd_max_depth, &ltd_mintemp, &ltd_airtemp, &ltd_log_id, &ltd_dc_soft, &ltd_gf_low, &ltd_gf_high);
		if (rc < 12) {
			report_error("[LogTrak import] Only %d var parsed by sscanf for dive %d with id %s. Please, check it\n", rc, lt_dive->number, ltd_id);
			runner = runner.substr(pos + 1);
			pos = runner.find("INSERT INTO T_DIVE ");
			runner = runner.substr(pos + 1);
			free(ltd_id);
			continue;
		}
		// Unused ATM
		free(ltd_log_id);
		free(ltd_id);
		// Process DC data
		std::string d(ltd_dive);
		if (!d.empty() && !is_null(d)) {
			d = lt_remove_quotes(d);
			int prf_size = (int)ceil(d.length() / 2);
			std::vector<unsigned char> prf_buffer(prf_size);
			if (!lt_convert_profile(reinterpret_cast<unsigned char *>(d.data()), prf_buffer.data()))
				report_error("[lt_convert_profile] FAILED for dive %d\n", lt_dive->number);
			free(ltd_dive);
			int dc_model = 0;
			if (!empty_string(ltd_dc))
				dc_model = lrint(strtod(ltd_dc, NULL)) & 0xFF;
			free(ltd_dc);
			// No DC_FAMILY_UWATEC_ALADIN coming from LogTrak, they aren't supported there
			devdata->descriptor = get_data_descriptor(dc_model, DC_FAMILY_UWATEC_SMART);
			if (devdata->descriptor) {
				// No need to check vendor or product if we got a correct descriptor
				devdata->vendor = dc_descriptor_get_vendor(devdata->descriptor);
				devdata->product = dc_descriptor_get_product(devdata->descriptor);
				devdata->model = format_string_std("%s %s", devdata->vendor.c_str(), devdata->product.c_str()).c_str();
				lt_dive->dcs[0].model = devdata->model;
				// Galileo TMX devices use a different data set and parsing model in libdc.
				// Libdc checks buffer's byte #43 to know which model to use, and fails if
				// it's not set to 0x80, but has Galileo TMX data set. Thus we need to
				// ensure this byte value, as some Galileo devices didn't set it.
				if (dc_model == 0x19)
					prf_buffer[43] = 0x80;

				libdc_buffer_parser(lt_dive.get(), devdata.get(), prf_buffer.data(), prf_size);

				lt_dive->dcs[0].serial = ltd_dc_id;
				Lt_String soft(ltd_dc_soft);
				lt_dive->dcs[0].fw_version = soft.string().c_str();
				create_device_node(log->devices, lt_dive->dcs[0].model, lt_dive->dcs[0].serial, "");
			} else {
				report_error("Unsuported DC model 0x%X. Dive num %d\n", dc_model, lt_dive->number);
			}
		}
		// Get some DC related data, but always prefer libdc processed data
		if (lt_dive->maxdepth.mm == 0 && ltd_max_depth > 0)
			lt_dive->maxdepth.mm = ltd_max_depth * 10;
		if (ltd_mintemp) {
			if (lt_dive->mintemp.mkelvin == 0)
				lt_dive->mintemp.mkelvin = C_to_mkelvin(ltd_mintemp / 10);
			if (lt_dive->dcs[0].watertemp.mkelvin == 0)
				lt_dive->dcs[0].watertemp.mkelvin = C_to_mkelvin(ltd_mintemp / 10);
		}
		if (lt_dive->airtemp.mkelvin == 0) {
			if (!is_null(ltd_airtemp)) {
				lt_dive->airtemp.mkelvin = C_to_mkelvin(strtod(ltd_airtemp, NULL) / 10);
				free(ltd_airtemp);
			}
		}

		// Get suit and weight, suit may be freely edited field
		if (!empty_string(ltd_ref_eq)) {
			std::string eq_ref(ltd_ref_eq);
			int i = 0;
			while (equipment_db[i].equipment_id != eq_ref)
				i++;
			lt_dive->suit = equipment_db[i].suit;
			if (equipment_db[i].weight > 0) {
				weightsystem_t ws = { { .grams = (int)lroundf(equipment_db[i].weight * 1000) }, translate("gettextFromC", "unknown"), false };
				lt_dive->weightsystems.push_back(std::move(ws));
			}
			// Get tanks info. Tanks are refered to dive via the T_EQUIPMENT id
			lt_get_tank_info(ltd_ref_eq, lt_dive.get());
		}
		free(ltd_ref_eq);

		// Get site/location info. Refered via T_SITE
		lt_build_dive_site(ltd_ref_site, log, &lt_dive->dive_site);
		free(ltd_ref_site);

		// Set some extra data which can be interesting
		add_extra_data(&lt_dive->dcs[0], "LogTrak Version", db_version);
		if (!empty_string(ltd_type))
			add_extra_data(&lt_dive->dcs[0], "DC Type", ltd_type);
		free(ltd_type);
		if (!lt_dive->dcs[0].fw_version.empty() && strcmp(lt_dive->dcs[0].fw_version.c_str(), "0"))
			add_extra_data(&lt_dive->dcs[0], "DC Firmware Version", lt_dive->dcs[0].fw_version);
		Lt_String gfl(ltd_gf_low);
		Lt_String gfh(ltd_gf_high);
		if (gfl.string() != "" && gfl.string() != "0")
			add_extra_data(&lt_dive->dcs[0], "GF Low", gfl.string().c_str());
		if (gfh.string() != "" && gfh.string() != "0")
			add_extra_data(&lt_dive->dcs[0], "GF High", gfh.string().c_str());
		add_extra_data(&lt_dive->dcs[0], "DC Serial", lt_dive->dcs[0].serial);
		free(ltd_gf_low);
		free(ltd_gf_high);

		log->dives.record_dive(std::move(lt_dive));
		runner = runner.substr(pos + 1);
		pos = runner.find("INSERT INTO T_DIVE ");
		runner = runner.substr(pos + 1);
	}

	// Clean DB
	bottle_db.resize(0);
	equipment_db.resize(0);
	location_db.resize(0);
	site_db.resize(0);
	return 0;
}
