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

#endif // CLI_CONFIG_H
