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

void add_sample_data(struct sample *sample, enum csv_format type, double val);
int parse_csv_file(const char *filename, char **params, int pnr, const char *csvtemplate);
int try_to_open_csv(struct memblock *mem, enum csv_format type);
int parse_txt_file(const char *filename, const char *csv);

int parse_seabear_log(const char *filename);
int parse_manual_file(const char *filename, char **params, int pnr);

#endif // IMPORTCSV_H
