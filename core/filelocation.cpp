// SPDX-License-Identifier: GPL-2.0
#include "core/filelocation.h"
#include "core/pref.h"
#include <QObject>
#include <QFileInfo>
#include <QDir>
#include <utility>

FileLocation currentFile;

FileLocation::FileLocation() : type(NONE)
{
}

bool isRemoteRepository(const QString &name)
{
	return name.startsWith("http://") || name.startsWith("https://") ||
	       name.startsWith("ssh://") || name.startsWith("git://");
}

// For git repositories, remove "file://" and extract username, if any.
// For local files or git repositories, do proper unicode encoding.
void FileLocation::fixName()
{
	switch (type) {
	default:
	case NONE:
		return;
	case LOCAL_FILE:
		name = QFile::encodeName(QDir::toNativeSeparators(name));
		return;
	case CLOUD_GIT:
	case CLOUD_GIT_OFFLINE:
	case GIT:
		if (name.startsWith("file://", Qt::CaseInsensitive)) {
			// Treat "file://..." URLs as local repositories.
			name = name.mid(7);
			name = QFile::encodeName(QDir::toNativeSeparators(name));
		} else if(!isRemoteRepository(name)) {
			name = QFile::encodeName(QDir::toNativeSeparators(name));
		} else if(name.startsWith("https://", Qt::CaseInsensitive)) {
			int at = name.indexOf('@');
			int slash = name.indexOf('/', 8);
			if (at >= 0 && slash >= 0 && slash > at) {
				user = name.mid(8, at - 8);
				name.remove(8, at - 7);
			}
		}
	}
}

// Check if filename is of the form "repository[branch]" or "repository[branch].ssrf".
static bool isGitFilename(const QString &name)
{
	QString trimmed = name.trimmed();
	if (trimmed.endsWith(".ssrf", Qt::CaseInsensitive))
		trimmed.truncate(trimmed.count() - 5);
	return trimmed.right(1) == "]" &&
	       trimmed.lastIndexOf('[') > 0;
}

// Returns (repository, branch) pair.
static std::pair<QString, QString> parseGitFilename(const QString &fn)
{
	if (!isGitFilename(fn))							// Oops. Filename doesn't follow the rules. Leave branch empty.
		return std::make_pair(fn, QString());

	int branch_from = fn.lastIndexOf('[');
	int branch_to = fn.lastIndexOf(']');
	QString repository = fn.left(branch_from);				// Guaranteed OK owing to isGitFilename() == true
	QString branch = fn.mid(branch_from + 1, branch_to - branch_from - 1);	// ditto
	return std::make_pair(repository, branch);
}

FileLocation::FileLocation(Type type_, const QString &name_, const QString &branch_) : type(type_), name(name_), branch(branch_)
{
	if (type == NONE)
		return;
	fixName();
}

FileLocation::FileLocation(Type type_, const QString &name_) : type(type_)
{
	if (type == NONE)
		return;
	if (type == LOCAL_FILE) {
		name = name_;
	} else {
		// Is a git repository
		std::pair<QString, QString> repo_branch = parseGitFilename(name_);
		name = repo_branch.first;
		branch = repo_branch.second;
	}
	fixName();
}

FileLocation::FileLocation(const QString &type_, const QString &name_, const QString &branch_) : name(name_), branch(branch_)
{
	if (type_ == "none")
		type = NONE;
	else if (type_ == "local")
		type = LOCAL_FILE;
	else if (type_ == "cloud")
		type = CLOUD_GIT;
	else if (type_ == "cloud_offline")
		type = CLOUD_GIT_OFFLINE;
	else if (type_ == "git")
		type = GIT;
	else
		*this = guessFromFileName(name_); // Unknown or empty type -> try to guess from filename
	fixName();
}

FileLocation FileLocation::guessFromFileName(const QString &name)
{
	if (name.simplified().isEmpty()) {
		return FileLocation();
	} else if (isGitFilename(name)) {
		std::pair<QString, QString> repo_branch = parseGitFilename(name);
		// TODO: Check for git repository if local!!
		return FileLocation(GIT, repo_branch.first, repo_branch.second);
	}
	return FileLocation(LOCAL_FILE, name, QString());
}

QString FileLocation::getName() const
{
	return name;
}

QString FileLocation::getBranch() const
{
	return branch;
}

QString FileLocation::getUser() const
{
	return user;
}

FileLocation::Type FileLocation::getType() const
{
	return type;
}

bool FileLocation::isNone() const
{
	return type == NONE;
}

bool FileLocation::isRemote() const
{
	if (type == CLOUD_GIT)
		return true;
	if (type != GIT)
		return false;
	return isRemoteRepository(name);
}

bool FileLocation::isCloud() const
{
	return type == CLOUD_GIT || type == CLOUD_GIT_OFFLINE;
}

QString FileLocation::typeAsString() const
{
	switch(type) {
	default:
	case NONE:		return QString("none");
	case LOCAL_FILE:	return QString("local");
	case CLOUD_GIT:		return QString("cloud");
	case CLOUD_GIT_OFFLINE:	return QString("cloud_offline");
	case GIT:		return QString("git");
	}
}

QString FileLocation::formatShort() const
{
	switch(type) {
	default:
	case NONE:		return QFileInfo(prefs.default_filename).fileName();
	case LOCAL_FILE:	return QFileInfo(name).fileName();
	case CLOUD_GIT:		return QObject::tr("[cloud storage for] %1").arg(branch);
	case CLOUD_GIT_OFFLINE:	return QObject::tr("[local cache for] %1").arg(branch);
	case GIT:		return QObject::tr("[%1] in repository %2").arg(branch, repoName());
	}
}

QString FileLocation::formatLong() const
{
	switch(type) {
	default:
	case NONE:		return prefs.default_filename;
	case LOCAL_FILE:	return name;
	case CLOUD_GIT:		return QObject::tr("[cloud storage for] %1").arg(branch);
	case CLOUD_GIT_OFFLINE:	return QObject::tr("[local cache for] %1").arg(branch);
	case GIT:		return QObject::tr("[%1] in repository %2").arg(branch, name);
	}
}

QString FileLocation::path() const
{
	QFileInfo fi(type == NONE ?  prefs.default_filename : name);
	return fi.absolutePath();
}

QString FileLocation::repoName() const
{
	if (isRemoteRepository(name))	// TODO: extract last element of URL?
		return name;
	else
		return QDir(name).dirName();
}

bool FileLocation::operator==(const FileLocation &fl) const
{
	if (type != fl.type)
		return false;
	switch(type) {
	default:
	case NONE:
		return true;
	case LOCAL_FILE: {
		QFileInfo f1(name);
		QFileInfo f2(fl.name);
		return f1 == f2;
	}
	case CLOUD_GIT:
	case CLOUD_GIT_OFFLINE:
	case GIT:
		// TODO: Text comparison is too crude. For local repositories use QFileInfo,
		// for remote repositories QUrl?
		return name == fl.name && branch == fl.branch;
	}
}
