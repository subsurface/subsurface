// SPDX-License-Identifier: GPL-2.0
#include "qPrefPrivate.h"
#include "core/subsurface-float.h"

#include <QSettings>

void qPrefPrivate::copy_txt(const char **name, const QString &string)
{
	free((void *)*name);
	*name = copy_qstring(string);
}

QString keyFromGroupAndName(QString group, QString name)
{
	QString slash = (group.endsWith('/') || name.startsWith('/')) ? "" : "/";
	return group + slash + name;
}

void qPrefPrivate::propSetValue(const QString &key, const QVariant &value, const QVariant &defaultValue)
{
	QSettings s;
	bool isDefault = false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	if (value.isValid() && value.typeId() == QMetaType::Double)
#else
	if (value.isValid() && value.type() == QVariant::Double)
#endif
		isDefault = nearly_equal(value.toDouble(), defaultValue.toDouble());
	else
		isDefault = (value == defaultValue);

	if (!isDefault)
		s.setValue(key, value);
	else
		s.remove(key);
}

QVariant qPrefPrivate::propValue(const QString &key, const QVariant &defaultValue)
{
	QSettings s;
	return  s.value(key, defaultValue);
}
