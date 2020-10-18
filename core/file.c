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
#include "subsurface-string.h"
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
	if (ret == (int)mem->size) // converting to int loses a bit but size will never be that big
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


static void zip_read(struct zip_file *file, const char *filename, struct dive_table *table, struct trip_table *trips,
		     struct dive_site_table *sites, struct device_table *devices, struct filter_preset_table *filter_presets)
{
	int size = 1024, n, read = 0;
	char *mem = malloc(size);

	while ((n = zip_fread(file, mem + read, size - read)) > 0) {
		read += n;
		size = read * 3 / 2;
		mem = realloc(mem, size);
	}
	mem[read] = 0;
	(void) parse_xml_buffer(filename, mem, read, table, trips, sites, devices, filter_presets, NULL);
	free(mem);
}

int try_to_open_zip(const char *filename, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
		    struct device_table *devices, struct filter_preset_table *filter_presets)
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
			zip_read(file, filename, table, trips, sites, devices, filter_presets);
			zip_fclose(file);
			success++;
		}
		subsurface_zip_close(zip);

		if (!success)
			return report_error(translate("gettextFromC", "No dives in the input file '%s'"), filename);
	}
	return success;
}

static int db_test_func(void *param, int columns, char **data, char **column)
{
	UNUSED(param);
	UNUSED(columns);
	UNUSED(column);
	return *data[0] == '0';
}

