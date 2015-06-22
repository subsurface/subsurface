/*
 * uemis-downloader.c
 *
 * Copyright (c) Dirk Hohndel <dirk@hohndel.org>
 * released under GPL2
 *
 * very (VERY) loosely based on the algorithms found in Java code by Fabian Gast <fgast@only640k.net>
 * which was released under the BSD-STYLE BEER WARE LICENSE
 * I believe that I only used the information about HOW to do this (download data from the Uemis
 * Zurich) but did not actually use any of his copyrighted code, therefore the license under which
 * he released his code does not apply to this new implementation in C
 */
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "gettext.h"
#include "libdivecomputer.h"
#include "uemis.h"
#include "divelist.h"

#define ERR_FS_ALMOST_FULL QT_TRANSLATE_NOOP("gettextFromC", "Uemis Zurich: the file system is almost full.\nDisconnect/reconnect the dive computer\nand click \'Retry\'")
#define ERR_FS_FULL QT_TRANSLATE_NOOP("gettextFromC", "Uemis Zurich: the file system is full.\nDisconnect/reconnect the dive computer\nand try again")
#define ERR_FS_SHORT_WRITE QT_TRANSLATE_NOOP("gettextFromC", "Short write to req.txt file.\nIs the Uemis Zurich plugged in correctly?")
#define BUFLEN 2048
#define NUM_PARAM_BUFS 10

#if UEMIS_DEBUG & 64	   /* we are reading from a copy of the filesystem, not the device - no need to wait */
#define UEMIS_TIMEOUT 50       /* 50ns */
#define UEMIS_LONG_TIMEOUT 500 /* 500ns */
#define UEMIS_MAX_TIMEOUT 2000 /* 2ms */
#else
#define UEMIS_TIMEOUT 50000       /* 50ms */
#define UEMIS_LONG_TIMEOUT 500000 /* 500ms */
#define UEMIS_MAX_TIMEOUT 2000000 /* 2s */
#endif

#ifdef UEMIS_DEBUG
#define debugfile stderr
#endif

static char *param_buff[NUM_PARAM_BUFS];
static char *reqtxt_path;
static int reqtxt_file;
static int filenr;
static int number_of_files;
static char *mbuf = NULL;
static int mbuf_size = 0;

static int nr_divespots = -1;

