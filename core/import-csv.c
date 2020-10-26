#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <libdivecomputer/parser.h>

#include "dive.h"
#include "errorhelper.h"
#include "ssrf.h"
#include "subsurface-string.h"
#include "divelist.h"
#include "file.h"
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
		fprintf(stderr, "DEBUG: No new line found\n");
		return NULL;
	}
	return iter;
}

static int try_to_xslt_open_csv(const char *filename, struct memblock *mem, const char *tag);
static int parse_dan_format(const char *filename, struct xml_params *params, struct dive_table *table,
			    struct trip_table *trips, struct dive_site_table *sites, struct device_table *devices,
			    struct filter_preset_table *filter_presets)
{
	int ret = 0, i;
	size_t end_ptr = 0;
	struct memblock mem, mem_csv;
	char tmpbuf[MAXCOLDIGITS];
	int params_orig_size = xml_params_count(params);

	char *ptr = NULL;
	const char *NL = NULL;
	char *iter = NULL;

	if (readfile(filename, &mem) < 0)
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);

	/* Determine NL (new line) character and the start of CSV data */
	if ((ptr = strstr(mem.buffer, "\r\n")) != NULL) {
		NL = "\r\n";
	} else if ((ptr = strstr(mem.buffer, "\n")) != NULL) {
		NL = "\n";
	} else {
		fprintf(stderr, "DEBUG: failed to detect NL\n");
		return -1;
	}

	while ((end_ptr < mem.size) && (ptr = strstr(mem.buffer + end_ptr, "ZDH"))) {
		xml_params_resize(params, params_orig_size); // restart with original parameter block
		char *iter_end = NULL;

		mem_csv.buffer = malloc(mem.size + 1);
		mem_csv.size = mem.size;

		iter = ptr + 4;
		iter = strchr(iter, '|');
		if (iter) {
			memcpy(tmpbuf, ptr + 4, iter - ptr - 4);
			tmpbuf[iter - ptr - 4] = 0;
			xml_params_add(params, "diveNro", tmpbuf);
		}

		//fprintf(stderr, "DEBUG: BEGIN end_ptr %d round %d <%s>\n", end_ptr, j++, ptr);
		iter = ptr + 1;
		for (i = 0; i <= 4 && iter; ++i) {
			iter = strchr(iter, '|');
			if (iter)
				++iter;
		}

		if (!iter) {
			fprintf(stderr, "DEBUG: Data corrupt");
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

		if (iter && iter + 1) {
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
			end_ptr = iter - (char *)mem.buffer;

			/* Water temperature */
			memset(tmpbuf, 0, sizeof(tmpbuf));
			for (i = 0; i < 5 && iter; ++i)
				iter = strchr(iter + 1, '|');

			if (iter && iter + 1) {
				iter = iter + 1;
				iter_end = strchr(iter, '|');

				if (iter_end) {
					memcpy(tmpbuf, iter, iter_end - iter);
					xml_params_add(params, "waterTemp", tmpbuf);
				}
			}
			ret |= parse_xml_buffer(filename, "<csv></csv>", 11, table, trips, sites, devices, filter_presets, params);
			continue;
		}

		/* After ZDH we should get either ZDT (above) or ZDP */
		if (strncmp(iter, "ZDP{", 4) != 0) {
			fprintf(stderr, "DEBUG: Input appears to violate DL7 specification\n");
			end_ptr = iter - (char *)mem.buffer;
			continue;
		}

		if (ptr && ptr[4] == '}') {
			end_ptr += ptr - (char *)mem_csv.buffer;
			return report_error(translate("gettextFromC", "No dive profile found from '%s'"), filename);
		}

		if (ptr)
			ptr = parse_dan_new_line(ptr, NL);
		if (!ptr)
			return -1;

		end_ptr = ptr - (char *)mem.buffer;

		/* Copy the current dive data to start of mem_csv buffer */
		memcpy(mem_csv.buffer, ptr, mem.size - (ptr - (char *)mem.buffer));
		ptr = strstr(mem_csv.buffer, "ZDP}");
		if (ptr) {
			*ptr = 0;
		} else {
			fprintf(stderr, "DEBUG: failed to find end ZDP\n");
			return -1;
		}
		mem_csv.size = ptr - (char*)mem_csv.buffer;

		iter = parse_dan_new_line(ptr + 1, NL);
		if (iter && strncmp(iter, "ZDT", 3) == 0) {
			/* Water temperature */
			memset(tmpbuf, 0, sizeof(tmpbuf));
			for (i = 0; i < 5 && iter; ++i)
				iter = strchr(iter + 1, '|');

			if (iter && iter + 1) {
				iter = iter + 1;
				iter_end = strchr(iter, '|');

				if (iter_end) {
					memcpy(tmpbuf, iter, iter_end - iter);
					xml_params_add(params, "waterTemp", tmpbuf);
				}
			}
		}

		if (try_to_xslt_open_csv(filename, &mem_csv, "csv"))
			return -1;

		ret |= parse_xml_buffer(filename, mem_csv.buffer, mem_csv.size, table, trips, sites, devices, filter_presets, params);
		end_ptr += ptr - (char *)mem_csv.buffer;
		free(mem_csv.buffer);
	}

	free(mem.buffer);

	return ret;
}

