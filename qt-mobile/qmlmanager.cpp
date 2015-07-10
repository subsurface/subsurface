#include "qmlmanager.h"
#include <QUrl>
#include <QSettings>

#include "qt-models/divelistmodel.h"
#include "divelist.h"
#include "pref.h"
#include "qthelper.h"

QMLManager::QMLManager()
{
	//Initialize cloud credentials.
	setCloudUserName(prefs.cloud_storage_email);
	setCloudPassword(prefs.cloud_storage_password);
}

QMLManager::~QMLManager()
{
}

void QMLManager::savePreferences()
{
	QSettings s;
	s.beginGroup("CloudStorage");
	s.setValue("email", cloudUserName());
	s.setValue("password", cloudPassword());

	s.sync();
}

void QMLManager::loadDives()
{
	QString url;
	if (getCloudURL(url)) {
		//TODO: Show error in QML
		return;
	}

	QByteArray fileNamePrt  = QFile::encodeName(url);
	int error = parse_file(fileNamePrt.data());
	if (!error) {
		set_filename(fileNamePrt.data(), true);
	}

	process_dives(false, false);
	int i;
	struct dive *d;

	for_each_dive(i, d)
		DiveListModel::instance()->addDive(d);
}

QString QMLManager::cloudPassword() const
{
	return m_cloudPassword;
}

void QMLManager::setCloudPassword(const QString &cloudPassword)
{
	m_cloudPassword = cloudPassword;
	emit cloudPasswordChanged();
}

QString QMLManager::cloudUserName() const
{
	return m_cloudUserName;
}

void QMLManager::setCloudUserName(const QString &cloudUserName)
{
	m_cloudUserName = cloudUserName;
	emit cloudUserNameChanged();
}
