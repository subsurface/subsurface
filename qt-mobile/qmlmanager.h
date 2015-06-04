#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QString>

class QMLManager : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
public:
	QMLManager();
	~QMLManager();

	QString filename();

	void getFile();


public slots:
	void setFilename(const QString &f);

private:
	QString m_fileName;
	void loadFile();

signals:
	void filenameChanged();
};

#endif
