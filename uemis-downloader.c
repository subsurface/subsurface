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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <glib/gi18n.h>

#include "uemis.h"
#include "dive.h"
#include "divelist.h"
#include "display.h"
#include "display-gtk.h"

#define ERR_FS_ALMOST_FULL N_("Uemis Zurich: File System is almost full\nDisconnect/reconnect the dive computer\nand click \'Retry\'")
#define ERR_FS_FULL N_("Uemis Zurich: File System is full\nDisconnect/reconnect the dive computer\nand try again")
#define ERR_FS_SHORT_WRITE N_("Short write to req.txt file\nIs the Uemis Zurich plugged in correctly?")
#define BUFLEN 2048
#define NUM_PARAM_BUFS 10

#if UEMIS_DEBUG & 64  /* we are reading from a copy of the filesystem, not the device - no need to wait */
#define UEMIS_TIMEOUT 50		/* 50ns */
#define UEMIS_LONG_TIMEOUT 500		/* 500ns */
#define UEMIS_MAX_TIMEOUT 2000		/* 2ms */
#else
#define UEMIS_TIMEOUT 50000		/* 50ms */
#define UEMIS_LONG_TIMEOUT 500000	/* 500ms */
#define UEMIS_MAX_TIMEOUT 2000000	/* 2s */
#endif

static char *param_buff[NUM_PARAM_BUFS];
static char *reqtxt_path;
static int reqtxt_file;
static int filenr;
static int number_of_files;
static char *mbuf = NULL;
static int mbuf_size = 0;

struct argument_block {
	const char *mountpath;
	progressbar_t *progress;
	gboolean force_download;
};

static int nr_divespots = 0;

static int import_thread_done = 0, import_thread_cancelled;
static const char *progress_bar_text = "";
static double progress_bar_fraction = 0.0;

static GError *error(const char *fmt, ...)
{
	va_list args;
	GError *error;

	va_start(args, fmt);
	error = g_error_new_valist(
		g_quark_from_string("subsurface"),
		DIVE_ERROR_PARSE, fmt, args);
	va_end(args);
	return error;
}

