#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gettext.h"
#include <zip.h>
#include <time.h>

#include "dive.h"
#include "file.h"
#include "git-access.h"
#include "qthelperfromc.h"

/* For SAMPLE_* */
#include <libdivecomputer/parser.h>

/* to check XSLT version number */
#include <libxslt/xsltconfig.h>

/* Crazy windows sh*t */
#ifndef O_BINARY
#define O_BINARY 0
#endif

int readfile(const char *filename, struct memblock *mem)
{
	int ret, fd;
	struct stat st;
	char *buf;

	mem->buffer = NULL;
	mem->size = 0;

	fd = subsurface_open(filename, O_RDONLY | O_BINARY, 0);
	if (fd < 0)
		return fd;
	ret = fstat(fd, &st);
	if (ret < 0)
		goto out;
	ret = -EINVAL;
	if (!S_ISREG(st.st_mode))
		goto out;
	ret = 0;
	if (!st.st_size)
		goto out;
	buf = malloc(st.st_size + 1);
	ret = -1;
	errno = ENOMEM;
	if (!buf)
		goto out;
	mem->buffer = buf;
	mem->size = st.st_size;
	ret = read(fd, buf, mem->size);
	if (ret < 0)
		goto free;
	buf[ret] = 0;
	if (ret == mem->size)
		goto out;
	errno = EIO;
	ret = -1;
free:
	free(mem->buffer);
	mem->buffer = NULL;
	mem->size = 0;
out:
	close(fd);
	return ret;
}


static void zip_read(struct zip_file *file, const char *filename)
{
	int size = 1024, n, read = 0;
	char *mem = malloc(size);

	while ((n = zip_fread(file, mem + read, size - read)) > 0) {
		read += n;
		size = read * 3 / 2;
		mem = realloc(mem, size);
	}
	mem[read] = 0;
	(void) parse_xml_buffer(filename, mem, read, &dive_table, NULL);
	free(mem);
}

static int try_to_open_zip(const char *filename, struct memblock *mem)
{
	int success = 0;
	/* Grr. libzip needs to re-open the file, it can't take a buffer */
	struct zip *zip = subsurface_zip_open_readonly(filename, ZIP_CHECKCONS, NULL);

	if (zip) {
		int index;
		for (index = 0;; index++) {
			struct zip_file *file = zip_fopen_index(zip, index, 0);
			if (!file)
				break;
			/* skip parsing the divelogs.de pictures */
			if (strstr(zip_get_name(zip, index, 0), "pictures/"))
				continue;
			zip_read(file, filename);
			zip_fclose(file);
			success++;
		}
		subsurface_zip_close(zip);
	}
	return success;
}

static int try_to_xslt_open_csv(const char *filename, struct memblock *mem, const char *tag)
{
	char *buf;

	if (mem->size == 0 && readfile(filename, mem) < 0)
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);

	/* Surround the CSV file content with XML tags to enable XSLT
	 * parsing
	 *
	 * Tag markers take: strlen("<></>") = 5
	 */
	buf = realloc(mem->buffer, mem->size + 7 + strlen(tag) * 2);
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
	} else {
		free(mem->buffer);
		return report_error("realloc failed in %s", __func__);
	}

	return 0;
}

int db_test_func(void *param, int columns, char **data, char **column)
{
	return *data[0] == '0';
}


