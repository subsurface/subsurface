// SPDX-License-Identifier: GPL-2.0
#include "ssrf.h"
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
#include "divelog.h"
#include "subsurface-string.h"
#include "format.h"
#include "errorhelper.h"
#include "file.h"
#include "git-access.h"
#include "qthelper.h"
#include "import-csv.h"
#include "parse.h"

/* For SAMPLE_* */
#include <libdivecomputer/parser.h>

/* to check XSLT version number */
#include <libxslt/xsltconfig.h>

/* Crazy windows sh*t */
#ifndef O_BINARY
#define O_BINARY 0
#endif

std::pair<std::string, int> readfile(const char *filename)
{
	int ret, fd;
	struct stat st;

	std::string res;
	fd = subsurface_open(filename, O_RDONLY | O_BINARY, 0);
	if (fd < 0)
		return std::make_pair(res, fd);
	ret = fstat(fd, &st);
	if (ret < 0)
		return std::make_pair(res, ret);
	if (!S_ISREG(st.st_mode))
		return std::make_pair(res, -EINVAL);
	if (!st.st_size)
		return std::make_pair(res, 0);
	// Sadly, this 0-initializes the string, just before overwriting it.
	// However, we use std::string, because that automatically 0-terminates
	// the data and the code expects that.
	res.resize(st.st_size);
	ret = read(fd, res.data(), res.size());
	if (ret < 0)
		return std::make_pair(res, ret);
	// converting to int loses a bit but size will never be that big
	if (ret == (int)res.size()) {
		return std::make_pair(res, ret);
	} else {
		errno = EIO;
		return std::make_pair(res, -1);
	}
}

static void zip_read(struct zip_file *file, const char *filename, struct divelog *log)
{
	int size = 1024, n, read = 0;
	std::vector<char> mem(size + 1);

	while ((n = zip_fread(file, mem.data() + read, size - read)) > 0) {
		read += n;
		size = read * 3 / 2;
		mem.resize(size + 1);
	}
	mem[read] = 0;
	(void) parse_xml_buffer(filename, mem.data(), read, log, NULL);
}

extern "C" int try_to_open_zip(const char *filename, struct divelog *log)
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
			zip_read(file, filename, log);
			zip_fclose(file);
			success++;
		}
		subsurface_zip_close(zip);

		if (!success)
			return report_error(translate("gettextFromC", "No dives in the input file '%s'"), filename);
	}
	return success;
}

static int db_test_func(void *, int, char **data, char **)
{
	return *data[0] == '0';
}

