#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <libdivecomputer/parser.h>

#include "dive.h"
#include "errorhelper.h"
#include "ssrf.h"
#include "subsurface-string.h"
#include "divelist.h"
#include "divelog.h"
#include "file.h"
#include "format.h"
#include "parse.h"
#include "sample.h"
#include "divelist.h"
#include "gettext.h"
#include "import-csv.h"
#include "qthelper.h"
#include "xmlparams.h"

#define MATCH(buffer, pattern) \
	memcmp(buffer, pattern, strlen(pattern))

static timestamp_t parse_date(const char *date)
{
	int hour, min, sec;
	struct tm tm;
	char *p;

	memset(&tm, 0, sizeof(tm));
	tm.tm_mday = strtol(date, &p, 10);
	if (tm.tm_mday < 1 || tm.tm_mday > 31)
		return 0;
	for (tm.tm_mon = 0; tm.tm_mon < 12; tm.tm_mon++) {
		if (!memcmp(p, monthname(tm.tm_mon), 3))
			break;
	}
	if (tm.tm_mon > 11)
		return 0;
	date = p + 3;
	tm.tm_year = strtol(date, &p, 10);
	if (date == p)
		return 0;
	if (tm.tm_year < 70)
		tm.tm_year += 2000;
	if (tm.tm_year < 100)
		tm.tm_year += 1900;
	if (sscanf(p, "%d:%d:%d", &hour, &min, &sec) != 3)
		return 0;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;
	return utc_mktime(&tm);
}

static void add_sample_data(struct sample *sample, enum csv_format type, double val)
{
	switch (type) {
	case CSV_DEPTH:
		sample->depth.mm = feet_to_mm(val);
		break;
	case CSV_TEMP:
		sample->temperature.mkelvin = F_to_mkelvin(val);
		break;
	case CSV_PRESSURE:
		sample->pressure[0].mbar = psi_to_mbar(val * 4);
		break;
	case POSEIDON_DEPTH:
		sample->depth.mm = lrint(val * 0.5 * 1000);
		break;
	case POSEIDON_TEMP:
		sample->temperature.mkelvin = C_to_mkelvin(val * 0.2);
		break;
	case POSEIDON_SETPOINT:
		sample->setpoint.mbar = lrint(val * 10);
		break;
	case POSEIDON_SENSOR1:
		sample->o2sensor[0].mbar = lrint(val * 10);
		break;
	case POSEIDON_SENSOR2:
		sample->o2sensor[1].mbar = lrint(val * 10);
		break;
	case POSEIDON_NDL:
		sample->ndl.seconds = lrint(val * 60);
		break;
	case POSEIDON_CEILING:
		sample->stopdepth.mm = lrint(val * 1000);
		break;
	}
}

static char *parse_dan_new_line(char *buf, const char *NL)
{
	char *iter = buf;

	if (!iter)
		return NULL;

	iter = strstr(iter, NL);
	if (iter) {
		iter += strlen(NL);
	} else {
		report_info("DEBUG: No new line found");
		return NULL;
	}
	return iter;
}