static int try_to_open_db(const char *filename, struct memblock *mem)
{
	sqlite3 *handle;
	char dm4_test[] = "select count(*) from sqlite_master where type='table' and name='Dive' and sql like '%ProfileBlob%'";
	char dm5_test[] = "select count(*) from sqlite_master where type='table' and name='Dive' and sql like '%SampleBlob%'";
	char shearwater_test[] = "select count(*) from sqlite_master where type='table' and name='system' and sql like '%dbVersion%'";
	char cobalt_test[] = "select count(*) from sqlite_master where type='table' and name='TrackPoints' and sql like '%DepthPressure%'";
	int retval;

	retval = sqlite3_open(filename, &handle);

	if (retval) {
		fprintf(stderr, translate("gettextFromC", "Database connection failed '%s'.\n"), filename);
		return 1;
	}

	/* Testing if DB schema resembles Suunto DM5 database format */
	retval = sqlite3_exec(handle, dm5_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_dm5_buffer(handle, filename, mem->buffer, mem->size, &dive_table);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Suunto DM4 database format */
	retval = sqlite3_exec(handle, dm4_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_dm4_buffer(handle, filename, mem->buffer, mem->size, &dive_table);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Shearwater database format */
	retval = sqlite3_exec(handle, shearwater_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_shearwater_buffer(handle, filename, mem->buffer, mem->size, &dive_table);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Atomic Cobalt database format */
	retval = sqlite3_exec(handle, cobalt_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_cobalt_buffer(handle, filename, mem->buffer, mem->size, &dive_table);
		sqlite3_close(handle);
		return retval;
	}

	sqlite3_close(handle);

	return retval;
}

timestamp_t parse_date(const char *date)
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

enum csv_format {
	CSV_DEPTH,
	CSV_TEMP,
	CSV_PRESSURE,
	POSEIDON_DEPTH,
	POSEIDON_TEMP,
	POSEIDON_SETPOINT,
	POSEIDON_SENSOR1,
	POSEIDON_SENSOR2,
	POSEIDON_PRESSURE,
	POSEIDON_O2CYLINDER,
	POSEIDON_NDL,
	POSEIDON_CEILING
};

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
		sample->cylinderpressure.mbar = psi_to_mbar(val * 4);
		break;
	case POSEIDON_DEPTH:
		sample->depth.mm = val * 0.5 *1000;
		break;
	case POSEIDON_TEMP:
		sample->temperature.mkelvin = C_to_mkelvin(val * 0.2);
		break;
	case POSEIDON_SETPOINT:
		sample->setpoint.mbar = val * 10;
		break;
	case POSEIDON_SENSOR1:
		sample->o2sensor[0].mbar = val * 10;
		break;
	case POSEIDON_SENSOR2:
		sample->o2sensor[1].mbar = val * 10;
		break;
	case POSEIDON_PRESSURE:
		sample->cylinderpressure.mbar = val * 1000;
		break;
	case POSEIDON_O2CYLINDER:
		sample->o2cylinderpressure.mbar = val * 1000;
		break;
	case POSEIDON_NDL:
		sample->ndl.seconds = val * 60;
		break;
	case POSEIDON_CEILING:
		sample->stopdepth.mm = val * 1000;
		break;
	}
}

/*
 * Cochran comma-separated values: depth in feet, temperature in F, pressure in psi.
 *
 * They start with eight comma-separated fields like:
 *
 *   filename: {C:\Analyst4\can\T036785.can},{C:\Analyst4\can\K031892.can}
 *   divenr: %d
 *   datetime: {03Sep11 16:37:22},{15Dec11 18:27:02}
 *   ??: 1
 *   serialnr??: {CCI134},{CCI207}
 *   computer??: {GeminiII},{CommanderIII}
 *   computer??: {GeminiII},{CommanderIII}
 *   ??: 1
 *
 * Followed by the data values (all comma-separated, all one long line).
 */
static int try_to_open_csv(const char *filename, struct memblock *mem, enum csv_format type)
{
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
	record_dive(dive);
	return 1;
}

static int open_by_filename(const char *filename, const char *fmt, struct memblock *mem)
{
	// hack to be able to provide a comment for the translated string
	static char *csv_warning = QT_TRANSLATE_NOOP3("gettextFromC",
						      "Cannot open CSV file %s; please use Import log file dialog",
						      "'Import log file' should be the same text as corresponding label in Import menu");

	/* Suunto Dive Manager files: SDE, ZIP; divelogs.de files: DLD */
	if (!strcasecmp(fmt, "SDE") || !strcasecmp(fmt, "ZIP") || !strcasecmp(fmt, "DLD"))
		return try_to_open_zip(filename, mem);

	/* CSV files */
	if (!strcasecmp(fmt, "CSV"))
		return report_error(translate("gettextFromC", csv_warning), filename);
	/* Truly nasty intentionally obfuscated Cochran Anal software */
	if (!strcasecmp(fmt, "CAN"))
		return try_to_open_cochran(filename, mem);
	/* Cochran export comma-separated-value files */
	if (!strcasecmp(fmt, "DPT"))
		return try_to_open_csv(filename, mem, CSV_DEPTH);
	if (!strcasecmp(fmt, "LVD"))
		return try_to_open_liquivision(filename, mem);
	if (!strcasecmp(fmt, "TMP"))
		return try_to_open_csv(filename, mem, CSV_TEMP);
	if (!strcasecmp(fmt, "HP1"))
		return try_to_open_csv(filename, mem, CSV_PRESSURE);

	return 0;
}

static int parse_file_buffer(const char *filename, struct memblock *mem)
{
	int ret;
	char *fmt = strrchr(filename, '.');
	if (fmt && (ret = open_by_filename(filename, fmt + 1, mem)) != 0)
		return ret;

	if (!mem->size || !mem->buffer)
		return report_error("Out of memory parsing file %s\n", filename);

	return parse_xml_buffer(filename, mem->buffer, mem->size, &dive_table, NULL);
}

int parse_file(const char *filename)
{
	struct git_repository *git;
	const char *branch;
	struct memblock mem;
	char *fmt;
	int ret;

	git = is_git_repository(filename, &branch, NULL);
	if (strstr(filename, prefs.cloud_git_url) && git == dummy_git_repository)
		/* opening the cloud storage repository failed for some reason
		 * give up here and don't send errors about git repositories */
		return 0;

	if (git && !git_load_dives(git, branch))
		return 0;

	if (readfile(filename, &mem) < 0) {
		/* we don't want to display an error if this was the default file or the cloud storage */
		if ((prefs.default_filename && !strcmp(filename, prefs.default_filename)) ||
		    isCloudUrl(filename))
			return 0;

		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);
	}

	fmt = strrchr(filename, '.');
	if (fmt && (!strcasecmp(fmt + 1, "DB") || !strcasecmp(fmt + 1, "BAK"))) {
		if (!try_to_open_db(filename, &mem)) {
			free(mem.buffer);
			return 0;
		}
	}

	/* Divesoft Freedom */
	if (fmt && (!strcasecmp(fmt + 1, "DLF"))) {
		if (!parse_dlf_buffer(mem.buffer, mem.size)) {
			free(mem.buffer);
			return 0;
		}
		return -1;
	}

	/* DataTrak/Wlog */
	if (fmt && !strcasecmp(fmt + 1, "LOG")) {
		datatrak_import(filename, &dive_table);
		return 0;
	}

	/* OSTCtools */
	if (fmt && (!strcasecmp(fmt + 1, "DIVE"))) {
		ostctools_import(filename, &dive_table);
		return 0;
	}

	ret = parse_file_buffer(filename, &mem);
	free(mem.buffer);
	return ret;
}

