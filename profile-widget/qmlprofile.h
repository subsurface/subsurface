// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPROFILE_H
#define QMLPROFILE_H

#include "profilewidget2.h"
#include <QQuickPaintedItem>

class QMLProfile : public QQuickPaintedItem
{
	Q_OBJECT
	Q_PROPERTY(int diveId MEMBER m_diveId WRITE setDiveId)
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio WRITE setDevicePixelRatio NOTIFY devicePixelRatioChanged)

public:
	explicit QMLProfile(QQuickItem *parent = 0);

	void paint(QPainter *painter);

	int diveId() const;
	void setDiveId(int diveId);
	qreal devicePixelRatio() const;
	void setDevicePixelRatio(qreal dpr);

public slots:
	void setMargin(int margin);
	void screenChanged(QScreen *screen);
private:
	int m_diveId;
	qreal m_devicePixelRatio;
	int m_margin;
	QScopedPointer<ProfileWidget2> m_profileWidget;

signals:
	void rightAlignedChanged();
	void devicePixelRatioChanged();
};

#endif // QMLPROFILE_H
