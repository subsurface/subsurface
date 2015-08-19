#ifndef QMLMANAGER_H
#define QMLMANAGER_H

#include <QObject>
#include <QString>

class QMLManager : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString cloudUserName READ cloudUserName WRITE setCloudUserName NOTIFY cloudUserNameChanged)
	Q_PROPERTY(QString cloudPassword READ cloudPassword WRITE setCloudPassword NOTIFY cloudPasswordChanged)
	Q_PROPERTY(bool saveCloudPassword READ saveCloudPassword WRITE setSaveCloudPassword NOTIFY saveCloudPasswordChanged)
	Q_PROPERTY(QString logText READ logText WRITE setLogText NOTIFY logTextChanged)
public:
	QMLManager();
	~QMLManager();

	QString cloudUserName() const;
	void setCloudUserName(const QString &cloudUserName);

	QString cloudPassword() const;
	void setCloudPassword(const QString &cloudPassword);

	bool saveCloudPassword() const;
	void setSaveCloudPassword(bool saveCloudPassword);

	QString logText() const;
	void setLogText(const QString &logText);
	void appendTextToLog(const QString &newText);

public slots:
	void savePreferences();
	void loadDives();
	void commitChanges(QString diveId, QString suit, QString buddy, QString diveMaster, QString notes);
	void saveChanges();
	void addDive();
private:
	QString m_cloudUserName;
	QString m_cloudPassword;
	bool m_saveCloudPassword;
	QString m_logText;

signals:
	void cloudUserNameChanged();
	void cloudPasswordChanged();
	void saveCloudPasswordChanged();
	void logTextChanged();
};

#endif
