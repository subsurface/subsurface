#include "qmlprofile.h"
#include "profile-widget/profilewidget2.h"
#include "subsurface-core/dive.h"
#include <QTransform>

QMLProfile::QMLProfile(QQuickItem *parent) :
	QQuickPaintedItem(parent)
{
}

void QMLProfile::paint(QPainter *painter)
{
	if (m_diveId.toInt() < 1)
		return;

	struct dive *d;
	d = get_dive_by_uniq_id(m_diveId.toInt());
	if (!d)
		return;


	profile = new ProfileWidget2(0);
	profile->setProfileState();
	profile->setToolTipVisibile(false);

	int old_animation_speed = prefs.animation_speed;
	prefs.animation_speed = 0; // no animations while rendering the QGraphicsView
	profile->plotDive(d);
	QTransform profileTransform;
	profileTransform.scale(this->height() / 100, this->height() / 100);
	profile->setTransform(profileTransform);
	profile->render(painter);
	prefs.animation_speed = old_animation_speed;

	profile->deleteLater();
}

QString QMLProfile::diveId() const
{
	return m_diveId;
}

void QMLProfile::setDiveId(const QString &diveId)
{
	m_diveId = diveId;
}
