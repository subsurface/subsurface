#include "qmlprofile.h"
#include "qmlmanager.h"
#include "profile-widget/profilewidget2.h"
#include "core/dive.h"
#include "core/metrics.h"
#include <QTransform>
#include <QScreen>

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
	connect(QMLManager::instance(), &QMLManager::sendScreenChanged, this, &QMLProfile::screenChanged);
	setDevicePixelRatio(QMLManager::instance()->lastDevicePixelRatio());
}

QMLProfile::~QMLProfile()
{
	m_profileWidget->deleteLater();
}

void QMLProfile::paint(QPainter *painter)
{
	// let's look at the intended size of the content and scale our scene accordingly
	QRect painterRect = painter->viewport();
	QRect profileRect = m_profileWidget->viewport()->rect();
	// qDebug() << "profile viewport and painter viewport" << profileRect << painterRect;
	qreal sceneSize = 104; // that should give us 2% margin all around (100x100 scene)
	qreal dprComp =  devicePixelRatio() * painterRect.width() / profileRect.width();
	qreal sx = painterRect.width() / sceneSize / dprComp;
	qreal sy = painterRect.height() / sceneSize / dprComp;

	// next figure out the weird magic by which we need to shift the painter so the profile is shown
	int dpr = lrint(devicePixelRatio());
	qreal magicShiftFactor = (dpr == 2 ? 0.25 : (dpr == 3 ? 0.33 : 0.0));

	// now set up the transformations scale the profile and
	// shift the painter (taking its existing transformation into account)
	QTransform profileTransform = QTransform();
	profileTransform.scale(sx, sy);
	QTransform painterTransform = painter->transform();
	painterTransform.translate(-painterRect.width() * magicShiftFactor ,-painterRect.height() * magicShiftFactor);

#if PROFILE_SCALING_DEBUG
	// some debugging messages to help adjust this in case the magic above is insufficient
	QMLManager::instance()->appendTextToLog(QString("dpr %1 profile viewport %2 %3 painter viewport %4 %5").arg(dpr).arg(profileRect.width()).arg(profileRect.height())
						.arg(painterRect.width()).arg(painterRect.height()));
	QMLManager::instance()->appendTextToLog(QString("profile matrix %1 %2 %3 %4 %5 %6 %7 %8 %9").arg(profileTransform.m11()).arg(profileTransform.m12()).arg(profileTransform.m13())
						.arg(profileTransform.m21()).arg(profileTransform.m22()).arg(profileTransform.m23())
						.arg(profileTransform.m31()).arg(profileTransform.m32()).arg(profileTransform.m33()));
	QMLManager::instance()->appendTextToLog(QString("painter matrix %1 %2 %3 %4 %5 %6 %7 %8 %9").arg(painterTransform.m11()).arg(painterTransform.m12()).arg(painterTransform.m13())
						.arg(painterTransform.m21()).arg(painterTransform.m22()).arg(painterTransform.m23())
						.arg(painterTransform.m31()).arg(painterTransform.m32()).arg(painterTransform.m33()));
	qDebug() << "profile scaled by" << profileTransform.m11() << profileTransform.m22() << "and translated by" << profileTransform.m31() << profileTransform.m32();
	qDebug() << "exist profile transform" << m_profileWidget->transform() << "painter transform" << painter->transform();
#endif
	// apply the transformation
	painter->setTransform(painterTransform);
	m_profileWidget->setTransform(profileTransform);

	// finally, render the profile
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
	if (verbose)
		qDebug() << "setDiveId(" << d->number << ")";
	m_profileWidget->plotDive(d, true);
}

qreal QMLProfile::devicePixelRatio() const
{
	return m_devicePixelRatio;
}

void QMLProfile::setDevicePixelRatio(qreal dpr)
{
	if (dpr != m_devicePixelRatio) {
		m_devicePixelRatio = dpr;
		m_profileWidget->setFontPrintScale(0.8 * dpr);
		updateDevicePixelRatio(dpr);
		emit devicePixelRatioChanged();
	}
}

void QMLProfile::screenChanged(QScreen *screen)
{
	setDevicePixelRatio(screen->devicePixelRatio());
}
