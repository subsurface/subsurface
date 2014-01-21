#ifndef DIVEPROFILEITEM_H
#define DIVEPROFILEITEM_H

#include <QObject>
#include <QGraphicsPolygonItem>
#include "graphicsview-common.h"
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

class DivePlotDataModel;
class DiveTextItem;
class DiveCartesianAxis;
class QAbstractTableModel;
struct plot_data;

class AbstractProfilePolygonItem : public QObject, public QGraphicsPolygonItem{
	Q_OBJECT
	Q_PROPERTY(QPointF pos WRITE setPos READ pos)
	Q_PROPERTY(qreal x WRITE setX READ x)
	Q_PROPERTY(qreal y WRITE setY READ y)
public:
	AbstractProfilePolygonItem();
	void setVerticalAxis(DiveCartesianAxis *vertical);
	void setHorizontalAxis(DiveCartesianAxis *horizontal);
	void setModel(DivePlotDataModel *model);
	void setHorizontalDataColumn(int column);
	void setVerticalDataColumn(int column);
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0) = 0;
public slots:
	virtual void preferencesChanged();
	virtual void modelDataChanged();
protected:
	DiveCartesianAxis *hAxis;
	DiveCartesianAxis *vAxis;
	DivePlotDataModel *dataModel;
	int hDataColumn;
	int vDataColumn;
	QList<DiveTextItem*> texts;
};

class DiveProfileItem : public AbstractProfilePolygonItem{
	Q_OBJECT

public:
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
	virtual void modelDataChanged();
	virtual void preferencesChanged();
	void plot_depth_sample(struct plot_data *entry,QFlags<Qt::AlignmentFlag> flags,const QColor& color);
private:
	unsigned int show_reported_ceiling;
	unsigned int reported_ceiling_in_red;
};

class DiveTemperatureItem : public AbstractProfilePolygonItem{
	Q_OBJECT
public:
	DiveTemperatureItem();
	virtual void modelDataChanged();
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
private:
	void createTextItem(int seconds, int mkelvin);
};

class DiveGasPressureItem : public AbstractProfilePolygonItem{
	Q_OBJECT

public:
	virtual void modelDataChanged();
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
private:
	void plot_pressure_value(int mbar, int sec, QFlags<Qt::AlignmentFlag> align);
	void plot_gas_value(int mbar, int sec, QFlags<Qt::AlignmentFlag> align, int o2, int he);
	QVector<QPolygonF> polygons;
};

class DiveCalculatedCeiling : public AbstractProfilePolygonItem{
	Q_OBJECT

public:
	virtual void modelDataChanged();
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
};

class DiveReportedCeiling : public AbstractProfilePolygonItem{
	Q_OBJECT

public:
	virtual void modelDataChanged();
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
	virtual void preferencesChanged();
};
#endif