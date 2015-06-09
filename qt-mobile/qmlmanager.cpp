#include "qmlmanager.h"
#include <QUrl>

#include "../qt-models/divelistmodel.h"

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
	loadFile();
	emit filenameChanged();
}

void QMLManager::loadFile()
{
	QUrl url(m_fileName);
	QString strippedFileName = url.toLocalFile();

	parse_file(strippedFileName.toUtf8().data());

	int i;
	struct dive *d;

	for_each_dive(i, d) {
		DiveListModel::instance()->addDive(d);
	}
}