static int try_to_xslt_open_csv(const char *filename, std::string &mem, const char *tag);
static int parse_dan_format(const char *filename, struct xml_params *params, struct divelog *log)
{
	int ret = 0, i;
	size_t end_ptr = 0;
	char tmpbuf[MAXCOLDIGITS];
	int params_orig_size = xml_params_count(params);

	char *ptr = NULL;
	const char *NL = NULL;
	char *iter = NULL;

	auto [mem, err] = readfile(filename);
	if (err < 0)
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);

	/* Determine NL (new line) character and the start of CSV data */
	if ((ptr = strstr(mem.data(), "\r\n")) != NULL) {
		NL = "\r\n";
	} else if ((ptr = strstr(mem.data(), "\n")) != NULL) {
		NL = "\n";
	} else {
		report_info("DEBUG: failed to detect NL");
		return -1;
	}

	while ((end_ptr < mem.size()) && (ptr = strstr(mem.data() + end_ptr, "ZDH"))) {
		xml_params_resize(params, params_orig_size); // restart with original parameter block
		char *iter_end = NULL;

		iter = ptr + 4;
		iter = strchr(iter, '|');
		if (iter) {
			memcpy(tmpbuf, ptr + 4, iter - ptr - 4);
			tmpbuf[iter - ptr - 4] = 0;
			xml_params_add(params, "diveNro", tmpbuf);
		}

		//report_info("DEBUG: BEGIN end_ptr %d round %d <%s>", end_ptr, j++, ptr);
		iter = ptr + 1;
		for (i = 0; i <= 4 && iter; ++i) {
			iter = strchr(iter, '|');
			if (iter)
				++iter;
		}

		if (!iter) {
			report_info("DEBUG: Data corrupt");
			return -1;
		}

		/* Setting date */
		memcpy(tmpbuf, iter, 8);
		tmpbuf[8] = 0;
		xml_params_add(params, "date", tmpbuf);

		/* Setting time, gotta prepend it with 1 to
		 * avoid octal parsing (this is stripped out in
		 * XSLT */
		tmpbuf[0] = '1';
		memcpy(tmpbuf + 1, iter + 8, 6);
		tmpbuf[7] = 0;
		xml_params_add(params, "time", tmpbuf);

		/* Air temperature */
		memset(tmpbuf, 0, sizeof(tmpbuf));
		iter = strchr(iter, '|');

		if (iter) {
			iter = iter + 1;
			iter_end = strchr(iter, '|');

			if (iter_end) {
				memcpy(tmpbuf, iter, iter_end - iter);
				xml_params_add(params, "airTemp", tmpbuf);
			}
		}

		/* Search for the next line */
		if (iter)
			iter = parse_dan_new_line(iter, NL);
		if (!iter)
			return -1;

		/* We got a trailer, no samples on this dive */
		if (strncmp(iter, "ZDT", 3) == 0) {
			end_ptr = iter - mem.data();

			/* Water temperature */
			memset(tmpbuf, 0, sizeof(tmpbuf));
			for (i = 0; i < 5 && iter; ++i)
				iter = strchr(iter + 1, '|');

			if (iter) {
				iter = iter + 1;
				iter_end = strchr(iter, '|');

				if (iter_end) {
					memcpy(tmpbuf, iter, iter_end - iter);
					xml_params_add(params, "waterTemp", tmpbuf);
				}
			}
			ret |= parse_xml_buffer(filename, "<csv></csv>", 11, log, params);
			continue;
		}

		/* After ZDH we should get either ZDT (above) or ZDP */
		if (strncmp(iter, "ZDP{", 4) != 0) {
			report_info("DEBUG: Input appears to violate DL7 specification");
			end_ptr = iter - mem.data();
			continue;
		}

		if (ptr && ptr[4] == '}')
			return report_error(translate("gettextFromC", "No dive profile found from '%s'"), filename);

		if (ptr)
			ptr = parse_dan_new_line(ptr, NL);
		if (!ptr)
			return -1;

		end_ptr = ptr - mem.data();

		/* Copy the current dive data to start of mem_csv buffer */
		std::string mem_csv(ptr,  mem.size() - (ptr - mem.data()));

		ptr = strstr(mem_csv.data(), "ZDP}");
		if (ptr) {
			*ptr = 0;
		} else {
			report_info("DEBUG: failed to find end ZDP");
			return -1;
		}
		mem_csv.resize(ptr - mem_csv.data());
		end_ptr += ptr - mem_csv.data();

		iter = parse_dan_new_line(ptr + 1, NL);
		if (iter && strncmp(iter, "ZDT", 3) == 0) {
			/* Water temperature */
			memset(tmpbuf, 0, sizeof(tmpbuf));
			for (i = 0; i < 5 && iter; ++i)
				iter = strchr(iter + 1, '|');

			if (iter) {
				iter = iter + 1;
				iter_end = strchr(iter, '|');

				if (iter_end) {
					memcpy(tmpbuf, iter, iter_end - iter);
					xml_params_add(params, "waterTemp", tmpbuf);
				}
			}
		}

		if (try_to_xslt_open_csv(filename, mem_csv, "csv"))
			return -1;

		ret |= parse_xml_buffer(filename, mem_csv.data(), mem_csv.size(), log, params);
	}

	return ret;
}

