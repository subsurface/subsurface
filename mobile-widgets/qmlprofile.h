#ifndef QMLPROFILE_H
#define QMLPROFILE_H

#include <QQuickPaintedItem>

class ProfileWidget2;

class QMLProfile : public QQuickPaintedItem
{
	Q_OBJECT
	Q_PROPERTY(QString diveId READ diveId WRITE setDiveId NOTIFY diveIdChanged)
	Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio WRITE setDevicePixelRatio NOTIFY devicePixelRatioChanged)

public:
	explicit QMLProfile(QQuickItem *parent = 0);
	virtual ~QMLProfile();

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
	ProfileWidget2 *m_profileWidget;

signals:
	void rightAlignedChanged();
	void diveIdChanged();
	void devicePixelRatioChanged();
};

#endif // QMLPROFILE_H
