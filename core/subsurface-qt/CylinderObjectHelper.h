// SPDX-License-Identifier: GPL-2.0
#ifndef CYLINDER_QOBJECT_H
#define CYLINDER_QOBJECT_H

#include "../equipment.h"
#include <QObject>
#include <QString>

class CylinderObjectHelper {
	Q_GADGET
	Q_PROPERTY(QString description MEMBER description CONSTANT)
	Q_PROPERTY(QString size MEMBER size CONSTANT)
	Q_PROPERTY(QString workingPressure MEMBER workingPressure CONSTANT)
	Q_PROPERTY(QString startPressure MEMBER startPressure CONSTANT)
	Q_PROPERTY(QString endPressure MEMBER endPressure CONSTANT)
	Q_PROPERTY(QString gasMix MEMBER gasMix CONSTANT)
public:
	CylinderObjectHelper(const cylinder_t *cylinder = NULL);
	QString description;
	QString size;
	QString workingPressure;
	QString startPressure;
	QString endPressure;
	QString gasMix;
};

Q_DECLARE_METATYPE(CylinderObjectHelper)

#endif
