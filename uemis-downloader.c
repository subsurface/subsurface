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

#define ERR_FS_ALMOST_FULL N_("Uemis Zurich: File System is almost full\nDisconnect/reconnect the dive computer\nand try again")
#define ERR_FS_FULL N_("Uemis Zurich: File System is full\nDisconnect/reconnect the dive computer\nand try again")
#define ERR_FS_SHORT_WRITE N_("Short write to req.txt file\nIs the Uemis Zurich plugged in correctly?")
#define BUFLEN 2048
#define NUM_PARAM_BUFS 6
#define UEMIS_TIMEOUT 100000
static char *param_buff[NUM_PARAM_BUFS];
static char *reqtxt_path;
static int reqtxt_file;
static int filenr;
static int number_of_files;
static char *mbuf = NULL;
static int mbuf_size = 0;

struct argument_block {
	const char *mountpath;
	char **max_dive_data;
	char **xml_buffer;
	progressbar_t *progress;
};

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

/* send text to the importer progress bar */
static void uemis_info(const char *fmt, ...)
{
	static char buffer[32];
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
	if (!reqtxt_file)
		return FALSE;
	if (bytes_available(reqtxt_file) > 5) {
		char tmp[6];
		read(reqtxt_file, tmp, 5);
		tmp[5] = '\0';
#if UEMIS_DEBUG > 2
		fprintf(debugfile, "::r req.txt \"%s\"\n", tmp);
#endif
		if (sscanf(tmp + 1, "%d", &filenr) != 1)
			return FALSE;
	}
#if UEMIS_DEBUG > 2
	else {
		fprintf(debugfile, "::r req.txt skipped as there were fewer than 5 bytes\n");
	}
#endif
	close (reqtxt_file);

	/* It would be nice if we could simply go back to the first set of
	 * ANS files. Something like the below - unfortunately this is known
	 * to fail - more information from Uemis is needed here.
	 * Without code like this it is very easy when downloading large amounts
	 * of dives to run out of space on the dive computer - which can only
	 * be fixed by unmounting and rebooting the DC
	 *	reqtxt_file = g_open(reqtxt_path, O_RDWR | O_CREAT | O_TRUNC, 0666);
	 *	write(reqtxt_file, "n0001", 5);
	 *	close(reqtxt_file);
	 *	filenr = 1;
	 */
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
#if UEMIS_DEBUG > 2
	fprintf(debugfile,"::: %s (after seeks)\n", fl);
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
	if (buf) {
		if (! *buffer) {
			*buffer = strdup(buf);
			*buffer_size = strlen(*buffer) + 1;
		} else {
			*buffer_size += strlen(buf);
			*buffer = realloc(*buffer, *buffer_size);
			strcat(*buffer, buf);
		}
#if UEMIS_DEBUG > 5
		fprintf(debugfile,"added \"%s\" to buffer - new length %d\n", buf, *buffer_size);
#endif
	}
}

/* are there more ANS files we can check? */
static gboolean next_file(int max)
{
	if (filenr >= max)
		return FALSE;
	filenr++;
	return TRUE;
}

/* ultra-simplistic; it doesn't deal with the case when the object_id is
 * split across two chunks. It also doesn't deal with the discrepancy between
 * object_id and dive number as understood by the dive computer */
static void show_progress(char *buf)
{
	char *object;
	object = strstr(buf, "object_id");
	if (object) {
		/* let the user know which dive we are working on */
		char tmp[10];
		char *p = object + 14;
		char *t = tmp;

		if (p < buf + strlen(buf)) {
			while (*p != '{' && t < tmp + 9)
				*t++ = *p++;
			*t = '\0';
			uemis_info(_("Reading dive %s"), tmp);
		}
	}
}