extern "C" int parse_csv_file(const char *filename, struct xml_params *params, const char *csvtemplate, struct divelog *log)
{
	int ret;
	std::string mem;
	time_t now;
	struct tm *timep = NULL;
	char tmpbuf[MAXCOLDIGITS];

	/* Increase the limits for recursion and variables on XSLT
	 * parsing */
	xsltMaxDepth = 30000;
#if LIBXSLT_VERSION > 10126
	xsltMaxVars = 150000;
#endif

	if (filename == NULL)
		return report_error("No CSV filename");

	if (!strcmp("DL7", csvtemplate)) {
		return parse_dan_format(filename, params, log);
	} else if (strcmp(xml_params_get_key(params, 0), "date")) {
		time(&now);
		timep = localtime(&now);

		strftime(tmpbuf, MAXCOLDIGITS, "%Y%m%d", timep);
		xml_params_add(params, "date", tmpbuf);

		/* As the parameter is numeric, we need to ensure that the leading zero
		 * is not discarded during the transform, thus prepend time with 1 */
		strftime(tmpbuf, MAXCOLDIGITS, "1%H%M", timep);
		xml_params_add(params, "time", tmpbuf);
	}

	if (try_to_xslt_open_csv(filename, mem, csvtemplate))
		return -1;

	/*
	 * Lets print command line for manual testing with xsltproc if
	 * verbosity level is high enough. The printed line needs the
	 * input file added as last parameter.
	 */

#ifndef SUBSURFACE_MOBILE
	if (verbose >= 2) {
		std::string info = format_string_std("(echo '<csv>'; cat %s;echo '</csv>') | xsltproc ", filename);
		for (int i = 0; i < xml_params_count(params); i++)
			info += format_string_std("--stringparam %s %s ", xml_params_get_key(params, i), xml_params_get_value(params, i));
		info += format_string_std("%s/xslt/%s -", SUBSURFACE_SOURCE, csvtemplate);
		report_info("%s", info.c_str());
	}
#endif
	ret = parse_xml_buffer(filename, mem.data(), mem.size(), log, params);

	return ret;
}


static int try_to_xslt_open_csv(const char *filename, std::string &mem, const char *tag)
{
	size_t amp = 0;

	if (mem.empty()) {
		auto [mem2, err] = readfile(filename);
		if (err < 0)
			return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);
		if (mem2.empty())
			return 0; // Empty file - nothing to do. Guess that's a "success".
		mem = std::move(mem2);
	}

	/* Count ampersand characters */
	for (size_t i = 0; i < mem.size(); ++i) {
		if (mem[i] == '&') {
			++amp;
		}
	}


	/* Surround the CSV file content with XML tags to enable XSLT
	 * parsing
	 *
	 * Tag markers take: strlen("<></>") = 5
	 * Reserve also room for encoding ampersands "&" => "&amp;"
	 *
	 * Attention: This code is quite subtle, because we reserve one
	 * more byte than we use and put a '\0' there.
	 */

	size_t tag_name_size = strlen(tag);
	size_t old_size = mem.size();
	mem.resize(mem.size() + tag_name_size * 2 + 5 + amp * 4);
	const char *ptr_in = mem.data() + old_size;
	char *ptr_out = mem.data() + mem.size();

	/* Add end tag */
	*--ptr_out = '>';
	ptr_out -= tag_name_size;
	memcpy(ptr_out, tag, tag_name_size);
	*--ptr_out = '/';
	*--ptr_out = '<';

	while (--ptr_in >= mem.data()) {
		if (*ptr_in == '&') {
			*--ptr_out = ';';
			*--ptr_out = 'p';
			*--ptr_out = 'm';
			*--ptr_out = 'a';
		}
		*--ptr_out = *ptr_in;
	}

	/* Add start tag */
	*--ptr_out = '>';
	ptr_out -= tag_name_size;
	memcpy(ptr_out, tag, tag_name_size);
	*--ptr_out = '<';

	// On Windows, ptrdiff_t is long long int, on Linux it is long int.
	// Windows doesn't support the ptrdiff_t format specifier "%td", so
	// let's cast to long int.
	if (ptr_out != mem.data())
		report_info("try_to_xslt_open_csv(): ptr_out off by %ld. This shouldn't happen", static_cast<long int>(ptr_out - mem.data()));

	return 0;
}