int parse_csv_file(const char *filename, struct xml_params *params, const char *csvtemplate,
		   struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
		   struct device_table *devices, struct filter_preset_table *filter_presets)
{
	int ret;
	struct memblock mem;
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

	mem.size = 0;
	if (!strcmp("DL7", csvtemplate)) {
		return parse_dan_format(filename, params, table, trips, sites, devices, filter_presets);
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

	if (try_to_xslt_open_csv(filename, &mem, csvtemplate))
		return -1;

	/*
	 * Lets print command line for manual testing with xsltproc if
	 * verbosity level is high enough. The printed line needs the
	 * input file added as last parameter.
	 */

#ifndef SUBSURFACE_MOBILE
	if (verbose >= 2) {
		fprintf(stderr, "(echo '<csv>'; cat %s;echo '</csv>') | xsltproc ", filename);
		for (int i = 0; i < xml_params_count(params); i++)
			fprintf(stderr, "--stringparam %s %s ", xml_params_get_key(params, i), xml_params_get_value(params, i));
		fprintf(stderr, "%s/xslt/%s -\n", SUBSURFACE_SOURCE, csvtemplate);
	}
#endif
	ret = parse_xml_buffer(filename, mem.buffer, mem.size, table, trips, sites, devices, filter_presets, params);

	free(mem.buffer);

	return ret;
}


static int try_to_xslt_open_csv(const char *filename, struct memblock *mem, const char *tag)
{
	char *buf;
	size_t i, amp = 0, rest = 0;

	if (mem->size == 0 && readfile(filename, mem) < 0)
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);

	/* Count ampersand characters */
	for (i = 0; i < mem->size; ++i) {
		if (((char *)mem->buffer)[i] == '&') {
			++amp;
		}
	}


	/* Surround the CSV file content with XML tags to enable XSLT
	 * parsing
	 *
	 * Tag markers take: strlen("<></>") = 5
	 * Reserve also room for encoding ampersands "&" => "&amp;"
	 */
	buf = realloc(mem->buffer, mem->size + 7 + strlen(tag) * 2 + amp * 4);
	if (buf != NULL) {
		char *starttag = NULL;
		char *endtag = NULL;

		starttag = malloc(3 + strlen(tag));
		endtag = malloc(5 + strlen(tag));

		if (starttag == NULL || endtag == NULL) {
			/* this is fairly silly - so the malloc fails, but we strdup the error?
			 * let's complete the silliness by freeing the two pointers in case one malloc succeeded
			 *  and the other one failed - this will make static analysis tools happy */
			free(starttag);
			free(endtag);
			free(buf);
			return report_error("Memory allocation failed in %s", __func__);
		}

		sprintf(starttag, "<%s>", tag);
		sprintf(endtag, "\n</%s>", tag);

		memmove(buf + 2 + strlen(tag), buf, mem->size);
		memcpy(buf, starttag, 2 + strlen(tag));
		memcpy(buf + mem->size + 2 + strlen(tag), endtag, 5 + strlen(tag));
		mem->size += (6 + 2 * strlen(tag));
		mem->buffer = buf;

		free(starttag);
		free(endtag);

		/* Expand ampersands to encoded version */
		for (i = mem->size, rest = 0; i > 0; --i, ++rest) {
			if (((char *)mem->buffer)[i] == '&') {
				memmove(((char *)mem->buffer) + i + 4 + 1, ((char *)mem->buffer) + i + 1, rest);
				memcpy(((char *)mem->buffer) + i + 1, "amp;", 4);
				rest += 4;
				mem->size += 4;
			}
		}
	} else {
		free(mem->buffer);
		return report_error("realloc failed in %s", __func__);
	}

