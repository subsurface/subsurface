// SPDX-License-Identifier: GPL-2.0
#ifndef DIVE_QOBJECT_H
#define DIVE_QOBJECT_H

#include "../dive.h"
#include "CylinderObjectHelper.h"
#include <QObject>
#include <QString>
#include <QStringList>

class DiveObjectHelper : public QObject {
	Q_OBJECT
	Q_PROPERTY(int number READ number CONSTANT)
	Q_PROPERTY(int id READ id CONSTANT)
	Q_PROPERTY(int rating READ rating CONSTANT)
	Q_PROPERTY(int visibility READ visibility CONSTANT)
	Q_PROPERTY(QString date READ date CONSTANT)
	Q_PROPERTY(QString time READ time CONSTANT)
	Q_PROPERTY(int timestamp READ timestamp CONSTANT)
	Q_PROPERTY(QString location READ location CONSTANT)
	Q_PROPERTY(QString gps READ gps CONSTANT)
	Q_PROPERTY(QString gps_decimal READ gps_decimal CONSTANT)
	Q_PROPERTY(QString duration READ duration CONSTANT)
	Q_PROPERTY(bool noDive READ noDive CONSTANT)
	Q_PROPERTY(QString depth READ depth CONSTANT)
	Q_PROPERTY(QString divemaster READ divemaster CONSTANT)
	Q_PROPERTY(QString buddy READ buddy CONSTANT)
	Q_PROPERTY(QString airTemp READ airTemp CONSTANT)
	Q_PROPERTY(QString waterTemp READ waterTemp CONSTANT)
	Q_PROPERTY(QString notes READ notes CONSTANT)
	Q_PROPERTY(QString tags READ tags CONSTANT)
	Q_PROPERTY(QString gas READ gas CONSTANT)
	Q_PROPERTY(QString sac READ sac CONSTANT)
	Q_PROPERTY(QString weightList READ weightList CONSTANT)
	Q_PROPERTY(QStringList weights READ weights CONSTANT)
	Q_PROPERTY(bool singleWeight READ singleWeight CONSTANT)
	Q_PROPERTY(QString suit READ suit CONSTANT)
	Q_PROPERTY(QStringList cylinderList READ cylinderList CONSTANT)
	Q_PROPERTY(QStringList cylinders READ cylinders CONSTANT)
	Q_PROPERTY(QList<CylinderObjectHelper*> cylinderObjects READ cylinderObjects CONSTANT)
	Q_PROPERTY(QString trip READ trip CONSTANT)
	Q_PROPERTY(QString tripMeta READ tripMeta CONSTANT)
	Q_PROPERTY(int maxcns READ maxcns CONSTANT)
	Q_PROPERTY(int otu READ otu CONSTANT)
	Q_PROPERTY(QString sumWeight READ sumWeight CONSTANT)
	Q_PROPERTY(QString getCylinder READ getCylinder CONSTANT)
	Q_PROPERTY(QString startPressure READ startPressure CONSTANT)
	Q_PROPERTY(QString endPressure READ endPressure CONSTANT)
	Q_PROPERTY(QString firstGas READ firstGas CONSTANT)
	Q_PROPERTY(QStringList suitList READ suitList CONSTANT)
	Q_PROPERTY(QStringList buddyList READ buddyList CONSTANT)
	Q_PROPERTY(QStringList divemasterList READ divemasterList CONSTANT)
public:
	DiveObjectHelper(struct dive *dive = NULL);
	~DiveObjectHelper();
	int number() const;
	int id() const;
	int rating() const;
	int visibility() const;
	QString date() const;
	timestamp_t timestamp() const;
	QString time() const;
	QString location() const;
	QString gps() const;
	QString gps_decimal() const;
	QString duration() const;
	bool noDive() const;
	QString depth() const;
	QString divemaster() const;
	QString buddy() const;
	QString airTemp() const;
	QString waterTemp() const;
	QString notes() const;
	QString tags() const;
	QString gas() const;
	QString sac() const;
	QString weightList() const;
	QStringList weights() const;
	QString weight(int idx) const;
	bool singleWeight() const;
	QString suit() const;
	QStringList cylinderList() const;
	QStringList cylinders() const;
	QString cylinder(int idx) const;
	QList<CylinderObjectHelper*> cylinderObjects() const;
	QString trip() const;
	QString tripMeta() const;
	int maxcns() const;
	int otu() const;
	QString sumWeight() const;
	QString getCylinder() const;
	QString startPressure() const;
	QString endPressure() const;
	QString firstGas() const;
	QStringList suitList() const;
	QStringList buddyList() const;
	QStringList divemasterList() const;

private:
	struct dive *m_dive;
	QList<CylinderObjectHelper*> m_cyls;
};
	Q_DECLARE_METATYPE(DiveObjectHelper *)

#endif