int try_to_open_csv(std::string &mem, enum csv_format type, struct divelog *log)
{
	char *p = mem.data();
	char *header[8];
	int i, time;
	timestamp_t date;
	struct dive *dive;
	struct divecomputer *dc;

	for (i = 0; i < 8; i++) {
		header[i] = p;
		p = strchr(p, ',');
		if (!p)
			return 0;
		p++;
	}

	date = parse_date(header[2]);
	if (!date)
		return 0;

	dive = alloc_dive();
	dive->when = date;
	dive->number = atoi(header[1]);
	dc = &dive->dc;

	time = 0;
	for (;;) {
		char *end;
		double val;
		struct sample *sample;

		errno = 0;
		val = strtod(p, &end); // FIXME == localization issue
		if (end == p)
			break;
		if (errno)
			break;

		sample = prepare_sample(dc);
		sample->time.seconds = time;
		add_sample_data(sample, type, val);
		finish_sample(dc);

		time++;
		dc->duration.seconds = time;
		if (*end != ',')
			break;
		p = end + 1;
	}
	record_dive_to_table(dive, log->dives);
	return 1;
}

static std::string parse_mkvi_value(const char *haystack, const char *needle)
{
	const char *lineptr, *valueptr, *endptr;

	if ((lineptr = strstr(haystack, needle)) != NULL) {
		if ((valueptr = strstr(lineptr, ": ")) != NULL) {
			valueptr += 2;

			if ((endptr = strstr(lineptr, "\n")) != NULL) {
				if (*(endptr - 1) == '\r')
					--endptr;
				return std::string(valueptr, endptr - valueptr);
			}
		}
	}
	return std::string();
}

static std::string next_mkvi_key(const char *haystack)
{
	const char *valueptr, *endptr;

	if ((valueptr = strstr(haystack, "\n")) != NULL) {
		valueptr += 1;
		if ((endptr = strstr(valueptr, ": ")) != NULL)
			return std::string(valueptr, endptr - valueptr);
	}
	return std::string();
}

