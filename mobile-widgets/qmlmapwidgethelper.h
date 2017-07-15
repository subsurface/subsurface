#ifndef QMLMAPWIDGETHELPER_H
#define QMLMAPWIDGETHELPER_H

#include <QObject>

class MapWidgetHelper : public QObject {

	Q_OBJECT

public:
	explicit MapWidgetHelper(QObject *parent = NULL);

	void test();
};

#endif
