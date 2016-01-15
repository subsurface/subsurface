#include "qmlprofile.h"
#include "profile-widget/profilewidget2.h"
#include "subsurface-core/dive.h"
#include <QTransform>

QMLProfile::QMLProfile(QQuickItem *parent) :
	QQuickPaintedItem(parent),
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
	m_profileWidget->setGeometry(QRect(0, 0, width(), height()));
	// scale the profile widget's image to devicePixelRatio and a magic number
	qreal dpr = 104; // that should give us 2% margin all around
	qreal sx = width() / dpr;
	qreal sy = height() / dpr;

	qDebug() << "paint called; rect" << x() << y() << width() << height() << "dpr" << dpr << "sx/sy" << sx << sy;

	QTransform profileTransform;
	profileTransform.scale(sx, sy);
	m_profileWidget->setTransform(profileTransform);
	qDebug() << "viewportTransform" << m_profileWidget->viewportTransform();
	qDebug() << "after scaling we have margin/rect" << m_profileWidget->contentsMargins() << m_profileWidget->contentsRect();
	qDebug() << "size of the QMLProfile:" << this->contentsSize() << this->contentsScale();
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
