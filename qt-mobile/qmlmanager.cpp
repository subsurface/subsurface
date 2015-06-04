#include "qmlmanager.h"


QMLManager::QMLManager()
{

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
	emit filenameChanged();
	loadFile();
}

void QMLManager::loadFile()
{
	QUrl url(m_fileName);
	QString strippedFileName = url.toLocalFile();
}
