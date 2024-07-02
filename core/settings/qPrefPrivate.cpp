// SPDX-License-Identifier: GPL-2.0
#include "qPrefPrivate.h"
#include "core/subsurface-float.h"

#include <QSettings>

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

void qPrefPrivate::propSetValue(const QString &key, const std::string &value, const std::string &defaultValue)
{
	propSetValue(key, QString::fromStdString(value), QString::fromStdString(defaultValue));
}

QVariant qPrefPrivate::propValue(const QString &key, const QVariant &defaultValue)
{
	QSettings s;
	return  s.value(key, defaultValue);
}

QVariant qPrefPrivate::propValue(const QString &key, const std::string &defaultValue)
{
	QSettings s;
	return  s.value(key, QString::fromStdString(defaultValue));
}
