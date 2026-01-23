// SPDX-License-Identifier: GPL-2.0
#include "config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

// Expand tilde in paths to home directory
static QString expandTilde(const QString &path)
{
	if (path.startsWith("~/"))
		return QDir::homePath() + path.mid(1);
	return path;
}

bool CliConfig::isValid() const
{
	return !repo_path.isEmpty() && !userid.isEmpty();
}

QString defaultConfigPath()
{
	QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
	return configDir + "/subsurface-cli/config.json";
}

CliConfig loadConfig(const QString &configPath)
{
	CliConfig config;

	QFile file(configPath);
	if (!file.open(QIODevice::ReadOnly)) {
		fprintf(stderr, "Cannot open config file: %s\n", qPrintable(configPath));
		return config;
	}

	QByteArray data = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError) {
		fprintf(stderr, "JSON parse error in config: %s\n", qPrintable(parseError.errorString()));
		return config;
	}

	if (!doc.isObject()) {
		fprintf(stderr, "Config file must contain a JSON object\n");
		return config;
	}

	QJsonObject obj = doc.object();

	config.repo_path = expandTilde(obj.value("repo_path").toString());
	config.userid = obj.value("userid").toString();

	if (obj.contains("temp_dir"))
		config.temp_dir = obj.value("temp_dir").toString();
	else
		config.temp_dir = QDir::tempPath() + "/subsurface-cli";

	if (obj.contains("cache_ttl_seconds"))
		config.cache_ttl_seconds = obj.value("cache_ttl_seconds").toInt(300);

	return config;
}
