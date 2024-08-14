// SPDX-License-Identifier: GPL-2.0
/*
 * uemis-downloader.cpp
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
#include <stdlib.h>
#include <array>
#include <string>
#include <charconv>
#include <unordered_map>

#include "gettext.h"
#include "libdivecomputer.h"
#include "uemis.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "divesite.h"
#include "errorhelper.h"
#include "file.h"
#include "format.h"
#include "subsurface-time.h"
#include "tag.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"

#define ACTION_RECONNECT QT_TRANSLATE_NOOP("gettextFromC", "Disconnect/reconnect the SDA")
#define ERR_FS_ALMOST_FULL QT_TRANSLATE_NOOP("gettextFromC", "Uemis Zurich: the file system is almost full.\nDisconnect/reconnect the dive computer\nand click \'Retry\'")
#define ERR_FS_FULL QT_TRANSLATE_NOOP("gettextFromC", "Uemis Zurich: the file system is full.\nDisconnect/reconnect the dive computer\nand click Retry")
#define ERR_FS_SHORT_WRITE QT_TRANSLATE_NOOP("gettextFromC", "Short write to req.txt file.\nIs the Uemis Zurich plugged in correctly?")
#define ERR_NO_FILES QT_TRANSLATE_NOOP("gettextFromC", "No dives to download.")
constexpr size_t num_param_bufs = 10;

// debugging setup
//#define UEMIS_DEBUG 1 + 2 + 4 + 8 + 16 + 32

static constexpr int uemis_max_files = 4000;
static constexpr int uemis_spot_block_size = 1;
static constexpr int uemis_dive_details_size = 2;
static constexpr int uemis_log_block_size = 10;

enum class uemis_mem_status {
	ok, full
};

enum class uemis_checkpoint {
	log, details, single_dive
};

#if UEMIS_DEBUG
static std::string home, user, d_time;
static int debug_round = 0;
#endif

#if UEMIS_DEBUG & 64		/* we are reading from a copy of the filesystem, not the device - no need to wait */
static constexpr int uemis_timeout = 50;		/* 50ns */
static constexpr int uemis_long_timeout = 500;		/* 500ns */
static constexpr int uemis_max_timeout = 2000;		/* 2ms */
#else
static constexpr int uemis_timeout = 50000;		/* 50ms */
static constexpr int uemis_long_timeout = 500000;	/* 500ms */
static constexpr int uemis_max_timeout = 2000000;	/* 2s */
#endif

static uemis uemis_obj;
static std::array<std::string, num_param_bufs> param_buff;
static std::string reqtxt_path;
static int filenr;
static int number_of_files;

static int max_mem_used = -1;
static int dive_to_read = 0;

/* Hash map to remember already executed divespot download requests */
static std::unordered_map<int, dive_site *> divespot_mapping;

