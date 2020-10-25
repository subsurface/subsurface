// SPDX-License-Identifier: GPL-2.0
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
 *
 * Modified by Guido Lerch guido.lerch@gmail.com in August 2015
 */
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "gettext.h"
#include "libdivecomputer.h"
#include "uemis.h"
#include "divelist.h"
#include "divesite.h"
#include "errorhelper.h"
#include "file.h"
#include "tag.h"
#include "subsurface-time.h"
#include "core/subsurface-string.h"

#define ERR_FS_ALMOST_FULL QT_TRANSLATE_NOOP("gettextFromC", "Uemis Zurich: the file system is almost full.\nDisconnect/reconnect the dive computer\nand click \'Retry\'")
#define ERR_FS_FULL QT_TRANSLATE_NOOP("gettextFromC", "Uemis Zurich: the file system is full.\nDisconnect/reconnect the dive computer\nand click Retry")
#define ERR_FS_SHORT_WRITE QT_TRANSLATE_NOOP("gettextFromC", "Short write to req.txt file.\nIs the Uemis Zurich plugged in correctly?")
#define ERR_NO_FILES QT_TRANSLATE_NOOP("gettextFromC", "No dives to download.")
#define BUFLEN 2048
#define BUFLEN 2048
#define NUM_PARAM_BUFS 10

// debugging setup
// #define UEMIS_DEBUG 1 + 2 + 4 + 8 + 16 + 32

#define UEMIS_MAX_FILES 4000
#define UEMIS_MEM_FULL 1
#define UEMIS_MEM_OK 0
#define UEMIS_SPOT_BLOCK_SIZE 1
#define UEMIS_DIVE_DETAILS_SIZE 2
#define UEMIS_LOG_BLOCK_SIZE 10
#define UEMIS_CHECK_LOG 1
#define UEMIS_CHECK_DETAILS 2
#define UEMIS_CHECK_SINGLE_DIVE 3

#if UEMIS_DEBUG
const char *home, *user, *d_time;
static int debug_round = 0;
#define debugfile stderr
#endif

#if UEMIS_DEBUG & 64		/* we are reading from a copy of the filesystem, not the device - no need to wait */
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

static int max_mem_used = -1;
static int next_table_index = 0;
static int dive_to_read = 0;
static uint32_t mindiveid;

/* Linked list to remember already executed divespot download requests */
struct divespot_mapping {
	int divespot_id;
	struct dive_site *dive_site;
	struct divespot_mapping *next;
};
static struct divespot_mapping *divespot_mapping = NULL;

static void erase_divespot_mapping()
{
	struct divespot_mapping *tmp;
	while (divespot_mapping != NULL) {
		tmp = divespot_mapping;
		divespot_mapping = tmp->next;
		free(tmp);
	}
	divespot_mapping = NULL;
}

static void add_to_divespot_mapping(int divespot_id, struct dive_site *ds)
{
	struct divespot_mapping *ndm = (struct divespot_mapping*)calloc(1, sizeof(struct divespot_mapping));
	struct divespot_mapping **pdm = &divespot_mapping;
	struct divespot_mapping *cdm = *pdm;

	while (cdm && cdm->next)
		cdm = cdm->next;

	ndm->divespot_id = divespot_id;
	ndm->dive_site = ds;
	ndm->next = NULL;
	if (cdm)
		cdm->next = ndm;
	else
		cdm = *pdm = ndm;
}

static bool is_divespot_mappable(int divespot_id)
{
	struct divespot_mapping *dm = divespot_mapping;
	while (dm) {
		if (dm->divespot_id == divespot_id)
			return true;
		dm = dm->next;
	}
	return false;
}

static struct dive_site *get_dive_site_by_divespot_id(int divespot_id)
{
	struct divespot_mapping *dm = divespot_mapping;
	while (dm) {
		if (dm->divespot_id == divespot_id)
			return dm->dive_site;
		dm = dm->next;
	}
	return NULL;
}

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
	*when = utc_mktime(&tm);
}

