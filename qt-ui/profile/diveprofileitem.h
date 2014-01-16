#ifndef DIVEPROFILEITEM_H
#define DIVEPROFILEITEM_H

#include <QObject>
#include <QGraphicsPolygonItem>

/* This is the Profile Item, it should be used for quite a lot of things
 on the profile view. The usage should be pretty simple:

 DiveProfileItem *profile = new DiveProfileItem();
 profile->setVerticalAxis( profileYAxis );
 profile->setHorizontalAxis( timeAxis );
 profile->setModel( DiveDataModel );
 profile->setHorizontalDataColumn( DiveDataModel::TIME );
 profile->setVerticalDataColumn( DiveDataModel::DEPTH );
 scene()->addItem(profile);

 This is a generically item and should be used as a base for others, I think...
*/

class DiveCartesianAxis;
class QAbstractTableModel;

class AbstractProfilePolygonItem : public QObject, public QGraphicsPolygonItem{
	Q_OBJECT
	Q_PROPERTY(QPointF pos WRITE setPos READ pos)
	Q_PROPERTY(qreal x WRITE setX READ x)
	Q_PROPERTY(qreal y WRITE setY READ y)
public:
	AbstractProfilePolygonItem();
	void setVerticalAxis(DiveCartesianAxis *vertical);
	void setHorizontalAxis(DiveCartesianAxis *horizontal);
	void setModel(QAbstractTableModel *model);
	void setHorizontalDataColumn(int column);
	void setVerticalDataColumn(int column);
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0) = 0;
public slots:
	virtual void modelDataChanged();
protected:
	DiveCartesianAxis *hAxis;
	DiveCartesianAxis *vAxis;
	QAbstractTableModel *dataModel;
	int hDataColumn;
	int vDataColumn;
};

class DiveProfileItem : public AbstractProfilePolygonItem{
	Q_OBJECT
public:
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
	virtual void modelDataChanged();
};

class DiveTemperatureItem : public AbstractProfilePolygonItem{
	Q_OBJECT
public:
	DiveTemperatureItem();
	virtual void modelDataChanged();
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
};
#endif