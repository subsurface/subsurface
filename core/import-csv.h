// SPDX-License-Identifier: GPL-2.0
#ifndef IMPORTCSV_H
#define IMPORTCSV_H

enum csv_format {
	CSV_DEPTH,
	CSV_TEMP,
	CSV_PRESSURE,
	POSEIDON_DEPTH,
	POSEIDON_TEMP,
	POSEIDON_SETPOINT,
	POSEIDON_SENSOR1,
	POSEIDON_SENSOR2,
	POSEIDON_NDL,
	POSEIDON_CEILING
};

#define MAXCOLDIGITS 10

#ifdef __cplusplus
extern "C" {
#endif

int parse_csv_file(const char *filename, char **params, int pnr, const char *csvtemplate, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int try_to_open_csv(struct memblock *mem, enum csv_format type, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int parse_txt_file(const char *filename, const char *csv, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);

int parse_seabear_log(const char *filename, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);
int parse_manual_file(const char *filename, char **params, int pnr, struct dive_table *table, struct trip_table *trips, struct dive_site_table *sites);

#ifdef __cplusplus
}
#endif

#endif // IMPORTCSV_H