int parse_txt_file(const char *filename, const char *csv, struct divelog *log)
{
	auto [memtxt, err] = readfile(filename);
	if (err < 0)
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);

	/*
	 * MkVI stores some information in .txt file but the whole profile and events are stored in .csv file. First
	 * make sure the input .txt looks like proper MkVI file, then start parsing the .csv.
	 */
	if (MATCH(memtxt.data(), "MkVI_Config") == 0) {
		int d, m, y, he;
		int hh = 0, mm = 0, ss = 0;
		int prev_depth = 0, cur_sampletime = 0, prev_setpoint = -1, prev_ndl = -1;
		bool has_depth = false, has_setpoint = false, has_ndl = false;
		char *lineptr;
		int prev_time = 0;
		cylinder_t cyl = empty_cylinder;

		struct dive *dive;
		struct divecomputer *dc;
		struct tm cur_tm;

		std::string value = parse_mkvi_value(memtxt.data(), "Dive started at");
		if (sscanf(value.c_str(), "%d-%d-%d %d:%d:%d", &y, &m, &d, &hh, &mm, &ss) != 6)
			return -1;
		cur_tm.tm_year = y;
		cur_tm.tm_mon = m - 1;
		cur_tm.tm_mday = d;
		cur_tm.tm_hour = hh;
		cur_tm.tm_min = mm;
		cur_tm.tm_sec = ss;

		dive = alloc_dive();
		dive->when = utc_mktime(&cur_tm);;
		dive->dc.model = strdup("Poseidon MkVI Discovery");
		value = parse_mkvi_value(memtxt.data(), "Rig Serial number");
		dive->dc.deviceid = atoi(value.c_str());
		dive->dc.divemode = CCR;
		dive->dc.no_o2sensors = 2;

		cyl.cylinder_use = OXYGEN;
		cyl.type.size.mliter = 3000;
		cyl.type.workingpressure.mbar = 200000;
		cyl.type.description = "3l Mk6";
		cyl.gasmix.o2.permille = 1000;
		cyl.manually_added = true;
		cyl.bestmix_o2 = 0;
		cyl.bestmix_he = 0;
		add_cloned_cylinder(&dive->cylinders, cyl);

		cyl.cylinder_use = DILUENT;
		cyl.type.size.mliter = 3000;
		cyl.type.workingpressure.mbar = 200000;
		cyl.type.description = "3l Mk6";
		value = parse_mkvi_value(memtxt.data(), "Helium percentage");
		he = atoi(value.c_str());
		value = parse_mkvi_value(memtxt.data(), "Nitrogen percentage");
		cyl.gasmix.o2.permille = (100 - atoi(value.c_str()) - he) * 10;
		cyl.gasmix.he.permille = he * 10;
		add_cloned_cylinder(&dive->cylinders, cyl);

		lineptr = strstr(memtxt.data(), "Dive started at");
		while (!empty_string(lineptr) && (lineptr = strchr(lineptr, '\n'))) {
			++lineptr;	// Skip over '\n'
			std::string key = next_mkvi_key(lineptr);
			if (key.empty())
				break;
			std::string value = parse_mkvi_value(lineptr, key.c_str());
			if (value.empty())
				break;
			add_extra_data(&dive->dc, key.c_str(), value.c_str());
		}
		dc = &dive->dc;

		/*
		 * Read samples from the CSV file. A sample contains all the lines with same timestamp. The CSV file has
		 * the following format:
		 *
		 * timestamp, type, value
		 *
		 * And following fields are of interest to us:
		 *
		 * 	6	sensor1
		 * 	7	sensor2
		 * 	8	depth
		 *	13	o2 tank pressure
		 *	14	diluent tank pressure
		 *	20	o2 setpoint
		 *	39	water temp
		 */

		auto [memcsv, err] = readfile(csv);
		if (err < 0) {
			free_dive(dive);
			return report_error(translate("gettextFromC", "Poseidon import failed: unable to read '%s'"), csv);
		}
		lineptr = memcsv.data();
		for (;;) {
			struct sample *sample;
			int type;
			int value;
			int sampletime;
			int gaschange = 0;

			/* Collect all the information for one sample */
			sscanf(lineptr, "%d,%d,%d", &cur_sampletime, &type, &value);

			has_depth = false;
			has_setpoint = false;
			has_ndl = false;
			sample = prepare_sample(dc);

			/*
			 * There was a bug in MKVI download tool that resulted in erroneous sample
			 * times. This fix should work similarly as the vendor's own.
			 */

			sample->time.seconds = cur_sampletime < 0xFFFF * 3 / 4 ? cur_sampletime : prev_time;
			prev_time = sample->time.seconds;

			do {
				int i = sscanf(lineptr, "%d,%d,%d", &sampletime, &type, &value);
				switch (i) {
				case 3:
					switch (type) {
					case 0:
						//Mouth piece position event: 0=OC, 1=CC, 2=UN, 3=NC
						switch (value) {
						case 0:
							add_event(dc, cur_sampletime, 0, 0, 0,
									QT_TRANSLATE_NOOP("gettextFromC", "Mouth piece position OC"));
							break;
						case 1:
							add_event(dc, cur_sampletime, 0, 0, 0,
									QT_TRANSLATE_NOOP("gettextFromC", "Mouth piece position CC"));
							break;
						case 2:
							add_event(dc, cur_sampletime, 0, 0, 0,
									QT_TRANSLATE_NOOP("gettextFromC", "Mouth piece position unknown"));
							break;
						case 3:
							add_event(dc, cur_sampletime, 0, 0, 0,
									QT_TRANSLATE_NOOP("gettextFromC", "Mouth piece position not connected"));
							break;
						}
						break;
					case 3:
						//Power Off event
						add_event(dc, cur_sampletime, 0, 0, 0,
								QT_TRANSLATE_NOOP("gettextFromC", "Power off"));
						break;
					case 4:
						//Battery State of Charge in %
#ifdef SAMPLE_EVENT_BATTERY
						add_event(dc, cur_sampletime, SAMPLE_EVENT_BATTERY, 0,
								value, QT_TRANSLATE_NOOP("gettextFromC", "battery"));
#endif
						break;
					case 6:
						//PO2 Cell 1 Average
						add_sample_data(sample, POSEIDON_SENSOR1, value);
						break;
					case 7:
						//PO2 Cell 2 Average
						add_sample_data(sample, POSEIDON_SENSOR2, value);
						break;
					case 8:
						//Depth * 2
						has_depth = true;
						prev_depth = value;
						add_sample_data(sample, POSEIDON_DEPTH, value);
						break;
						//9 Max Depth * 2
						//10 Ascent/Descent Rate * 2
					case 11:
						//Ascent Rate Alert >10 m/s
						add_event(dc, cur_sampletime, SAMPLE_EVENT_ASCENT, 0, 0,
								QT_TRANSLATE_NOOP("gettextFromC", "ascent"));
						break;
					case 13:
						//O2 Tank Pressure
						add_sample_pressure(sample, 0, lrint(value * 1000));
						break;
					case 14:
						//Diluent Tank Pressure
						add_sample_pressure(sample, 1, lrint(value * 1000));
						break;
						//16 Remaining dive time #1?
						//17 related to O2 injection
					case 20:
						//PO2 Setpoint
						has_setpoint = true;
						prev_setpoint = value;
						add_sample_data(sample, POSEIDON_SETPOINT, value);
						break;
					case 22:
						//End of O2 calibration Event: 0 = OK, 2 = Failed, rest of dive setpoint 1.0
						if (value == 2)
							add_event(dc, cur_sampletime, 0, SAMPLE_FLAGS_END, 0,
									QT_TRANSLATE_NOOP("gettextFromC", "O₂ calibration failed"));
						add_event(dc, cur_sampletime, 0, SAMPLE_FLAGS_END, 0,
								QT_TRANSLATE_NOOP("gettextFromC", "O₂ calibration"));
						break;
					case 25:
						//25 Max Ascent depth
						add_sample_data(sample, POSEIDON_CEILING, value);
						break;
					case 31:
						//Start of O2 calibration Event
						add_event(dc, cur_sampletime, 0, SAMPLE_FLAGS_BEGIN, 0,
								QT_TRANSLATE_NOOP("gettextFromC", "O₂ calibration"));
						break;
					case 37:
						//Remaining dive time #2?
						has_ndl = true;
						prev_ndl = value;
						add_sample_data(sample, POSEIDON_NDL, value);
						break;
					case 39:
						// Water Temperature in Celsius
						add_sample_data(sample, POSEIDON_TEMP, value);
						break;
					case 85:
						//He diluent part in %
						gaschange += value << 16;
						break;
					case 86:
						//O2 diluent part in %
						gaschange += value;
						break;
						//239 Unknown, maybe PO2 at sensor validation?
						//240 Unknown, maybe PO2 at sensor validation?
						//247 Unknown, maybe PO2 Cell 1 during pressure test
						//248 Unknown, maybe PO2 Cell 2 during pressure test
						//250 PO2 Cell 1
						//251 PO2 Cell 2
					default:
						break;
					} /* sample types */
					break;
				case EOF:
					break;
				default:
					report_info("Unable to parse input: %s\n", lineptr);
					break;
				}

				lineptr = strchr(lineptr, '\n');
				if (!lineptr || !*lineptr)
					break;
				lineptr++;

				/* Grabbing next sample time */
				sscanf(lineptr, "%d,%d,%d", &cur_sampletime, &type, &value);
			} while (sampletime == cur_sampletime);

			if (gaschange)
				add_event(dc, cur_sampletime, SAMPLE_EVENT_GASCHANGE2, 0, gaschange,
						QT_TRANSLATE_NOOP("gettextFromC", "gaschange"));
			if (!has_depth)
				add_sample_data(sample, POSEIDON_DEPTH, prev_depth);
			if (!has_setpoint && prev_setpoint >= 0)
				add_sample_data(sample, POSEIDON_SETPOINT, prev_setpoint);
			if (!has_ndl && prev_ndl >= 0)
				add_sample_data(sample, POSEIDON_NDL, prev_ndl);
			finish_sample(dc);

			if (!lineptr || !*lineptr)
				break;
		}
		record_dive_to_table(dive, log->dives);
		return 1;
	} else {
		return 0;
	}

	return 0;
}