/* helper function to parse the Uemis data structures */
static timestamp_t uemis_ts(std::string_view buffer)
{
	struct tm tm;

	memset(&tm, 0, sizeof(tm));
	sscanf(buffer.begin(), "%d-%d-%dT%d:%d:%d",
	       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
	       &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
	tm.tm_mon -= 1;
	return utc_mktime(&tm);
}

/* helper function to make the std::from_chars() interface more
 * palatable.
 * std::from_chars(s.begin(), s.end(), v)
 * works for the compilers we use, but is not guaranteed to work
 * as per standard.
 * see: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2007r0.html
 */
template <typename T>
auto from_chars(std::string_view s, T &v)
{
	return std::from_chars(s.data(), s.data() + s.size(), v);
}

/* Sadly, a number of supported compilers do not support from_chars()
 * to double. Therefore, implement our own version.
 * Frustrating, but oh well. TODO: Try to find the proper check for the
 * existence of this function.
 */
static std::from_chars_result from_chars(const std::string &s, double &v)
{
	const char *end;
	double res = ascii_strtod(s.c_str(), &end);
	if (end == s.c_str())
		return { end, std::errc::invalid_argument };
	v = res;
	return { end, std::errc() };
}

template <>
auto from_chars<double>(std::string_view sv, double &v)
{
	std::string s(sv); // Generate a null-terminated string to work on.
	return from_chars(s, v);
}

/* float minutes */
static void uemis_duration(std::string_view buffer, duration_t &duration)
{
	from_chars(buffer, duration.seconds);
}

/* int cm */
static void uemis_depth(std::string_view buffer, depth_t &depth)
{
	if (from_chars(buffer, depth.mm).ec != std::errc::invalid_argument)
		depth.mm *= 10;
}

static void uemis_get_index(std::string_view buffer, int &idx)
{
	from_chars(buffer, idx);
}

/* space separated */
static void uemis_add_string(std::string_view buffer, std::string &text, const char *delimit)
{
	/* do nothing if this is an empty buffer (Uemis sometimes returns a single
	 * space for empty buffers) */
	if (buffer.empty() || buffer == " ")
		return;
	if (!text.empty())
		text += delimit;
	text += buffer;
}

/* still unclear if it ever reports lbs */
static void uemis_get_weight(std::string_view buffer, weightsystem_t &weight, int diveid)
{
	double val;
	if (from_chars(buffer, val).ec != std::errc::invalid_argument) {
		weight.weight.grams = uemis_obj.get_weight_unit(diveid) ?
			lbs_to_grams(val) : lrint(val * 1000);
	}
	weight.description = translate("gettextFromC", "unknown");
}

static std::unique_ptr<dive> uemis_start_dive(uint32_t deviceid)
{
	auto dive = std::make_unique<struct dive>();
	dive->dcs[0].model = "Uemis Zurich";
	dive->dcs[0].deviceid = deviceid;
	return dive;
}

static struct dive *get_dive_by_uemis_diveid(device_data_t *devdata, uint32_t object_id)
{
	for (auto &d: devdata->log->dives) {
		if (object_id == d->dcs[0].diveid)
			return d.get();
	}
	return NULL;
}

/* send text to the importer progress bar */
static void uemis_info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	progress_bar_text = vformat_string_std(fmt, ap);
	va_end(ap);
	if (verbose)
		report_info("Uemis downloader: %s", progress_bar_text.c_str());
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

static int number_of_file(const std::string &path)
{
	int count = 0;
#ifdef WIN32
	struct _wdirent *entry;
	_WDIR *dirp = (_WDIR *)subsurface_opendir(path.c_str());
#else
	struct dirent *entry;
	DIR *dirp = (DIR *)subsurface_opendir(path.c_str());
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

static std::string build_filename(const std::string &path, const std::string &name)
{
#if WIN32
	return path + "\\" + name;
#else
	return path + "/" + name;
#endif
}

/* Check if there's a req.txt file and get the starting filenr from it.
 * Test for the maximum number of ANS files (I believe this is always
 * 4000 but in case there are differences depending on firmware, this
 * code is easy enough */
static bool uemis_init(const std::string &path)
{
	using namespace std::string_literals;

	divespot_mapping.clear();
	if (path.empty())
		return false;
	/* let's check if this is indeed a Uemis DC */
	reqtxt_path = build_filename(path, "req.txt"s);
	int reqtxt_file = subsurface_open(reqtxt_path.c_str(), O_RDONLY | O_CREAT, 0666);
	if (reqtxt_file < 0) {
#if UEMIS_DEBUG & 1
		report_info(":EE req.txt can't be opened\n");
#endif
		return false;
	}
	if (bytes_available(reqtxt_file) > 5) {
		char tmp[6];
		if (read(reqtxt_file, tmp, 5) != 5) {
			close(reqtxt_file);
			return false;
		}
		tmp[5] = '\0';
#if UEMIS_DEBUG & 2
		report_info("::r req.txt \"%s\"\n", tmp);
#endif
		if (sscanf(tmp + 1, "%d", &filenr) != 1) {
			close(reqtxt_file);
			return false;
		}
	} else {
		filenr = 0;
#if UEMIS_DEBUG & 2
		report_info("::r req.txt skipped as there were fewer than 5 bytes\n");
#endif
	}
	close(reqtxt_file);

	/* It would be nice if we could simply go back to the first set of
	 * ANS files. But with a FAT filesystem that isn't possible */
	std::string ans_path = build_filename(path, "ANS"s);
	number_of_files = number_of_file(ans_path);
	/* initialize the array in which we collect the answers */
	for (std::string &s: param_buff)
		s.clear();
	return true;
}

static void str_append_with_delim(std::string &s, const std::string &t)
{
	s += t;
	s += '{';
}

/* The communication protocol with the DC is truly funky.
 * After you write your request to the req.txt file you call this function.
 * It writes the number of the next ANS file at the beginning of the req.txt
 * file (prefixed by 'n' or 'r') and then again at the very end of it, after
 * the full request (this time without the prefix).
 * Then it syncs (not needed on Windows) and closes the file. */
static void trigger_response(int file, const char *command, int nr, long tailpos)
{
	char fl[10];

	snprintf(fl, 8, "%s%04d", command, nr);
#if UEMIS_DEBUG & 4
	report_info(":tr %s (after seeks)\n", fl);
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

static std::string_view next_token(std::string_view &buf)
{
	size_t pos = buf.find('{');
	std::string_view res;
	if (pos == std::string::npos) {
		res = buf;
		buf = std::string_view();
	} else {
		res = buf.substr(0, pos);
		buf = buf.substr(pos + 1);
	}
	return res;
}

/* poor man's tokenizer that understands a quoted delimiter ('{') */
static std::string next_segment(const std::string &buf, int &offset)
{
	size_t i = static_cast<size_t>(offset);
	std::string segment;

	while (i < buf.size()) {
		if (i + 1 < buf.size() && buf[i] == '\\' &&
		    (buf[i + 1] == '\\' || buf[i + 1] == '{')) {
			i++;
		} else if (buf[i] == '{') {
			i++;
			break;
		}
		segment += buf[i++];
	}
	offset = i;
	return segment;
}

/* are there more ANS files we can check? */
static bool next_file(int max)
{
	if (filenr >= max)
		return false;
	filenr++;
	return true;
}

/* try and do a quick decode - without trying to get too fancy in case the data
 * straddles a block boundary...
 * we are parsing something that looks like this:
 * object_id{int{2{date{ts{2011-04-05T12:38:04{duration{float{12.000...
 */
static std::string first_object_id_val(std::string_view buf)
{
	size_t object = buf.find( "object_id");
	if (object != std::string::npos && object + 14 < buf.size()) {
		/* get the value */
		size_t p = object + 14;

#if UEMIS_DEBUG & 4
		char debugbuf[50];
		strncpy(debugbuf, buf.begin() + object, 49);
		debugbuf[49] = '\0';
		report_info("buf |%s|\n", debugbuf);
#endif
		std::string res;
		while (p < buf.size() && buf[p] != '{' && res.size() < 9)
			res += buf[p++];
		if (buf[p] == '{') {
			/* found the object_id - let's quickly look for the date */
			std::string_view buf2 = buf.substr(p);
			if (starts_with(buf2, "{date{ts{") && buf2.find("{duration{") != std::string::npos) {
				/* cool - that was easy */
				res += ',';
				res += ' ';
				/* skip the 9 characters we just found and take the date, ignoring the seconds
				 * and replace the silly 'T' in the middle with a space */
				res += buf2.substr(9, 16);
				size_t pos = res.size() - 16 + 10;
				if (res[pos] == 'T')
					res[pos] = ' ';
			}
			return res;
		}
	}
	return NULL;
}

/* ultra-simplistic; it doesn't deal with the case when the object_id is
 * split across two chunks. It also doesn't deal with the discrepancy between
 * object_id and dive number as understood by the dive computer */
static void show_progress(const std::string &buf, const char *what)
{
	std::string val = first_object_id_val(buf);
	if (!val.empty()) {
/* let the user know what we are working on */
#if UEMIS_DEBUG & 8
		report_info("reading %s\n %s\n %s\n", what, val.c_str(), buf.c_str());
#endif
		uemis_info(translate("gettextFromC", "%s %s"), what, val.c_str());
	}
}

static void uemis_increased_timeout(int *timeout)
{
	if (*timeout < uemis_max_timeout)
		*timeout += uemis_long_timeout;
	usleep(*timeout);
}

static std::string build_ans_path(const std::string &path, int filenumber)
{
	using namespace std::string_literals;

	/* Clamp filenumber into the 0..9999 range. This is never necessary,
	 * as filenumber can never go above uemis_max_files, but gcc doesn't
	 * recognize that and produces very noisy warnings. */
	filenumber = filenumber < 0 ? 0 : filenumber % 10000;

	std::string fl = "ANS"s + std::to_string(filenumber) + ".TXT"s;
	std::string intermediate = build_filename(path, "ANS"s);
	return build_filename(intermediate, fl);
}

/* send a request to the dive computer and collect the answer */
static std::string uemis_get_answer(const std::string &path, const std::string &request, int n_param_in,
				   int n_param_out, std::string &error_text)
{
	int i = 0;
	char tmp[101];
	const char *what = translate("gettextFromC", "data");
	bool searching = true;
	bool assembling_mbuf = false;
	bool ismulti = false;
	bool found_answer = false;
	bool more_files = true;
	bool answer_in_mbuf = false;
	int ans_file;
	int timeout = uemis_long_timeout;

	int reqtxt_file = subsurface_open(reqtxt_path.c_str(), O_RDWR | O_CREAT, 0666);
	if (reqtxt_file < 0) {
		error_text = "can't open req.txt";
#ifdef UEMIS_DEBUG
		report_info("open %s failed with errno %d\n", reqtxt_path.c_str(), errno);
#endif
		return std::string();
	}
	std::string sb;
	str_append_with_delim(sb, request);
	for (i = 0; i < n_param_in; i++)
		str_append_with_delim(sb, param_buff[i]);
	if (request == "getDivelogs" || request == "getDeviceData" || request == "getDirectory" ||
	    request == "getDivespot" || request == "getDive") {
		answer_in_mbuf = true;
		str_append_with_delim(sb, std::string());
		if (request == "getDivelogs")
			what = translate("gettextFromC", "dive log #");
		else if (request == "getDivespot")
			what = translate("gettextFromC", "dive spot #");
		else if (request == "getDive")
			what = translate("gettextFromC", "details for #");
	}
	str_append_with_delim(sb, std::string());
	size_t file_length = sb.size();
	std::string header = format_string_std("n%04d%08lu", filenr, (unsigned long)file_length);
	sb = header + sb;
#if UEMIS_DEBUG & 4
	report_info("::w req.txt \"%s\"\n", sb.c_str());
#endif
	int written = write(reqtxt_file, sb.c_str(), sb.size());
	if (written == -1 || (size_t)written != sb.size()) {
		error_text = translate("gettextFromC", ERR_FS_SHORT_WRITE);
		close(reqtxt_file);
		return std::string();
	}
	if (!next_file(number_of_files)) {
		error_text = translate("gettextFromC", ERR_FS_FULL);
		more_files = false;
	}
	trigger_response(reqtxt_file, "n", filenr, file_length);
	usleep(timeout);
	std::string mbuf;
	while (searching || assembling_mbuf) {
		if (import_thread_cancelled)
			return std::string();
		progress_bar_fraction = filenr / (double)uemis_max_files;
		std::string ans_path = build_ans_path(path, filenr - 1);
		ans_file = subsurface_open(ans_path.c_str(), O_RDONLY, 0666);
		if (ans_file < 0) {
			error_text = "can't open Uemis response file";
#ifdef UEMIS_DEBUG
			report_info("open %s failed with errno %d\n", ans_path.c_str(), errno);
#endif
			return std::string();
		}
		if (read(ans_file, tmp, 100) < 3) {
			close(ans_file);
			return std::string();
		}
		close(ans_file);
#if UEMIS_DEBUG & 8
		tmp[100] = '\0';
		report_info("::t %s \"%s\"\n", ans_path.c_str(), tmp);
#elif UEMIS_DEBUG & 4
		char pbuf[4];
		pbuf[0] = tmp[0];
		pbuf[1] = tmp[1];
		pbuf[2] = tmp[2];
		pbuf[3] = 0;
		report_info("::t %s \"%s...\"\n", ans_path.c_str(), pbuf);
#endif
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
					error_text = translate("gettextFromC", ERR_FS_FULL);
					more_files = false;
					assembling_mbuf = false;
				}
				reqtxt_file = subsurface_open(reqtxt_path.c_str(), O_RDWR | O_CREAT, 0666);
				if (reqtxt_file < 0) {
					error_text = "can't open req.txt";
					report_info("open %s failed with errno %d", reqtxt_path.c_str(), errno);
					return std::string();
				}
				trigger_response(reqtxt_file, "n", filenr, file_length);
			}
		} else {
			if (!next_file(number_of_files - 1)) {
				error_text = translate("gettextFromC", ERR_FS_FULL);
				more_files = false;
				assembling_mbuf = false;
				searching = false;
			}
			reqtxt_file = subsurface_open(reqtxt_path.c_str(), O_RDWR | O_CREAT, 0666);
			if (reqtxt_file < 0) {
				error_text = "can't open req.txt";
				report_info("open %s failed with errno %d", reqtxt_path.c_str(), errno);
				return std::string();
			}
			trigger_response(reqtxt_file, "r", filenr, file_length);
			uemis_increased_timeout(&timeout);
		}
		if (ismulti && more_files && tmp[0] == '1') {
			int size;
			std::string ans_path = build_ans_path(std::string(path), assembling_mbuf ? filenr - 2 : filenr - 1);
			ans_file = subsurface_open(ans_path.c_str(), O_RDONLY, 0666);
			if (ans_file < 0) {
				error_text = "can't open Uemis response file";
#ifdef UEMIS_DEBUG
				report_info("open %s failed with errno %d\n", ans_path.c_str(), errno);
#endif
				return std::string();
			}
			size = bytes_available(ans_file);
			if (size > 3) {
				int r;
				if (lseek(ans_file, 3, SEEK_CUR) == -1)
					goto fs_error;
				std::string buf(size - 3, ' ');
				if ((r = read(ans_file, buf.data(), size - 3)) != size - 3)
					goto fs_error;
				mbuf += buf;
				show_progress(buf, what);
				if (param_buff[3].size() > 1)
					param_buff[3] = param_buff[3].substr(1);
			}
			close(ans_file);
			timeout = uemis_timeout;
			usleep(uemis_timeout);
		}
	}
	if (more_files) {
		int j = 0;
		std::string buf;

		if (!ismulti) {
			std::string ans_path = build_ans_path(std::string(path), filenr - 1);
			ans_file = subsurface_open(ans_path.c_str(), O_RDONLY, 0666);
			if (ans_file < 0) {
				error_text = "can't open Uemis response file";
#ifdef UEMIS_DEBUG
				report_info("open %s failed with errno %d\n", ans_path.c_str(), errno);
#endif
				return std::string();
			}

			int size = bytes_available(ans_file);
			if (size > 3) {
				int r;
				if (lseek(ans_file, 3, SEEK_CUR) == -1)
					goto fs_error;
				buf = std::string(size - 3, ' ');
				if ((r = read(ans_file, buf.data(), size - 3)) != size - 3)
					goto fs_error;
				mbuf += buf;
				show_progress(buf, what);
#if UEMIS_DEBUG & 8
				report_info("::r %s \"%s\"\n", ans_path.c_str(), buf.c_str());
#endif
			}
			close(ans_file);
		} else {
			ismulti = false;
		}
#if UEMIS_DEBUG & 8
		report_info(":r: %s\n", buf.c_str());
#endif
		if (!answer_in_mbuf)
			for (i = 0; i < n_param_out && (size_t)j < buf.size(); i++)
				param_buff[i] = next_segment(buf, j);
		found_answer = true;
	}
#if UEMIS_DEBUG & 1
	for (i = 0; i < n_param_out; i++)
		report_info("::: %d: %s\n", i, param_buff[i].c_str());
#endif
	return found_answer ? std::move(mbuf) : std::string();
fs_error:
	close(ans_file);
	return std::string();
}

