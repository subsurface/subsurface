// SPDX-License-Identifier: GPL-2.0
#ifndef CYLINDER_QOBJECT_H
#define CYLINDER_QOBJECT_H

#include "../dive.h"
#include <QObject>
#include <QString>

class CylinderObjectHelper : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString description READ description CONSTANT)
	Q_PROPERTY(QString size READ size CONSTANT)
	Q_PROPERTY(QString workingPressure READ workingPressure CONSTANT)
	Q_PROPERTY(QString startPressure READ startPressure CONSTANT)
	Q_PROPERTY(QString endPressure READ endPressure CONSTANT)
	Q_PROPERTY(QString gasMix READ gasMix CONSTANT)
public:
	CylinderObjectHelper(cylinder_t *cylinder = NULL);
	~CylinderObjectHelper();
	QString description() const;
	QString size() const;
	QString workingPressure() const;
	QString startPressure() const;
	QString endPressure() const;
	QString gasMix() const;

private:
	cylinder_t *m_cyl;
};

	Q_DECLARE_METATYPE(CylinderObjectHelper *)
#endif