/* send a request to the dive computer and collect the answer */
static gboolean uemis_get_answer(const char *path, char *request, int n_param_in,
			int n_param_out, char **error_text)
{
	int i = 0, file_length;
	char sb[BUFLEN];
	char fl[13];
	char tmp[101];
	gboolean searching = TRUE;
	gboolean assembling_mbuf = FALSE;
	gboolean ismulti = FALSE;
	gboolean found_answer = FALSE;
	gboolean more_files = TRUE;
	gboolean answer_in_mbuf = FALSE;
	char *ans_path;
	int ans_file;

	reqtxt_file = g_open(reqtxt_path, O_RDWR | O_CREAT, 0666);
	snprintf(sb, BUFLEN, "n%04d12345678", filenr);
	str_append_with_delim(sb, request);
	for (i = 0; i < n_param_in; i++)
		str_append_with_delim(sb, param_buff[i]);
	if (! strcmp(request, "getDivelogs") || ! strcmp(request, "getDeviceData")) {
		answer_in_mbuf = TRUE;
		str_append_with_delim(sb, "");
	}
	str_append_with_delim(sb, "");
	file_length = strlen(sb);
	snprintf(fl, 10, "%08d", file_length - 13);
	memcpy(sb + 5, fl, strlen(fl));
#ifdef UEMIS_DEBUG
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
	usleep(UEMIS_TIMEOUT);
	mbuf = NULL;
	mbuf_size = 0;
	while (searching || assembling_mbuf) {
		progress_bar_fraction = filenr / 4000.0;
		snprintf(fl, 13, "ANS%d.TXT", filenr - 1);
		ans_path = g_build_filename(path, "ANS", fl, NULL);
		ans_file = g_open(ans_path, O_RDONLY, 0666);
		read(ans_file, tmp, 100);
		close(ans_file);
#if UEMIS_DEBUG > 3
		tmp[100]='\0';
		fprintf(debugfile, "::t %s \"%s\"\n", ans_path, tmp);
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
			usleep(UEMIS_TIMEOUT);
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
				buf[size -3 ] = '\0';
				buffer_add(&mbuf, &mbuf_size, buf);
				show_progress(buf);
				free(buf);
				param_buff[3]++;
			}
			close(ans_file);
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
#if UEMIS_DEBUG > 2
				fprintf(debugfile, "::r %s \"%s\"\n", ans_path, buf);
#endif
			}
			size -= 3;
			close(ans_file);
			free(ans_path);
		} else {
			ismulti = FALSE;
		}
#if UEMIS_DEBUG > 1
		fprintf(debugfile,":r: %s\n", buf);
#endif
		if (!answer_in_mbuf)
			for (i = 0; i < n_param_out && j < size; i++)
				param_buff[i] = next_segment(buf, &j, size);
		found_answer = TRUE;
		free(buf);
	}
#if UEMIS_DEBUG
	for (i = 0; i < n_param_out; i++)
		fprintf(debugfile,"::: %d: %s\n", i, param_buff[i]);
#endif
	return found_answer;
}

/* Turn what we get from the dive computer into something
 * that we can pass to the parse_xml function.
 * The last 'object_id' that we see is returned as our current
 * approximation of a last dive number */
static char *process_raw_buffer(char *inbuf, char **max_divenr)
{
	char *buf = strdup(inbuf);
	char *tp, *bp, *tag, *type, *val;
	gboolean done = FALSE;
	int inbuflen = strlen(inbuf);
	char *endptr = buf + inbuflen;
	char *conv_buffer = NULL;
	int conv_buffer_size = 0;

	bp = buf + 1;
	tp = next_token(&bp);
	if (strcmp(tp,"divelog") != 0)
		return NULL;
	tp = next_token(&bp);
	if (strcmp(tp,"1.0") != 0)
		return NULL;
	buffer_add(&conv_buffer, &conv_buffer_size,
		"<dive type=\"uemis\" ref=\"divelog\" version=\"1.0\">\n");
	while (!done) {
		char *tmp;
		int tmp_size;
		tag = next_token(&bp);
		type = next_token(&bp);
		val = next_token(&bp);
		if (! strcmp(tag, "object_id")) {
			free(*max_divenr);
			*max_divenr = strdup(val);
		}
		if (! strcmp(tag, "file_content")) {
			tmp_size = 44 + strlen(tag) + strlen(val);
			done = TRUE;
		} else {
			tmp_size = 27 + strlen(tag) + 2 * strlen(type) + strlen(val);
		}
		tmp = malloc(tmp_size);
		snprintf(tmp, tmp_size,"<val key=\"%s\">\n<%s>%s</%s>\n</val>\n",
			tag, type, val, type);
		buffer_add(&conv_buffer, &conv_buffer_size, tmp);
		free(tmp);
		/* done with one dive (got the file_content tag), but there could be more:
		 * a '{' indicates the end of the record - but we need to see another "{{"
		 * later in the buffer to know that the next record is complete (it could
		 * be a short read because of some error */
		if (done && ++bp < endptr && *bp != '{' && strstr(bp, "{{")) {
			done = FALSE;
			buffer_add(&conv_buffer, &conv_buffer_size,
				"</dive>\n<dive type=\"uemis\" ref=\"divelog\" version=\"1.0\">\n");
		}
	}
	buffer_add(&conv_buffer, &conv_buffer_size, "</dive>\n");
	free(buf);
	return strdup(conv_buffer);
}

/* to keep track of multiple computers we simply encode the last dive read
   in tuples "{deviceid,nr},{deviceid,nr}..." no spaces to make parsing easier */

static char *find_deviceid(char *max_dive_data, char *deviceid)
{
	char *pattern;
	char *result;
	if (! deviceid || *deviceid == '\0')
		return NULL;
	pattern = malloc(3 + strlen(deviceid));
	sprintf(pattern, "{%s,", deviceid);
	result = strstr(max_dive_data, pattern);
	free(pattern);
	return result;
}