/* helper function to parse the Uemis data structures */
static void uemis_ts(char *buffer, void *_when)
{
	struct tm tm;
	timestamp_t *when = _when;

	memset(&tm, 0, sizeof(tm));
	sscanf(buffer, "%d-%d-%dT%d:%d:%d",
	       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
	       &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	tm.tm_mon -= 1;
	tm.tm_year -= 1900;
	*when = utc_mktime(&tm);
}

/* float minutes */
static void uemis_duration(char *buffer, duration_t *duration)
{
	duration->seconds = rint(ascii_strtod(buffer, NULL) * 60);
}

/* int cm */
static void uemis_depth(char *buffer, depth_t *depth)
{
	depth->mm = atoi(buffer) * 10;
}

static void uemis_get_index(char *buffer, int *idx)
{
	*idx = atoi(buffer);
}

/* space separated */
static void uemis_add_string(const char *buffer, char **text)
{
	/* do nothing if this is an empty buffer (Uemis sometimes returns a single
	 * space for empty buffers) */
	if (!buffer || !*buffer || (*buffer == ' ' && *(buffer + 1) == '\0'))
		return;
	if (!*text) {
		*text = strdup(buffer);
	} else {
		char *buf = malloc(strlen(buffer) + strlen(*text) + 2);
		strcpy(buf, *text);
		strcat(buf, " ");
		strcat(buf, buffer);
		free(*text);
		*text = buf;
	}
}

/* still unclear if it ever reports lbs */
static void uemis_get_weight(char *buffer, weightsystem_t *weight, int diveid)
{
	weight->weight.grams = uemis_get_weight_unit(diveid) ?
				   lbs_to_grams(ascii_strtod(buffer, NULL)) :
				   ascii_strtod(buffer, NULL) * 1000;
	weight->description = strdup(translate("gettextFromC", "unknown"));
}

static struct dive *uemis_start_dive(uint32_t deviceid)
{
	struct dive *dive = alloc_dive();
	dive->downloaded = true;
	dive->dc.model = strdup("Uemis Zurich");
	dive->dc.deviceid = deviceid;
	return dive;
}

static void record_uemis_dive(device_data_t *devdata, struct dive *dive)
{
	if (devdata->create_new_trip) {
		if (!devdata->trip)
			devdata->trip = create_and_hookup_trip_from_dive(dive);
		else
			add_dive_to_trip(dive, devdata->trip);
	}
	record_dive_to_table(dive, devdata->download_table);
}

/* send text to the importer progress bar */
static void uemis_info(const char *fmt, ...)
{
	static char buffer[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);
	progress_bar_text = buffer;
}

static long bytes_available(int file)
{
	long result;
	long now = lseek(file, 0, SEEK_CUR);
	result = lseek(file, 0, SEEK_END);
	lseek(file, now, SEEK_SET);
	if (now == -1 || result == -1)
		return 0;
	return result;
}

static int number_of_file(char *path)
{
	int count = 0;
#ifdef WIN32
	struct _wdirent *entry;
	_WDIR *dirp = (_WDIR *)subsurface_opendir(path);
#else
	struct dirent *entry;
	DIR *dirp = (DIR *)subsurface_opendir(path);
#endif

	while (dirp) {
#ifdef WIN32
		entry = _wreaddir(dirp);
		if (!entry)
			break;
#else
		entry = readdir(dirp);
		if (!entry)
			break;
		if (entry->d_type == DT_REG) /* If the entry is a regular file */
#endif
		count++;
	}
#ifdef WIN32
	_wclosedir(dirp);
#else
	closedir(dirp);
#endif
	return count;
}

static char *build_filename(const char *path, const char *name)
{
	int len = strlen(path) + strlen(name) + 2;
	char *buf = malloc(len);
#if WIN32
	snprintf(buf, len, "%s\\%s", path, name);
#else
	snprintf(buf, len, "%s/%s", path, name);
#endif
	return buf;
}

/* Check if there's a req.txt file and get the starting filenr from it.
 * Test for the maximum number of ANS files (I believe this is always
 * 4000 but in case there are differences depending on firmware, this
 * code is easy enough */
static bool uemis_init(const char *path)
{
	char *ans_path;
	int i;

	if (!path)
		return false;
	/* let's check if this is indeed a Uemis DC */
	reqtxt_path = build_filename(path, "req.txt");
	reqtxt_file = subsurface_open(reqtxt_path, O_RDONLY, 0666);
	if (reqtxt_file < 0) {
#if UEMIS_DEBUG & 1
		fprintf(debugfile, ":EE req.txt can't be opened\n");
#endif
		return false;
	}
	if (bytes_available(reqtxt_file) > 5) {
		char tmp[6];
		read(reqtxt_file, tmp, 5);
		tmp[5] = '\0';
#if UEMIS_DEBUG & 2
		fprintf(debugfile, "::r req.txt \"%s\"\n", tmp);
#endif
		if (sscanf(tmp + 1, "%d", &filenr) != 1)
			return false;
	} else {
		filenr = 0;
#if UEMIS_DEBUG & 2
		fprintf(debugfile, "::r req.txt skipped as there were fewer than 5 bytes\n");
#endif
	}
	close(reqtxt_file);

	/* It would be nice if we could simply go back to the first set of
	 * ANS files. But with a FAT filesystem that isn't possible */
	ans_path = build_filename(path, "ANS");
	number_of_files = number_of_file(ans_path);
	free(ans_path);
	/* initialize the array in which we collect the answers */
	for (i = 0; i < NUM_PARAM_BUFS; i++)
		param_buff[i] = "";
	return true;
}

static void str_append_with_delim(char *s, char *t)
{
	int len = strlen(s);
	snprintf(s + len, BUFLEN - len, "%s{", t);
}

/* The communication protocoll with the DC is truly funky.
 * After you write your request to the req.txt file you call this function.
 * It writes the number of the next ANS file at the beginning of the req.txt
 * file (prefixed by 'n' or 'r') and then again at the very end of it, after
 * the full request (this time without the prefix).
 * Then it syncs (not needed on Windows) and closes the file. */
static void trigger_response(int file, char *command, int nr, long tailpos)
{
	char fl[10];

	snprintf(fl, 8, "%s%04d", command, nr);
#if UEMIS_DEBUG & 4
	fprintf(debugfile, ":tr %s (after seeks)\n", fl);
#endif
	if (lseek(file, 0, SEEK_SET) == -1)
		goto fs_error;
	if (write(file, fl, strlen(fl)) == -1)
		goto fs_error;
	if (lseek(file, tailpos, SEEK_SET) == -1)
		goto fs_error;
	if (write(file, fl + 1, strlen(fl + 1)) == -1)
		goto fs_error;
#ifndef WIN32
	fsync(file);
#endif
fs_error:
	close(file);
}

static char *next_token(char **buf)
{
	char *q, *p = strchr(*buf, '{');
	if (p)
		*p = '\0';
	else
		p = *buf + strlen(*buf) - 1;
	q = *buf;
	*buf = p + 1;
	return q;
}

/* poor man's tokenizer that understands a quoted delimter ('{') */
static char *next_segment(char *buf, int *offset, int size)
{
	int i = *offset;
	int seg_size;
	bool done = false;
	char *segment;

	while (!done) {
		if (i < size) {
			if (i < size - 1 && buf[i] == '\\' &&
			    (buf[i + 1] == '\\' || buf[i + 1] == '{'))
				memcpy(buf + i, buf + i + 1, size - i - 1);
			else if (buf[i] == '{')
				done = true;
			i++;
		} else {
			done = true;
		}
	}
	seg_size = i - *offset - 1;
	if (seg_size < 0)
		seg_size = 0;
	segment = malloc(seg_size + 1);
	memcpy(segment, buf + *offset, seg_size);
	segment[seg_size] = '\0';
	*offset = i;
	return segment;
}

/* a dynamically growing buffer to store the potentially massive responses.
 * The binary data block can be more than 100k in size (base64 encoded) */
static void buffer_add(char **buffer, int *buffer_size, char *buf)
{
	if (!buf)
		return;
	if (!*buffer) {
		*buffer = strdup(buf);
		*buffer_size = strlen(*buffer) + 1;
	} else {
		*buffer_size += strlen(buf);
		*buffer = realloc(*buffer, *buffer_size);
		strcat(*buffer, buf);
	}
#if UEMIS_DEBUG & 16
	fprintf(debugfile, "added \"%s\" to buffer - new length %d\n", buf, *buffer_size);
#endif
}

/* are there more ANS files we can check? */
static bool next_file(int max)
{
	if (filenr >= max)
		return false;
	filenr++;
	return true;
}

/* try and do a quick decode - without trying to get to fancy in case the data
 * straddles a block boundary...
 * we are parsing something that looks like this:
 * object_id{int{2{date{ts{2011-04-05T12:38:04{duration{float{12.000...
 */
static char *first_object_id_val(char *buf)
{
	char *object, *bufend;
	if (!buf)
		return NULL;
	bufend = buf + strlen(buf);
	object = strstr(buf, "object_id");
	if (object && object + 14 < bufend) {
		/* get the value */
		char tmp[36];
		char *p = object + 14;
		char *t = tmp;

#if UEMIS_DEBUG & 2
		char debugbuf[50];
		strncpy(debugbuf, object, 49);
		debugbuf[49] = '\0';
		fprintf(debugfile, "buf |%s|\n", debugbuf);
#endif
		while (p < bufend && *p != '{' && t < tmp + 9)
			*t++ = *p++;
		if (*p == '{') {
			/* found the object_id - let's quickly look for the date */
			if (strncmp(p, "{date{ts{", 9) == 0 && strstr(p, "{duration{") != NULL) {
				/* cool - that was easy */
				*t++ = ',';
				*t++ = ' ';
				/* skip the 9 characters we just found and take the date, ignoring the seconds
				 * and replace the silly 'T' in the middle with a space */
				strncpy(t, p + 9, 16);
				if (*(t + 10) == 'T')
					*(t + 10) = ' ';
				t += 16;
			}
			*t = '\0';
			return strdup(tmp);
		}
	}
	return NULL;
}

/* ultra-simplistic; it doesn't deal with the case when the object_id is
 * split across two chunks. It also doesn't deal with the discrepancy between
 * object_id and dive number as understood by the dive computer */
static void show_progress(char *buf, const char *what)
{
	char *val = first_object_id_val(buf);
	if (val) {
/* let the user know what we are working on */
#if UEMIS_DEBUG & 2
		fprintf(stderr, "reading %s %s %s\n", what, val, buf);
#endif
		uemis_info(translate("gettextFromC", "%s %s"), what, val);
		free(val);
	}
}

static void uemis_increased_timeout(int *timeout)
{
	if (*timeout < UEMIS_MAX_TIMEOUT)
		*timeout += UEMIS_LONG_TIMEOUT;
	usleep(*timeout);
}

/* send a request to the dive computer and collect the answer */
static bool uemis_get_answer(const char *path, char *request, int n_param_in,
			     int n_param_out, const char **error_text)
{
	int i = 0, file_length;
	char sb[BUFLEN];
	char fl[13];
	char tmp[101];
	const char *what = translate("gettextFromC", "data");
	bool searching = true;
	bool assembling_mbuf = false;
	bool ismulti = false;
	bool found_answer = false;
	bool more_files = true;
	bool answer_in_mbuf = false;
	char *ans_path;
	int ans_file;
	int timeout = UEMIS_LONG_TIMEOUT;

	reqtxt_file = subsurface_open(reqtxt_path, O_RDWR | O_CREAT, 0666);
	if (reqtxt_file == -1) {
		*error_text = "can't open req.txt";
		fprintf(stderr, "open %s failed with errno %d\n", reqtxt_path, errno);
		return false;
	}
	snprintf(sb, BUFLEN, "n%04d12345678", filenr);
	str_append_with_delim(sb, request);
	for (i = 0; i < n_param_in; i++)
		str_append_with_delim(sb, param_buff[i]);
	if (!strcmp(request, "getDivelogs") || !strcmp(request, "getDeviceData") || !strcmp(request, "getDirectory") ||
	    !strcmp(request, "getDivespot") || !strcmp(request, "getDive")) {
		answer_in_mbuf = true;
		str_append_with_delim(sb, "");
		if (!strcmp(request, "getDivelogs"))
			what = translate("gettextFromC", "divelog #");
		else if (!strcmp(request, "getDivespot"))
			what = translate("gettextFromC", "divespot #");
		else if (!strcmp(request, "getDive"))
			what = translate("gettextFromC", "details for #");
	}
	str_append_with_delim(sb, "");
	file_length = strlen(sb);
	snprintf(fl, 10, "%08d", file_length - 13);
	memcpy(sb + 5, fl, strlen(fl));
#if UEMIS_DEBUG & 1
	fprintf(debugfile, "::w req.txt \"%s\"\n", sb);
#endif
	if (write(reqtxt_file, sb, strlen(sb)) != strlen(sb)) {
		*error_text = translate("gettextFromC", ERR_FS_SHORT_WRITE);
		return false;
	}
	if (!next_file(number_of_files)) {
		*error_text = translate("gettextFromC", ERR_FS_FULL);
		more_files = false;
	}
	trigger_response(reqtxt_file, "n", filenr, file_length);
	usleep(timeout);
	mbuf = NULL;
	mbuf_size = 0;
	while (searching || assembling_mbuf) {
		if (import_thread_cancelled)
			return false;
		progress_bar_fraction = filenr / 4000.0;
		snprintf(fl, 13, "ANS%d.TXT", filenr - 1);
		ans_path = build_filename(build_filename(path, "ANS"), fl);
		ans_file = subsurface_open(ans_path, O_RDONLY, 0666);
		read(ans_file, tmp, 100);
		close(ans_file);
#if UEMIS_DEBUG & 8
		tmp[100] = '\0';
		fprintf(debugfile, "::t %s \"%s\"\n", ans_path, tmp);
#elif UEMIS_DEBUG & 4
		char pbuf[4];
		pbuf[0] = tmp[0];
		pbuf[1] = tmp[1];
		pbuf[2] = tmp[2];
		pbuf[3] = 0;
		fprintf(debugfile, "::t %s \"%s...\"\n", ans_path, pbuf);
#endif
		free(ans_path);
		if (tmp[0] == '1') {
			searching = false;
			if (tmp[1] == 'm') {
				assembling_mbuf = true;
				ismulti = true;
			}
			if (tmp[2] == 'e')
				assembling_mbuf = false;
			if (assembling_mbuf) {
				if (!next_file(number_of_files)) {
					*error_text = translate("gettextFromC", ERR_FS_FULL);
					more_files = false;
					assembling_mbuf = false;
				}
				reqtxt_file = subsurface_open(reqtxt_path, O_RDWR | O_CREAT, 0666);
				trigger_response(reqtxt_file, "n", filenr, file_length);
			}
		} else {
			if (!next_file(number_of_files - 1)) {
				*error_text = translate("gettextFromC", ERR_FS_FULL);
				more_files = false;
				assembling_mbuf = false;
				searching = false;
			}
			reqtxt_file = subsurface_open(reqtxt_path, O_RDWR | O_CREAT, 0666);
			trigger_response(reqtxt_file, "r", filenr, file_length);
			uemis_increased_timeout(&timeout);
		}
		if (ismulti && more_files && tmp[0] == '1') {
			int size;
			snprintf(fl, 13, "ANS%d.TXT", assembling_mbuf ? filenr - 2 : filenr - 1);
			ans_path = build_filename(build_filename(path, "ANS"), fl);
			ans_file = subsurface_open(ans_path, O_RDONLY, 0666);
			free(ans_path);
			size = bytes_available(ans_file);
			if (size > 3) {
				char *buf;
				int r;
				if (lseek(ans_file, 3, SEEK_CUR) == -1)
					goto fs_error;
				buf = malloc(size - 2);
				if ((r = read(ans_file, buf, size - 3)) != size - 3) {
					free(buf);
					goto fs_error;
				}
				buf[r] = '\0';
				buffer_add(&mbuf, &mbuf_size, buf);
				show_progress(buf, what);
				free(buf);
				param_buff[3]++;
			}
			close(ans_file);
			timeout = UEMIS_TIMEOUT;
			usleep(UEMIS_TIMEOUT);
		}
	}
	if (more_files) {
		int size = 0, j = 0;
		char *buf = NULL;

		if (!ismulti) {
			snprintf(fl, 13, "ANS%d.TXT", filenr - 1);
			ans_path = build_filename(build_filename(path, "ANS"), fl);
			ans_file = subsurface_open(ans_path, O_RDONLY, 0666);
			free(ans_path);
			size = bytes_available(ans_file);
			if (size > 3) {
				int r;
				if (lseek(ans_file, 3, SEEK_CUR) == -1)
					goto fs_error;
				buf = malloc(size - 2);
				if ((r = read(ans_file, buf, size - 3)) != size - 3) {
					free(buf);
					goto fs_error;
				}
				buf[r] = '\0';
				buffer_add(&mbuf, &mbuf_size, buf);
				show_progress(buf, what);
#if UEMIS_DEBUG & 8
				fprintf(debugfile, "::r %s \"%s\"\n", fl, buf);
#endif
			}
			size -= 3;
			close(ans_file);
		} else {
			ismulti = false;
		}
#if UEMIS_DEBUG & 8
		fprintf(debugfile, ":r: %s\n", buf);
#endif
		if (!answer_in_mbuf)
			for (i = 0; i < n_param_out && j < size; i++)
				param_buff[i] = next_segment(buf, &j, size);
		found_answer = true;
		free(buf);
	}
#if UEMIS_DEBUG & 1
	for (i = 0; i < n_param_out; i++)
		fprintf(debugfile, "::: %d: %s\n", i, param_buff[i]);
#endif
	return found_answer;
fs_error:
	close (ans_file);
	return false;
}

static void parse_divespot(char *buf)
{
	char *bp = buf + 1;
	char *tp = next_token(&bp);
	char *tag, *type, *val;
	char locationstring[1024] = "";
	int divespot, len;
	double latitude = 0.0, longitude = 0.0;


	if (strcmp(tp, "divespot"))
		return;
	do
		tag = next_token(&bp);
	while (*tag && strcmp(tag, "object_id"));
	if (!*tag)
		return;
	next_token(&bp);
	val = next_token(&bp);
	divespot = atoi(val);
	do {
		tag = next_token(&bp);
		type = next_token(&bp);
		val = next_token(&bp);
		if (!strcmp(type, "string") && *val && strcmp(val, " ")) {
			len = strlen(locationstring);
			snprintf(locationstring + len, sizeof(locationstring) - len,
				 "%s%s", len ? ", " : "", val);
		} else if (!strcmp(type, "float")) {
			if (!strcmp(tag, "longitude"))
				longitude = ascii_strtod(val, NULL);
			else if (!strcmp(tag, "latitude"))
				latitude = ascii_strtod(val, NULL);
		}
	} while (tag && *tag);
	uemis_set_divelocation(divespot, locationstring, longitude, latitude);
}

static void track_divespot(char *val, int diveid, uint32_t dive_site_uuid)
{
	int id = atoi(val);
	if (id >= 0 && id > nr_divespots)
		nr_divespots = id;
	uemis_mark_divelocation(diveid, id, dive_site_uuid);
	return;
}

static char *suit[] = { "", QT_TRANSLATE_NOOP("gettextFromC", "wetsuit"), QT_TRANSLATE_NOOP("gettextFromC", "semidry"), QT_TRANSLATE_NOOP("gettextFromC", "drysuit") };
static char *suit_type[] = { "", QT_TRANSLATE_NOOP("gettextFromC", "shorty"), QT_TRANSLATE_NOOP("gettextFromC", "vest"), QT_TRANSLATE_NOOP("gettextFromC", "long john"), QT_TRANSLATE_NOOP("gettextFromC", "jacket"), QT_TRANSLATE_NOOP("gettextFromC", "full suit"), QT_TRANSLATE_NOOP("gettextFromC", "2 pcs full suit") };
static char *suit_thickness[] = { "", "0.5-2mm", "2-3mm", "3-5mm", "5-7mm", "8mm+", QT_TRANSLATE_NOOP("gettextFromC", "membrane") };

static void parse_tag(struct dive *dive, char *tag, char *val)
{
	/* we can ignore computer_id, water and gas as those are redundant
	 * with the binary data and would just get overwritten */
	if (!strcmp(tag, "date")) {
		uemis_ts(val, &dive->when);
	} else if (!strcmp(tag, "duration")) {
		uemis_duration(val, &dive->dc.duration);
	} else if (!strcmp(tag, "depth")) {
		uemis_depth(val, &dive->dc.maxdepth);
	} else if (!strcmp(tag, "file_content")) {
		uemis_parse_divelog_binary(val, dive);
	} else if (!strcmp(tag, "altitude")) {
		uemis_get_index(val, &dive->dc.surface_pressure.mbar);
	} else if (!strcmp(tag, "f32Weight")) {
		uemis_get_weight(val, &dive->weightsystem[0], dive->dc.diveid);
	} else if (!strcmp(tag, "notes")) {
		uemis_add_string(val, &dive->notes);
	} else if (!strcmp(tag, "u8DiveSuit")) {
		if (*suit[atoi(val)])
			uemis_add_string(translate("gettextFromC", suit[atoi(val)]), &dive->suit);
	} else if (!strcmp(tag, "u8DiveSuitType")) {
		if (*suit_type[atoi(val)])
			uemis_add_string(translate("gettextFromC", suit_type[atoi(val)]), &dive->suit);
	} else if (!strcmp(tag, "u8SuitThickness")) {
		if (*suit_thickness[atoi(val)])
			uemis_add_string(translate("gettextFromC", suit_thickness[atoi(val)]), &dive->suit);
	}
}

/* This function is called for both divelog and dive information that we get
 * from the SDA (what an insane design, btw). The object_id in the divelog
 * matches the logfilenr in the dive information (which has its own, often
 * different object_id) - we use this as the diveid.
 * We create the dive when parsing the divelog and then later, when we parse
 * the dive information we locate the already created dive via its diveid.
 * Most things just get parsed and converted into our internal data structures,
 * but the dive location API is even more crazy. We just get an id that is an
 * index into yet another data store that we read out later. In order to
 * correctly populate the location and gps data from that we need to remember
 * the adresses of those fields for every dive that references the divespot. */
static bool process_raw_buffer(device_data_t *devdata, uint32_t deviceid, char *inbuf, char **max_divenr, bool keep_number, int *for_dive)
{
	char *buf = strdup(inbuf);
	char *tp, *bp, *tag, *type, *val;
	bool done = false;
	int inbuflen = strlen(inbuf);
	char *endptr = buf + inbuflen;
	bool log = false;
	char *sections[10];
	int s, nr_sections = 0;
	struct dive *dive = NULL;

#if UEMIS_DEBUG & 4
	fprintf(debugfile, "p_r_b %s\n", inbuf);
#endif
	if (for_dive)
		*for_dive = -1;
	bp = buf + 1;
	tp = next_token(&bp);
	if (strcmp(tp, "divelog") == 0) {
		/* this is a divelog */
		log = true;
		tp = next_token(&bp);
		/* is it a valid entry or nothing ? */
		if (strcmp(tp, "1.0") != 0 || strstr(inbuf, "divelog{1.0{{{{")) {
			free(buf);
			return false;
		}
	} else if (strcmp(tp, "dive") == 0) {
		/* this is dive detail */
		tp = next_token(&bp);
		if (strcmp(tp, "1.0") != 0) {
			free(buf);
			return false;
		}
	} else {
		/* don't understand the buffer */
		free(buf);
		return false;
	}
	if (log) {
		dive = uemis_start_dive(deviceid);
	} else {
		/* remember, we don't know if this is the right entry,
		 * so first test if this is even a valid entry */
		if (strstr(inbuf, "deleted{bool{true")) {
#if UEMIS_DEBUG & 4
			fprintf(debugfile, "p_r_b entry deleted\n");
#endif
			/* oops, this one isn't valid, suggest to try the previous one */
			free(buf);
			return false;
		}
	}
	while (!done) {
		/* the valid buffer ends with a series of delimiters */
		if (bp >= endptr - 2 || !strcmp(bp, "{{"))
			break;
		tag = next_token(&bp);
		/* we also end if we get an empty tag */
		if (*tag == '\0')
			break;
		for (s = 0; s < nr_sections; s++)
			if (!strcmp(tag, sections[s])) {
				tag = next_token(&bp);
				break;
			}
		type = next_token(&bp);
		if (!strcmp(type, "1.0")) {
			/* this tells us the sections that will follow; the tag here
			 * is of the format dive-<section> */
			sections[nr_sections] = strchr(tag, '-') + 1;
#if UEMIS_DEBUG & 4
			fprintf(debugfile, "Expect to find section %s\n", sections[nr_sections]);
#endif
			if (nr_sections < sizeof(sections) - 1)
				nr_sections++;
			continue;
		}
		val = next_token(&bp);
		if (log && !strcmp(tag, "object_id")) {
			free(*max_divenr);
			*max_divenr = strdup(val);
			dive->dc.diveid = atoi(val);
			if (keep_number)
				dive->number = atoi(val);
		} else if (!log && !strcmp(tag, "logfilenr")) {
			/* this one tells us which dive we are adding data to */
			dive = get_dive_by_uemis_diveid(atoi(val), deviceid);
			if (for_dive)
				*for_dive = atoi(val);
		} else if (!log && dive && !strcmp(tag, "divespot_id")) {
			dive->dive_site_uuid = create_dive_site("from Uemis");
			track_divespot(val, dive->dc.diveid, dive->dive_site_uuid);
		} else if (dive) {
			parse_tag(dive, tag, val);
		}
		if (log && !strcmp(tag, "file_content"))
			done = true;
		/* done with one dive (got the file_content tag), but there could be more:
		 * a '{' indicates the end of the record - but we need to see another "{{"
		 * later in the buffer to know that the next record is complete (it could
		 * be a short read because of some error */
		if (done && ++bp < endptr && *bp != '{' && strstr(bp, "{{")) {
			done = false;
			record_uemis_dive(devdata, dive);
			mark_divelist_changed(true);
			dive = uemis_start_dive(deviceid);
		}
	}
	if (log) {
		if (dive->dc.diveid) {
			record_uemis_dive(devdata, dive);
			mark_divelist_changed(true);
		} else { /* partial dive */
			free(dive);
			free(buf);
			return false;
		}
	}
	free(buf);
	return true;
}

static int max_diveid_from_dialog;

void uemis_set_max_diveid_from_dialog(int diveid)
{
	max_diveid_from_dialog = diveid;
}

static char *uemis_get_divenr(char *deviceidstr)
{
	uint32_t deviceid, maxdiveid = 0;
	int i;
	char divenr[10];

	deviceid = atoi(deviceidstr);
	struct dive *d;
	for_each_dive (i, d) {
		struct divecomputer *dc;
		for_each_dc(d, dc) {
			if (dc->model && !strcmp(dc->model, "Uemis Zurich") &&
			    (dc->deviceid == 0 || dc->deviceid == 0x7fffffff || dc->deviceid == deviceid) &&
			    dc->diveid > maxdiveid)
				maxdiveid = dc->diveid;
		}
	}
	snprintf(divenr, 10, "%d", maxdiveid > max_diveid_from_dialog ? maxdiveid : max_diveid_from_dialog);
	return strdup(divenr);
}

const char *do_uemis_import(device_data_t *data)
{
	const char *mountpath = data->devname;
	short force_download = data->force_download;
	char *newmax = NULL;
	int first, start, end = -2;
	int i, offset = 0;
	uint32_t deviceidnr;
	char objectid[10];
	char *deviceid = NULL;
	const char *result = NULL;
	char *endptr;
	bool success, keep_number = false, once = true;

	if (dive_table.nr == 0)
		keep_number = true;
	uemis_info(translate("gettextFromC", "Initialise communication"));
	if (!uemis_init(mountpath)) {
		free(reqtxt_path);
		return translate("gettextFromC", "Uemis init failed");
	}
	if (!uemis_get_answer(mountpath, "getDeviceId", 0, 1, &result))
		goto bail;
	deviceid = strdup(param_buff[0]);
	deviceidnr = atoi(deviceid);
	/* the answer from the DeviceId call becomes the input parameter for getDeviceData */
	if (!uemis_get_answer(mountpath, "getDeviceData", 1, 0, &result))
		goto bail;
	/* param_buff[0] is still valid */
	if (!uemis_get_answer(mountpath, "initSession", 1, 6, &result))
		goto bail;
	uemis_info(translate("gettextFromC", "Start download"));
	if (!uemis_get_answer(mountpath, "processSync", 0, 2, &result))
		goto bail;
	/* before starting the long download, check if user pressed cancel */
	if (import_thread_cancelled)
		goto bail;
	param_buff[1] = "notempty";
	/* if we force it we start downloading from the first dive on the Uemis;
	 *  otherwise check which was the last dive downloaded */
	if (!force_download)
		newmax = uemis_get_divenr(deviceid);
	else
		newmax = strdup("0");
	first = start = atoi(newmax);
	for (;;) {
#if UEMIS_DEBUG & 4
		fprintf(debugfile, "d_u_i inner loop start %d end %d newmax %s\n", start, end, newmax);
#endif
		param_buff[2] = newmax;
		param_buff[3] = 0;
		success = uemis_get_answer(mountpath, "getDivelogs", 3, 0, &result);
		/* process the buffer we have assembled */
		if (mbuf)
			if (!process_raw_buffer(data, deviceidnr, mbuf, &newmax, keep_number, NULL)) {
				/* if no dives were downloaded, mark end appropriately */
				if (end == -2)
					end = start - 1;
				success = false;
			}
		if (once) {
			char *t = first_object_id_val(mbuf);
			if (t && atoi(t) > start)
				start = atoi(t);
			free(t);
			once = false;
		}
		/* clean up mbuf */
		endptr = strstr(mbuf, "{{{");
		if (endptr)
			*(endptr + 2) = '\0';
		/* last object_id we parsed */
		sscanf(newmax, "%d", &end);
#if UEMIS_DEBUG & 4
		fprintf(debugfile, "d_u_i after download and parse start %d end %d newmax %s progress %4.2f\n", start, end, newmax, progress_bar_fraction);
#endif
		/* now download the additional dive data with "getDive" for the dives
		 * we just downloaded - yes, this is completely insane - why isn't all of
		 * this downloaded in the first place??? */
		for (i = start; i <= end; i++) {
			snprintf(objectid, sizeof(objectid), "%d", i + offset);
			param_buff[2] = objectid;
#if UEMIS_DEBUG & 2
			fprintf(debugfile, "getDive %d, object_id %s\n", i, objectid);
#endif
			/* there is no way I have found to directly get the dive information
			 * for dive #i as the object_id and logfilenr can be different in the
			 * getDive call; so we get the first one, compare the actual divenr
			 * with the one that we wanted, calculate the offset and try again.
			 * What an insane design... */
			success = uemis_get_answer(mountpath, "getDive", 3, 0, &result);
			if (mbuf) {
				int divenr;
				(void)process_raw_buffer(data, deviceidnr, mbuf, &newmax, false, &divenr);
#if UEMIS_DEBUG & 2
				fprintf(debugfile, "got dive %d, looking for dive %d\n", divenr, i);
#endif
				if (divenr != i) {
					if (divenr == -1) {
						offset--;
					} else {
						offset += i - divenr;
					}
#if UEMIS_DEBUG & 2
					fprintf(debugfile, " -> trying again with offset %d\n", offset);
#endif
					i = start - 1;
					if (i + offset < 0)
						break;
					continue;
				}
			}
			if (!success || import_thread_cancelled)
				break;
		}
		start = end + 1;
		/* if the user clicked cancel, exit gracefully */
		if (import_thread_cancelled)
			goto bail;
		/* if we got an error or got nothing back, stop trying */
		if (!success || !param_buff[3])
			break;
		/* finally, if the memory is getting too full, maybe we better stop, too */
		if (progress_bar_fraction > 0.80) {
			result = translate("gettextFromC", ERR_FS_ALMOST_FULL);
			break;
		}
	}
	if (end == -2 && sscanf(newmax, "%d", &end) != 1)
		end = start;
#if UEMIS_DEBUG & 2
	fprintf(debugfile, "done: read from object_id %d to %d\n", first, end);
#endif
	free(newmax);
	success = true;
	for (i = 0; i <= nr_divespots; i++) {
		char divespotnr[10];
		snprintf(divespotnr, sizeof(divespotnr), "%d", i);
		param_buff[2] = divespotnr;
#if UEMIS_DEBUG & 2
		fprintf(debugfile, "getDivespot %d\n", i);
#endif
		success = uemis_get_answer(mountpath, "getDivespot", 3, 0, &result);
		if (mbuf)
			parse_divespot(mbuf);
	}
bail:
	(void)uemis_get_answer(mountpath, "terminateSync", 0, 3, &result);
	if (!strcmp(param_buff[0], "error")) {
		if (!strcmp(param_buff[2], "Out of Memory"))
			result = translate("gettextFromC", ERR_FS_FULL);
		else
			result = param_buff[2];
	}
	free(deviceid);
	free(reqtxt_path);
	return result;
}
