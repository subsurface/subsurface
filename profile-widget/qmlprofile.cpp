// SPDX-License-Identifier: GPL-2.
#include "qmlprofile.h"
#include "profilescene.h"
#include "mobile-widgets/qmlmanager.h"
#include "core/errorhelper.h"
#include "core/subsurface-float.h"
#include "core/metrics.h"
#include "core/subsurface-string.h"
#include <QTransform>
#include <QScreen>
#include <QElapsedTimer>

QMLProfile::QMLProfile(QQuickItem *parent) :
	QQuickPaintedItem(parent),
	m_diveId(0),
	m_dc(0),
	m_devicePixelRatio(1.0),
	m_margin(0),
	m_xOffset(0.0),
	m_yOffset(0.0)
{
	createProfileView();
	setAntialiasing(true);
	setFlags(QQuickItem::ItemClipsChildrenToShape | QQuickItem::ItemHasContents );
	connect(QMLManager::instance(), &QMLManager::sendScreenChanged, this, &QMLProfile::screenChanged);
	connect(this, &QMLProfile::scaleChanged, this, &QMLProfile::triggerUpdate);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &QMLProfile::divesChanged);
	setDevicePixelRatio(QMLManager::instance()->lastDevicePixelRatio());
}

QMLProfile::~QMLProfile()
{
}

void QMLProfile::createProfileView()
{
	m_profileWidget.reset(new ProfileScene(m_devicePixelRatio * 0.8, false, false));
}

// we need this so we can connect update() to the scaleChanged() signal - which the connect above cannot do
// directly as it chokes on the default parameter for update().
// If the scale changes we may need to change our offsets to ensure that we still only show a subset of
// the profile and not empty space around it, which the paint() method below will take care of, which will
// eventually get called after we call update()
void QMLProfile::triggerUpdate()
{
	update();
}

void QMLProfile::paint(QPainter *painter)
{
	QElapsedTimer timer;
	if (verbose)
		timer.start();

	// let's look at the intended size of the content and scale our scene accordingly
	// for some odd reason the painter transformation is set up to scale by the dpr - which results
	// in applying that dpr scaling twice. So we hard-code it here to be the identity matrix
	QRect painterRect = painter->viewport();
	painter->resetTransform();
	if (m_diveId < 0)
		return;
	struct dive *d = get_dive_by_uniq_id(m_diveId);
	if (!d)
		return;
	m_profileWidget->draw(painter, painterRect, d, m_dc, nullptr, false);
}

void QMLProfile::setMargin(int margin)
{
	m_margin = margin;
}

int QMLProfile::diveId() const
{
	return m_diveId;
}

void QMLProfile::setDiveId(int diveId)
{
	m_diveId = diveId;
	emit numDCChanged();
}

qreal QMLProfile::devicePixelRatio() const
{
	return m_devicePixelRatio;
}

void QMLProfile::setDevicePixelRatio(qreal dpr)
{
	if (dpr != m_devicePixelRatio) {
		m_devicePixelRatio = dpr;
		// Recreate the view to redraw the text items with the new scale.
		createProfileView();
		emit devicePixelRatioChanged();
	}
}

// don't update the profile here, have the user update x and y and then manually trigger an update
void QMLProfile::setXOffset(qreal value)
{
	if (nearly_equal(value, m_xOffset))
		return;
	m_xOffset = value;
	emit xOffsetChanged();
}

// don't update the profile here, have the user update x and y and then manually trigger an update
void QMLProfile::setYOffset(qreal value)
{
	if (nearly_equal(value, m_yOffset))
		return;
	m_yOffset = value;
	emit yOffsetChanged();
}

void QMLProfile::screenChanged(QScreen *screen)
{
	setDevicePixelRatio(screen->devicePixelRatio());
}

void QMLProfile::divesChanged(const QVector<dive *> &dives, DiveField)
{
	for (struct dive *d: dives) {
		if (d->id == m_diveId) {
			report_info("dive #%d changed, trigger profile update", d->number);
			triggerUpdate();
			return;
		}
	}
}

void QMLProfile::nextDC()
{
	rotateDC(1);
}

void QMLProfile::prevDC()
{
	rotateDC(-1);
}

void QMLProfile::rotateDC(int dir)
{
	struct dive *d = get_dive_by_uniq_id(m_diveId);
	if (!d)
		return;
	int numDC = number_of_computers(d);
	if (numDC == 1)
		return;
	m_dc = (m_dc + dir) % numDC;
	if (m_dc < 0)
		m_dc += numDC;
	triggerUpdate();
}

int QMLProfile::numDC() const
{
	struct dive *d = get_dive_by_uniq_id(m_diveId);
	return d ? number_of_computers(d) : 0;
}
