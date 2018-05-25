// SPDX-License-Identifier: GPL-2.0
#ifndef QMLPROFILE_H
#define QMLPROFILE_H

#include "profile-widget/profilewidget2.h"
#include <QQuickPaintedItem>

class QMLProfile : public QQuickPaintedItem
{
	Q_OBJECT
	Q_PROPERTY(QString diveId READ diveId WRITE setDiveId NOTIFY diveIdChanged)
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio WRITE setDevicePixelRatio NOTIFY devicePixelRatioChanged)

public:
	explicit QMLProfile(QQuickItem *parent = 0);

	void paint(QPainter *painter);

	QString diveId() const;
	void setDiveId(const QString &diveId);
	qreal devicePixelRatio() const;
	void setDevicePixelRatio(qreal dpr);

public slots:
	void setMargin(int margin);
	void screenChanged(QScreen *screen);
private:
	QString m_diveId;
	qreal m_devicePixelRatio;
	int m_margin;
	QScopedPointer<ProfileWidget2> m_profileWidget;

signals:
	void rightAlignedChanged();
	void diveIdChanged();
	void devicePixelRatioChanged();
};

#endif // QMLPROFILE_H
