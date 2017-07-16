#ifndef QMLMAPWIDGETHELPER_H
#define QMLMAPWIDGETHELPER_H

#include <QObject>

class MapWidgetHelper : public QObject {

	Q_OBJECT
	Q_PROPERTY(QObject *map MEMBER m_map)

public:
	explicit MapWidgetHelper(QObject *parent = NULL);

private:
	QObject *m_map;
};

#endif