#define MATCH(buffer, pattern) \
	memcmp(buffer, pattern, strlen(pattern))

char *parse_mkvi_value(const char *haystack, const char *needle)
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
			ret = strdup(valueptr);
			*endptr = terminator;

		}
	}
	return ret;
}

char *next_mkvi_key(const char *haystack)
{
	char *valueptr, *endptr, *ret = NULL;

	if ((valueptr = strstr(haystack, "\n")) != NULL) {
		valueptr += 1;
	}
	if ((endptr = strstr(valueptr, ": ")) != NULL) {
		*endptr = 0;
		ret = strdup(valueptr);
		*endptr = ':';

	}
	return ret;
}

int parse_txt_file(const char *filename, const char *csv)
{
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
		int o2cylinder_pressure = 0, cylinder_pressure = 0, cur_cylinder_index = 0;

		struct dive *dive;
		struct divecomputer *dc;
		struct tm cur_tm;

		if (sscanf(parse_mkvi_value(memtxt.buffer, "Dive started at"), "%d-%d-%d %d:%d:%d",
					&y, &m, &d, &hh, &mm, &ss) != 6) {
			return -1;
		}

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

		dive->cylinder[cur_cylinder_index].cylinder_use = OXYGEN;
		dive->cylinder[cur_cylinder_index].type.size.mliter = 3000;
		dive->cylinder[cur_cylinder_index].type.workingpressure.mbar = 200000;
		dive->cylinder[cur_cylinder_index].type.description = strdup("3l Mk6");
		dive->cylinder[cur_cylinder_index].gasmix.o2.permille = 1000;
		cur_cylinder_index++;

		dive->cylinder[cur_cylinder_index].cylinder_use = DILUENT;
		dive->cylinder[cur_cylinder_index].type.size.mliter = 3000;
		dive->cylinder[cur_cylinder_index].type.workingpressure.mbar = 200000;
		dive->cylinder[cur_cylinder_index].type.description = strdup("3l Mk6");
		value = parse_mkvi_value(memtxt.buffer, "Helium percentage");
		he = atoi(value);
		free(value);
		value = parse_mkvi_value(memtxt.buffer, "Nitrogen percentage");
		dive->cylinder[cur_cylinder_index].gasmix.o2.permille = (100 - atoi(value) - he) * 10;
		free(value);
		dive->cylinder[cur_cylinder_index].gasmix.he.permille = he * 10;
		cur_cylinder_index++;

		lineptr = strstr(memtxt.buffer, "Dive started at");
		while (lineptr && *lineptr && (lineptr = strchr(lineptr, '\n')) && ++lineptr) {
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
			sample->time.seconds = cur_sampletime;

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
						add_sample_data(sample, POSEIDON_O2CYLINDER, value);
						if (!o2cylinder_pressure) {
							dive->cylinder[0].sample_start.mbar = value * 1000;
							o2cylinder_pressure = value;
						} else
							o2cylinder_pressure = value;
						break;
					case 14:
						//Diluent Tank Pressure
						add_sample_data(sample, POSEIDON_PRESSURE, value);
						if (!cylinder_pressure) {
							dive->cylinder[1].sample_start.mbar = value * 1000;
							cylinder_pressure = value;
						} else
							cylinder_pressure = value;
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
						// Water Temperature in Celcius
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
			if (cylinder_pressure)
				dive->cylinder[1].sample_end.mbar = cylinder_pressure * 1000;
			if (o2cylinder_pressure)
				dive->cylinder[0].sample_end.mbar = o2cylinder_pressure * 1000;
			finish_sample(dc);

			if (!lineptr || !*lineptr)
				break;
		}
		record_dive(dive);
		return 1;
	} else {
		return report_error(translate("gettextFromC", "No matching DC found for file '%s'"), csv);
	}

	return 0;
}

