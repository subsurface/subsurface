#ifndef QMLPROFILE_H
#define QMLPROFILE_H

#include <QQuickPaintedItem>

class ProfileWidget2;

class QMLProfile : public QQuickPaintedItem
{
	Q_OBJECT
	Q_PROPERTY(QString diveId READ diveId WRITE setDiveId NOTIFY diveIdChanged)
public:
	explicit QMLProfile(QQuickItem *parent = 0);
	void paint(QPainter *painter);

	QString diveId() const;
	void setDiveId(const QString &diveId);

private:
	QString m_diveId;
	ProfileWidget2 *profile;
signals:
	void rightAlignedChanged();
	void diveIdChanged();
};

#endif // QMLPROFILE_H