#define DATESTR 9
#define TIMESTR 6

#define SBPARAMS 40
static int parse_seabear_csv_file(const char *filename, struct xml_params *params, const char *csvtemplate, struct divelog *log);
int parse_seabear_log(const char *filename, struct divelog *log)
{
	struct xml_params *params = alloc_xml_params();
	int ret;

	parse_seabear_header(filename, params);
	ret = parse_seabear_csv_file(filename, params, "csv", log) < 0 ? -1 : 0;

	free_xml_params(params);

	return ret;
}


static int parse_seabear_csv_file(const char *filename, struct xml_params *params, const char *csvtemplate, struct divelog *log)
{
	int ret, i;
	time_t now;
	struct tm *timep = NULL;
	char *ptr, *ptr_old = NULL;
	const char *NL = NULL;
	char tmpbuf[MAXCOLDIGITS];

	/* Increase the limits for recursion and variables on XSLT
	 * parsing */
	xsltMaxDepth = 30000;
#if LIBXSLT_VERSION > 10126
	xsltMaxVars = 150000;
#endif

	time(&now);
	timep = localtime(&now);

	strftime(tmpbuf, MAXCOLDIGITS, "%Y%m%d", timep);
	xml_params_add(params, "date", tmpbuf);

	/* As the parameter is numeric, we need to ensure that the leading zero
	* is not discarded during the transform, thus prepend time with 1 */
	strftime(tmpbuf, MAXCOLDIGITS, "1%H%M", timep);
	xml_params_add(params, "time", tmpbuf);

	if (filename == NULL)
		return report_error("No CSV filename");

	auto [mem, err] = readfile(filename);
	if (err < 0)
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);

	/* Determine NL (new line) character and the start of CSV data */
	ptr = (char *)mem.data();
	while ((ptr = strstr(ptr, "\r\n\r\n")) != NULL) {
		ptr_old = ptr;
		ptr += 1;
		NL = "\r\n";
	}

	if (!ptr_old) {
		ptr = (char *)mem.data();
		while ((ptr = strstr(ptr, "\n\n")) != NULL) {
			ptr_old = ptr;
			ptr += 1;
			NL = "\n";
		}
		ptr_old += 2;
	} else {
		ptr_old += 4;
	}

	/*
	 * If file does not contain empty lines, it is not a valid
	 * Seabear CSV file.
	 */
	if (NL == NULL)
		return -1;

	/*
	 * On my current sample of Seabear DC log file, the date is
	 * without any identifier. Thus we must search for the previous
	 * line and step through from there. That is the line after
	 * Serial number.
	 */
	ptr = strstr((char *)mem.data(), "Serial number:");
	if (ptr)
		ptr = strstr(ptr, NL);

	/*
	 * Write date and time values to params array, if available in
	 * the CSV header
	 */

	if (ptr) {
		/*
		 * The two last entries should be date and time.
		 * Here we overwrite them with the data from the
		 * CSV header.
		 */
		char buf[10];

		ptr += strlen(NL) + 2;
		memcpy(buf, ptr, 4);
		memcpy(buf + 4, ptr + 5, 2);
		memcpy(buf + 6, ptr + 8, 2);
		buf[8] = 0;
		xml_params_set_value(params, xml_params_count(params) - 2, buf);

		buf[0] = xml_params_get_value(params, xml_params_count(params) - 1)[0];
		memcpy(buf + 1, ptr + 11, 2);
		memcpy(buf + 3, ptr + 14, 2);
		buf[5] = 0;
		xml_params_set_value(params, xml_params_count(params) - 1, buf);
	}

	/* Move the CSV data to the start of mem buffer */
	memmove(mem.data(), ptr_old, mem.size() - (ptr_old - mem.data()));
	mem.resize(mem.size() - (ptr_old - mem.data()));

	if (try_to_xslt_open_csv(filename, mem, csvtemplate))
		return -1;

	/*
	 * Lets print command line for manual testing with xsltproc if
	 * verbosity level is high enough. The printed line needs the
	 * input file added as last parameter.
	 */

	if (verbose >= 2) {
		std::string info = "xsltproc ";
		for (i = 0; i < xml_params_count(params); i++)
			info += format_string_std("--stringparam %s %s ", xml_params_get_key(params, i), xml_params_get_value(params, i));
		info += "xslt/csv2xml.xslt";
		report_info("%s", info.c_str());
	}

	ret = parse_xml_buffer(filename, mem.data(), mem.size(), log, params);

	return ret;
}