#define MAXCOLDIGITS 3
#define MAXCOLS 100
#define DATESTR 9
#define TIMESTR 6
void init_csv_file_parsing(char **params, char *timebuf, char *depthbuf, char *tempbuf, char *po2buf, char *cnsbuf, char *ndlbuf, char *ttsbuf, char *stopdepthbuf, char *pressurebuf, char *unitbuf, char *separator_index, time_t *now, struct tm *timep, char *curdate, char *curtime, int timef, int depthf, int tempf, int po2f, int cnsf, int ndlf, int ttsf, int stopdepthf, int pressuref, int sepidx, const char *csvtemplate, int unitidx)
{
	int pnr = 0;

	snprintf(timebuf, MAXCOLDIGITS, "%d", timef);
	snprintf(depthbuf, MAXCOLDIGITS, "%d", depthf);
	snprintf(tempbuf, MAXCOLDIGITS, "%d", tempf);
	snprintf(po2buf, MAXCOLDIGITS, "%d", po2f);
	snprintf(cnsbuf, MAXCOLDIGITS, "%d", cnsf);
	snprintf(ndlbuf, MAXCOLDIGITS, "%d", ndlf);
	snprintf(ttsbuf, MAXCOLDIGITS, "%d", ttsf);
	snprintf(stopdepthbuf, MAXCOLDIGITS, "%d", stopdepthf);
	snprintf(pressurebuf, MAXCOLDIGITS, "%d", pressuref);
	snprintf(separator_index, MAXCOLDIGITS, "%d", sepidx);
	snprintf(unitbuf, MAXCOLDIGITS, "%d", unitidx);
	time(now);
	timep = localtime(now);
	strftime(curdate, DATESTR, "%Y%m%d", timep);

	/* As the parameter is numeric, we need to ensure that the leading zero
	* is not discarded during the transform, thus prepend time with 1 */
	strftime(curtime, TIMESTR, "1%H%M", timep);

	params[pnr++] = "timeField";
	params[pnr++] = timebuf;
	params[pnr++] = "depthField";
	params[pnr++] = depthbuf;
	params[pnr++] = "tempField";
	params[pnr++] = tempbuf;
	params[pnr++] = "po2Field";
	params[pnr++] = po2buf;
	params[pnr++] = "cnsField";
	params[pnr++] = cnsbuf;
	params[pnr++] = "ndlField";
	params[pnr++] = ndlbuf;
	params[pnr++] = "ttsField";
	params[pnr++] = ttsbuf;
	params[pnr++] = "stopdepthField";
	params[pnr++] = stopdepthbuf;
	params[pnr++] = "pressureField";
	params[pnr++] = pressurebuf;
	params[pnr++] = "date";
	params[pnr++] = curdate;
	params[pnr++] = "time";
	params[pnr++] = curtime;
	params[pnr++] = "units";
	params[pnr++] = unitbuf;
	params[pnr++] = "separatorIndex";
	params[pnr++] = separator_index;
	params[pnr++] = NULL;
}

