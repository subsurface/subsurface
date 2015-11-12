#include "qmlprofile.h"
#include "profile-widget/profilewidget2.h"
#include "subsurface-core/dive.h"
#include <QTransform>

QMLProfile::QMLProfile(QQuickItem *parent) :
	QQuickPaintedItem(parent)
{
	m_profileWidget = new ProfileWidget2(0);
	m_profileWidget->setProfileState();
	m_profileWidget->setToolTipVisibile(false);
	//m_profileWidget->setGeometry(this->geometry());
}

QMLProfile::~QMLProfile()
{
	m_profileWidget->deleteLater();
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

	m_profileWidget->setGeometry(QRect(x(), y(), width(), height()));
	m_profileWidget->plotDive(d);
	QTransform profileTransform;
	profileTransform.scale(this->height() / 100, this->height() / 100);
	m_profileWidget->setTransform(profileTransform);
	m_profileWidget->render(painter);
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
