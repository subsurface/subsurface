// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFUPDATEMANAGER_H
#define QPREFUPDATEMANAGER_H

#include <QObject>
#include <QDate>


class qPrefUpdateManager : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool dont_check_for_updates READ dontCheckForUpdates WRITE setDontCheckForUpdates NOTIFY dontCheckForUpdatesChanged);
	Q_PROPERTY(bool dont_check_exists READ dontCheckExists WRITE setDontCheckExists NOTIFY dontCheckExistsChanged);
	Q_PROPERTY(const QString last_version_used READ lastVersionUsed WRITE setLastVersionUsed NOTIFY lastVersionUsedChanged);
	Q_PROPERTY(const QDate next_check READ nextCheck WRITE setNextCheck NOTIFY nextCheckChanged);


public:
	qPrefUpdateManager(QObject *parent = NULL) : QObject(parent) {};
	~qPrefUpdateManager() {};
	static qPrefUpdateManager *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	bool dontCheckForUpdates() const;
	bool dontCheckExists() const;
	const QString lastVersionUsed() const;
	const QDate nextCheck() const;

public slots:
	void setDontCheckForUpdates(bool value);
	void setDontCheckExists(bool value);
	void setLastVersionUsed(const QString& value);
	void setNextCheck(const QDate& value);

signals:
	void dontCheckForUpdatesChanged(bool value);
	void dontCheckExistsChanged(bool value);
	void lastVersionUsedChanged(const QString& value);
	void nextCheckChanged(const QDate& value);

private:
	const QString group = QStringLiteral("UpdateManager");
	static qPrefUpdateManager *m_instance;

	void diskDontCheckForUpdates(bool doSync);
	void diskLastVersionUsed(bool doSync);
	void diskNextCheck(bool doSync);
};

#endif