static bool parse_divespot(const std::string &buf)
{
	std::string_view bp = std::string_view(buf).substr(1);
	std::string_view tp = next_token(bp);
	std::string_view tag, type, val;
	std::string locationstring;
	int divespot;
	double latitude = 0.0, longitude = 0.0;

	// dive spot got deleted, so fail here
	if (bp.find("deleted{bool{true") != std::string::npos)
		return false;
	// not a dive spot, fail here
	if (tp != "divespot")
		return false;
	do
		tag = next_token(bp);
	while (!bp.empty() && tag != "object_id");
	if (bp.empty())
		return false;
	next_token(bp);
	val = next_token(bp);
	if (from_chars(val, divespot).ec == std::errc::invalid_argument)
		return false;
	do {
		tag = next_token(bp);
		type = next_token(bp);
		val = next_token(bp);
		if (type == "string" && !val.empty() && val != " ") {
			if (!locationstring.empty())
				locationstring += ", ";
			locationstring += val;
		} else if (type == "float") {
			if (tag == "longitude")
				from_chars(val, longitude);
			else if (tag == "latitude")
				from_chars(val, latitude);
		}
	} while (!tag.empty());

	uemis_obj.set_divelocation(divespot, locationstring, longitude, latitude);
	return true;
}

static const char *suit[] = {"", QT_TRANSLATE_NOOP("gettextFromC", "wetsuit"), QT_TRANSLATE_NOOP("gettextFromC", "semidry"), QT_TRANSLATE_NOOP("gettextFromC", "drysuit")};
static const char *suit_type[] = {"", QT_TRANSLATE_NOOP("gettextFromC", "shorty"), QT_TRANSLATE_NOOP("gettextFromC", "vest"), QT_TRANSLATE_NOOP("gettextFromC", "long john"), QT_TRANSLATE_NOOP("gettextFromC", "jacket"), QT_TRANSLATE_NOOP("gettextFromC", "full suit"), QT_TRANSLATE_NOOP("gettextFromC", "2 pcs full suit")};
static const char *suit_thickness[] = {"", "0.5-2mm", "2-3mm", "3-5mm", "5-7mm", "8mm+", QT_TRANSLATE_NOOP("gettextFromC", "membrane")};