/* helper function to parse the Uemis data structures */
static void uemis_ts(char *buffer, void *_when)
{
	struct tm tm;
	timestamp_t *when = _when;

	memset(&tm, 0, sizeof(tm));
	sscanf(buffer,"%d-%d-%dT%d:%d:%d",
		&tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		&tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	tm.tm_mon  -= 1;
	tm.tm_year -= 1900;
	*when = utc_mktime(&tm);
}

/* float minutes */
static void uemis_duration(char *buffer, duration_t *duration)
{
	duration->seconds = g_ascii_strtod(buffer, NULL) * 60 + 0.5;
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
static void uemis_add_string(char *buffer, char **text)
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
		lbs_to_grams(g_ascii_strtod(buffer, NULL)) : g_ascii_strtod(buffer, NULL) * 1000;
	weight->description = strdup(_("unknown"));
}

static struct dive *uemis_start_dive(uint32_t deviceid)
{
	struct dive *dive = alloc_dive();
	dive->downloaded = TRUE;
	dive->dc.model = strdup("Uemis Zurich");
	dive->dc.deviceid = deviceid;
	return dive;
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
	return result;
}

static int number_of_file(char *path)
{
	int count = 0;
	GDir *dir = g_dir_open(path, 0, NULL);
	while (g_dir_read_name(dir))
		count++;
	g_dir_close(dir);
	return count;
}

/* Check if there's a req.txt file and get the starting filenr from it.
 * Test for the maximum number of ANS files (I believe this is always
 * 4000 but in case there are differences depending on firmware, this
 * code is easy enough */
static gboolean uemis_init(const char *path)
{
	char *ans_path;
	int i;

	if (!path)
		return FALSE;
	/* let's check if this is indeed a Uemis DC */
	reqtxt_path = g_build_filename(path, "/req.txt", NULL);
	reqtxt_file = g_open(reqtxt_path, O_RDONLY, 0666);
	if (!reqtxt_file) {
#if UEMIS_DEBUG & 1
		fprintf(debugfile, ":EE req.txt can't be opened\n");
#endif
		return FALSE;
	}
	if (bytes_available(reqtxt_file) > 5) {
		char tmp[6];
		read(reqtxt_file, tmp, 5);
		tmp[5] = '\0';
#if UEMIS_DEBUG & 2
		fprintf(debugfile, "::r req.txt \"%s\"\n", tmp);
#endif
		if (sscanf(tmp + 1, "%d", &filenr) != 1)
			return FALSE;
	} else {
		filenr = 0;
#if UEMIS_DEBUG & 2
		fprintf(debugfile, "::r req.txt skipped as there were fewer than 5 bytes\n");
#endif
	}
	close (reqtxt_file);

	/* It would be nice if we could simply go back to the first set of
	 * ANS files. But with a FAT filesystem that isn't possible */
	ans_path = g_build_filename(path, "ANS", NULL);
	number_of_files = number_of_file(ans_path);
	g_free(ans_path);
	/* initialize the array in which we collect the answers */
	for (i = 0; i < NUM_PARAM_BUFS; i++)
		param_buff[i] = "";
	return TRUE;
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
	fprintf(debugfile,":tr %s (after seeks)\n", fl);
#endif
	lseek(file, 0, SEEK_SET);
	write(file, fl, strlen(fl));
	lseek(file, tailpos, SEEK_SET);
	write(file, fl + 1, strlen(fl + 1));
#ifndef WIN32
	fsync(file);
#endif
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
	gboolean done = FALSE;
	char *segment;

	while (!done) {
		if (i < size) {
			if (buf[i] == '\\' && i < size - 1 &&
				(buf[i+1] == '\\' || buf[i+1] == '{'))
				memcpy(buf + i, buf + i + 1, size - i - 1);
			else if (buf[i] == '{')
				done = TRUE;
			i++;
		} else {
			done = TRUE;
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
	if (! *buffer) {
		*buffer = strdup(buf);
		*buffer_size = strlen(*buffer) + 1;
	} else {
		*buffer_size += strlen(buf);
		*buffer = realloc(*buffer, *buffer_size);
		strcat(*buffer, buf);
	}
#if UEMIS_DEBUG & 16
	fprintf(debugfile,"added \"%s\" to buffer - new length %d\n", buf, *buffer_size);
#endif
}

/* are there more ANS files we can check? */
static gboolean next_file(int max)
{
	if (filenr >= max)
		return FALSE;
	filenr++;
	return TRUE;
}

static char *first_object_id_val(char* buf)
{
	char *object, *bufend;
	if (!buf)
		return NULL;
	bufend = buf + strlen(buf);
	object = strstr(buf, "object_id");
	if (object && object + 14 < bufend) {
		/* get the value */
		char tmp[10];
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
			*t = '\0';
			return strdup(tmp);
		}
	}
	return NULL;
}

/* ultra-simplistic; it doesn't deal with the case when the object_id is
 * split across two chunks. It also doesn't deal with the discrepancy between
 * object_id and dive number as understood by the dive computer */
static void show_progress(char *buf, char *what)
{
	char *val = first_object_id_val(buf);
	if (val) {
		/* let the user know what we are working on */
#if UEMIS_DEBUG & 2
		fprintf(debugfile,"reading %s %s\n", what, val);
#endif
		uemis_info(_("Reading %s %s"), what, val);
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
static gboolean uemis_get_answer(const char *path, char *request, int n_param_in,
			int n_param_out, char **error_text)
{
	int i = 0, file_length;
	char sb[BUFLEN];
	char fl[13];
	char tmp[101];
	char *what = _("data");
	gboolean searching = TRUE;
	gboolean assembling_mbuf = FALSE;
	gboolean ismulti = FALSE;
	gboolean found_answer = FALSE;
	gboolean more_files = TRUE;
	gboolean answer_in_mbuf = FALSE;
	char *ans_path;
	int ans_file;
	int timeout = UEMIS_LONG_TIMEOUT;

	reqtxt_file = g_open(reqtxt_path, O_RDWR | O_CREAT, 0666);
	snprintf(sb, BUFLEN, "n%04d12345678", filenr);
	str_append_with_delim(sb, request);
	for (i = 0; i < n_param_in; i++)
		str_append_with_delim(sb, param_buff[i]);
	if (! strcmp(request, "getDivelogs") || ! strcmp(request, "getDeviceData") || ! strcmp(request, "getDirectory") ||
		! strcmp(request, "getDivespot") || ! strcmp(request, "getDive")) {
		answer_in_mbuf = TRUE;
		str_append_with_delim(sb, "");
		if (! strcmp(request, "getDivelogs"))
			what = _("divelog entry id");
		else if (!strcmp(request, "getDivespot"))
			what = _("divespot data id");
		else if (!strcmp(request, "getDive"))
			what = _("more data dive id");
	}
	str_append_with_delim(sb, "");
	file_length = strlen(sb);
	snprintf(fl, 10, "%08d", file_length - 13);
	memcpy(sb + 5, fl, strlen(fl));
#if UEMIS_DEBUG & 1
	fprintf(debugfile,"::w req.txt \"%s\"\n", sb);
#endif
	if (write(reqtxt_file, sb, strlen(sb)) != strlen(sb)) {
		*error_text = _(ERR_FS_SHORT_WRITE);
		return FALSE;
	}
	if (! next_file(number_of_files)) {
		*error_text = _(ERR_FS_FULL);
		more_files = FALSE;
	}
	trigger_response(reqtxt_file, "n", filenr, file_length);
	usleep(timeout);
	mbuf = NULL;
	mbuf_size = 0;
	while (searching || assembling_mbuf) {
		if (import_thread_cancelled)
			return FALSE;
		progress_bar_fraction = filenr / 4000.0;
		snprintf(fl, 13, "ANS%d.TXT", filenr - 1);
		ans_path = g_build_filename(path, "ANS", fl, NULL);
		ans_file = g_open(ans_path, O_RDONLY, 0666);
		read(ans_file, tmp, 100);
		close(ans_file);
#if UEMIS_DEBUG & 8
		tmp[100]='\0';
		fprintf(debugfile, "::t %s \"%s\"\n", ans_path, tmp);
#elif UEMIS_DEBUG & 4
		char pbuf[4];
		pbuf[0] = tmp[0];
		pbuf[1] = tmp[1];
		pbuf[2] = tmp[2];
		pbuf[3] = 0;
		fprintf(debugfile, "::t %s \"%s...\"\n", ans_path, pbuf);
#endif
		g_free(ans_path);
		if (tmp[0] == '1') {
			searching = FALSE;
			if (tmp[1] == 'm') {
				assembling_mbuf = TRUE;
				ismulti = TRUE;
			}
			if (tmp[2] == 'e')
				assembling_mbuf = FALSE;
			if (assembling_mbuf) {
				if (! next_file(number_of_files)) {
					*error_text = _(ERR_FS_FULL);
					more_files = FALSE;
					assembling_mbuf = FALSE;
				}
				reqtxt_file = g_open(reqtxt_path, O_RDWR | O_CREAT, 0666);
				trigger_response(reqtxt_file, "n", filenr, file_length);
			}
		} else {
			if (! next_file(number_of_files - 1)) {
				*error_text = _(ERR_FS_FULL);
				more_files = FALSE;
				assembling_mbuf = FALSE;
				searching = FALSE;
			}
			reqtxt_file = g_open(reqtxt_path, O_RDWR | O_CREAT, 0666);
			trigger_response(reqtxt_file, "r", filenr, file_length);
			uemis_increased_timeout(&timeout);
		}
		if (ismulti && more_files && tmp[0] == '1') {
			int size;
			snprintf(fl, 13, "ANS%d.TXT", assembling_mbuf ? filenr - 2 : filenr - 1);
			ans_path = g_build_filename(path, "ANS", fl, NULL);
			ans_file = g_open(ans_path, O_RDONLY, 0666);
			size = bytes_available(ans_file);
			if (size > 3) {
				char *buf = malloc(size - 2);
				lseek(ans_file, 3, SEEK_CUR);
				read(ans_file, buf, size - 3);
				buf[size - 3] = '\0';
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
			ans_path = g_build_filename(path, "ANS", fl, NULL);
			ans_file = g_open(ans_path, O_RDONLY, 0666);
			size = bytes_available(ans_file);
			if (size > 3) {
				buf = malloc(size - 2);
				lseek(ans_file, 3, SEEK_CUR);
				read(ans_file, buf, size - 3);
				buf[size - 3] = '\0';
				buffer_add(&mbuf, &mbuf_size, buf);
				show_progress(buf, what);
#if UEMIS_DEBUG & 8
				fprintf(debugfile, "::r %s \"%s\"\n", ans_path, buf);
#endif
			}
			size -= 3;
			close(ans_file);
			free(ans_path);
		} else {
			ismulti = FALSE;
		}
#if UEMIS_DEBUG & 8
		fprintf(debugfile,":r: %s\n", buf);
#endif
		if (!answer_in_mbuf)
			for (i = 0; i < n_param_out && j < size; i++)
				param_buff[i] = next_segment(buf, &j, size);
		found_answer = TRUE;
		free(buf);
	}
#if UEMIS_DEBUG & 1
	for (i = 0; i < n_param_out; i++)
		fprintf(debugfile,"::: %d: %s\n", i, param_buff[i]);
#endif
	return found_answer;
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
	if (! *tag)
		return;
	type = next_token(&bp);
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
				longitude = g_ascii_strtod(val, NULL);
			else if (!strcmp(tag, "latitude"))
				latitude = g_ascii_strtod(val, NULL);
		}
	} while (tag && *tag);
	uemis_set_divelocation(divespot, locationstring, latitude, longitude);
}

static void track_divespot(char *val, int diveid, char **location, degrees_t *latitude, degrees_t *longitude)
{
	int id = atoi(val);
	if (id >= 0 && id > nr_divespots)
		nr_divespots = id;
	uemis_mark_divelocation(diveid, id, location, latitude, longitude);
	return;
}

static char *suit[] = { "", N_("wetsuit"), N_("semidry"), N_("drysuit") };
static char *suit_type[] = { "", N_("shorty"), N_("vest"), N_("long john"), N_("jacket"), N_("full suit"), N_("2 pcs full suit") };
static char *suit_thickness[] = { "", "0.5-2mm", "2-3mm", "3-5mm", "5-7mm", "8mm+", N_("membrane") };

static void parse_tag(struct dive *dive, char *tag, char *val)
{
	/* we can ignore computer_id, water and gas as those are redundant
	 * with the binary data and would just get overwritten */
	if (! strcmp(tag, "date")) {
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
			uemis_add_string(_(suit[atoi(val)]), &dive->suit);
	} else if (!strcmp(tag, "u8DiveSuitType")) {
		if (*suit_type[atoi(val)])
			uemis_add_string(_(suit_type[atoi(val)]), &dive->suit);
	} else if (!strcmp(tag, "u8SuitThickness")) {
		if (*suit_thickness[atoi(val)])
			uemis_add_string(_(suit_thickness[atoi(val)]), &dive->suit);
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
static void process_raw_buffer(uint32_t deviceid, char *inbuf, char **max_divenr, gboolean keep_number, int *for_dive)
{
	char *buf = strdup(inbuf);
	char *tp, *bp, *tag, *type, *val;
	gboolean done = FALSE;
	int inbuflen = strlen(inbuf);
	char *endptr = buf + inbuflen;
	gboolean log = FALSE;
	char *sections[10];
	int s, nr_sections = 0;
	struct dive *dive = NULL;

	if (for_dive)
		*for_dive = -1;
	bp = buf + 1;
	tp = next_token(&bp);
	if (strcmp(tp, "divelog") == 0) {
		/* this is a divelog */
		log = TRUE;
		tp = next_token(&bp);
		if (strcmp(tp,"1.0") != 0)
			return;
	} else if (strcmp(tp, "dive") == 0) {
		/* this is dive detail */
		tp = next_token(&bp);
		if (strcmp(tp,"1.0") != 0)
			return;
	} else {
		/* don't understand the buffer */
		return;
	}
	if (log)
		dive = uemis_start_dive(deviceid);
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
		if (log && ! strcmp(tag, "object_id")) {
			free(*max_divenr);
			*max_divenr = strdup(val);
			dive->dc.diveid = atoi(val);
			if (keep_number)
				dive->number = atoi(val);
		} else if (!log && ! strcmp(tag, "logfilenr")) {
			/* this one tells us which dive we are adding data to */
			dive = get_dive_by_diveid(atoi(val), deviceid);
			if (for_dive)
				*for_dive = atoi(val);
		} else if (!log && dive && ! strcmp(tag, "divespot_id")) {
			track_divespot(val, dive->dc.diveid, &dive->location, &dive->latitude, &dive->longitude);
		} else if (dive) {
			parse_tag(dive, tag, val);
		}
		if (log && ! strcmp(tag, "file_content"))
			done = TRUE;
		/* done with one dive (got the file_content tag), but there could be more:
		 * a '{' indicates the end of the record - but we need to see another "{{"
		 * later in the buffer to know that the next record is complete (it could
		 * be a short read because of some error */
		if (done && ++bp < endptr && *bp != '{' && strstr(bp, "{{")) {
			done = FALSE;
			record_dive(dive);
			mark_divelist_changed(TRUE);
			dive = uemis_start_dive(deviceid);
		}
	}
	if (log) {
		if (dive->dc.diveid) {
			record_dive(dive);
			mark_divelist_changed(TRUE);
		} else { /* partial dive */
			free(dive);
		}
	}
	free(buf);
	return;
}

static char *uemis_get_divenr(char *deviceidstr)
{
	uint32_t deviceid, maxdiveid = 0;
	int i;
	char divenr[10];

	deviceid = atoi(deviceidstr);
	for (i = 0; i < dive_table.nr; i++) {
		struct divecomputer *dc = &dive_table.dives[i]->dc;
		while (dc) {
			if (dc->model && !strcmp(dc->model, "Uemis Zurich") &&
			    (dc->deviceid == 0 || dc->deviceid == 0x7fffffff || dc->deviceid == deviceid) &&
			    dc->diveid > maxdiveid)
				maxdiveid = dc->diveid;
			dc = dc->next;
		}
	}
	snprintf(divenr, 10, "%d", maxdiveid);
	return strdup(divenr);
}

static char *do_uemis_download(struct argument_block *args)
{
	const char *mountpath = args->mountpath;
	char *newmax = NULL;
	int start, end, i, offset;
	uint32_t deviceidnr;
	char objectid[10];
	char *deviceid = NULL;
	char *result = NULL;
	char *endptr;
	gboolean success, keep_number = FALSE, once = TRUE;

	if (dive_table.nr == 0)
		keep_number = TRUE;
	uemis_info(_("Init Communication"));
	if (! uemis_init(mountpath))
		return _("Uemis init failed");
	if (! uemis_get_answer(mountpath, "getDeviceId", 0, 1, &result))
		goto bail;
	deviceid = strdup(param_buff[0]);
	deviceidnr = atoi(deviceid);
	/* the answer from the DeviceId call becomes the input parameter for getDeviceData */
	if (! uemis_get_answer(mountpath, "getDeviceData", 1, 0, &result))
		goto bail;
	/* param_buff[0] is still valid */
	if (! uemis_get_answer(mountpath, "initSession", 1, 6, &result))
		goto bail;
	uemis_info(_("Start download"));
	if (! uemis_get_answer(mountpath, "processSync", 0, 2, &result))
		goto bail;
	/* before starting the long download, check if user pressed cancel */
	if (import_thread_cancelled)
		goto bail;
	param_buff[1] = "notempty";
	/* if we have an empty divelist or force it, then we start downloading from the
	 * first dive on the Uemis; otherwise check which was the last dive downloaded */
	if (!args->force_download && dive_table.nr > 0)
		newmax = uemis_get_divenr(deviceid);
	else
		newmax = strdup("0");
	start = atoi(newmax);
	for (;;) {
		param_buff[2] = newmax;
		param_buff[3] = 0;
		success = uemis_get_answer(mountpath, "getDivelogs", 3, 0, &result);
		/* process the buffer we have assembled */
		if (mbuf)
			process_raw_buffer(deviceidnr, mbuf, &newmax, keep_number, NULL);
		if (once) {
			char *t = first_object_id_val(mbuf);
			if (t && atoi(t) > start)
				start = atoi(t);
			free(t);
			once = FALSE;
		}
		/* if the user clicked cancel, exit gracefully */
		if (import_thread_cancelled)
			goto bail;
		/* if we got an error or got nothing back, stop trying */
		if (!success || !param_buff[3])
			break;
		/* finally, if the memory is getting too full, maybe we better stop, too */
		if (progress_bar_fraction > 0.85) {
			result = _(ERR_FS_ALMOST_FULL);
			break;
		}
		/* clean up mbuf */
		endptr = strstr(mbuf, "{{{");
		if (endptr)
			*(endptr + 2) = '\0';
	}
	if (sscanf(newmax, "%d", &end) != 1)
		end = start;
#if UEMIS_DEBUG & 2
	fprintf(debugfile, "done: read from object_id %d to %d\n", start, end);
#endif
	free(newmax);
	offset = 0;
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
			process_raw_buffer(deviceidnr, mbuf, &newmax, FALSE, &divenr);
			if (divenr > -1 && divenr != i) {
				offset = i - divenr;
#if UEMIS_DEBUG & 2
				fprintf(debugfile, "got dive %d -> trying again with offset %d\n", divenr, offset);
#endif
				i = start - 1;
				continue;
			}
		}
		if (!success || import_thread_cancelled)
			break;
	}
	success = TRUE;
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
	(void) uemis_get_answer(mountpath, "terminateSync", 0, 3, &result);
	if (! strcmp(param_buff[0], "error")) {
		if (! strcmp(param_buff[2],"Out of Memory"))
			result = _(ERR_FS_FULL);
		else
			result = param_buff[2];
	}
	free(deviceid);
	return result;
}

static void *pthread_wrapper(void *_data)
{
	struct argument_block *args = _data;
	const char *err_string = do_uemis_download(args);
	import_thread_done = 1;
	return (void *)err_string;
}

/* this simply ends the dialog without a response and asks not to be fired again
 * as we set this function up in every loop while uemis_download is waiting for
 * the download to finish */
static gboolean timeout_func(gpointer _data)
{
	GtkDialog *dialog = _data;
	if (!import_thread_cancelled)
		gtk_dialog_response(dialog, GTK_RESPONSE_NONE);
	return FALSE;
}

GError *uemis_download(const char *mountpath, progressbar_t *progress,
			GtkDialog *dialog, gboolean force_download)
{
	pthread_t pthread;
	void *retval;
	struct argument_block args = {mountpath, progress, force_download};

	/* I'm sure there is some better interface for waiting on a thread in a UI main loop */
	import_thread_done = 0;
	progress_bar_text = "";
	progress_bar_fraction = 0.0;
	pthread_create(&pthread, NULL, pthread_wrapper, &args);
	/* loop here until the import is done or was cancelled by the user;
	 * in order to get control back from gtk we register a timeout function
	 * that ends the dialog with no response every 100ms; we then update the
	 * progressbar and setup the timeout again - unless of course the user
	 * pressed cancel, in which case we just wait for the download thread
	 * to react to that and exit */
	while (!import_thread_done) {
		if (!import_thread_cancelled) {
			int result;
			g_timeout_add(100, timeout_func, dialog);
#if USE_GTK_UI
			update_progressbar(args.progress, progress_bar_fraction);
			update_progressbar_text(args.progress, progress_bar_text);
#endif
			result = gtk_dialog_run(dialog);
			if (result == GTK_RESPONSE_CANCEL)
				import_thread_cancelled = TRUE;
		} else {
#if USE_GTK_UI
			update_progressbar(args.progress, progress_bar_fraction);
			update_progressbar_text(args.progress, _("Cancelled, exiting cleanly..."));
#endif
			usleep(100000);
		}
	}
	if (pthread_join(pthread, &retval) < 0)
		return error("Pthread return with error");
	if (retval)
		return error(retval);
	return NULL;
}
