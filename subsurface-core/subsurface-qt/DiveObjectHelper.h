#ifndef DIVE_QOBJECT_H
#define DIVE_QOBJECT_H

#include "../dive.h"
#include <QObject>
#include <QString>
#include <QStringList>

class DiveObjectHelper : public QObject {
	Q_OBJECT
	Q_PROPERTY(int number READ number CONSTANT)
	Q_PROPERTY(int id READ id CONSTANT)
	Q_PROPERTY(int rating READ rating CONSTANT)
	Q_PROPERTY(QString date READ date CONSTANT)
	Q_PROPERTY(QString time READ time CONSTANT)
	Q_PROPERTY(QString location READ location CONSTANT)
	Q_PROPERTY(QString gps READ gps CONSTANT)
	Q_PROPERTY(QString duration READ duration CONSTANT)
	Q_PROPERTY(QString depth READ depth CONSTANT)
	Q_PROPERTY(QString divemaster READ divemaster CONSTANT)
	Q_PROPERTY(QString buddy READ buddy CONSTANT)
	Q_PROPERTY(QString airTemp READ airTemp CONSTANT)
	Q_PROPERTY(QString waterTemp READ waterTemp CONSTANT)
	Q_PROPERTY(QString notes READ notes CONSTANT)
	Q_PROPERTY(QString tags READ tags CONSTANT)
	Q_PROPERTY(QString gas READ gas CONSTANT)
	Q_PROPERTY(QString sac READ sac CONSTANT)
	Q_PROPERTY(QStringList weights READ weights CONSTANT)
	Q_PROPERTY(QString suit READ suit CONSTANT)
	Q_PROPERTY(QStringList cylinders READ cylinders CONSTANT)
	Q_PROPERTY(QString trip READ trip CONSTANT)
	Q_PROPERTY(QString maxcns READ maxcns CONSTANT)
	Q_PROPERTY(QString otu READ otu CONSTANT)
public:
	DiveObjectHelper(struct dive *dive = NULL);
	~DiveObjectHelper();
	int number() const;
	int id() const;
	int rating() const;
	QString date() const;
	timestamp_t timestamp() const;
	QString time() const;
	QString location() const;
	QString gps() const;
	QString duration() const;
	QString depth() const;
	QString divemaster() const;
	QString buddy() const;
	QString airTemp() const;
	QString waterTemp() const;
	QString notes() const;
	QString tags() const;
	QString gas() const;
	QString sac() const;
	QStringList weights() const;
	QString weight(int idx) const;
	QString suit() const;
	QStringList cylinders() const;
	QString cylinder(int idx) const;
	QString trip() const;
	QString maxcns() const;
	QString otu() const;

private:
	QString m_date;
	QString m_time;
	QString m_gas;
	QStringList m_weights;
	QStringList m_cylinders;
	struct dive *m_dive;
};
	Q_DECLARE_METATYPE(DiveObjectHelper *)

#endif
