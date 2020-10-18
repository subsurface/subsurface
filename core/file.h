// SPDX-License-Identifier: GPL-2.0
#ifndef FILE_H
#define FILE_H

#include "filterpreset.h"

#include <sys/stat.h>
#include <stdio.h>

struct memblock {
	void *buffer;
	size_t size;
};

struct trip_table;
struct device_table;
struct dive_site_table;
struct dive_table;
struct zip;

#ifdef __cplusplus
extern "C" {
#endif
extern int try_to_open_cochran(const char *filename, struct memblock *mem, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
extern int try_to_open_liquivision(const char *filename, struct memblock *mem, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
extern int datatrak_import(struct memblock *mem, struct memblock *wl_mem, struct dive_table *table, struct trip_table *trips,
			   struct dive_site_table *sites, struct device_table *devices);
extern void ostctools_import(const char *file, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);

extern int readfile(const char *filename, struct memblock *mem);
extern int parse_file(const char *filename, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
		      struct device_table *devices, struct filter_preset_table *filter_presets);
extern int try_to_open_zip(const char *filename, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites,
			   struct device_table *devices, struct filter_preset_table *filter_presets);

// Platform specific functions
extern int subsurface_rename(const char *path, const char *newpath);
extern int subsurface_dir_rename(const char *path, const char *newpath);
extern int subsurface_open(const char *path, int oflags, mode_t mode);
extern FILE *subsurface_fopen(const char *path, const char *mode);
extern void *subsurface_opendir(const char *path);
extern int subsurface_access(const char *path, int mode);
extern int subsurface_stat(const char* path, struct stat* buf);
extern struct zip *subsurface_zip_open_readonly(const char *path, int flags, int *errorp);
extern int subsurface_zip_close(struct zip *zip);

#ifdef __cplusplus
}
#endif

#endif // FILE_H