/* float minutes */
static void uemis_duration(char *buffer, duration_t *duration)
{
	duration->seconds = lrint(ascii_strtod(buffer, NULL) * 60);
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
static void uemis_add_string(const char *buffer, char **text, const char *delimit)
{
	/* do nothing if this is an empty buffer (Uemis sometimes returns a single
	 * space for empty buffers) */
	if (empty_string(buffer) || (*buffer == ' ' && *(buffer + 1) == '\0'))
		return;
	if (!*text) {
		*text = strdup(buffer);
	} else {
		char *buf = malloc(strlen(buffer) + strlen(*text) + 2);
		strcpy(buf, *text);
		strcat(buf, delimit);
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
		lrint(ascii_strtod(buffer, NULL) * 1000);
	weight->description = translate("gettextFromC", "unknown");
}

static struct dive *uemis_start_dive(uint32_t deviceid)
{
	struct dive *dive = alloc_dive();
	dive->dc.model = strdup("Uemis Zurich");
	dive->dc.deviceid = deviceid;
	return dive;
}

static struct dive *get_dive_by_uemis_diveid(device_data_t *devdata, uint32_t object_id)
{
	for (int i = 0; i < devdata->download_table->nr; i++) {
		if (object_id == devdata->download_table->dives[i]->dc.diveid)
			return devdata->download_table->dives[i];
	}
	return NULL;
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
	if (verbose)
		fprintf(stderr, "Uemis downloader: %s\n", buffer);
}

static long bytes_available(int file)
{
	long result, r2;
	long now = lseek(file, 0, SEEK_CUR);
	if (now == -1)
		return 0;
	result = lseek(file, 0, SEEK_END);
	r2 = lseek(file, now, SEEK_SET);
	if (now == -1 || result == -1 || r2 == -1)
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
		if (strstr(entry->d_name, ".TXT") || strstr(entry->d_name, ".txt")) /* If the entry is a regular file */
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
	erase_divespot_mapping();
	if (!path)
		return false;
	/* let's check if this is indeed a Uemis DC */
	reqtxt_path = build_filename(path, "req.txt");
	reqtxt_file = subsurface_open(reqtxt_path, O_RDONLY | O_CREAT, 0666);
	if (reqtxt_file < 0) {
#if UEMIS_DEBUG & 1
		fprintf(debugfile, ":EE req.txt can't be opened\n");
#endif
		return false;
	}
	if (bytes_available(reqtxt_file) > 5) {
		char tmp[6];
		if (read(reqtxt_file, tmp, 5) != 5)
			return false;
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

/* The communication protocol with the DC is truly funky.
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

/* poor man's tokenizer that understands a quoted delimiter ('{') */
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
#if UEMIS_DEBUG & 8
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

#if UEMIS_DEBUG & 4
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
#if UEMIS_DEBUG & 8
		fprintf(debugfile, "reading %s\n %s\n %s\n", what, val, buf);
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

static char *build_ans_path(const char *path, int filenumber)
{
	char *intermediate, *ans_path, fl[13];

	/* Clamp filenumber into the 0..9999 range. This is never necessary,
	 * as filenumber can never go above UEMIS_MAX_FILES, but gcc doesn't
	 * recognize that and produces very noisy warnings. */
	filenumber = filenumber < 0 ? 0 : filenumber % 10000;

	snprintf(fl, 13, "ANS%d.TXT", filenumber);
	intermediate = build_filename(path, "ANS");
	ans_path = build_filename(intermediate, fl);
	free(intermediate);
	return ans_path;
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
	if (reqtxt_file < 0) {
		*error_text = "can't open req.txt";
#ifdef UEMIS_DEBUG
		fprintf(debugfile, "open %s failed with errno %d\n", reqtxt_path, errno);
#endif
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
			what = translate("gettextFromC", "dive log #");
		else if (!strcmp(request, "getDivespot"))
			what = translate("gettextFromC", "dive spot #");
		else if (!strcmp(request, "getDive"))
			what = translate("gettextFromC", "details for #");
	}
	str_append_with_delim(sb, "");
	file_length = strlen(sb);
	snprintf(fl, 10, "%08d", file_length - 13);
	memcpy(sb + 5, fl, strlen(fl));
#if UEMIS_DEBUG & 4
	fprintf(debugfile, "::w req.txt \"%s\"\n", sb);
#endif
	int written = write(reqtxt_file, sb, strlen(sb));
	if (written == -1 || (size_t)written != strlen(sb)) {
		*error_text = translate("gettextFromC", ERR_FS_SHORT_WRITE);
		return false;
	}
	if (!next_file(number_of_files)) {
		*error_text = translate("gettextFromC", ERR_FS_FULL);
		more_files = false;
	}
	trigger_response(reqtxt_file, "n", filenr, file_length);
	usleep(timeout);
	free(mbuf);
	mbuf = NULL;
	mbuf_size = 0;
	while (searching || assembling_mbuf) {
		if (import_thread_cancelled)
			return false;
		progress_bar_fraction = filenr / (double)UEMIS_MAX_FILES;
		ans_path = build_ans_path(path, filenr - 1);
		ans_file = subsurface_open(ans_path, O_RDONLY, 0666);
		if (ans_file < 0) {
			*error_text = "can't open Uemis response file";
#ifdef UEMIS_DEBUG
			fprintf(debugfile, "open %s failed with errno %d\n", ans_path, errno);
#endif
			free(ans_path);
			return false;
		}
		if (read(ans_file, tmp, 100) < 3) {
			free(ans_path);
			close(ans_file);
			return false;
		}
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
				if (reqtxt_file < 0) {
					*error_text = "can't open req.txt";
					fprintf(stderr, "open %s failed with errno %d\n", reqtxt_path, errno);
					return false;
				}
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
			if (reqtxt_file < 0) {
				*error_text = "can't open req.txt";
				fprintf(stderr, "open %s failed with errno %d\n", reqtxt_path, errno);
				return false;
			}
			trigger_response(reqtxt_file, "r", filenr, file_length);
			uemis_increased_timeout(&timeout);
		}
		if (ismulti && more_files && tmp[0] == '1') {
			int size;
			ans_path = build_ans_path(path, assembling_mbuf ? filenr - 2 : filenr - 1);
			ans_file = subsurface_open(ans_path, O_RDONLY, 0666);
			if (ans_file < 0) {
				*error_text = "can't open Uemis response file";
#ifdef UEMIS_DEBUG
				fprintf(debugfile, "open %s failed with errno %d\n", ans_path, errno);
#endif
				free(ans_path);
				return false;
			}
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
			ans_path = build_ans_path(path, filenr - 1);
			ans_file = subsurface_open(ans_path, O_RDONLY, 0666);
			if (ans_file < 0) {
				*error_text = "can't open Uemis response file";
#ifdef UEMIS_DEBUG
				fprintf(debugfile, "open %s failed with errno %d\n", ans_path, errno);
#endif
				free(ans_path);
				return false;
			}

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
				fprintf(debugfile, "::r %s \"%s\"\n", ans_path, buf);
#endif
			}
			size -= 3;
			free(ans_path);
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
	close(ans_file);
	return false;
}

static bool parse_divespot(char *buf)
{
	char *bp = buf + 1;
	char *tp = next_token(&bp);
	char *tag, *type, *val;
	char locationstring[1024] = "";
	int divespot, len;
	double latitude = 0.0, longitude = 0.0;

	// dive spot got deleted, so fail here
	if (strstr(bp, "deleted{bool{true"))
		return false;
	// not a dive spot, fail here
	if (strcmp(tp, "divespot"))
		return false;
	do
		tag = next_token(&bp);
	while (*tag && strcmp(tag, "object_id"));
	if (!*tag)
		return false;
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
	return true;
}

static char *suit[] = {"", QT_TRANSLATE_NOOP("gettextFromC", "wetsuit"), QT_TRANSLATE_NOOP("gettextFromC", "semidry"), QT_TRANSLATE_NOOP("gettextFromC", "drysuit")};
static char *suit_type[] = {"", QT_TRANSLATE_NOOP("gettextFromC", "shorty"), QT_TRANSLATE_NOOP("gettextFromC", "vest"), QT_TRANSLATE_NOOP("gettextFromC", "long john"), QT_TRANSLATE_NOOP("gettextFromC", "jacket"), QT_TRANSLATE_NOOP("gettextFromC", "full suit"), QT_TRANSLATE_NOOP("gettextFromC", "2 pcs full suit")};
static char *suit_thickness[] = {"", "0.5-2mm", "2-3mm", "3-5mm", "5-7mm", "8mm+", QT_TRANSLATE_NOOP("gettextFromC", "membrane")};

static void parse_tag(struct dive *dive, char *tag, char *val)
{
	/* we can ignore computer_id, water and gas as those are redundant
	 * with the binary data and would just get overwritten */
#if UEMIS_DEBUG & 4
	if (strcmp(tag, "file_content"))
		fprintf(debugfile, "Adding to dive %d : %s = %s\n", dive->dc.diveid, tag, val);
#endif
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
		weightsystem_t ws = empty_weightsystem;
		uemis_get_weight(val, &ws, dive->dc.diveid);
		add_cloned_weightsystem(&dive->weightsystems, ws);
	} else if (!strcmp(tag, "notes")) {
		uemis_add_string(val, &dive->notes, " ");
	} else if (!strcmp(tag, "u8DiveSuit")) {
		if (*suit[atoi(val)])
			uemis_add_string(translate("gettextFromC", suit[atoi(val)]), &dive->suit, " ");
	} else if (!strcmp(tag, "u8DiveSuitType")) {
		if (*suit_type[atoi(val)])
			uemis_add_string(translate("gettextFromC", suit_type[atoi(val)]), &dive->suit, " ");
	} else if (!strcmp(tag, "u8SuitThickness")) {
		if (*suit_thickness[atoi(val)])
			uemis_add_string(translate("gettextFromC", suit_thickness[atoi(val)]), &dive->suit, " ");
	} else if (!strcmp(tag, "nickname")) {
		uemis_add_string(val, &dive->buddy, ",");
	}
}

static bool uemis_delete_dive(device_data_t *devdata, uint32_t diveid)
{
	struct dive *dive = NULL;

	if (devdata->download_table->dives[devdata->download_table->nr - 1]->dc.diveid == diveid) {
		/* we hit the last one in the array */
		dive = devdata->download_table->dives[devdata->download_table->nr - 1];
	} else {
		for (int i = 0; i < devdata->download_table->nr - 1; i++) {
			if (devdata->download_table->dives[i]->dc.diveid == diveid) {
				dive = devdata->download_table->dives[i];
				for (int x = i; x < devdata->download_table->nr - 1; x++)
					devdata->download_table->dives[i] = devdata->download_table->dives[x + 1];
			}
		}
	}
	if (dive) {
		devdata->download_table->dives[--devdata->download_table->nr] = NULL;

		free(dive->dc.sample);
		free((void *)dive->notes);
		free((void *)dive->divemaster);
		free((void *)dive->buddy);
		free((void *)dive->suit);
		taglist_free(dive->tag_list);
		free(dive);

		return true;
	}
	return false;
}

/* This function is called for both dive log and dive information that we get
 * from the SDA (what an insane design, btw). The object_id in the dive log
 * matches the logfilenr in the dive information (which has its own, often
 * different object_id) - we use this as the diveid.
 * We create the dive when parsing the dive log and then later, when we parse
 * the dive information we locate the already created dive via its diveid.
 * Most things just get parsed and converted into our internal data structures,
 * but the dive location API is even more crazy. We just get an id that is an
 * index into yet another data store that we read out later. In order to
 * correctly populate the location and gps data from that we need to remember
 * the addresses of those fields for every dive that references the dive spot. */
static bool process_raw_buffer(device_data_t *devdata, uint32_t deviceid, char *inbuf, char **max_divenr, int *for_dive)
{
	char *buf = strdup(inbuf);
	char *tp, *bp, *tag, *type, *val;
	bool done = false;
	int inbuflen = strlen(inbuf);
	char *endptr = buf + inbuflen;
	bool is_log = false, is_dive = false;
	char *sections[10];
	size_t s, nr_sections = 0;
	struct dive *dive = NULL;
	char dive_no[10];

#if UEMIS_DEBUG & 8
	fprintf(debugfile, "p_r_b %s\n", inbuf);
#endif
	if (for_dive)
		*for_dive = -1;
	bp = buf + 1;
	tp = next_token(&bp);
	if (strcmp(tp, "divelog") == 0) {
		/* this is a dive log */
		is_log = true;
		tp = next_token(&bp);
		/* is it a valid entry or nothing ? */
		if (strcmp(tp, "1.0") != 0 || strstr(inbuf, "divelog{1.0{{{{")) {
			free(buf);
			return false;
		}
	} else if (strcmp(tp, "dive") == 0) {
		/* this is dive detail */
		is_dive = true;
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
	if (is_log) {
		dive = uemis_start_dive(deviceid);
	} else {
		/* remember, we don't know if this is the right entry,
		 * so first test if this is even a valid entry */
		if (strstr(inbuf, "deleted{bool{true")) {
#if UEMIS_DEBUG & 2
			fprintf(debugfile, "p_r_b entry deleted\n");
#endif
			/* oops, this one isn't valid, suggest to try the previous one */
			free(buf);
			return false;
		}
		/* quickhack and workaround to capture the original dive_no
		 * I am doing this so I don't have to change the original design
		 * but when parsing a dive we never parse the dive number because
		 * at the time it's being read the *dive variable is not set because
		 * the dive_no tag comes before the object_id in the uemis ans file
		 */
		dive_no[0] = '\0';
		char *dive_no_buf = strdup(inbuf);
		char *dive_no_ptr = strstr(dive_no_buf, "dive_no{int{") + 12;
		if (dive_no_ptr) {
			char *dive_no_end = strstr(dive_no_ptr, "{");
			if (dive_no_end) {
				*dive_no_end = '\0';
				strncpy(dive_no, dive_no_ptr, 9);
				dive_no[9] = '\0';
			}
		}
		free(dive_no_buf);
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
			if (nr_sections < sizeof(sections) / sizeof(*sections) - 1)
				nr_sections++;
			continue;
		}
		val = next_token(&bp);
#if UEMIS_DEBUG & 8
		if (strlen(val) < 20)
			fprintf(debugfile, "Parsed %s, %s, %s\n*************************\n", tag, type, val);
#endif
		if (is_log && strcmp(tag, "object_id") == 0) {
			free(*max_divenr);
			*max_divenr = strdup(val);
			dive->dc.diveid = atoi(val);
#if UEMIS_DEBUG % 2
			fprintf(debugfile, "Adding new dive from log with object_id %d.\n", atoi(val));
#endif
		} else if (is_dive && strcmp(tag, "logfilenr") == 0) {
			/* this one tells us which dive we are adding data to */
			dive = get_dive_by_uemis_diveid(devdata, atoi(val));
			if (strcmp(dive_no, "0"))
				dive->number = atoi(dive_no);
			if (for_dive)
				*for_dive = atoi(val);
		} else if (!is_log && dive && !strcmp(tag, "divespot_id")) {
			int divespot_id = atoi(val);
			if (divespot_id != -1) {
				struct dive_site *ds = create_dive_site("from Uemis", devdata->sites);
				unregister_dive_from_dive_site(dive);
				add_dive_to_dive_site(dive, ds);
				uemis_mark_divelocation(dive->dc.diveid, divespot_id, ds);
			}
#if UEMIS_DEBUG & 2
			fprintf(debugfile, "Created divesite %d for diveid : %d\n", dive->dive_site->uuid, dive->dc.diveid);
#endif
		} else if (dive) {
			parse_tag(dive, tag, val);
		}
		if (is_log && !strcmp(tag, "file_content"))
			done = true;
		/* done with one dive (got the file_content tag), but there could be more:
		 * a '{' indicates the end of the record - but we need to see another "{{"
		 * later in the buffer to know that the next record is complete (it could
		 * be a short read because of some error */
		if (done && ++bp < endptr && *bp != '{' && strstr(bp, "{{")) {
			done = false;
			record_dive_to_table(dive, devdata->download_table);
			dive = uemis_start_dive(deviceid);
		}
	}
	if (is_log) {
		if (dive->dc.diveid) {
			record_dive_to_table(dive, devdata->download_table);
		} else { /* partial dive */
			free(dive);
			free(buf);
			return false;
		}
	}
	free(buf);
	return true;
}

static char *uemis_get_divenr(char *deviceidstr, struct dive_table *table, int force)
{
	uint32_t deviceid, maxdiveid = 0;
	int i;
	char divenr[10];
	deviceid = atoi(deviceidstr);
	mindiveid = 0xFFFFFFFF;

	/*
	 * If we are are retrying after a disconnect/reconnect, we
	 * will look up the highest dive number in the dives we
	 * already have.
	 *
	 * Also, if "force_download" is true, do this even if we
	 * don't have any dives (maxdiveid will remain ~0).
	 *
	 * Otherwise, use the global dive table.
	 */
	if (!force && !table->nr)
		table = &dive_table;

	for (i = 0; i < table->nr; i++) {
		struct dive *d = table->dives[i];
		struct divecomputer *dc;
		if (!d)
			continue;
		for_each_dc (d, dc) {
			if (dc->model && !strcmp(dc->model, "Uemis Zurich") &&
			    (dc->deviceid == 0 || dc->deviceid == 0x7fffffff || dc->deviceid == deviceid)) {
				if (dc->diveid > maxdiveid)
					maxdiveid = dc->diveid;
				if (dc->diveid < mindiveid)
					mindiveid = dc->diveid;
			}
		}
	}
	snprintf(divenr, 10, "%d", maxdiveid);
	return strdup(divenr);
}

#if UEMIS_DEBUG
static int bufCnt = 0;
static bool do_dump_buffer_to_file(char *buf, char *prefix)
{
	char path[100];
	char date[40];
	char obid[40];
	bool success;
	if (!buf)
		return false;

	if (strstr(buf, "date{ts{"))
		strncpy(date, strstr(buf, "date{ts{"), sizeof(date));
	else
		strncpy(date, "date{ts{no-date{", sizeof(date));

	if (!strstr(buf, "object_id{int{"))
		return false;

	strncpy(obid, strstr(buf, "object_id{int{"), sizeof(obid));
	char *ptr1 = strstr(date, "date{ts{");
	char *ptr2 = strstr(obid, "object_id{int{");
	char *pdate = next_token(&ptr1);
	pdate = next_token(&ptr1);
	pdate = next_token(&ptr1);
	char *pobid = next_token(&ptr2);
	pobid = next_token(&ptr2);
	pobid = next_token(&ptr2);
	snprintf(path, sizeof(path), "/%s/%s/UEMIS Dump/%s_%s_Uemis_dump_%s_in_round_%d_%d.txt", home, user, prefix, pdate, pobid, debug_round, bufCnt);
	int dumpFile = subsurface_open(path, O_RDWR | O_CREAT, 0666);
	if (dumpFile == -1)
		return false;
	success = write(dumpFile, buf, strlen(buf)) == strlen(buf);
	close(dumpFile);
	bufCnt++;
	return success;
}
#endif

/* do some more sophisticated calculations here to try and predict if the next round of
 * divelog/divedetail reads will fit into the UEMIS buffer,
 * filenr holds now the uemis filenr after having read several logs including the dive details,
 * fCapacity will five us the average number of files needed for all currently loaded data
 * remember the maximum file usage per dive
 * return : UEMIS_MEM_OK       if there is enough memory for a full round
 *          UEMIS_MEM_CRITICAL if the memory is good for reading the dive logs
 *          UEMIS_MEM_FULL     if the memory is exhausted
 */
static int get_memory(struct dive_table *td, int checkpoint)
{
	if (td->nr <= 0)
		return UEMIS_MEM_OK;

	switch (checkpoint) {
	case UEMIS_CHECK_LOG:
		if (filenr / td->nr > max_mem_used)
			max_mem_used = filenr / td->nr;

		/* check if a full block of dive logs + dive details and dive spot fit into the UEMIS buffer */
#if UEMIS_DEBUG & 4
		fprintf(debugfile, "max_mem_used %d (from td->nr %d) * block_size %d > max_files %d - filenr %d?\n", max_mem_used, td->nr, UEMIS_LOG_BLOCK_SIZE, UEMIS_MAX_FILES, filenr);
#endif
		if (max_mem_used * UEMIS_LOG_BLOCK_SIZE > UEMIS_MAX_FILES - filenr)
			return UEMIS_MEM_FULL;
		break;
	case UEMIS_CHECK_DETAILS:
		/* check if the next set of dive details and dive spot fit into the UEMIS buffer */
		if ((UEMIS_DIVE_DETAILS_SIZE + UEMIS_SPOT_BLOCK_SIZE) * UEMIS_LOG_BLOCK_SIZE > UEMIS_MAX_FILES - filenr)
			return UEMIS_MEM_FULL;
		break;
	case UEMIS_CHECK_SINGLE_DIVE:
		if (UEMIS_DIVE_DETAILS_SIZE + UEMIS_SPOT_BLOCK_SIZE > UEMIS_MAX_FILES - filenr)
			return UEMIS_MEM_FULL;
		break;
	}
	return UEMIS_MEM_OK;
}

/* we misuse the hidden_by_filter flag to mark a dive as deleted.
 * this will be picked up by some cleaning statement later. */
static void do_delete_dives(struct dive_table *td, int idx)
{
	for (int x = idx; x < td->nr; x++)
		td->dives[x]->hidden_by_filter = true;
}

static bool load_uemis_divespot(const char *mountpath, int divespot_id)
{
	char divespotnr[32];
	snprintf(divespotnr, sizeof(divespotnr), "%d", divespot_id);
	param_buff[2] = divespotnr;
#if UEMIS_DEBUG & 2
	fprintf(debugfile, "getDivespot %d\n", divespot_id);
#endif
	bool success = uemis_get_answer(mountpath, "getDivespot", 3, 0, NULL);
	if (mbuf && success) {
#if UEMIS_DEBUG & 16
		do_dump_buffer_to_file(mbuf, "Spot");
#endif
		return parse_divespot(mbuf);
	}
	return false;
}

static void get_uemis_divespot(device_data_t *devdata, const char *mountpath, int divespot_id, struct dive *dive)
{
	struct dive_site *nds = dive->dive_site;

	if (is_divespot_mappable(divespot_id)) {
		struct dive_site *ds = get_dive_site_by_divespot_id(divespot_id);
		unregister_dive_from_dive_site(dive);
		add_dive_to_dive_site(dive, ds);
	} else if (nds && nds->name && strstr(nds->name,"from Uemis")) {
		if (load_uemis_divespot(mountpath, divespot_id)) {
			/* get the divesite based on the diveid, this should give us
			* the newly created site
			*/
			struct dive_site *ods;
			/* with the divesite name we got from parse_dive, that is called on load_uemis_divespot
			 * we search all existing divesites if we have one with the same name already. The function
			 * returns the first found which is luckily not the newly created.
			 */
			ods = get_dive_site_by_name(nds->name, devdata->sites);
			if (ods) {
				/* if the uuid's are the same, the new site is a duplicate and can be deleted */
				if (nds->uuid != ods->uuid) {
					delete_dive_site(nds, devdata->sites);
					unregister_dive_from_dive_site(dive);
					add_dive_to_dive_site(dive, ods);
				}
			}
			add_to_divespot_mapping(divespot_id, dive->dive_site);
		} else {
			/* if we can't load the dive site details, delete the site we
			* created in process_raw_buffer
			*/
			delete_dive_site(dive->dive_site, devdata->sites);
		}
	}
}

static bool get_matching_dive(int idx, char *newmax, int *uemis_mem_status, device_data_t *data, const char *mountpath, const char deviceidnr)
{
	struct dive *dive = data->download_table->dives[idx];
	char log_file_no_to_find[20];
	char dive_to_read_buf[10];
	bool found = false;
	bool found_below = false;
	bool found_above = false;
	int deleted_files = 0;
	int fail_count = 0;

	snprintf(log_file_no_to_find, sizeof(log_file_no_to_find), "logfilenr{int{%d", dive->dc.diveid);
#if UEMIS_DEBUG & 2
	fprintf(debugfile, "Looking for dive details to go with dive log id %d\n", dive->dc.diveid);
#endif
	while (!found) {
		if (import_thread_cancelled)
			break;
		snprintf(dive_to_read_buf, sizeof(dive_to_read_buf), "%d", dive_to_read);
		param_buff[2] = dive_to_read_buf;
		(void)uemis_get_answer(mountpath, "getDive", 3, 0, NULL);
#if UEMIS_DEBUG & 16
		do_dump_buffer_to_file(mbuf, "Dive");
#endif
		*uemis_mem_status = get_memory(data->download_table, UEMIS_CHECK_SINGLE_DIVE);
		if (*uemis_mem_status == UEMIS_MEM_OK) {
			/* if the memory isn's completely full we can try to read more dive log vs. dive details
			 * UEMIS_MEM_CRITICAL means not enough space for a full round but the dive details
			 * and the dive spots should fit into the UEMIS memory
			 * The match we do here is to map the object_id to the logfilenr, we do this
			 * by iterating through the last set of loaded dive logs and then find the corresponding
			 * dive with the matching logfilenr */
			if (mbuf) {
				if (strstr(mbuf, log_file_no_to_find)) {
					/* we found the logfilenr that matches our object_id from the dive log we were looking for
					 * we mark the search successful even if the dive has been deleted. */
					found = true;
					if (strstr(mbuf, "deleted{bool{true") == NULL) {
						process_raw_buffer(data, deviceidnr, mbuf, &newmax, NULL);
						/* remember the last log file number as it is very likely that subsequent dives
						 * have the same or higher logfile number.
						 * UEMIS unfortunately deletes dives by deleting the dive details and not the logs. */
#if UEMIS_DEBUG & 2
						d_time = get_dive_date_c_string(dive->when);
						fprintf(debugfile, "Matching dive log id %d from %s with dive details %d\n", dive->dc.diveid, d_time, dive_to_read);
#endif
						int divespot_id = uemis_get_divespot_id_by_diveid(dive->dc.diveid);
						if (divespot_id >= 0)
							get_uemis_divespot(data, mountpath, divespot_id, dive);

					} else {
						/* in this case we found a deleted file, so let's increment */
#if UEMIS_DEBUG & 2
						d_time = get_dive_date_c_string(dive->when);
						fprintf(debugfile, "TRY matching dive log id %d from %s with dive details %d but details are deleted\n", dive->dc.diveid, d_time, dive_to_read);
#endif
						deleted_files++;
						/* mark this log entry as deleted and cleanup later, otherwise we mess up our array */
						dive->hidden_by_filter = true;
#if UEMIS_DEBUG & 2
						fprintf(debugfile, "Deleted dive from %s, with id %d from table -- newmax is %s\n", d_time, dive->dc.diveid, newmax);
#endif
					}
				} else {
					uint32_t nr_found = 0;
					char *logfilenr = strstr(mbuf, "logfilenr");
					if (logfilenr && strstr(mbuf, "act{")) {
						sscanf(logfilenr, "logfilenr{int{%u", &nr_found);
						if (nr_found >= dive->dc.diveid || nr_found == 0) {
							found_above = true;
							dive_to_read = dive_to_read - 2;
						} else {
							found_below = true;
						}
						if (dive_to_read < -1)
							dive_to_read = -1;
					} else if (!strstr(mbuf, "act{") && ++fail_count == 10) {
						if (verbose)
							fprintf(stderr, "Uemis downloader: Cannot access dive details - searching from start\n");
						dive_to_read = -1;
					}
				}
			}
			if (found_above && found_below)
				break;
			dive_to_read++;
		} else {
			/* At this point the memory of the UEMIS is full, let's cleanup all dive log files were
			 * we could not match the details to. */
			do_delete_dives(data->download_table, idx);
			return false;
		}
	}
	/* decrement iDiveToRead by the amount of deleted entries found to assure
	 * we are not missing any valid matches when processing subsequent logs */
	dive_to_read = (dive_to_read - deleted_files > 0 ? dive_to_read - deleted_files : 0);
	deleted_files = 0;
	return true;
}

const char *do_uemis_import(device_data_t *data)
{
	const char *mountpath = data->devname;
	short force_download = data->force_download;
	char *newmax = NULL;
	int first, start, end = -2;
	uint32_t deviceidnr;
	char *deviceid = NULL;
	const char *result = NULL;
	char *endptr;
	bool success, once = true;
	int match_dive_and_log = 0;
	int uemis_mem_status = UEMIS_MEM_OK;

#if UEMIS_DEBUG
	home = getenv("HOME");
	user = getenv("LOGNAME");
#endif
	uemis_info(translate("gettextFromC", "Initialise communication"));
	if (!uemis_init(mountpath)) {
		free(reqtxt_path);
		return translate("gettextFromC", "Uemis init failed");
	}

	if (!uemis_get_answer(mountpath, "getDeviceId", 0, 1, &result))
		goto bail;
	deviceid = strdup(param_buff[0]);
	deviceidnr = atoi(deviceid);

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
	newmax = uemis_get_divenr(deviceid, data->download_table, force_download);
	if (verbose)
		fprintf(stderr, "Uemis downloader: start looking at dive nr %s\n", newmax);

	first = start = atoi(newmax);
	dive_to_read = mindiveid < first ? first - mindiveid : first;
	for (;;) {
#if UEMIS_DEBUG & 2
		debug_round++;
#endif
#if UEMIS_DEBUG & 4
		fprintf(debugfile, "d_u_i inner loop start %d end %d newmax %s\n", start, end, newmax);
#endif
		/* start at the last filled download table index */
		match_dive_and_log = data->download_table->nr;
		sprintf(newmax, "%d", start);
		param_buff[2] = newmax;
		param_buff[3] = 0;
		success = uemis_get_answer(mountpath, "getDivelogs", 3, 0, &result);
		uemis_mem_status = get_memory(data->download_table, UEMIS_CHECK_DETAILS);
		/* first, remove any leading garbage... this needs to start with a '{' */
		char *realmbuf = mbuf;
		if (mbuf)
			realmbuf = strchr(mbuf, '{');
		if (success && realmbuf && uemis_mem_status != UEMIS_MEM_FULL) {
#if UEMIS_DEBUG & 16
			do_dump_buffer_to_file(realmbuf, "Dive logs");
#endif
			/* process the buffer we have assembled */
			if (!process_raw_buffer(data, deviceidnr, realmbuf, &newmax, NULL)) {
				/* if no dives were downloaded, mark end appropriately */
				if (end == -2)
					end = start - 1;
				success = false;
			}
			if (once) {
				char *t = first_object_id_val(realmbuf);
				if (t && atoi(t) > start)
					start = atoi(t);
				free(t);
				once = false;
			}
			/* clean up mbuf */
			endptr = strstr(realmbuf, "{{{");
			if (endptr)
				*(endptr + 2) = '\0';
			/* last object_id we parsed */
			sscanf(newmax, "%d", &end);
#if UEMIS_DEBUG & 4
			fprintf(debugfile, "d_u_i after download and parse start %d end %d newmax %s progress %4.2f\n", start, end, newmax, progress_bar_fraction);
#endif
			/* The way this works is that I am reading the current dive from what has been loaded during the getDiveLogs call to the UEMIS.
			 * As the object_id of the dive log entry and the object_id of the dive details are not necessarily the same, the match needs
			 * to happen based on the logfilenr.
			 * What the following part does is to optimize the mapping by using
			 * dive_to_read = the dive details entry that need to be read using the object_id
			 * logFileNoToFind = map the logfilenr of the dive details with the object_id = diveid from the get dive logs */
			for (int i = match_dive_and_log; i < data->download_table->nr; i++) {
				bool success = get_matching_dive(i, newmax, &uemis_mem_status, data, mountpath, deviceidnr);
				if (!success)
					break;
				if (import_thread_cancelled)
					break;
			}

			start = end;

			/* Do some memory checking here */
			uemis_mem_status = get_memory(data->download_table, UEMIS_CHECK_LOG);
			if (uemis_mem_status != UEMIS_MEM_OK) {
#if UEMIS_DEBUG & 4
				fprintf(debugfile, "d_u_i out of memory, bailing\n");
#endif
				break;
			}
			/* if the user clicked cancel, exit gracefully */
			if (import_thread_cancelled) {
#if UEMIS_DEBUG & 4
				fprintf(debugfile, "d_u_i thread canceled, bailing\n");
#endif
				break;
			}
			/* if we got an error or got nothing back, stop trying */
			if (!success || !param_buff[3]) {
#if UEMIS_DEBUG & 4
				fprintf(debugfile, "d_u_i after download nothing found, giving up\n");
#endif
				break;
			}
#if UEMIS_DEBUG & 2
			if (debug_round != -1)
				if (debug_round-- == 0) {
					fprintf(debugfile, "d_u_i debug_round is now 0, bailing\n");
					goto bail;
				}
#endif
		} else {
			/* some of the loading from the UEMIS failed at the dive log level
			 * if the memory status = full, we can't even load the dive spots and/or buddies.
			 * The loaded block of dive logs is useless and all new loaded dive logs need to
			 * be deleted from the download_table.
			 */
			if (uemis_mem_status == UEMIS_MEM_FULL)
				do_delete_dives(data->download_table, match_dive_and_log);
#if UEMIS_DEBUG & 4
			fprintf(debugfile, "d_u_i out of memory, bailing instead of processing\n");
#endif
			break;
		}
	}

	if (end == -2 && sscanf(newmax, "%d", &end) != 1)
		end = start;

#if UEMIS_DEBUG & 2
	fprintf(debugfile, "Done: read from object_id %d to %d\n", first, end);
#endif

	/* Regardless on where we are with the memory situation, it's time now
	 * to see if we have to clean some dead bodies from our download table */
	next_table_index = 0;
	while (next_table_index < data->download_table->nr) {
		if (data->download_table->dives[next_table_index]->hidden_by_filter)
			uemis_delete_dive(data, data->download_table->dives[next_table_index]->dc.diveid);
		else
			next_table_index++;
	}

	if (uemis_mem_status != UEMIS_MEM_OK)
		result = translate("gettextFromC", ERR_FS_ALMOST_FULL);

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
	if (!data->download_table->nr)
		result = translate("gettextFromC", ERR_NO_FILES);
	return result;
}
