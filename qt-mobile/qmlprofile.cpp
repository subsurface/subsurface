#include "qmlprofile.h"
#include "profile-widget/profilewidget2.h"
#include "subsurface-core/dive.h"
#include <QTransform>

QMLProfile::QMLProfile(QQuickItem *parent) :
	QQuickPaintedItem(parent),
	m_devicePixelRatio(1.0),
	m_margin(0)
{
	setAntialiasing(true);
	m_profileWidget = new ProfileWidget2(0);
	m_profileWidget->setProfileState();
	m_profileWidget->setPrintMode(true);
	m_profileWidget->setFontPrintScale(0.8);
	//m_profileWidget->setGeometry(this->geometry());
}

QMLProfile::~QMLProfile()
{
	m_profileWidget->deleteLater();
}

void QMLProfile::paint(QPainter *painter)
{
	// let's look at the intended size of the content and scale our scene accordingly
	QRect rect = m_profileWidget->contentsRect();
	qreal sceneSize = 104; // that should give us 2% margin all around (100x100 scene)
	qreal sx = rect.width() / sceneSize;
	qreal sy = rect.height() / sceneSize;
	QTransform profileTransform;
	profileTransform.scale(sx, sy);
	m_profileWidget->setTransform(profileTransform);
	m_profileWidget->render(painter);
}

void QMLProfile::setMargin(int margin)
{
	m_margin = margin;
}

QString QMLProfile::diveId() const
{
	return m_diveId;
}

void QMLProfile::setDiveId(const QString &diveId)
{
	m_diveId = diveId;
	struct dive *d = get_dive_by_uniq_id(m_diveId.toInt());
	if (m_diveId.toInt() < 1)
		return;
	if (!d)
		return;
	qDebug() << "setDiveId called with valid dive" << d->number;
	m_profileWidget->plotDive(d);
}

qreal QMLProfile::devicePixelRatio() const
{
	return m_devicePixelRatio;
}

void QMLProfile::setDevicePixelRatio(qreal dpr)
{
	if (dpr != m_devicePixelRatio) {
		m_devicePixelRatio = dpr;
		emit devicePixelRatioChanged();
	}
}
