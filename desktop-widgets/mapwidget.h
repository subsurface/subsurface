#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include <QQuickWidget>

class QResizeEvent;
class QQuickItem;

class MapWidget : public QQuickWidget {

public:
	MapWidget(QWidget *parent = NULL);
	~MapWidget();

	static MapWidget *instance();

private:
	static MapWidget *m_instance;
	QQuickItem *m_rootItem;

};

#endif // MAPWIDGET_H
