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
	parse_xml_buffer(filename, mem, read, &dive_table, NULL);
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

	if (readfile(filename, mem) < 0)
		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);

	/* Surround the CSV file content with XML tags to enable XSLT
	 * parsing
	 *
	 * Tag markers take: strlen("<></>") = 5
	 */
	buf = realloc(mem->buffer, mem->size + 5 + strlen(tag) * 2);
	if (buf != NULL) {
		char *starttag = NULL;
		char *endtag = NULL;

		starttag = malloc(3 + strlen(tag));
		endtag = malloc(4 + strlen(tag));

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
		sprintf(endtag, "</%s>", tag);

		memmove(buf + 2 + strlen(tag), buf, mem->size);
		memcpy(buf, starttag, 2 + strlen(tag));
		memcpy(buf + mem->size + 2 + strlen(tag), endtag, 4 + strlen(tag));
		mem->size += (5 + 2 * strlen(tag));
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
	char shearwater_test[] = "select count(*) from sqlite_master where type='table' and name='system' and sql like '%dbVersion%'";
	int retval;

	retval = sqlite3_open(filename, &handle);

	if (retval) {
		fprintf(stderr, translate("gettextFromC", "Database connection failed '%s'.\n"), filename);
		return 1;
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
	POSEIDON_DILUENT
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
	case POSEIDON_DILUENT:
		sample->diluentpressure.mbar = val * 1000;
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
	/* Suunto Dive Manager files: SDE, ZIP; divelogs.de files: DLD */
	if (!strcasecmp(fmt, "SDE") || !strcasecmp(fmt, "ZIP") || !strcasecmp(fmt, "DLD"))
		return try_to_open_zip(filename, mem);

	/* CSV files */
	if (!strcasecmp(fmt, "CSV"))
		return 1;

#if ONCE_COCHRAN_IS_SUPPORTED
	/* Truly nasty intentionally obfuscated Cochran Anal software */
	if (!strcasecmp(fmt, "CAN"))
		return try_to_open_cochran(filename, mem);
#endif

	/* Cochran export comma-separated-value files */
	if (!strcasecmp(fmt, "DPT"))
		return try_to_open_csv(filename, mem, CSV_DEPTH);
	if (!strcasecmp(fmt, "TMP"))
		return try_to_open_csv(filename, mem, CSV_TEMP);
	if (!strcasecmp(fmt, "HP1"))
		return try_to_open_csv(filename, mem, CSV_PRESSURE);

	return 0;
}

static void parse_file_buffer(const char *filename, struct memblock *mem)
{
	char *fmt = strrchr(filename, '.');
	if (fmt && open_by_filename(filename, fmt + 1, mem))
		return;

	if (!mem->size || !mem->buffer)
		return;

	parse_xml_buffer(filename, mem->buffer, mem->size, &dive_table, NULL);
}

int parse_file(const char *filename)
{
	struct git_repository *git;
	const char *branch;
	struct memblock mem;
	char *fmt;

	git = is_git_repository(filename, &branch);
	if (git && !git_load_dives(git, branch))
		return 0;

	if (readfile(filename, &mem) < 0) {
		/* we don't want to display an error if this was the default file */
		if (prefs.default_filename && !strcmp(filename, prefs.default_filename))
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

	parse_file_buffer(filename, &mem);
	free(mem.buffer);
	return 0;
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
			*endptr = 0;
			ret = strdup(valueptr);
			*endptr = '\n';

		}
	}
	return ret;
}