int parse_csv_file(const char *filename, int timef, int depthf, int tempf, int po2f, int cnsf, int ndlf, int ttsf, int stopdepthf, int pressuref, int sepidx, const char *csvtemplate, int unitidx)
{
	int ret;
	struct memblock mem;
	char *params[27];
	char timebuf[MAXCOLDIGITS];
	char depthbuf[MAXCOLDIGITS];
	char tempbuf[MAXCOLDIGITS];
	char po2buf[MAXCOLDIGITS];
	char cnsbuf[MAXCOLDIGITS];
	char ndlbuf[MAXCOLDIGITS];
	char ttsbuf[MAXCOLDIGITS];
	char stopdepthbuf[MAXCOLDIGITS];
	char pressurebuf[MAXCOLDIGITS];
	char unitbuf[MAXCOLDIGITS];
	char separator_index[MAXCOLDIGITS];
	time_t now;
	struct tm *timep = NULL;
	char curdate[DATESTR];
	char curtime[TIMESTR];
	int previous;

	/* Increase the limits for recursion and variables on XSLT
	 * parsing */
	xsltMaxDepth = 30000;
#if LIBXSLT_VERSION > 10126
	xsltMaxVars = 150000;
#endif

	if (timef >= MAXCOLS || depthf >= MAXCOLS || tempf >= MAXCOLS || po2f >= MAXCOLS || cnsf >= MAXCOLS || ndlf >= MAXCOLS || cnsf >= MAXCOLS || stopdepthf >= MAXCOLS || pressuref >= MAXCOLS)
		return report_error(translate("gettextFromC", "Maximum number of supported columns on CSV import is %d"), MAXCOLS);

	init_csv_file_parsing(params, timebuf, depthbuf, tempbuf, po2buf, cnsbuf,ndlbuf, ttsbuf, stopdepthbuf, pressurebuf, unitbuf, separator_index, &now, timep, curdate, curtime, timef, depthf, tempf, po2f, cnsf, ndlf, ttsf, stopdepthf, pressuref, sepidx, csvtemplate, unitidx);

	if (filename == NULL)
		return report_error("No CSV filename");

	mem.size = 0;
	if (try_to_xslt_open_csv(filename, &mem, csvtemplate))
		return -1;

	previous = dive_table.nr;
	ret = parse_xml_buffer(filename, mem.buffer, mem.size, &dive_table, (const char **)params);

	// mark imported dives as imported from CSV
	for (int i = previous; i < dive_table.nr; i++)
		if (same_string(get_dive(i)->dc.model, ""))
			get_dive(i)->dc.model = copy_string("Imported from CSV");

	free(mem.buffer);
	return ret;
}

