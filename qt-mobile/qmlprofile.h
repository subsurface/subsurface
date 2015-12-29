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
	virtual ~QMLProfile();

	void paint(QPainter *painter);

	QString diveId() const;
	void setDiveId(const QString &diveId);
public slots:
	void setMargin(int margin);
private:
	QString m_diveId;
	int m_margin;
	ProfileWidget2 *m_profileWidget;
signals:
	void rightAlignedChanged();
	void diveIdChanged();
};

#endif // QMLPROFILE_H
