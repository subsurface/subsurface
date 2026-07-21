// SPDX-License-Identifier: GPL-2.0
#ifndef CLI_COMMANDS_H
#define CLI_COMMANDS_H

#include "config.h"
#include <QString>

// Command return codes
constexpr int CMD_SUCCESS = 0;
constexpr int CMD_ERROR_INVALID_ARGS = 1;
constexpr int CMD_ERROR_NOT_FOUND = 2;
constexpr int CMD_ERROR_IO = 3;
constexpr int CMD_ERROR_INTERNAL = 4;

// List dives with optional pagination
// Output: JSON array of dive summaries
int cmdListDives(const CliConfig &config, int start, int count);

// Get full details for a single dive
// Output: JSON object with dive details
int cmdGetDive(const CliConfig &config, const QString &diveRef);

// Get trip information
// Output: JSON object with trip details and list of dives
int cmdGetTrip(const CliConfig &config, const QString &tripRef);

// Generate dive profile image
// Output: JSON with path to generated PNG file
int cmdGetProfile(const CliConfig &config, const QString &diveRef, int dcIndex, int width, int height);

// Get overall statistics
// Output: JSON object with statistics
int cmdGetStats(const CliConfig &config);

#endif // CLI_COMMANDS_H
