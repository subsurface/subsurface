// SPDX-License-Identifier: GPL-2.0
#include "qPrefPrivate.h"
#include "core/subsurface-string.h"

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
	// REMARK: making s static (which would be logical) does NOT work
	// because it gets initialized too early.
	// Having it as a local variable is light weight, because it is an
	// interface class.
	QSettings s;
	bool isDefault = false;
	if (value.isValid() && value.type() == QVariant::Double)
		isDefault = IS_FP_SAME(value.toDouble(), defaultValue.toDouble());
	else
		isDefault = (value == defaultValue);

	if (!isDefault)
		s.setValue(key, value);
	else
		s.remove(key);
}

QVariant qPrefPrivate::propValue(const QString &key, const QVariant &defaultValue)
{
	// REMARK: making s static (which would be logical) does NOT work
	// because it gets initialized too early.
	// Having it as a local variable is light weight, because it is an
	// interface class.
	QSettings s;
	return  s.value(key, defaultValue);
}
