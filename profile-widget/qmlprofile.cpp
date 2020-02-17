// SPDX-License-Identifier: GPL-2.0
#include "qmlprofile.h"
#include "mobile-widgets/qmlmanager.h"
#include "core/errorhelper.h"
#include "core/subsurface-string.h"
#include "core/metrics.h"
#include <QTransform>
#include <QScreen>
#include <QElapsedTimer>

QMLProfile::QMLProfile(QQuickItem *parent) :
	QQuickPaintedItem(parent),
	m_devicePixelRatio(1.0),
	m_margin(0),
	m_profileWidget(new ProfileWidget2),
	m_xOffset(0.0),
	m_yOffset(0.0)
{
	setAntialiasing(true);
	setFlags(QQuickItem::ItemClipsChildrenToShape | QQuickItem::ItemHasContents );
	m_profileWidget->setProfileState();
	m_profileWidget->setPrintMode(true);
	m_profileWidget->setFontPrintScale(0.8);
	connect(QMLManager::instance(), &QMLManager::sendScreenChanged, this, &QMLProfile::screenChanged);
	setDevicePixelRatio(QMLManager::instance()->lastDevicePixelRatio());
}

void QMLProfile::paint(QPainter *painter)
{
	QElapsedTimer timer;
	if (verbose)
		timer.start();

	// let's look at the intended size of the content and scale our scene accordingly
	QRect painterRect = painter->viewport();
	QRect profileRect = m_profileWidget->viewport()->rect();
	// qDebug() << "profile viewport and painter viewport" << profileRect << painterRect;
	qreal sceneSize = 104; // that should give us 2% margin all around (100x100 scene)
	qreal dprComp =  devicePixelRatio() * painterRect.width() / profileRect.width();
	qreal sx = painterRect.width() / sceneSize / dprComp;
	qreal sy = painterRect.height() / sceneSize / dprComp;

	// next figure out the weird magic by which we need to shift the painter so the profile is shown
	double dpr = devicePixelRatio();
	double magicValues[] = { 0.0, 0.1, 0.25, 0.33, 0.375, 0.40, 0.42};
	qreal magicShiftFactor = 0.0;
	if (dpr < 1.3) {
		magicShiftFactor = magicValues[0];
	} else if (dpr > 6.0) {
		magicShiftFactor = magicValues[6];
	} else if (IS_FP_SAME(dpr, rint(dpr))) {
		magicShiftFactor = magicValues[lrint(dpr)];
	} else {
		int lower = (int)dpr;
		magicShiftFactor = (magicValues[lower] * (lower + 1 - dpr) + magicValues[lower + 1] * (dpr - lower));
		if (dpr < 1.45)
			magicShiftFactor -= 0.03;
	}
	// now set up the transformations scale the profile and
	// shift the painter (taking its existing transformation into account)
	QTransform profileTransform = QTransform();
	profileTransform.scale(sx, sy);
	QTransform painterTransform = painter->transform();
	painterTransform.translate(dpr * m_xOffset - painterRect.width() * magicShiftFactor, dpr * m_yOffset - painterRect.height() * magicShiftFactor);

#if defined(PROFILE_SCALING_DEBUG)
	// some debugging messages to help adjust this in case the magic above is insufficient
	qDebug() << QString("dpr %1 profile viewport %2 %3 painter viewport %4 %5").arg(dpr).arg(profileRect.width()).arg(profileRect.height())
		    .arg(painterRect.width()).arg(painterRect.height());
	qDebug() << QString("profile matrix %1 %2 %3 %4 %5 %6 %7 %8 %9").arg(profileTransform.m11()).arg(profileTransform.m12()).arg(profileTransform.m13())
		    .arg(profileTransform.m21()).arg(profileTransform.m22()).arg(profileTransform.m23())
		    .arg(profileTransform.m31()).arg(profileTransform.m32()).arg(profileTransform.m33()));
	qDebug() << QString("painter matrix %1 %2 %3 %4 %5 %6 %7 %8 %9").arg(painterTransform.m11()).arg(painterTransform.m12()).arg(painterTransform.m13())
		    .arg(painterTransform.m21()).arg(painterTransform.m22()).arg(painterTransform.m23())
		    .arg(painterTransform.m31()).arg(painterTransform.m32()).arg(painterTransform.m33()));
	qDebug() << "exist profile transform" << m_profileWidget->transform() << "painter transform" << painter->transform();
#endif
	// apply the transformation
	painter->setTransform(painterTransform);
	m_profileWidget->setTransform(profileTransform);

	// finally, render the profile
	m_profileWidget->render(painter);
	if (verbose)
		qDebug() << "finished rendering profile with offset" << QString::number(m_xOffset, 'f', 1) << "/" << QString::number(m_yOffset, 'f', 1)  << "in" << timer.elapsed() << "ms";
}

void QMLProfile::setMargin(int margin)
{
	m_margin = margin;
}

int QMLProfile::diveId() const
{
	return m_diveId;
}

void QMLProfile::updateProfile()
{
	struct dive *d = get_dive_by_uniq_id(m_diveId);
	if (!d)
		return;
	if (verbose)
		qDebug() << "update profile for dive #" << d->number << "offeset" << QString::number(m_xOffset, 'f', 1) << "/" << QString::number(m_yOffset, 'f', 1);
	m_profileWidget->plotDive(d, true);
}

void QMLProfile::setDiveId(int diveId)
{
	m_diveId = diveId;
	if (m_diveId < 0)
		return;
	updateProfile();
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

// don't update the profile here, have the user update x and y and then manually trigger an update
void QMLProfile::setXOffset(qreal value)
{
	if (IS_FP_SAME(value, m_xOffset))
		return;
	m_xOffset = value;
	emit xOffsetChanged();
}

// don't update the profile here, have the user update x and y and then manually trigger an update
void QMLProfile::setYOffset(qreal value)
{
	if (IS_FP_SAME(value, m_yOffset))
		return;
	m_yOffset = value;
	emit yOffsetChanged();
}

void QMLProfile::screenChanged(QScreen *screen)
{
	setDevicePixelRatio(screen->devicePixelRatio());
}
