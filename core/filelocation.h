// SPDX-License-Identifier: GPL-2.0
#ifndef FILELOCATION_H
#define FILELOCATION_H

#include <QString>

struct git_state;

class FileLocation {
public:
	enum Type {
		NONE,
		LOCAL_FILE,
		CLOUD_GIT,
		CLOUD_GIT_OFFLINE,
		GIT,
	};
private:
	Type type;
	QString name;					// Name of file or git repository
	QString branch;					// For git repos
	QString user;					// For https:// git repos

	void fixName();
public:
	FileLocation();					// Default to none
	FileLocation(const QString &type, const QString &name, const QString &branch);
	FileLocation(Type type, const QString &name, const QString &branch);
	FileLocation(Type type, const QString &name);	// For git repos parse as "repo[branch]"

	static FileLocation none();
	static FileLocation guessFromFileName(const QString &name_);

	QString getName() const;
	QString getBranch() const;
	QString getUser() const;
	Type getType() const;
	bool isNone() const;
	bool isRemote() const;
	bool isCloud() const;
	QString formatShort() const;
	QString formatLong() const;
	QString typeAsString() const;
	QString path() const;
	QString repoName() const;
	git_state gitState() const;

	bool operator==(const FileLocation &) const;	// So that we can find location in list
};

extern FileLocation currentFile;

#endif