static void parse_tag(struct dive *dive, std::string_view tag, std::string_view val)
{
	/* we can ignore computer_id, water and gas as those are redundant
	 * with the binary data and would just get overwritten */
#if UEMIS_DEBUG & 4
	if (tag == "file_content")
		report_info("Adding to dive %d : %s = %s\n", dive->dcs[0].diveid, std::string(tag).c_str(), std::string(val).c_str());
#endif
	if (tag == "date") {
		dive->when = uemis_ts(val);
	} else if (tag == "duration") {
		uemis_duration(val, dive->dcs[0].duration);
	} else if (tag == "depth") {
		uemis_depth(val, dive->dcs[0].maxdepth);
	} else if (tag == "file_content") {
		uemis_obj.parse_divelog_binary(val, dive);
	} else if (tag == "altitude") {
		uemis_get_index(val, dive->dcs[0].surface_pressure.mbar);
	} else if (tag == "f32Weight") {
		weightsystem_t ws;
		uemis_get_weight(val, ws, dive->dcs[0].diveid);
		dive->weightsystems.push_back(std::move(ws));
	} else if (tag == "notes") {
		uemis_add_string(val, dive->notes, " ");
	} else if (tag == "u8DiveSuit") {
		int idx = 0;
		uemis_get_index(val, idx);
		if (idx > 0 && idx < (int)std::size(suit))
			uemis_add_string(translate("gettextFromC", suit[idx]), dive->suit, " ");
	} else if (tag == "u8DiveSuitType") {
		int idx = 0;
		uemis_get_index(val, idx);
		if (idx > 0 && idx < (int)std::size(suit_type))
			uemis_add_string(translate("gettextFromC", suit_type[idx]), dive->suit, " ");
	} else if (tag == "u8SuitThickness") {
		int idx = 0;
		uemis_get_index(val, idx);
		if (idx > 0 && idx < (int)std::size(suit_thickness))
			uemis_add_string(translate("gettextFromC", suit_thickness[idx]), dive->suit, " ");
	} else if (tag == "nickname") {
		uemis_add_string(val, dive->buddy, ",");
	}
}

