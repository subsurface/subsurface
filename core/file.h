// SPDX-License-Identifier: GPL-2.0
#ifndef FILE_H
#define FILE_H

struct memblock {
	void *buffer;
	size_t size;
};

#ifdef __cplusplus
extern "C" {
#endif
extern int try_to_open_cochran(const char *filename, struct memblock *mem, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
extern int try_to_open_liquivision(const char *filename, struct memblock *mem, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
extern int datatrak_import(struct memblock *mem, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
extern void ostctools_import(const char *file, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);

extern int readfile(const char *filename, struct memblock *mem);
extern int try_to_open_zip(const char *filename, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
#ifdef __cplusplus
}
#endif

#endif // FILE_H
