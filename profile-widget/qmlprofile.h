// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPROFILE_H
#define QMLPROFILE_H

#include "core/subsurface-qt/divelistnotifier.h"
#include <QQuickPaintedItem>
#include <memory>

class ProfileScene;

class QMLProfile : public QQuickPaintedItem
{
	Q_OBJECT
	Q_PROPERTY(int diveId MEMBER m_diveId WRITE setDiveId)
	Q_PROPERTY(int numDC READ numDC NOTIFY numDCChanged)
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio WRITE setDevicePixelRatio NOTIFY devicePixelRatioChanged)
	Q_PROPERTY(qreal xOffset MEMBER m_xOffset WRITE setXOffset NOTIFY xOffsetChanged)
	Q_PROPERTY(qreal yOffset MEMBER m_yOffset WRITE setYOffset NOTIFY yOffsetChanged)

public:
	explicit QMLProfile(QQuickItem *parent = 0);
	~QMLProfile();

	void paint(QPainter *painter);

	int diveId() const;
	void setDiveId(int diveId);
	qreal devicePixelRatio() const;
	void setDevicePixelRatio(qreal dpr);
	void setXOffset(qreal value);
	void setYOffset(qreal value);
	Q_INVOKABLE void nextDC();
	Q_INVOKABLE void prevDC();

public slots:
	void setMargin(int margin);
	void screenChanged(QScreen *screen);
	void triggerUpdate();

private:
	int m_diveId;
	int m_dc;
	qreal m_devicePixelRatio;
	int m_margin;
	qreal m_xOffset, m_yOffset;
	std::unique_ptr<ProfileScene> m_profileWidget;
	void createProfileView();
	void rotateDC(int dir);
	int numDC() const;

private slots:
	void divesChanged(const QVector<dive *> &dives, DiveField);

signals:
	void rightAlignedChanged();
	void devicePixelRatioChanged();
	void xOffsetChanged();
	void yOffsetChanged();
	void numDCChanged();
};

#endif // QMLPROFILE_H
