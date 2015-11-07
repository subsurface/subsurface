#include "abstractpreferenceswidget.h"

AbstractPreferencesWidget::AbstractPreferencesWidget(const QString& name, const QIcon& icon, float positionHeight)
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

float AbstractPreferencesWidget::positionHeight() const
{
	return _positionHeight;
}