static bool uemis_delete_dive(device_data_t *devdata, uint32_t diveid)
{
	auto it = std::find_if(devdata->log->dives.begin(), devdata->log->dives.end(),
			       [diveid] (auto &d) { return d->dcs[0].diveid == diveid; });
	if (it == devdata->log->dives.end())
		return false;

	devdata->log->dives.erase(it);
	return true;
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
static bool process_raw_buffer(device_data_t *devdata, uint32_t deviceid, std::string_view buf, int &max_divenr, int *for_dive)
{
	using namespace std::string_literals;
	bool done = false;
	bool is_log = false;
	std::vector<std::string_view> sections;
	std::unique_ptr<dive> owned_dive;	// in log mode
	struct dive *non_owned_dive = nullptr;	// in dive (non-log) mode
	int dive_no = 0;

#if UEMIS_DEBUG & 8
	report_info("p_r_b %s\n", std::string(buf).c_str());
#endif
	if (for_dive)
		*for_dive = -1;
	std::string_view bp = buf.substr(1);
	std::string_view tp = next_token(bp);
	if (tp == "divelog") {
		/* this is a dive log */
		is_log = true;
		tp = next_token(bp);
		/* is it a valid entry or nothing ? */
		if (tp != "1.0" || buf.find("divelog{1.0{{{{") != std::string::npos)
			return false;
	} else if (tp == "dive") {
		/* this is dive detail */
		is_log = false;
		tp = next_token(bp);
		if (tp != "1.0")
			return false;
	} else {
		/* don't understand the buffer */
		return false;
	}
	if (is_log) {
		owned_dive = uemis_start_dive(deviceid);
	} else {
		/* remember, we don't know if this is the right entry,
		 * so first test if this is even a valid entry */
		if (buf.find("deleted{bool{true") != std::string::npos) {
#if UEMIS_DEBUG & 2
			report_info("p_r_b entry deleted\n");
#endif
			/* oops, this one isn't valid, suggest to try the previous one */
			return false;
		}
		/* quickhack and workaround to capture the original dive_no
		 * I am doing this so I don't have to change the original design
		 * but when parsing a dive we never parse the dive number because
		 * at the time it's being read the *dive variable is not set because
		 * the dive_no tag comes before the object_id in the uemis ans file
		 */
		size_t dive_no_pos = buf.find("dive_no{int{");
		if (dive_no_pos != std::string::npos) {
			dive_no_pos += 12;
			size_t dive_no_end = buf.find('{', dive_no_pos);
			if (dive_no_end != std::string::npos) {
				std::string_view dive_no_str = buf.substr(dive_no_pos, dive_no_end - dive_no_pos);
				if (from_chars(dive_no_str, dive_no).ec == std::errc::invalid_argument)
					dive_no = 0;
			}
		}
	}
	while (!done) {
		/* the valid buffer ends with a series of delimiters */
		if (bp.size() < 2 || starts_with(bp, "{{"))
			break;
		std::string_view tag = next_token(bp);
		/* we also end if we get an empty tag */
		if (tag.empty())
			break;
		if (std::find(sections.begin(), sections.end(), tag) != sections.end())
			tag = next_token(bp);
		std::string_view type = next_token(bp);
		if (type == "1.0") {
			/* this tells us the sections that will follow; the tag here
			 * is of the format dive-<section> */
			size_t pos = tag.find('-');
			if (pos != std::string::npos)
				sections.push_back(tag.substr(pos + 1));
#if UEMIS_DEBUG & 4
			report_info("Expect to find section %s\n", std::string(sections.back()).c_str());
#endif
			continue;
		}
		std::string_view val = next_token(bp);
#if UEMIS_DEBUG & 8
		if (val.size() < 20)
			report_info("Parsed %s, %s, %s\n*************************\n", std::string(tag).c_str(),
										      std::string(type).c_str(),
										      std::string(val).c_str());
#endif
		if (is_log) {
			// Is log
			if (tag == "object_id") {
				from_chars(val, max_divenr);
				owned_dive->dcs[0].diveid = max_divenr;
#if UEMIS_DEBUG % 2
				report_info("Adding new dive from log with object_id %d.\n", max_divenr);
#endif
			}
			parse_tag(owned_dive.get(), tag, val);

			if (tag == "file_content")
				done = true;
			/* done with one dive (got the file_content tag), but there could be more:
			 * a '{' indicates the end of the record - but we need to see another "{{"
			 * later in the buffer to know that the next record is complete (it could
			 * be a short read because of some error */
			if (done && bp.size() > 3) {
				bp = bp.substr(1);
				if (bp[0] != '{' && bp.find("{{") != std::string::npos) {
					done = false;
					devdata->log->dives.record_dive(std::move(owned_dive));
					owned_dive = uemis_start_dive(deviceid);
				}
			}
		} else {
			// Is dive
			if (tag == "logfilenr") {
				/* this one tells us which dive we are adding data to */
				int diveid = 0;
				from_chars(val, diveid);
				non_owned_dive = get_dive_by_uemis_diveid(devdata, diveid);
				if (dive_no != 0)
					non_owned_dive->number = dive_no;
				if (for_dive)
					*for_dive = diveid;
			} else if (non_owned_dive && tag == "divespot_id") {
				int divespot_id;
				if (from_chars(val, divespot_id).ec != std::errc::invalid_argument) {
					struct dive_site *ds = devdata->log->sites.create("from Uemis"s);
					unregister_dive_from_dive_site(non_owned_dive);
					ds->add_dive(non_owned_dive);
					uemis_obj.mark_divelocation(non_owned_dive->dcs[0].diveid, divespot_id, ds);
				}
#if UEMIS_DEBUG & 2
				report_info("Created divesite %d for diveid : %d\n", non_owned_dive->dive_site->uuid, non_owned_dive->dcs[0].diveid);
#endif
			} else if (non_owned_dive) {
				parse_tag(non_owned_dive, tag, val);
			}
		}
	}
	if (is_log) {
		if (owned_dive->dcs[0].diveid)
			devdata->log->dives.record_dive(std::move(owned_dive));
		else /* partial dive */
			return false;
	}
	return true;
}

// Returns (mindiveid, maxdiveid)
static std::pair<uint32_t, uint32_t> uemis_get_divenr(uint32_t deviceid, struct dive_table *table, int force)
{
	uint32_t maxdiveid = 0;
	uint32_t mindiveid = 0xFFFFFFFF;

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
	if (!force && table->empty())
		table = &divelog.dives;

	for (auto &d: *table) {
		if (!d)
			continue;
		for (auto &dc: d->dcs) {
			if (dc.model == "Uemis Zurich" &&
			    (dc.deviceid == 0 || dc.deviceid == 0x7fffffff || dc.deviceid == deviceid)) {
				if (dc.diveid > maxdiveid)
					maxdiveid = dc.diveid;
				if (dc.diveid < mindiveid)
					mindiveid = dc.diveid;
			}
		}
	}
	return std::make_pair(mindiveid, maxdiveid);
}

#if UEMIS_DEBUG
static bool do_dump_buffer_to_file(std::string_view buf, const char *prefix)
{
	using namespace std::string_literals;
	static int bufCnt = 0;
	if (buf.empty())
		return false;

	size_t datepos = buf.find("date{ts{");
	std::string pdate;
	if (datepos != std::string::npos) {
		std::string_view ptr1 = buf.substr(datepos);
		next_token(ptr1);
		next_token(ptr1);
		pdate = next_token(ptr1);
	} else {
		pdate = "date{ts{no-date{"s;
	}

	size_t obidpos = buf.find("object_id{int{");
	if (obidpos == std::string::npos)
		return false;

	std::string_view ptr2 = buf.substr(obidpos);
	next_token(ptr2);
	next_token(ptr2);
	std::string pobid = std::string(next_token(ptr2));
	std::string path = format_string_std("/%s/%s/UEMIS Dump/%s_%s_Uemis_dump_%s_in_round_%d_%d.txt", home.c_str(), user.c_str(), prefix, pdate.c_str(), pobid.c_str(), debug_round, bufCnt);
	int dumpFile = subsurface_open(path.c_str(), O_RDWR | O_CREAT, 0666);
	if (dumpFile == -1)
		return false;
	bool success = (size_t)write(dumpFile, buf.data(), buf.size()) == buf.size();
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
 * return : ok       if there is enough memory for a full round
 *          full     if the memory is exhausted
 */
static uemis_mem_status get_memory(struct dive_table &td, uemis_checkpoint checkpoint)
{
	if (td.empty())
		return uemis_mem_status::ok;

	switch (checkpoint) {
	case uemis_checkpoint::log:
		if (filenr / static_cast<int>(td.size()) > max_mem_used)
			max_mem_used = filenr / static_cast<int>(td.size());

		/* check if a full block of dive logs + dive details and dive spot fit into the UEMIS buffer */
#if UEMIS_DEBUG & 4
		report_info("max_mem_used %d (from td.size() %d) * block_size %d > max_files %d - filenr %d?\n", max_mem_used, static_cast<int>(td.size()), uemis_log_block_size, uemis_max_files, filenr);
#endif
		if (max_mem_used * uemis_log_block_size > uemis_max_files - filenr)
			return uemis_mem_status::full;
		break;
	case uemis_checkpoint::details:
		/* check if the next set of dive details and dive spot fit into the UEMIS buffer */
		if ((uemis_dive_details_size + uemis_spot_block_size) * uemis_log_block_size > uemis_max_files - filenr)
			return uemis_mem_status::full;
		break;
	case uemis_checkpoint::single_dive:
		if (uemis_dive_details_size + uemis_spot_block_size > uemis_max_files - filenr)
			return uemis_mem_status::full;
		break;
	}
	return uemis_mem_status::ok;
}

/* we misuse the hidden_by_filter flag to mark a dive as deleted.
 * this will be picked up by some cleaning statement later. */
static void do_delete_dives(struct dive_table &td, size_t idx)
{
	for (auto it = td.begin() + idx; it != td.end(); ++it)
		(*it)->hidden_by_filter = true;
}

static bool load_uemis_divespot(const std::string &mountpath, int divespot_id)
{
	param_buff[2] = std::to_string(divespot_id);
#if UEMIS_DEBUG & 2
	report_info("getDivespot %d\n", divespot_id);
#endif
	std::string error_text; // unused
	std::string mbuf = uemis_get_answer(mountpath, "getDivespot", 3, 0, error_text);
	if (!mbuf.empty()) {
#if UEMIS_DEBUG & 16
		do_dump_buffer_to_file(mbuf, "Spot");
#endif
		return parse_divespot(mbuf);
	}
	return false;
}

static void get_uemis_divespot(device_data_t *devdata, const std::string &mountpath, int divespot_id, struct dive *dive)
{
	struct dive_site *nds = dive->dive_site;

	auto it = divespot_mapping.find(divespot_id);
	if (it != divespot_mapping.end()) {
		struct dive_site *ds = it->second;
		unregister_dive_from_dive_site(dive);
		ds->add_dive(dive);
	} else if (nds && !nds->name.empty() && nds->name.find("from Uemis") != std::string::npos) {
		if (load_uemis_divespot(mountpath, divespot_id)) {
			/* get the divesite based on the diveid, this should give us
			* the newly created site
			*/
			/* with the divesite name we got from parse_dive, that is called on load_uemis_divespot
			 * we search all existing divesites if we have one with the same name already. The function
			 * returns the first found which is luckily not the newly created.
			 */
			struct dive_site *ods = devdata->log->sites.get_by_name(nds->name);
			if (ods && nds->uuid != ods->uuid) {
				/* if the uuid's are the same, the new site is a duplicate and can be deleted */
				unregister_dive_from_dive_site(dive);
				ods->add_dive(dive);
				devdata->log->sites.pull(nds);
			}
			divespot_mapping[divespot_id] = dive->dive_site;
		} else {
			/* if we can't load the dive site details, delete the site we
			* created in process_raw_buffer
			*/
			devdata->log->sites.pull(dive->dive_site);
			dive->dive_site = nullptr;
		}
	}
}

static bool get_matching_dive(size_t idx, int &newmax, uemis_mem_status &mem_status, device_data_t *data, const std::string &mountpath, const char deviceidnr)
{
	auto &dive = data->log->dives[idx];
	char log_file_no_to_find[20];
	bool found = false;
	bool found_below = false;
	bool found_above = false;
	int deleted_files = 0;
	int fail_count = 0;

	snprintf(log_file_no_to_find, sizeof(log_file_no_to_find), "logfilenr{int{%d", dive->dcs[0].diveid);
#if UEMIS_DEBUG & 2
	report_info("Looking for dive details to go with dive log id %d\n", dive->dcs[0].diveid);
#endif
	while (!found) {
		if (import_thread_cancelled)
			break;
		param_buff[2] = std::to_string(dive_to_read);
		std::string error_text; // unused
		std::string mbuf = uemis_get_answer(mountpath, "getDive", 3, 0, error_text);
#if UEMIS_DEBUG & 16
		do_dump_buffer_to_file(mbuf, "Dive");
#endif
		mem_status = get_memory(data->log->dives, uemis_checkpoint::single_dive);
		if (mem_status == uemis_mem_status::ok) {
			/* if the memory isn's completely full we can try to read more dive log vs. dive details
			 * and the dive spots should fit into the UEMIS memory
			 * The match we do here is to map the object_id to the logfilenr, we do this
			 * by iterating through the last set of loaded dive logs and then find the corresponding
			 * dive with the matching logfilenr */
			if (!mbuf.empty()) {
				if (strstr(mbuf.c_str(), log_file_no_to_find)) {
					/* we found the logfilenr that matches our object_id from the dive log we were looking for
					 * we mark the search successful even if the dive has been deleted. */
					found = true;
					if (strstr(mbuf.c_str(), "deleted{bool{true") == NULL) {
						process_raw_buffer(data, deviceidnr, mbuf, newmax, NULL);
						/* remember the last log file number as it is very likely that subsequent dives
						 * have the same or higher logfile number.
						 * UEMIS unfortunately deletes dives by deleting the dive details and not the logs. */
#if UEMIS_DEBUG & 2
						d_time = get_dive_date_c_string(dive->when);
						report_info("Matching dive log id %d from %s with dive details %d\n", dive->dcs[0].diveid, d_time.c_str(), dive_to_read);
#endif
						int divespot_id = uemis_obj.get_divespot_id_by_diveid(dive->dcs[0].diveid);
						if (divespot_id >= 0)
							get_uemis_divespot(data, mountpath, divespot_id, dive.get());

					} else {
						/* in this case we found a deleted file, so let's increment */
#if UEMIS_DEBUG & 2
						d_time = get_dive_date_c_string(dive->when);
						report_info("TRY matching dive log id %d from %s with dive details %d but details are deleted\n", dive->dcs[0].diveid, d_time.c_str(), dive_to_read);
#endif
						deleted_files++;
						/* mark this log entry as deleted and cleanup later, otherwise we mess up our array */
						dive->hidden_by_filter = true;
#if UEMIS_DEBUG & 2
						report_info("Deleted dive from %s, with id %d from table -- newmax is %d\n", d_time.c_str(), dive->dcs[0].diveid, newmax);
#endif
					}
				} else {
					uint32_t nr_found = 0;
					size_t pos = mbuf.find("logfilenr");
					if (pos != std::string::npos && mbuf.find("act{") != std::string::npos) {
						sscanf(mbuf.c_str() + pos, "logfilenr{int{%u", &nr_found);
						if (nr_found >= dive->dcs[0].diveid || nr_found == 0) {
							found_above = true;
							dive_to_read = dive_to_read - 2;
						} else {
							found_below = true;
						}
						if (dive_to_read < -1)
							dive_to_read = -1;
					} else if (mbuf.find("act{") == std::string::npos && ++fail_count == 10) {
						if (verbose)
							report_info("Uemis downloader: Cannot access dive details - searching from start");
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
			do_delete_dives(data->log->dives, idx);
			return false;
		}
	}
	/* decrement iDiveToRead by the amount of deleted entries found to assure
	 * we are not missing any valid matches when processing subsequent logs */
	dive_to_read = (dive_to_read - deleted_files > 0 ? dive_to_read - deleted_files : 0);
	deleted_files = 0;
	return true;
}

std::string do_uemis_import(device_data_t *data)
{
	const std::string &mountpath = data->devname;
	short force_download = data->force_download;
	int newmax = -1;
	int first, start, end = -2;
	uint32_t deviceidnr;
	std::string deviceid;
	std::string result;
	bool once = true;
	int dive_offset = 0;
	uemis_mem_status mem_status = uemis_mem_status::ok;

	// To speed up sync you can skip downloading old dives by defining UEMIS_DIVE_OFFSET
	if (getenv("UEMIS_DIVE_OFFSET")) {
		dive_offset = atoi(getenv("UEMIS_DIVE_OFFSET"));
		report_info("Uemis: Using dive # offset %d\n", dive_offset);
	}

#if UEMIS_DEBUG
	home = getenv("HOME");
	user = getenv("LOGNAME");
#endif
	uemis_info(translate("gettextFromC", "Initialise communication"));
	if (!uemis_init(mountpath)) {
		return translate("gettextFromC", "Uemis init failed");
	}

	if (uemis_get_answer(mountpath, "getDeviceId", 0, 1, result).empty())
		goto bail;
	deviceid = param_buff[0];
	deviceidnr = atoi(deviceid.c_str());

	/* param_buff[0] is still valid */
	if (uemis_get_answer(mountpath, "initSession", 1, 6, result).empty())
		goto bail;

	uemis_info(translate("gettextFromC", "Start download"));
	if (uemis_get_answer(mountpath, "processSync", 0, 2, result).empty())
		goto bail;

	/* before starting the long download, check if user pressed cancel */
	if (import_thread_cancelled)
		goto bail;

	param_buff[1] = "notempty";
	{
		auto [mindiveid, maxdiveid] = uemis_get_divenr(deviceidnr, &data->log->dives, force_download);
		newmax = maxdiveid;
		if (verbose)
			report_info("Uemis downloader: start looking at dive nr %d", newmax);

		first = start = newmax;
		dive_to_read = (int)mindiveid < first ? first - mindiveid : first;
	}
	if (dive_offset > 0)
		start += dive_offset;
	for (;;) {
#if UEMIS_DEBUG & 2
		debug_round++;
#endif
#if UEMIS_DEBUG & 4
		report_info("d_u_i inner loop start %d end %d newmax %d\n", start, end, newmax);
#endif
		/* start at the last filled download table index */
		size_t match_dive_and_log = data->log->dives.size();
		newmax = start;
		std::string newmax_str = std::to_string(newmax);
		param_buff[2] = newmax_str.c_str();
		param_buff[3].clear();
		std::string mbuf = uemis_get_answer(mountpath, "getDivelogs", 3, 0, result);
		mem_status = get_memory(data->log->dives, uemis_checkpoint::details);
		/* first, remove any leading garbage... this needs to start with a '{' */
		std::string_view realmbuf = mbuf;
		size_t pos = realmbuf.find('{');
		if (pos != std::string::npos)
			realmbuf = realmbuf.substr(pos);
		if (!realmbuf.empty() && mem_status != uemis_mem_status::full) {
#if UEMIS_DEBUG & 16
			do_dump_buffer_to_file(realmbuf, "Dive logs");
#endif
			bool success = true;
			/* process the buffer we have assembled */
			if (!process_raw_buffer(data, deviceidnr, realmbuf, newmax, NULL)) {
				/* if no dives were downloaded, mark end appropriately */
				/* Might be related to the "clean up mbuf" below.
				 * Disable for now, since end will be overwritten anyway */
				//if (end == -2)
					//end = start - 1;
				success = false;
			}
			if (once) {
				std::string t = first_object_id_val(realmbuf);
				int val;
				if (from_chars(t, val).ec != std::errc::invalid_argument) {
					start = std::max(val, start);
				}
				once = false;
			}
			/* clean up mbuf */
			/* reason unclear:
			const char *endptr = strstr(realmbuf, "{{{");
			if (endptr)
				*(endptr + 2) = '\0';
			*/
			/* last object_id we parsed */
			end = newmax;
#if UEMIS_DEBUG & 4
			report_info("d_u_i after download and parse start %d end %d newmax %d progress %4.2f\n", start, end, newmax, progress_bar_fraction);
#endif
			/* The way this works is that I am reading the current dive from what has been loaded during the getDiveLogs call to the UEMIS.
			 * As the object_id of the dive log entry and the object_id of the dive details are not necessarily the same, the match needs
			 * to happen based on the logfilenr.
			 * What the following part does is to optimize the mapping by using
			 * dive_to_read = the dive details entry that need to be read using the object_id
			 * logFileNoToFind = map the logfilenr of the dive details with the object_id = diveid from the get dive logs */
			for (size_t i = match_dive_and_log; i < data->log->dives.size(); i++) {
				if (!get_matching_dive(i, newmax, mem_status, data, mountpath, deviceidnr))
					break;
				if (import_thread_cancelled)
					break;
			}

			start = end;

			/* Do some memory checking here */
			mem_status = get_memory(data->log->dives, uemis_checkpoint::log);
			if (mem_status != uemis_mem_status::ok) {
#if UEMIS_DEBUG & 4
				report_info("d_u_i out of memory, bailing\n");
#endif
				mbuf = uemis_get_answer(mountpath, "terminateSync", 0, 3, result);
				const char *errormsg = translate("gettextFromC", ACTION_RECONNECT);
				for (int wait=60; wait >=0; wait--){
					uemis_info("%s %ds", errormsg, wait);
					usleep(1000000);
				}
				// Resetting to original state
				filenr = 0;
				max_mem_used = -1;
				mem_status = get_memory(data->log->dives, uemis_checkpoint::details);
				if (uemis_get_answer(mountpath, "getDeviceId", 0, 1, result).empty())
					goto bail;
				if (deviceid != param_buff[0]) {
					report_info("Uemis: Device id has changed after reconnect!");
					goto bail;
				}
				param_buff[0] = deviceid;
				if (uemis_get_answer(mountpath, "initSession", 1, 6, result).empty())
					goto bail;
				uemis_info(translate("gettextFromC", "Start download"));
				if (uemis_get_answer(mountpath, "processSync", 0, 2, result).empty())
					goto bail;
				param_buff[1] = "notempty";
			}
			/* if the user clicked cancel, exit gracefully */
			if (import_thread_cancelled) {
#if UEMIS_DEBUG & 4
				report_info("d_u_i thread canceled, bailing\n");
#endif
				break;
			}
			/* if we got an error or got nothing back, stop trying */
			if (!success || param_buff[3].empty()) {
#if UEMIS_DEBUG & 4
				report_info("d_u_i after download nothing found, giving up\n");
#endif
				break;
			}
#if UEMIS_DEBUG & 2
			if (debug_round != -1)
				if (debug_round-- == 0) {
					report_info("d_u_i debug_round is now 0, bailing\n");
					goto bail;
				}
#endif
		} else {
			/* some of the loading from the UEMIS failed at the dive log level
			 * if the memory status = full, we can't even load the dive spots and/or buddies.
			 * The loaded block of dive logs is useless and all new loaded dive logs need to
			 * be deleted from the download_table.
			 */
			if (mem_status == uemis_mem_status::full)
				do_delete_dives(data->log->dives, match_dive_and_log);
#if UEMIS_DEBUG & 4
			report_info("d_u_i out of memory, bailing instead of processing\n");
#endif
			break;
		}
	}

	if (end == -2)
		end = newmax;

#if UEMIS_DEBUG & 2
	report_info("Done: read from object_id %d to %d\n", first, end);
#endif

	/* Regardless on where we are with the memory situation, it's time now
	 * to see if we have to clean some dead bodies from our download table */
	for (size_t next_table_index = 0; next_table_index < data->log->dives.size(); ) {
		if (data->log->dives[next_table_index]->hidden_by_filter)
			uemis_delete_dive(data, data->log->dives[next_table_index]->dcs[0].diveid);
		else
			next_table_index++;
	}

	if (mem_status != uemis_mem_status::ok)
		result = translate("gettextFromC", ERR_FS_ALMOST_FULL);

	if (data->sync_time)
		uemis_info(translate("gettextFromC", "Time sync not supported by dive computer"));

bail:
	(void)uemis_get_answer(mountpath, "terminateSync", 0, 3, result);
	if (param_buff[0] == "error") {
		if (param_buff[2] == "Out of Memory")
			result = translate("gettextFromC", ERR_FS_FULL);
		else
			result = param_buff[2];
	}
	if (data->log->dives.empty())
		result = translate("gettextFromC", ERR_NO_FILES);
	return result;
}
