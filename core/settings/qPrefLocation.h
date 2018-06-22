// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLOCATION_H
#define QPREFLOCATION_H

#include <QObject>


class qPrefLocation : public QObject {
	Q_OBJECT
	Q_PROPERTY(int		distance_threshold
				READ	distanceThreshold
				WRITE	setDistanceThreshold
				NOTIFY  distanceThresholdChanged);
	Q_PROPERTY(int		time_threshold
				READ	timeThreshold
				WRITE	setTimeThreshold
				NOTIFY  timeThresholdChanged);

public:
	qPrefLocation(QObject *parent = NULL) : QObject(parent) {};
	~qPrefLocation() {};
	static qPrefLocation *instance();

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
	static qPrefLocation *m_instance;

	void diskDistanceThreshold(bool doSync);
	void diskTimeThreshold(bool doSync);
};

#endif