int parse_manual_file(const char *filename, struct xml_params *params, struct divelog *log)
{
	std::string mem;
	time_t now;
	struct tm *timep;
	char curdate[9];
	char curtime[6];
	int ret;


	time(&now);
	timep = localtime(&now);
	strftime(curdate, DATESTR, "%Y%m%d", timep);

	/* As the parameter is numeric, we need to ensure that the leading zero
	* is not discarded during the transform, thus prepend time with 1 */
	strftime(curtime, TIMESTR, "1%H%M", timep);

	xml_params_add(params, "date", curdate);
	xml_params_add(params, "time", curtime);

	if (filename == NULL)
		return report_error("No manual CSV filename");

	if (try_to_xslt_open_csv(filename, mem, "manualCSV"))
		return -1;

#ifndef SUBSURFACE_MOBILE
	if (verbose >= 2) {
		std::string info = format_string_std("(echo '<manualCSV>'; cat %s;echo '</manualCSV>') | xsltproc ", filename);
		for (int i = 0; i < xml_params_count(params); i++)
			info += format_string_std("--stringparam %s %s ", xml_params_get_key(params, i), xml_params_get_value(params, i));
		info += format_string_std("%s/xslt/manualcsv2xml.xslt -", SUBSURFACE_SOURCE);
		report_info("%s", info.c_str());
	}
#endif
	ret = parse_xml_buffer(filename, mem.data(), mem.size(), log, params);

	return ret;
}
