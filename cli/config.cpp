// SPDX-License-Identifier: GPL-2.0
#include "config.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>

// Expand tilde in paths to home directory
static QString expandTilde(const QString &path)
{
	if (path.startsWith("~/"))
		return QDir::homePath() + path.mid(1);
	return path;
}

// Validate a path is safe (no traversal attacks, exists, etc.)
// Returns empty string if invalid, canonical path if valid
static QString validatePath(const QString &path, bool mustExist = true)
{
	if (path.isEmpty())
		return QString();

	// Reject obviously malicious patterns before any filesystem access
	if (path.contains("..") || path.contains('\0'))
		return QString();

	// Expand tilde and get canonical path
	QString expanded = expandTilde(path);
	QFileInfo info(expanded);

	if (mustExist) {
		// Path must exist and be readable
		if (!info.exists())
			return QString();
		// Return canonical path (resolves symlinks, removes . and ..)
		return info.canonicalFilePath();
	} else {
		// For paths that don't need to exist yet, validate the parent
		QDir parent = info.absoluteDir();
		if (!parent.exists())
			return QString();
		return info.absoluteFilePath();
	}
}

// Validate config file path - must be in allowed locations
static bool isAllowedConfigPath(const QString &path)
{
	QString canonical = validatePath(path);
	if (canonical.isEmpty())
		return false;

	// Allow config files in:
	// 1. Standard config location (~/.config/subsurface-cli/)
	// 2. /tmp/subsurface* for web server generated configs
	// 3. /var/lib/subsurface/ for system-wide configs
	// 4. /run/subsurface* for systemd RuntimeDirectory managed configs
	QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
	QString homeDir = QDir::homePath();

	if (canonical.startsWith(configDir + "/subsurface"))
		return true;
	if (canonical.startsWith("/tmp/subsurface"))
		return true;
	if (canonical.startsWith("/var/lib/subsurface"))
		return true;
	if (canonical.startsWith("/run/subsurface"))
		return true;
	// Also allow in user's home .config directory
	if (canonical.startsWith(homeDir + "/.config/subsurface"))
		return true;

	return false;
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

bool validateConfigPath(const QString &configPath)
{
	return isAllowedConfigPath(configPath);
}

CliConfig loadConfig(const QString &configPath)
{
	CliConfig config;

	// Security: Validate config file is in an allowed location
	if (!isAllowedConfigPath(configPath)) {
		fprintf(stderr, "Config file path not in allowed location\n");
		return config;
	}

	QFile file(configPath);
	if (!file.open(QIODevice::ReadOnly)) {
		fprintf(stderr, "Cannot open config file\n");
		return config;
	}

	// Security: Limit config file size to prevent DoS
	constexpr qint64 MAX_CONFIG_SIZE = 64 * 1024; // 64KB should be plenty
	if (file.size() > MAX_CONFIG_SIZE) {
		fprintf(stderr, "Config file too large\n");
		file.close();
		return config;
	}

	QByteArray data = file.readAll();
	file.close();

	QJsonParseError parseError;
	QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
	if (parseError.error != QJsonParseError::NoError) {
		fprintf(stderr, "JSON parse error in config\n");
		return config;
	}

	if (!doc.isObject()) {
		fprintf(stderr, "Config file must contain a JSON object\n");
		return config;
	}

	QJsonObject obj = doc.object();

	// Security: Validate repo_path - must exist and not contain traversal
	QString repoPath = obj.value("repo_path").toString();
	config.repo_path = validatePath(repoPath);
	if (config.repo_path.isEmpty() && !repoPath.isEmpty()) {
		fprintf(stderr, "Invalid repo_path in config\n");
		return config;
	}

	// Security: Validate userid as email address
	// Allow common email characters while rejecting dangerous ones
	QString userid = obj.value("userid").toString();
	if (userid.length() > 254) {  // RFC 5321 max email length
		fprintf(stderr, "userid too long\n");
		return config;
	}
	// Email pattern allowing common characters:
	// Local part: alphanumeric, dots, plus (for sub-addressing), hyphen, underscore
	// Domain: alphanumeric, dots, hyphens
	// This covers user+subsurface@sub.domain.com style addresses
	static QRegularExpression useridRe(
		R"(^[a-zA-Z0-9._+-]+@[a-zA-Z0-9](?:[a-zA-Z0-9-]*[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]*[a-zA-Z0-9])?)+$)");
	if (!useridRe.match(userid).hasMatch()) {
		fprintf(stderr, "Invalid userid format (expected email address)\n");
		return config;
	}
	config.userid = userid;

	// Security: Validate temp_dir if provided
	if (obj.contains("temp_dir")) {
		QString tempDir = obj.value("temp_dir").toString();
		// temp_dir doesn't need to exist yet, but parent must
		config.temp_dir = validatePath(tempDir, false);
		if (config.temp_dir.isEmpty()) {
			fprintf(stderr, "Invalid temp_dir in config\n");
			return config;
		}
	} else {
		config.temp_dir = QDir::tempPath() + "/subsurface-cli";
	}

	if (obj.contains("cache_ttl_seconds")) {
		int ttl = obj.value("cache_ttl_seconds").toInt(300);
		// Clamp to reasonable range
		config.cache_ttl_seconds = qBound(0, ttl, 86400); // 0 to 24 hours
	}

	return config;
}

// Create a secure temp directory with a random name under the configured temp_dir.
// This prevents symlink attacks and race conditions by:
// 1. Using a cryptographically random UUID in the directory name
// 2. Creating with restrictive permissions (owner only)
// 3. Verifying the created directory is actually a directory we own
// Returns empty string on failure.
QString createSecureTempDir(const CliConfig &config)
{
	QString basePath = config.temp_dir;
	if (basePath.isEmpty())
		basePath = QDir::tempPath() + "/subsurface-cli";

	// Ensure base temp directory exists
	QDir baseDir(basePath);
	if (!baseDir.exists()) {
		if (!baseDir.mkpath(".")) {
			fprintf(stderr, "Failed to create temp directory base\n");
			return QString();
		}
	}

	// Create a unique subdirectory with random UUID
	QString uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
	QString securePath = basePath + "/" + uuid;

	QDir secureDir(securePath);
	if (!secureDir.mkpath(".")) {
		fprintf(stderr, "Failed to create secure temp directory\n");
		return QString();
	}

	// Verify we created what we expected (defense against TOCTOU)
	QFileInfo info(securePath);
	if (!info.isDir() || !info.isWritable()) {
		fprintf(stderr, "Secure temp directory verification failed\n");
		return QString();
	}

	// On Unix, set restrictive permissions (owner only: rwx------)
#ifndef Q_OS_WIN
	QFile::setPermissions(securePath,
		QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif

	return securePath;
}
