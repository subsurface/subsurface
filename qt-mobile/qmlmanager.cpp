#include "qmlmanager.h"
#include <QUrl>

#include "qt-models/divelistmodel.h"
#include "divelist.h"

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
