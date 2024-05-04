// SPDX-License-Identifier: GPL-2.0
#ifndef IMPORTCSV_H
#define IMPORTCSV_H

#include "filterpreset.h"

struct xml_params;

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

int parse_csv_file(const char *filename, struct xml_params *params, const char *csvtemplate, struct divelog *log);
int try_to_open_csv(std::string &mem, enum csv_format type, struct divelog *log);
int parse_txt_file(const char *filename, const char *csv, struct divelog *log);

int parse_seabear_log(const char *filename, struct divelog *log);
int parse_manual_file(const char *filename, struct xml_params *params, struct divelog *log);

#endif // IMPORTCSV_H