static int cur_cylinder_index;
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
		int d, m, y;
		int hh = 0, mm = 0, ss = 0;
		int prev_depth = 0, cur_sampletime = 0, prev_setpoint;
		bool has_depth = false, has_setpoint = false;
		char *lineptr;

		struct dive *dive;
		struct divecomputer *dc;
		struct tm cur_tm;
		timestamp_t date;

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
		dive->dc.deviceid = atoi(parse_mkvi_value(memtxt.buffer, "Rig Serial number"));
		dive->dc.dctype = CCR;

		dive->cylinder[cur_cylinder_index].type.size.mliter = 3000;
		dive->cylinder[cur_cylinder_index].type.workingpressure.mbar = 200000;
		dive->cylinder[cur_cylinder_index].type.description = strdup("3l Mk6");
		cur_cylinder_index++;

		dive->cylinder[cur_cylinder_index].type.size.mliter = 3000;
		dive->cylinder[cur_cylinder_index].type.workingpressure.mbar = 200000;
		dive->cylinder[cur_cylinder_index].type.description = strdup("3l Mk6");
		cur_cylinder_index++;

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
			char *end;
			double val;
			struct sample *sample;
			int type;
			int value;
			int sampletime;

			/* Collect all the information for one sample */
			sscanf(lineptr, "%d,%d,%d", &cur_sampletime, &type, &value);

			has_depth = false;
			has_setpoint = false;
			sample = prepare_sample(dc);
			sample->time.seconds = cur_sampletime;

			do {
				int i = sscanf(lineptr, "%d,%d,%d", &sampletime, &type, &value);
				switch (i) {
				case 3:
					switch (type) {
					case 6:
						add_sample_data(sample, POSEIDON_SENSOR1, value);
						break;
					case 7:
						add_sample_data(sample, POSEIDON_SENSOR2, value);
						break;
					case 8:
						has_depth = true;
						prev_depth = value;
						add_sample_data(sample, POSEIDON_DEPTH, value);
						break;
					case 13:
						add_sample_data(sample, POSEIDON_PRESSURE, value);
						break;
					case 14:
						add_sample_data(sample, POSEIDON_DILUENT, value);
						break;
					case 20:
						has_setpoint = true;
						prev_setpoint = value;
						add_sample_data(sample, POSEIDON_SETPOINT, value);
						break;
					case 39:
						add_sample_data(sample, POSEIDON_TEMP, value);
						break;
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

			if (!has_depth)
				add_sample_data(sample, POSEIDON_DEPTH, prev_depth);
			if (!has_setpoint)
				add_sample_data(sample, POSEIDON_SETPOINT, prev_setpoint);
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
int parse_csv_file(const char *filename, int timef, int depthf, int tempf, int po2f, int cnsf, int ndlf, int ttsf, int stopdepthf, int pressuref, int sepidx, const char *csvtemplate, int unitidx)
{
	struct memblock mem;
	int pnr = 0;
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
	struct tm *timep;
	char curdate[9];
	char curtime[6];

	if (timef >= MAXCOLS || depthf >= MAXCOLS || tempf >= MAXCOLS || po2f >= MAXCOLS || cnsf >= MAXCOLS || ndlf >= MAXCOLS || cnsf >= MAXCOLS || stopdepthf >= MAXCOLS || pressuref >= MAXCOLS)
		return report_error(translate("gettextFromC", "Maximum number of supported columns on CSV import is %d"), MAXCOLS);

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
	time(&now);
	timep = localtime(&now);
	strftime(curdate, sizeof(curdate), "%Y%m%d", timep);

	/* As the parameter is numeric, we need to ensure that the leading zero
	* is not discarded during the transform, thus prepend time with 1 */
	strftime(curtime, sizeof(curtime), "1%H%M", timep);

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

	if (filename == NULL)
		return report_error("No CSV filename");

	if (try_to_xslt_open_csv(filename, &mem, csvtemplate))
		return -1;

	parse_xml_buffer(filename, mem.buffer, mem.size, &dive_table, (const char **)params);
	free(mem.buffer);
	return 0;
}

int parse_manual_file(const char *filename, int sepidx, int units, int numberf, int datef, int timef, int durationf, int locationf, int gpsf, int maxdepthf, int meandepthf, int buddyf, int notesf, int weightf, int tagsf)
{
	struct memblock mem;
	int pnr = 0;
	char *params[33];
	char numberbuf[MAXCOLDIGITS];
	char datebuf[MAXCOLDIGITS];
	char timebuf[MAXCOLDIGITS];
	char durationbuf[MAXCOLDIGITS];
	char locationbuf[MAXCOLDIGITS];
	char gpsbuf[MAXCOLDIGITS];
	char maxdepthbuf[MAXCOLDIGITS];
	char meandepthbuf[MAXCOLDIGITS];
	char buddybuf[MAXCOLDIGITS];
	char notesbuf[MAXCOLDIGITS];
	char weightbuf[MAXCOLDIGITS];
	char tagsbuf[MAXCOLDIGITS];
	char separator_index[MAXCOLDIGITS];
	char unit[MAXCOLDIGITS];
	time_t now;
	struct tm *timep;
	char curdate[9];
	char curtime[6];

	if (numberf >= MAXCOLS || datef >= MAXCOLS || timef >= MAXCOLS || durationf >= MAXCOLS || locationf >= MAXCOLS || gpsf >= MAXCOLS || maxdepthf >= MAXCOLS || meandepthf >= MAXCOLS || buddyf >= MAXCOLS || notesf >= MAXCOLS || weightf >= MAXCOLS || tagsf >= MAXCOLS)
		return report_error(translate("gettextFromC", "Maximum number of supported columns on CSV import is %d"), MAXCOLS);

	snprintf(numberbuf, MAXCOLDIGITS, "%d", numberf);
	snprintf(datebuf, MAXCOLDIGITS, "%d", datef);
	snprintf(timebuf, MAXCOLDIGITS, "%d", timef);
	snprintf(durationbuf, MAXCOLDIGITS, "%d", durationf);
	snprintf(locationbuf, MAXCOLDIGITS, "%d", locationf);
	snprintf(gpsbuf, MAXCOLDIGITS, "%d", gpsf);
	snprintf(maxdepthbuf, MAXCOLDIGITS, "%d", maxdepthf);
	snprintf(meandepthbuf, MAXCOLDIGITS, "%d", meandepthf);
	snprintf(buddybuf, MAXCOLDIGITS, "%d", buddyf);
	snprintf(notesbuf, MAXCOLDIGITS, "%d", notesf);
	snprintf(weightbuf, MAXCOLDIGITS, "%d", weightf);
	snprintf(tagsbuf, MAXCOLDIGITS, "%d", tagsf);
	snprintf(separator_index, MAXCOLDIGITS, "%d", sepidx);
	snprintf(unit, MAXCOLDIGITS, "%d", units);
	time(&now);
	timep = localtime(&now);
	strftime(curdate, sizeof(curdate), "%Y%m%d", timep);

	/* As the parameter is numeric, we need to ensure that the leading zero
	* is not discarded during the transform, thus prepend time with 1 */
	strftime(curtime, sizeof(curtime), "1%H%M", timep);

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
	params[pnr++] = "buddyField";
	params[pnr++] = buddybuf;
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
	params[pnr++] = NULL;

	if (filename == NULL)
		return report_error("No manual CSV filename");

	if (try_to_xslt_open_csv(filename, &mem, "manualCSV"))
		return -1;

	parse_xml_buffer(filename, mem.buffer, mem.size, &dive_table, (const char **)params);
	free(mem.buffer);
	return 0;
}