static int try_to_open_db(const char *filename, std::string &mem, struct divelog *log)
{
	sqlite3 *handle;
	char dm4_test[] = "select count(*) from sqlite_master where type='table' and name='Dive' and sql like '%ProfileBlob%'";
	char dm5_test[] = "select count(*) from sqlite_master where type='table' and name='Dive' and sql like '%SampleBlob%'";
	char shearwater_test[] = "select count(*) from sqlite_master where type='table' and name='system' and sql like '%dbVersion%'";
	char shearwater_cloud_test[] = "select count(*) from sqlite_master where type='table' and name='SyncV3MetadataDiveLog' and sql like '%CreatedDevice%'";
	char cobalt_test[] = "select count(*) from sqlite_master where type='table' and name='TrackPoints' and sql like '%DepthPressure%'";
	char divinglog_test[] = "select count(*) from sqlite_master where type='table' and name='DBInfo' and sql like '%PrgName%'";
	char seacsync_test[] = "select count(*) from sqlite_master where type='table' and name='dive_data' and sql like '%ndl_tts_s%'";
	int retval;

	retval = sqlite3_open(filename, &handle);

	if (retval) {
		report_info("Database connection failed '%s'", filename);
		return 1;
	}

	/* Testing if DB schema resembles Suunto DM5 database format */
	retval = sqlite3_exec(handle, dm5_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_dm5_buffer(handle, filename, mem.data(), mem.size(), log);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Suunto DM4 database format */
	retval = sqlite3_exec(handle, dm4_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_dm4_buffer(handle, filename, mem.data(), mem.size(), log);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Shearwater database format */
	retval = sqlite3_exec(handle, shearwater_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_shearwater_buffer(handle, filename, mem.data(), mem.size(), log);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Shearwater cloud database format */
	retval = sqlite3_exec(handle, shearwater_cloud_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_shearwater_cloud_buffer(handle, filename, mem.data(), mem.size(), log);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Atomic Cobalt database format */
	retval = sqlite3_exec(handle, cobalt_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_cobalt_buffer(handle, filename, mem.data(), mem.size(), log);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Divinglog database format */
	retval = sqlite3_exec(handle, divinglog_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_divinglog_buffer(handle, filename, mem.data(), mem.size(), log);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Seac database format */
	retval = sqlite3_exec(handle, seacsync_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_seac_buffer(handle, filename, mem.data(), mem.size(), log);
		sqlite3_close(handle);
		return retval;
	}


	sqlite3_close(handle);

	return retval;
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
static int open_by_filename(const char *filename, const char *fmt, std::string &mem, struct divelog *log)
{
	// hack to be able to provide a comment for the translated string
	static struct { const char *s; const char *comment; } csv_warning =
		QT_TRANSLATE_NOOP3("gettextFromC",
				   "Cannot open CSV file %s; please use Import log file dialog",
				   "'Import log file' should be the same text as corresponding label in Import menu");

	/* Suunto Dive Manager files: SDE, ZIP; divelogs.de files: DLD */
	if (!strcasecmp(fmt, "SDE") || !strcasecmp(fmt, "ZIP") || !strcasecmp(fmt, "DLD"))
		return try_to_open_zip(filename, log);

	/* CSV files */
	if (!strcasecmp(fmt, "CSV"))
		return report_error(translate("gettextFromC", csv_warning.s), filename);
	/* Truly nasty intentionally obfuscated Cochran Anal software */
	if (!strcasecmp(fmt, "CAN"))
		return try_to_open_cochran(filename, mem, log);
	/* Cochran export comma-separated-value files */
	if (!strcasecmp(fmt, "DPT"))
		return try_to_open_csv(mem, CSV_DEPTH, log);
	if (!strcasecmp(fmt, "LVD"))
		return try_to_open_liquivision(filename, mem, log);
	if (!strcasecmp(fmt, "TMP"))
		return try_to_open_csv(mem, CSV_TEMP, log);
	if (!strcasecmp(fmt, "HP1"))
		return try_to_open_csv(mem, CSV_PRESSURE, log);

	return 0;
}

static int parse_file_buffer(const char *filename, std::string &mem, struct divelog *log)
{
	int ret;
	const char *fmt = strrchr(filename, '.');
	if (fmt && (ret = open_by_filename(filename, fmt + 1, mem, log)) != 0)
		return ret;

	if (mem.empty())
		return report_error("Out of memory parsing file %s\n", filename);

	return parse_xml_buffer(filename, mem.data(), mem.size(), log, NULL);
}

bool remote_repo_uptodate(const char *filename, struct git_info *info)
{
	std::string current_sha = saved_git_id;

	if (is_git_repository(filename, info) && open_git_repository(info)) {
		std::string sha = get_sha(info->repo, info->branch);
		if (!sha.empty() && current_sha == sha) {
			report_info("already have loaded SHA %s - don't load again", sha.c_str());
			return true;
		}
	}

	// Either the repository couldn't be opened, or the SHA couldn't
	// be found.
	return false;
}

extern "C" int parse_file(const char *filename, struct divelog *log)
{
	struct git_info info;
	const char *fmt;

	if (is_git_repository(filename, &info)) {
		if (!open_git_repository(&info)) {
			/*
			 * Opening the cloud storage repository failed for some reason
			 * give up here and don't send errors about git repositories
			 */
			if (info.is_subsurface_cloud)
				return -1;
		}

		int ret = git_load_dives(&info, log);
		return ret;
	}

	auto [mem, err] = readfile(filename);
	if (err < 0) {
		/* we don't want to display an error if this was the default file  */
		if (same_string(filename, prefs.default_filename))
			return 0;

		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);
	} else if (err == 0) {
		return report_error(translate("gettextFromC", "Empty file '%s'"), filename);
	}

	fmt = strrchr(filename, '.');
	if (fmt && (!strcasecmp(fmt + 1, "DB") || !strcasecmp(fmt + 1, "BAK") || !strcasecmp(fmt + 1, "SQL"))) {
		if (!try_to_open_db(filename, mem, log))
			return 0;
	}

	/* Divesoft Freedom */
	if (fmt && (!strcasecmp(fmt + 1, "DLF")))
		return parse_dlf_buffer((unsigned char *)mem.data(), mem.size(), log);

	/* DataTrak/Wlog */
	if (fmt && !strcasecmp(fmt + 1, "LOG")) {
		const char *t = strrchr(filename, '.');
		std::string wl_name = std::string(filename, t - filename) + ".add";
		auto [wl_mem, err] = readfile(wl_name.c_str());
		if (err < 0) {
			report_info("No file %s found. No WLog extensions.", wl_name.c_str());
			wl_mem.clear();
		}
		return datatrak_import(mem, wl_mem, log);
	}

	/* OSTCtools */
	if (fmt && (!strcasecmp(fmt + 1, "DIVE"))) {
		ostctools_import(filename, log);
		return 0;
	}

	return parse_file_buffer(filename, mem, log);
}
