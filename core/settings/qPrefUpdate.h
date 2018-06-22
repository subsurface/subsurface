// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFUPDATE_H
#define QPREFUPDATE_H

#include <QObject>
#include <QDate>


class qPrefUpdate : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool     dont_check_exists
				READ	dontCheckExists
				WRITE	setDontCheckExists
				NOTIFY  dontCheckForExistsChanged);
	Q_PROPERTY(bool	 dont_check_for_updates
				READ	dontCheckForUpdates
				WRITE	setDontCheckForUpdates
				NOTIFY  dontCheckForUpdatesChanged);
	Q_PROPERTY(QString	last_version_used
				READ	lastVersionUsed
				WRITE	setLastVersionUsed
				NOTIFY  lastVersionUsedChanged);
	Q_PROPERTY(QDate	next_check
				READ	nextCheck
				WRITE	setNextCheck
				NOTIFY	nextCheckChanged);

public:
	qPrefUpdate(QObject *parent = NULL) : QObject(parent) {};
	~qPrefUpdate() {};
	static qPrefUpdate *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	bool dontCheckExists() const;
	bool dontCheckForUpdates() const;
	const QString lastVersionUsed() const;
	const QDate nextCheck() const;

public slots:
	void setDontCheckExists(bool value);
	void setDontCheckForUpdates(bool value);
	void setLastVersionUsed(const QString& value);
	void setNextCheck(const QDate& date);

signals:
	void dontCheckForExistsChanged(bool value);
	void dontCheckForUpdatesChanged(bool value);
	void lastVersionUsedChanged(const QString& value);
	void nextCheckChanged(const QDate& date);

private:
	const QString group = QStringLiteral("UpdateManager");
	static qPrefUpdate *m_instance;

	void diskDontCheckForUpdates(bool doSync);
	void diskLastVersionUsed(bool doSync);
	void diskNextCheck(bool doSync);
};

#endif