#define SBPARAMS 29
int parse_seabear_csv_file(const char *filename, int timef, int depthf, int tempf, int po2f, int cnsf, int ndlf, int ttsf, int stopdepthf, int pressuref, int sepidx, const char *csvtemplate, int unitidx, const char *delta)
{
	int ret;
	struct memblock mem;
	char *params[SBPARAMS];
	char timebuf[MAXCOLDIGITS];
	char depthbuf[MAXCOLDIGITS];
	char tempbuf[MAXCOLDIGITS];
	char po2buf[MAXCOLDIGITS];
	char cnsbuf[MAXCOLDIGITS];
	char ndlbuf[MAXCOLDIGITS];
	char ttsbuf[MAXCOLDIGITS];
	char stopdepthbuf[MAXCOLDIGITS];
	char pressurebuf[MAXCOLDIGITS];
	char unitbuf[MAXCOLDIGITS];
	char separator_index[MAXCOLDIGITS];
	char deltabuf[MAXCOLDIGITS];
	time_t now;
	struct tm *timep = NULL;
	char curdate[DATESTR];
	char curtime[TIMESTR];
	char *ptr, *ptr_old = NULL;
	char *NL = NULL;

	if (timef >= MAXCOLS || depthf >= MAXCOLS || tempf >= MAXCOLS || po2f >= MAXCOLS || cnsf >= MAXCOLS || ndlf >= MAXCOLS || cnsf >= MAXCOLS || stopdepthf >= MAXCOLS || pressuref >= MAXCOLS)
		return report_error(translate("gettextFromC", "Maximum number of supported columns on CSV import is %d"), MAXCOLS);

	init_csv_file_parsing(params, timebuf, depthbuf, tempbuf, po2buf, cnsbuf,ndlbuf, ttsbuf, stopdepthbuf, pressurebuf, unitbuf, separator_index, &now, timep, curdate, curtime, timef, depthf, tempf, po2f, cnsf, ndlf, ttsf, stopdepthf, pressuref, sepidx, csvtemplate, unitidx);

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
		while ((ptr = strstr(ptr, "\n\n")) != NULL) {
			ptr_old = ptr;
			ptr += 1;
			NL = "\n";
		}
		ptr_old += 2;
	} else
		ptr_old += 4;

	/*
	 * If file does not contain empty lines, it is not a valid
	 * Seabear CSV file.
	 */
	if (NL == NULL)
		return -1;

	/*
	 * On my current sample of Seabear DC log file, the date is
	 * without any identifier. Thus we must search for the previous
	 * line and step through from there.
	 */
	ptr = strstr(mem.buffer, "Serial number:");
	if (ptr)
		ptr = strstr(ptr, NL);

	/* Write date and time values to params array */
	if (ptr) {
		ptr += strlen(NL) + 2;
		memcpy(params[19], ptr, 4);
		memcpy(params[19] + 4, ptr + 5, 2);
		memcpy(params[19] + 6, ptr + 8, 2);
		params[19][8] = 0;

		params[21][0] = '1';
		memcpy(params[21] + 1, ptr + 11, 2);
		memcpy(params[21] + 3, ptr + 14, 2);
		params[21][5] = 0;
	}

	snprintf(deltabuf, MAXCOLDIGITS, "%s", delta);
	params[SBPARAMS - 3] = "delta";
	params[SBPARAMS - 2] = deltabuf;
	params[SBPARAMS - 1] = NULL;

	/* Move the CSV data to the start of mem buffer */
	memmove(mem.buffer, ptr_old, mem.size - (ptr_old - (char*)mem.buffer));
	mem.size = (int)mem.size - (ptr_old - (char*)mem.buffer);

	if (try_to_xslt_open_csv(filename, &mem, csvtemplate))
		return -1;

	ret = parse_xml_buffer(filename, mem.buffer, mem.size, &dive_table, (const char **)params);
	free(mem.buffer);
	return ret;
}

