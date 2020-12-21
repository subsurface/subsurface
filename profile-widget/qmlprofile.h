// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPROFILE_H
#define QMLPROFILE_H

#include "profilewidget2.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include <QQuickPaintedItem>

class QMLProfile : public QQuickPaintedItem
{
	Q_OBJECT
	Q_PROPERTY(int diveId MEMBER m_diveId WRITE setDiveId)
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio WRITE setDevicePixelRatio NOTIFY devicePixelRatioChanged)
	Q_PROPERTY(qreal xOffset MEMBER m_xOffset WRITE setXOffset NOTIFY xOffsetChanged)
	Q_PROPERTY(qreal yOffset MEMBER m_yOffset WRITE setYOffset NOTIFY yOffsetChanged)

public:
	explicit QMLProfile(QQuickItem *parent = 0);

	void paint(QPainter *painter);

	int diveId() const;
	void setDiveId(int diveId);
	qreal devicePixelRatio() const;
	void setDevicePixelRatio(qreal dpr);
	void setXOffset(qreal value);
	void setYOffset(qreal value);

public slots:
	void setMargin(int margin);
	void screenChanged(QScreen *screen);
	void triggerUpdate();

private:
	int m_diveId;
	qreal m_devicePixelRatio;
	int m_margin;
	qreal m_xOffset, m_yOffset;
	QScopedPointer<ProfileWidget2> m_profileWidget;
	void updateProfile();

private slots:
	void divesChanged(const QVector<dive *> &dives, DiveField);

signals:
	void rightAlignedChanged();
	void devicePixelRatioChanged();
	void xOffsetChanged();
	void yOffsetChanged();
};

#endif // QMLPROFILE_H
