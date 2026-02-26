// SPDX-License-Identifier: GPL-2.0
#ifndef CLI_CONFIG_H
#define CLI_CONFIG_H

#include <QString>

struct CliConfig {
	QString repo_path;
	QString temp_dir;
	int cache_ttl_seconds = 300;
	QString userid;

	bool isValid() const;
};

// Load config from JSON file, returns empty config on error
CliConfig loadConfig(const QString &configPath);

// Get default config path (~/.config/subsurface-cli/config.json)
QString defaultConfigPath();

// Validate that a config path is in an allowed location
// (security check before loading config)
bool validateConfigPath(const QString &configPath);

// Create a secure temp directory with a random UUID subdirectory.
// Returns the path to the created directory, or empty string on failure.
// The directory is created with restrictive permissions (owner-only on Unix).
QString createSecureTempDir(const CliConfig &config);

#endif // CLI_CONFIG_H
