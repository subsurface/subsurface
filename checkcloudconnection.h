#ifndef CHECKCLOUDCONNECTION_H
#define CHECKCLOUDCONNECTION_H

#include <QObject>

#include "checkcloudconnection.h"

class CheckCloudConnection : public QObject {
	Q_OBJECT
public:
	CheckCloudConnection(QObject *parent = 0);
	static bool checkServer();
};

#endif // CHECKCLOUDCONNECTION_H