	return 0;
}

int try_to_open_csv(struct memblock *mem, enum csv_format type, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites)
{
	UNUSED(sites);
	char *p = mem->buffer;
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
	record_dive_to_table(dive, table);
	return 1;
}

static char *parse_mkvi_value(const char *haystack, const char *needle)
{
	char *lineptr, *valueptr, *endptr, *ret = NULL;

	if ((lineptr = strstr(haystack, needle)) != NULL) {
		if ((valueptr = strstr(lineptr, ": ")) != NULL) {
			valueptr += 2;
		}
		if ((endptr = strstr(lineptr, "\n")) != NULL) {
			char terminator = '\n';
			if (*(endptr - 1) == '\r') {
				--endptr;
				terminator = '\r';
			}
			*endptr = 0;
			ret = copy_string(valueptr);
			*endptr = terminator;

		}
	}
	return ret;
}

static char *next_mkvi_key(const char *haystack)
{
	char *valueptr, *endptr, *ret = NULL;

	if ((valueptr = strstr(haystack, "\n")) != NULL) {
		valueptr += 1;
		if ((endptr = strstr(valueptr, ": ")) != NULL) {
			*endptr = 0;
			ret = strdup(valueptr);
			*endptr = ':';
		}
	}
	return ret;
}

int parse_txt_file(const char *filename, const char *csv, struct dive_table *table, struct trip_table *trips,
		  struct dive_site_table *sites, struct device_table *devices)
{
	UNUSED(sites);
	struct memblock memtxt, memcsv;

	if (readfile(filename, &memtxt) < 0) {
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);
	}

	/*
	 * MkVI stores some information in .txt file but the whole profile and events are stored in .csv file. First
	 * make sure the input .txt looks like proper MkVI file, then start parsing the .csv.
	 */
	if (MATCH(memtxt.buffer, "MkVI_Config") == 0) {
		int d, m, y, he;
		int hh = 0, mm = 0, ss = 0;
		int prev_depth = 0, cur_sampletime = 0, prev_setpoint = -1, prev_ndl = -1;
		bool has_depth = false, has_setpoint = false, has_ndl = false;
		char *lineptr, *key, *value;
		unsigned int prev_time = 0;
		cylinder_t cyl;

		struct dive *dive;
		struct divecomputer *dc;
		struct tm cur_tm;

		value = parse_mkvi_value(memtxt.buffer, "Dive started at");
		if (sscanf(value, "%d-%d-%d %d:%d:%d", &y, &m, &d, &hh, &mm, &ss) != 6) {
			free(value);
			return -1;
		}
		free(value);
		cur_tm.tm_year = y;
		cur_tm.tm_mon = m - 1;
		cur_tm.tm_mday = d;
		cur_tm.tm_hour = hh;
		cur_tm.tm_min = mm;
		cur_tm.tm_sec = ss;

		dive = alloc_dive();
		dive->when = utc_mktime(&cur_tm);;
		dive->dc.model = strdup("Poseidon MkVI Discovery");
		value = parse_mkvi_value(memtxt.buffer, "Rig Serial number");
		dive->dc.deviceid = atoi(value);
		free(value);
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
		value = parse_mkvi_value(memtxt.buffer, "Helium percentage");
		he = atoi(value);
		free(value);
		value = parse_mkvi_value(memtxt.buffer, "Nitrogen percentage");
		cyl.gasmix.o2.permille = (100 - atoi(value) - he) * 10;
		free(value);
		cyl.gasmix.he.permille = he * 10;
		add_cloned_cylinder(&dive->cylinders, cyl);

		lineptr = strstr(memtxt.buffer, "Dive started at");
		while (!empty_string(lineptr) && (lineptr = strchr(lineptr, '\n'))) {
			++lineptr;	// Skip over '\n'
			key = next_mkvi_key(lineptr);
			if (!key)
				break;
			value = parse_mkvi_value(lineptr, key);
			if (!value) {
				free(key);
				break;
			}
			add_extra_data(&dive->dc, key, value);
			free(key);
			free(value);
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

		if (readfile(csv, &memcsv) < 0) {
			free(dive);
			return report_error(translate("gettextFromC", "Poseidon import failed: unable to read '%s'"), csv);
		}
		lineptr = memcsv.buffer;
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
					printf("Unable to parse input: %s\n", lineptr);
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
		record_dive_to_table(dive, table);
		return 1;
	} else {
		return 0;
	}

	return 0;
}