static int try_to_open_db(const char *filename, struct memblock *mem, struct dive_table *table, struct trip_table *trips,
			  struct dive_site_table *sites, struct device_table *devices)
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
		fprintf(stderr, "Database connection failed '%s'.\n", filename);
		return 1;
	}

	/* Testing if DB schema resembles Suunto DM5 database format */
	retval = sqlite3_exec(handle, dm5_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_dm5_buffer(handle, filename, mem->buffer, mem->size, table, trips, sites, devices);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Suunto DM4 database format */
	retval = sqlite3_exec(handle, dm4_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_dm4_buffer(handle, filename, mem->buffer, mem->size, table, trips, sites, devices);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Shearwater database format */
	retval = sqlite3_exec(handle, shearwater_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_shearwater_buffer(handle, filename, mem->buffer, mem->size, table, trips, sites, devices);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Shearwater cloud database format */
	retval = sqlite3_exec(handle, shearwater_cloud_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_shearwater_cloud_buffer(handle, filename, mem->buffer, mem->size, table, trips, sites, devices);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Atomic Cobalt database format */
	retval = sqlite3_exec(handle, cobalt_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_cobalt_buffer(handle, filename, mem->buffer, mem->size, table, trips, sites, devices);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Divinglog database format */
	retval = sqlite3_exec(handle, divinglog_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_divinglog_buffer(handle, filename, mem->buffer, mem->size, table, trips, sites, devices);
		sqlite3_close(handle);
		return retval;
	}

	/* Testing if DB schema resembles Seac database format */
	retval = sqlite3_exec(handle, seacsync_test, &db_test_func, 0, NULL);
	if (!retval) {
		retval = parse_seac_buffer(handle, filename, mem->buffer, mem->size, table, trips, sites, devices);
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
static int open_by_filename(const char *filename, const char *fmt, struct memblock *mem,
			    struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
			    struct device_table *devices, struct filter_preset_table *filter_presets)
{
	// hack to be able to provide a comment for the translated string
	static char *csv_warning = QT_TRANSLATE_NOOP3("gettextFromC",
						      "Cannot open CSV file %s; please use Import log file dialog",
						      "'Import log file' should be the same text as corresponding label in Import menu");

	/* Suunto Dive Manager files: SDE, ZIP; divelogs.de files: DLD */
	if (!strcasecmp(fmt, "SDE") || !strcasecmp(fmt, "ZIP") || !strcasecmp(fmt, "DLD"))
		return try_to_open_zip(filename, table, trips, sites, devices, filter_presets);

	/* CSV files */
	if (!strcasecmp(fmt, "CSV"))
		return report_error(translate("gettextFromC", csv_warning), filename);
	/* Truly nasty intentionally obfuscated Cochran Anal software */
	if (!strcasecmp(fmt, "CAN"))
		return try_to_open_cochran(filename, mem, table, trips, sites);
	/* Cochran export comma-separated-value files */
	if (!strcasecmp(fmt, "DPT"))
		return try_to_open_csv(mem, CSV_DEPTH, table, trips, sites);
	if (!strcasecmp(fmt, "LVD"))
		return try_to_open_liquivision(filename, mem, table, trips, sites);
	if (!strcasecmp(fmt, "TMP"))
		return try_to_open_csv(mem, CSV_TEMP, table, trips, sites);
	if (!strcasecmp(fmt, "HP1"))
		return try_to_open_csv(mem, CSV_PRESSURE, table, trips, sites);

	return 0;
}

static int parse_file_buffer(const char *filename, struct memblock *mem, struct dive_table *table,
			     struct trip_table *trips, struct dive_site_table *sites,
			     struct device_table *devices, struct filter_preset_table *filter_presets)
{
	int ret;
	char *fmt = strrchr(filename, '.');
	if (fmt && (ret = open_by_filename(filename, fmt + 1, mem, table, trips, sites, devices, filter_presets)) != 0)
		return ret;

	if (!mem->size || !mem->buffer)
		return report_error("Out of memory parsing file %s\n", filename);

	return parse_xml_buffer(filename, mem->buffer, mem->size, table, trips, sites, devices, filter_presets, NULL);
}

int check_git_sha(const char *filename, struct git_repository **git_p, const char **branch_p)
{
	struct git_repository *git;
	const char *branch = NULL;

	char *current_sha = copy_string(saved_git_id);
	git = is_git_repository(filename, &branch, NULL, false);
	if (git_p)
		*git_p = git;
	if (branch_p)
		*branch_p = branch;
	if (prefs.cloud_git_url &&
	    strstr(filename, prefs.cloud_git_url)
	    && git == dummy_git_repository) {
		/* opening the cloud storage repository failed for some reason,
		 * so we don't know if there is additional data in the remote */
		free(current_sha);
		return 1;
	}
	/* if this is a git repository, do we already have this exact state loaded ?
	 * get the SHA and compare with what we currently have */
	if (git && git != dummy_git_repository) {
		const char *sha = get_sha(git, branch);
		if (!empty_string(sha) &&
		    same_string(sha, current_sha)) {
			fprintf(stderr, "already have loaded SHA %s - don't load again\n", sha);
			free(current_sha);
			return 0;
		}
	}
	free(current_sha);
	return 1;
}

int parse_file(const char *filename, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
	       struct device_table *devices, struct filter_preset_table *filter_presets)
{
	struct git_repository *git;
	const char *branch = NULL;
	struct memblock mem;
	char *fmt;
	int ret;

	git = is_git_repository(filename, &branch, NULL, false);
	if (prefs.cloud_git_url &&
	    strstr(filename, prefs.cloud_git_url)
	    && git == dummy_git_repository) {
		/* opening the cloud storage repository failed for some reason
		 * give up here and don't send errors about git repositories */
		return -1;
	}
	if (git)
		return git_load_dives(git, branch, table, trips, sites, devices, filter_presets);

	if ((ret = readfile(filename, &mem)) < 0) {
		/* we don't want to display an error if this was the default file  */
		if (same_string(filename, prefs.default_filename))
			return 0;

		return report_error(translate("gettextFromC", "Failed to read '%s'"), filename);
	} else if (ret == 0) {
		return report_error(translate("gettextFromC", "Empty file '%s'"), filename);
	}

	fmt = strrchr(filename, '.');
	if (fmt && (!strcasecmp(fmt + 1, "DB") || !strcasecmp(fmt + 1, "BAK") || !strcasecmp(fmt + 1, "SQL"))) {
		if (!try_to_open_db(filename, &mem, table, trips, sites, devices)) {
			free(mem.buffer);
			return 0;
		}
	}

	/* Divesoft Freedom */
	if (fmt && (!strcasecmp(fmt + 1, "DLF"))) {
		ret = parse_dlf_buffer(mem.buffer, mem.size, table, trips, sites, devices);
		free(mem.buffer);
		return ret;
	}

	/* DataTrak/Wlog */
	if (fmt && !strcasecmp(fmt + 1, "LOG")) {
		struct memblock wl_mem;
		const char *t = strrchr(filename, '.');
		char *wl_name = memcpy(calloc(t - filename + 1, 1), filename, t - filename);
		wl_name = realloc(wl_name, strlen(wl_name) + 5);
		wl_name = strcat(wl_name, ".add");
		if((ret = readfile(wl_name, &wl_mem)) < 0) {
			fprintf(stderr, "No file %s found. No WLog extensions.\n", wl_name);
			ret = datatrak_import(&mem, NULL, table, trips, sites, devices);
		} else {
			ret = datatrak_import(&mem, &wl_mem, table, trips, sites, devices);
			free(wl_mem.buffer);
		}
		free(mem.buffer);
		free(wl_name);
		return ret;
	}

	/* OSTCtools */
	if (fmt && (!strcasecmp(fmt + 1, "DIVE"))) {
		free(mem.buffer);
		ostctools_import(filename, table, trips, sites);
		return 0;
	}

	ret = parse_file_buffer(filename, &mem, table, trips, sites, devices, filter_presets);
	free(mem.buffer);
	return ret;
}
