// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_QOBJECT_H
#define DIVE_QOBJECT_H

#include "CylinderObjectHelper.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>

class DiveObjectHelper {
	Q_GADGET
	Q_PROPERTY(int number MEMBER number CONSTANT)
	Q_PROPERTY(int id MEMBER id CONSTANT)
	Q_PROPERTY(int rating MEMBER rating CONSTANT)
	Q_PROPERTY(int visibility MEMBER visibility CONSTANT)
	Q_PROPERTY(QString date READ date CONSTANT)
	Q_PROPERTY(QString time READ time CONSTANT)
	Q_PROPERTY(int timestamp MEMBER timestamp CONSTANT)
	Q_PROPERTY(QString location MEMBER location CONSTANT)
	Q_PROPERTY(QString gps MEMBER gps CONSTANT)
	Q_PROPERTY(QString gps_decimal MEMBER gps_decimal CONSTANT)
	Q_PROPERTY(QVariant dive_site MEMBER dive_site CONSTANT)
	Q_PROPERTY(QString duration MEMBER duration CONSTANT)
	Q_PROPERTY(bool noDive MEMBER noDive CONSTANT)
	Q_PROPERTY(QString depth MEMBER depth CONSTANT)
	Q_PROPERTY(QString divemaster MEMBER divemaster CONSTANT)
	Q_PROPERTY(QString buddy MEMBER buddy CONSTANT)
	Q_PROPERTY(QString airTemp MEMBER airTemp CONSTANT)
	Q_PROPERTY(QString waterTemp MEMBER waterTemp CONSTANT)
	Q_PROPERTY(QString notes MEMBER notes CONSTANT)
	Q_PROPERTY(QString tags MEMBER tags CONSTANT)
	Q_PROPERTY(QString gas MEMBER gas CONSTANT)
	Q_PROPERTY(QString sac MEMBER sac CONSTANT)
	Q_PROPERTY(QString weightList MEMBER weightList CONSTANT)
	Q_PROPERTY(QStringList weights MEMBER weights CONSTANT)
	Q_PROPERTY(bool singleWeight MEMBER singleWeight CONSTANT)
	Q_PROPERTY(QString suit MEMBER suit CONSTANT)
	Q_PROPERTY(QStringList cylinderList READ cylinderList CONSTANT)
	Q_PROPERTY(QStringList cylinders MEMBER cylinders CONSTANT)
	Q_PROPERTY(int maxcns MEMBER maxcns CONSTANT)
	Q_PROPERTY(int otu MEMBER otu CONSTANT)
	Q_PROPERTY(QString sumWeight MEMBER sumWeight CONSTANT)
	Q_PROPERTY(QStringList getCylinder MEMBER getCylinder CONSTANT)
	Q_PROPERTY(QStringList startPressure MEMBER startPressure CONSTANT)
	Q_PROPERTY(QStringList endPressure MEMBER endPressure CONSTANT)
	Q_PROPERTY(QStringList firstGas MEMBER firstGas CONSTANT)
public:
	DiveObjectHelper(); // This is only to be used by Qt's metatype system!
	DiveObjectHelper(const struct dive *dive);
	int number;
	int id;
	int rating;
	int visibility;
	QString date() const;
	timestamp_t timestamp;
	QString time() const;
	QString location;
	QString gps;
	QString gps_decimal;
	QVariant dive_site;
	QString duration;
	bool noDive;
	QString depth;
	QString divemaster;
	QString buddy;
	QString airTemp;
	QString waterTemp;
	QString notes;
	QString tags;
	QString gas;
	QString sac;
	QString weightList;
	QStringList weights;
	bool singleWeight;
	QString suit;
	QStringList cylinderList() const;
	QStringList cylinders;
	int maxcns;
	int otu;
	QString sumWeight;
	QStringList getCylinder;
	QStringList startPressure;
	QStringList endPressure;
	QStringList firstGas;
};

// This is an extended version of DiveObjectHelper that also keeps track of cylinder data.
// It is used by grantlee to display structured cylinder data.
// Note: this grantlee feature is undocumented. If there turns out to be no users, we might
// want to remove this class.
class DiveObjectHelperGrantlee : public DiveObjectHelper {
	Q_GADGET
	Q_PROPERTY(QVector<CylinderObjectHelper> cylinderObjects MEMBER cylinderObjects CONSTANT)
public:
	DiveObjectHelperGrantlee();
	DiveObjectHelperGrantlee(const struct dive *dive);
	QVector<CylinderObjectHelper> cylinderObjects;
};

Q_DECLARE_METATYPE(DiveObjectHelper)
Q_DECLARE_METATYPE(DiveObjectHelperGrantlee)

#endif