#define DATESTR 9
#define TIMESTR 6

#define SBPARAMS 40
static int parse_seabear_csv_file(const char *filename, struct xml_params *params, const char *csvtemplate,
				  struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
				  struct device_table *devices, struct filter_preset_table *filter_presets);
int parse_seabear_log(const char *filename, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
		      struct device_table *devices, struct filter_preset_table *filter_presets)
{
	struct xml_params *params = alloc_xml_params();
	int ret;

	parse_seabear_header(filename, params);
	ret = parse_seabear_csv_file(filename, params, "csv", table, trips, sites, devices, filter_presets) < 0 ? -1 : 0;

	free_xml_params(params);

	return ret;
}


static int parse_seabear_csv_file(const char *filename, struct xml_params *params, const char *csvtemplate,
				  struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
				  struct device_table *devices, struct filter_preset_table *filter_presets)
{
	int ret, i;
	struct memblock mem;
	time_t now;
	struct tm *timep = NULL;
	char *ptr, *ptr_old = NULL;
	char *NL = NULL;
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

	if (readfile(filename, &mem) < 0)
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);

	/* Determine NL (new line) character and the start of CSV data */
	ptr = mem.buffer;
	while ((ptr = strstr(ptr, "\r\n\r\n")) != NULL) {
		ptr_old = ptr;
		ptr += 1;
		NL = "\r\n";
	}

	if (!ptr_old) {
		ptr = mem.buffer;
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
	ptr = strstr(mem.buffer, "Serial number:");
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
	memmove(mem.buffer, ptr_old, mem.size - (ptr_old - (char*)mem.buffer));
	mem.size = (int)mem.size - (ptr_old - (char*)mem.buffer);

	if (try_to_xslt_open_csv(filename, &mem, csvtemplate))
		return -1;

	/*
	 * Lets print command line for manual testing with xsltproc if
	 * verbosity level is high enough. The printed line needs the
	 * input file added as last parameter.
	 */

	if (verbose >= 2) {
		fprintf(stderr, "xsltproc ");
		for (i = 0; i < xml_params_count(params); i++)
			fprintf(stderr, "--stringparam %s %s ", xml_params_get_key(params, i), xml_params_get_value(params, i));
		fprintf(stderr, "xslt/csv2xml.xslt\n");
	}

	ret = parse_xml_buffer(filename, mem.buffer, mem.size, table, trips, sites, devices, filter_presets, params);
	free(mem.buffer);

	return ret;
}

int parse_manual_file(const char *filename, struct xml_params *params, struct dive_table *table, struct trip_table *trips,
		      struct dive_site_table *sites, struct device_table *devices, struct filter_preset_table *filter_presets)
{
	struct memblock mem;
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

	mem.size = 0;
	if (try_to_xslt_open_csv(filename, &mem, "manualCSV"))
		return -1;

#ifndef SUBSURFACE_MOBILE
	if (verbose >= 2) {
		fprintf(stderr, "(echo '<manualCSV>'; cat %s;echo '</manualCSV>') | xsltproc ", filename);
		for (int i = 0; i < xml_params_count(params); i++)
			fprintf(stderr, "--stringparam %s %s ", xml_params_get_key(params, i), xml_params_get_value(params, i));
		fprintf(stderr, "%s/xslt/manualcsv2xml.xslt -\n", SUBSURFACE_SOURCE);
	}
#endif
	ret = parse_xml_buffer(filename, mem.buffer, mem.size, table, trips, sites, devices, filter_presets, params);

	free(mem.buffer);
	return ret;
}