static char *get_divenr(char *max_dive_data, char *deviceid)
{
	char *q, *p = max_dive_data;
	char *result = NULL;

	if (!p || !deviceid)
		return strdup("0");
	p = find_deviceid(max_dive_data, deviceid);
	if (p) {
		p += strlen(deviceid) + 2;
		q = strchr(p, '}');
		if (!q)
			return result;
		result = malloc(q - p + 1);
		strncpy(result, p, q - p);
		result[q - p] = '\0';
	}
	if (!result)
		result = strdup("0");
	return result;
}

static char *update_max_dive_data(char *max_dive_data, char *deviceid, char *newmax)
{
	char *p;
	char *result;
	int len;

	if (! newmax || *newmax == '\0')
		return max_dive_data;
	p = find_deviceid(max_dive_data, deviceid);
	if (p) {
		/* if there are more entries after this one, copy them,
		   otherwise just remove the existing entry for this device */
		char *q = strstr(p, "},{");
		if (q) {
			memcpy(p + 1, q + 3, strlen(q + 3) + 1);
		} else {
			if (p > max_dive_data)
				*(p-1) = '\0';
			else
				*p = '\0';
		}
	}
	/* now add the new one at the end */
	len = strlen(max_dive_data) + strlen(deviceid) + strlen(newmax) + 4 + (strlen(max_dive_data) ? 1 : 0);
	result = malloc(len);
	snprintf(result, len, "%s%s{%s,%s}", max_dive_data, strlen(max_dive_data) ? "," : "", deviceid, newmax);
	free(max_dive_data);
	return result;
}

static char *do_uemis_download(struct argument_block *args)
{
	const char *mountpath = args->mountpath;
	char **max_dive_data = args->max_dive_data;
	char **xml_buffer = args->xml_buffer;
	int xml_buffer_size;
	char *newmax = NULL;
	char *deviceid = NULL;
	char *result = NULL;
	char *endptr;
	gboolean success;

	buffer_add(xml_buffer, &xml_buffer_size, "<dives type='uemis'><string></string>\n<list>\n");
	uemis_info("Init Communication");
	if (! uemis_init(mountpath))
		return _("Uemis init failed");
	if (! uemis_get_answer(mountpath, "getDeviceId", 0, 1, &result))
		goto bail;
	deviceid = strdup(param_buff[0]);
	/* the answer from the DeviceId call becomes the input parameter for getDeviceData */
	if (! uemis_get_answer(mountpath, "getDeviceData", 1, 0, &result))
		goto bail;
	/* param_buff[0] is still valid */
	if (! uemis_get_answer(mountpath, "initSession", 1, 6, &result))
		goto bail;
	uemis_info("Start download");
	if (! uemis_get_answer(mountpath, "processSync", 0, 2, &result))
		goto bail;
	param_buff[1] = "notempty";
	newmax = get_divenr(*max_dive_data, deviceid);
	for (;;) {
		param_buff[2] = newmax;
		param_buff[3] = 0;
		success = uemis_get_answer(mountpath, "getDivelogs", 3, 0, &result);
		/* process the buffer we have assembled */
		if (mbuf) {
			char *next_seg = process_raw_buffer(mbuf, &newmax);
			buffer_add(xml_buffer, &xml_buffer_size, next_seg);
			free(next_seg);
		}
		/* if we got an error, deal with it */
		if (!success)
			break;
		/* also, if we got nothing back, we should stop trying */
		if (!param_buff[3])
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
	*args->max_dive_data = update_max_dive_data(*max_dive_data, deviceid, newmax);
	free(newmax);
	if (! uemis_get_answer(mountpath, "terminateSync", 0, 3, &result))
		goto bail;
	if (! strcmp(param_buff[0], "error")) {
		if (! strcmp(param_buff[2],"Out of Memory"))
			result = _(ERR_FS_FULL);
		else
			result = param_buff[2];
	}
	buffer_add(xml_buffer, &xml_buffer_size, "</list></dives>");
#if UEMIS_DEBUG > 5
	fprintf(debugfile, "XML buffer \"%s\"", *xml_buffer);
#endif
bail:
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

GError *uemis_download(const char *mountpath, char **max_dive_data, char **xml_buffer, progressbar_t *progress)
{
	pthread_t pthread;
	void *retval;
	struct argument_block args = {mountpath, max_dive_data, xml_buffer, progress};

	/* I'm sure there is some better interface for waiting on a thread in a UI main loop */
	import_thread_done = 0;
	progress_bar_text = "";
	progress_bar_fraction = 0.0;
	pthread_create(&pthread, NULL, pthread_wrapper, &args);
	while (!import_thread_done) {
		import_thread_cancelled = process_ui_events();
		update_progressbar(args.progress, progress_bar_fraction);
		update_progressbar_text(args.progress, progress_bar_text);
		usleep(100000);
	}
	if (pthread_join(pthread, &retval) < 0)
		return error("Pthread return with error");
	if (retval)
		return error(retval);
	return NULL;
}
