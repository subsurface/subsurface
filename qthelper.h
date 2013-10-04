#ifndef QTHELPER_H
#define QTHELPER_H

#include <QMultiMap>
#include <QString>
#include <stdint.h>
#include "dive.h"

class DiveComputerNode {
public:
	DiveComputerNode(QString m, uint32_t d, QString s, QString f, QString n) : model(m), deviceId(d), serialNumber(s), firmware(f), nickName(n) {};
	bool operator ==(const DiveComputerNode &a) const;
	bool operator !=(const DiveComputerNode &a) const;
	bool changesValues(const DiveComputerNode &b) const;
	QString model;
	uint32_t deviceId;
	QString serialNumber;
	QString firmware;
	QString nickName;
};

class DiveComputerList {
public:
	DiveComputerList();
	~DiveComputerList();
	const DiveComputerNode *getExact(QString m, uint32_t d);
	const DiveComputerNode *get(QString m);
	void addDC(QString m, uint32_t d, QString n = "", QString s = "", QString f = "");
	void rmDC(QString m, uint32_t d);
	DiveComputerNode matchDC(QString m, uint32_t d);
	DiveComputerNode matchModel(QString m);
	QMultiMap<QString, struct DiveComputerNode> dcMap;
	QMultiMap<QString, struct DiveComputerNode> dcWorkingMap;
};

QString weight_string(int weight_in_grams);

#endif // QTHELPER_H
