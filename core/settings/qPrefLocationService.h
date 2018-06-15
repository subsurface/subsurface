// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLOCATIONSERVICE_H
#define QPREFLOCATIONSERVICE_H

#include <QObject>


class qPrefLocationService : public QObject {
	Q_OBJECT
	Q_PROPERTY(int distance_threshold READ distanceThreshold WRITE setDistanceThreshold NOTIFY distanceThresholdChanged);
	Q_PROPERTY(int time_threshold READ timeThreshold WRITE setTimeThreshold NOTIFY timeThresholdChanged);

public:
	qPrefLocationService(QObject *parent = NULL) : QObject(parent) {};
	~qPrefLocationService() {};
	static qPrefLocationService *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	int distanceThreshold() const;
	int timeThreshold() const;

public slots:
	void setDistanceThreshold(int value);
	void setTimeThreshold(int value);

signals:
	void distanceThresholdChanged(int value);
	void timeThresholdChanged(int value);


private:
	const QString group = QStringLiteral("LocationService");
	static qPrefLocationService *m_instance;

	void diskDistanceThreshold(bool doSync);
	void diskTimeThreshold(bool doSync);
};

#endif
