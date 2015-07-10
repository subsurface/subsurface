#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QString>

class QMLManager : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString filename READ filename WRITE setFilename NOTIFY filenameChanged)
	Q_PROPERTY(QString cloudUserName READ cloudUserName WRITE setCloudUserName NOTIFY cloudUserNameChanged)
	Q_PROPERTY(QString cloudPassword READ cloudPassword WRITE setCloudPassword NOTIFY cloudPasswordChanged)
public:
	QMLManager();
	~QMLManager();

	QString filename();

	void getFile();


	QString cloudUserName() const;
	void setCloudUserName(const QString &cloudUserName);

	QString cloudPassword() const;
	void setCloudPassword(const QString &cloudPassword);

public slots:
	void setFilename(const QString &f);
	void savePreferences();

private:
	QString m_fileName;
	QString m_cloudUserName;
	QString m_cloudPassword;
	void loadFile();

signals:
	void filenameChanged();
	void cloudUserNameChanged();
	void cloudPasswordChanged();
};

#endif
