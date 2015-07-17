#include "qmlprofile.h"
#include "profilewidget2.h"
#include "dive.h"

QMLProfile::QMLProfile(QQuickItem *parent) :
	QQuickPaintedItem(parent)
{
	profile = new ProfileWidget2(0);
	profile->setProfileState();
	profile->setToolTipVisibile(false);
}

void QMLProfile::paint(QPainter *painter)
{
	if (m_diveId.toInt() < 1)
		return;

	struct dive *d;
	d = get_dive_by_uniq_id(m_diveId.toInt());
	if (!d)
		return;

	int old_animation_speed = prefs.animation_speed;
	prefs.animation_speed = 0; // no animations while rendering the QGraphicsView
	profile->plotDive(d);
	// we need to show the widget so it gets populated, but then
	// hide it right away so we get to draw it ourselves below
	profile->show();
	profile->hide();
	profile->resize(this->width(), this->height());
	profile->render(painter, profile->geometry());
	prefs.animation_speed = old_animation_speed;
}

QString QMLProfile::diveId() const
{
	return m_diveId;
}

void QMLProfile::setDiveId(const QString &diveId)
{
	m_diveId = diveId;
}
