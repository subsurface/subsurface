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
	QList<DiveTextItem*> texts;
};

class DiveProfileItem : public AbstractProfilePolygonItem{
	Q_OBJECT

public:
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
	virtual void modelDataChanged();
	void plot_depth_sample(struct plot_data *entry,QFlags<Qt::AlignmentFlag> flags,const QColor& color);
};

class DiveTemperatureItem : public AbstractProfilePolygonItem{
	Q_OBJECT
public:
	DiveTemperatureItem();
	virtual void modelDataChanged();
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
private:
	void createTextItem(int seconds, int mkelvin);
	QList<DiveTextItem*> texts;
};

class DiveGasPressureItem : public AbstractProfilePolygonItem{
	Q_OBJECT
public:
	virtual void modelDataChanged();
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
private:
	QVector<QPolygonF> polygons;
};

QGraphicsItemGroup *plotText(text_render_options_t *tro,const QPointF& pos, const QString& text, QGraphicsItem *parent);

#endif