int parse_manual_file(const char *filename, int sepidx, int units, int dateformat, int durationformat,
		      int numberf, int datef, int timef, int durationf, int locationf, int gpsf, int maxdepthf, int meandepthf,
		      int divemasterf, int buddyf, int suitf, int notesf, int weightf, int tagsf, int cylsizef, int startpresf, int endpresf,
		      int o2f, int hef, int airtempf, int watertempf)
{
	if (verbose > 4) {
		fprintf(stderr, "filename %s, sepidx %d, units %d, dateformat %d, durationformat %d\n", filename, sepidx, units, dateformat, durationformat);
		fprintf(stderr, "numberf %d, datef %d, timef %d, durationf %d, locationf %d, gpsf %d, maxdepthf %d, meandepthf %d\n", numberf, datef, timef, durationf, locationf, gpsf, maxdepthf, meandepthf);
		fprintf(stderr, "divemasterf %d, buddyf %d, suitf %d, notesf %d, weightf %d, tagsf %d, cylsizef %d, startpresf %d, endpresf %d\n", divemasterf, buddyf, suitf, notesf, weightf, tagsf, cylsizef, startpresf, endpresf);
		fprintf(stderr, "o2f %d, hef %d, airtempf %d, watertempf %d\n", o2f, hef, airtempf, watertempf);
	}
	struct memblock mem;
	int pnr = 0;
	char *params[55];
	char numberbuf[MAXCOLDIGITS];
	char datebuf[MAXCOLDIGITS];
	char timebuf[MAXCOLDIGITS];
	char durationbuf[MAXCOLDIGITS];
	char locationbuf[MAXCOLDIGITS];
	char gpsbuf[MAXCOLDIGITS];
	char maxdepthbuf[MAXCOLDIGITS];
	char meandepthbuf[MAXCOLDIGITS];
	char divemasterbuf[MAXCOLDIGITS];
	char buddybuf[MAXCOLDIGITS];
	char suitbuf[MAXCOLDIGITS];
	char notesbuf[MAXCOLDIGITS];
	char weightbuf[MAXCOLDIGITS];
	char tagsbuf[MAXCOLDIGITS];
	char separator_index[MAXCOLDIGITS];
	char unit[MAXCOLDIGITS];
	char datefmt[MAXCOLDIGITS];
	char durationfmt[MAXCOLDIGITS];
	char cylsizebuf[MAXCOLDIGITS];
	char startpresbuf[MAXCOLDIGITS];
	char endpresbuf[MAXCOLDIGITS];
	char o2buf[MAXCOLDIGITS];
	char hebuf[MAXCOLDIGITS];
	char airtempbuf[MAXCOLDIGITS];
	char watertempbuf[MAXCOLDIGITS];
	time_t now;
	struct tm *timep;
	char curdate[9];
	char curtime[6];
	int ret;

	if (numberf >= MAXCOLS || datef >= MAXCOLS || timef >= MAXCOLS || durationf >= MAXCOLS || locationf >= MAXCOLS || gpsf >= MAXCOLS || maxdepthf >= MAXCOLS || meandepthf >= MAXCOLS || buddyf >= MAXCOLS || suitf >= MAXCOLS || notesf >= MAXCOLS || weightf >= MAXCOLS || tagsf >= MAXCOLS || cylsizef >= MAXCOLS || startpresf >= MAXCOLS || endpresf >= MAXCOLS || o2f >= MAXCOLS || hef >= MAXCOLS || airtempf >= MAXCOLS || watertempf >= MAXCOLS)
		return report_error(translate("gettextFromC", "Maximum number of supported columns on CSV import is %d"), MAXCOLS);

	snprintf(numberbuf, MAXCOLDIGITS, "%d", numberf);
	snprintf(datebuf, MAXCOLDIGITS, "%d", datef);
	snprintf(timebuf, MAXCOLDIGITS, "%d", timef);
	snprintf(durationbuf, MAXCOLDIGITS, "%d", durationf);
	snprintf(locationbuf, MAXCOLDIGITS, "%d", locationf);
	snprintf(gpsbuf, MAXCOLDIGITS, "%d", gpsf);
	snprintf(maxdepthbuf, MAXCOLDIGITS, "%d", maxdepthf);
	snprintf(meandepthbuf, MAXCOLDIGITS, "%d", meandepthf);
	snprintf(divemasterbuf, MAXCOLDIGITS, "%d", divemasterf);
	snprintf(buddybuf, MAXCOLDIGITS, "%d", buddyf);
	snprintf(suitbuf, MAXCOLDIGITS, "%d", suitf);
	snprintf(notesbuf, MAXCOLDIGITS, "%d", notesf);
	snprintf(weightbuf, MAXCOLDIGITS, "%d", weightf);
	snprintf(tagsbuf, MAXCOLDIGITS, "%d", tagsf);
	snprintf(separator_index, MAXCOLDIGITS, "%d", sepidx);
	snprintf(unit, MAXCOLDIGITS, "%d", units);
	snprintf(datefmt, MAXCOLDIGITS, "%d", dateformat);
	snprintf(durationfmt, MAXCOLDIGITS, "%d", durationformat);
	snprintf(cylsizebuf, MAXCOLDIGITS, "%d", cylsizef);
	snprintf(startpresbuf, MAXCOLDIGITS, "%d", startpresf);
	snprintf(endpresbuf, MAXCOLDIGITS, "%d", endpresf);
	snprintf(o2buf, MAXCOLDIGITS, "%d", o2f);
	snprintf(hebuf, MAXCOLDIGITS, "%d", hef);
	snprintf(airtempbuf, MAXCOLDIGITS, "%d", airtempf);
	snprintf(watertempbuf, MAXCOLDIGITS, "%d", watertempf);
	time(&now);
	timep = localtime(&now);
	strftime(curdate, DATESTR, "%Y%m%d", timep);

	/* As the parameter is numeric, we need to ensure that the leading zero
	* is not discarded during the transform, thus prepend time with 1 */
	strftime(curtime, TIMESTR, "1%H%M", timep);

	params[pnr++] = "numberField";
	params[pnr++] = numberbuf;
	params[pnr++] = "dateField";
	params[pnr++] = datebuf;
	params[pnr++] = "timeField";
	params[pnr++] = timebuf;
	params[pnr++] = "durationField";
	params[pnr++] = durationbuf;
	params[pnr++] = "locationField";
	params[pnr++] = locationbuf;
	params[pnr++] = "gpsField";
	params[pnr++] = gpsbuf;
	params[pnr++] = "maxDepthField";
	params[pnr++] = maxdepthbuf;
	params[pnr++] = "meanDepthField";
	params[pnr++] = meandepthbuf;
	params[pnr++] = "divemasterField";
	params[pnr++] = divemasterbuf;
	params[pnr++] = "buddyField";
	params[pnr++] = buddybuf;
	params[pnr++] = "suitField";
	params[pnr++] = suitbuf;
	params[pnr++] = "notesField";
	params[pnr++] = notesbuf;
	params[pnr++] = "weightField";
	params[pnr++] = weightbuf;
	params[pnr++] = "tagsField";
	params[pnr++] = tagsbuf;
	params[pnr++] = "date";
	params[pnr++] = curdate;
	params[pnr++] = "time";
	params[pnr++] = curtime;
	params[pnr++] = "separatorIndex";
	params[pnr++] = separator_index;
	params[pnr++] = "units";
	params[pnr++] = unit;
	params[pnr++] = "datefmt";
	params[pnr++] = datefmt;
	params[pnr++] = "durationfmt";
	params[pnr++] = durationfmt;
	params[pnr++] = "cylindersizeField";
	params[pnr++] = cylsizebuf;
	params[pnr++] = "startpressureField";
	params[pnr++] = startpresbuf;
	params[pnr++] = "endpressureField";
	params[pnr++] = endpresbuf;
	params[pnr++] = "o2Field";
	params[pnr++] = o2buf;
	params[pnr++] = "heField";
	params[pnr++] = hebuf;
	params[pnr++] = "airtempField";
	params[pnr++] = airtempbuf;
	params[pnr++] = "watertempField";
	params[pnr++] = watertempbuf;
	params[pnr++] = NULL;

	if (filename == NULL)
		return report_error("No manual CSV filename");

	mem.size = 0;
	if (try_to_xslt_open_csv(filename, &mem, "manualCSV"))
		return -1;

	// right now input files created by XSLT processing report being v2 XML which makes
	// the parse function abort until the dialog about importing v2 files has been shown.
	// Until the XSLT has been updated we just override this check
	//
	// FIXME
	//
	bool remember = v2_question_shown;
	v2_question_shown = true;
	ret = parse_xml_buffer(filename, mem.buffer, mem.size, &dive_table, (const char **)params);
	v2_question_shown = remember;

	free(mem.buffer);
	return ret;
}
