// SPDX-License-Identifier: GPL-2.0
#include "abstractpreferenceswidget.h"

AbstractPreferencesWidget::AbstractPreferencesWidget(const QString& name, const QIcon& icon, double positionHeight)
: QWidget(), _icon(icon), _name(name), _positionHeight(positionHeight)
{
}

QIcon AbstractPreferencesWidget::icon() const
{
	return _icon;
}

QString AbstractPreferencesWidget::name() const
{
	return _name;
}

double AbstractPreferencesWidget::positionHeight() const
{
	return _positionHeight;
}
