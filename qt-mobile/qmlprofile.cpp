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
	QTransform profileTransform;
	profileTransform.scale(width() / 100, height() / 100);
	m_profileWidget->setTransform(profileTransform);
	m_profileWidget->render(painter);
}

QString QMLProfile::diveId() const
{
	return m_diveId;
}

void QMLProfile::setDiveId(const QString &diveId)
{
	m_diveId = diveId;
	int no = -1;
	struct dive *d = get_dive_by_uniq_id(m_diveId.toInt());
	if (d)
		no = d->number;
	if (m_diveId.toInt() < 1)
		return;

	if (!d)
		return;

	m_profileWidget->setGeometry(QRect(x(), y(), width(), height()));
	m_profileWidget->plotDive(d);
}
