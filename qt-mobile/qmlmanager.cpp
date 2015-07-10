#include "qmlmanager.h"
#include <QUrl>
#include <QSettings>

#include "qt-models/divelistmodel.h"
#include "divelist.h"
#include "pref.h"

QMLManager::QMLManager()
{
	//Initialize cloud credentials.
	setCloudUserName(prefs.cloud_storage_email);
	setCloudPassword(prefs.cloud_storage_password);
}


QMLManager::~QMLManager()
{
}

QString QMLManager::filename()
{
	return m_fileName;
}

void QMLManager::setFilename(const QString &f)
{
	m_fileName = f;
	loadFile();
}

void QMLManager::savePreferences()
{
	QSettings s;
	s.beginGroup("CloudStorage");
	s.setValue("email", cloudUserName());
	s.setValue("password", cloudPassword());

	s.sync();
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


void QMLManager::loadFile()
{
	QUrl url(m_fileName);
	QString strippedFileName = url.toLocalFile();

	parse_file(strippedFileName.toUtf8().data());
	process_dives(false, false);
	int i;
	struct dive *d;

	for_each_dive(i, d) {
		DiveListModel::instance()->addDive(d);
	}
}
