#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "dive.h"
#include "file.h"

static int readfile(const char *filename, struct memblock *mem)
{
	int ret, fd;
	struct stat st;
	char *buf;

	mem->buffer = NULL;
	mem->size = 0;

	fd = open(filename, O_RDONLY);
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
	buf = malloc(st.st_size+1);
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

#ifdef LIBZIP
#include <zip.h>

static void suunto_read(struct zip_file *file, GError **error)
{
	int size = 1024, n, read = 0;
	char *mem = malloc(size);

	while ((n = zip_fread(file, mem+read, size-read)) > 0) {
		read += n;
		size = read * 3 / 2;
		mem = realloc(mem, size);
	}
	parse_xml_buffer("SDE file", mem, read, error);
	free(mem);
}
#endif

static int try_to_open_suunto(const char *filename, struct memblock *mem, GError **error)
{
	int success = 0;
#ifdef LIBZIP
	/* Grr. libzip needs to re-open the file, it can't take a buffer */
	struct zip *zip = zip_open(filename, ZIP_CHECKCONS, NULL);

	if (zip) {
		int index;
		for (index = 0; ;index++) {
			struct zip_file *file = zip_fopen_index(zip, index, 0);
			if (!file)
				break;
			suunto_read(file, error);
			zip_fclose(file);
			success++;
		}
		zip_close(zip);
	}
#endif
	return success;
}

static time_t parse_date(const char *date)
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
	date = p+3;
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
	CSV_DEPTH, CSV_TEMP, CSV_PRESSURE
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
		sample->cylinderpressure.mbar = psi_to_mbar(val*4);
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
	time_t date;
	struct dive *dive;

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

	time = 0;
	for (;;) {
		char *end;
		double val;
		struct sample *sample;

		errno = 0;
		val = strtod(p,&end);
		if (end == p)
			break;
		if (errno)
			break;

		sample = prepare_sample(&dive);
		sample->time.seconds = time;
		add_sample_data(sample, type, val);
		finish_sample(dive);

		time++;
		dive->duration.seconds = time;
		if (*end != ',')
			break;
		p = end+1;
	}
	record_dive(dive);
	return 1;
}

static int open_by_filename(const char *filename, const char *fmt, struct memblock *mem, GError **error)
{
	/* Suunto Dive Manager files: SDE */
	if (!strcasecmp(fmt, "SDE"))
		return try_to_open_suunto(filename, mem, error);

	/* Truly nasty intentionally obfuscated Cochran Anal software */
	if (!strcasecmp(fmt, "CAN"))
		return try_to_open_cochran(filename, mem, error);

	/* Cochran export comma-separated-value files */
	if (!strcasecmp(fmt, "DPT"))
		return try_to_open_csv(filename, mem, CSV_DEPTH);
	if (!strcasecmp(fmt, "TMP"))
		return try_to_open_csv(filename, mem, CSV_TEMP);
	if (!strcasecmp(fmt, "HP1"))
		return try_to_open_csv(filename, mem, CSV_PRESSURE);

	return 0;
}

static void parse_file_buffer(const char *filename, struct memblock *mem, GError **error)
{
	char *fmt = strrchr(filename, '.');
	if (fmt && open_by_filename(filename, fmt+1, mem, error))
		return;

	parse_xml_buffer(filename, mem->buffer, mem->size, error);
}

void parse_file(const char *filename, GError **error)
{
	struct memblock mem;

	if (readfile(filename, &mem) < 0) {
		fprintf(stderr, "Failed to read '%s'.\n", filename);
		if (error) {
			*error = g_error_new(g_quark_from_string("subsurface"),
					     DIVE_ERROR_PARSE,
					     "Failed to read '%s'",
					     filename);
		}
		return;
	}

	parse_file_buffer(filename, &mem, error);
	free(mem.buffer);
}
