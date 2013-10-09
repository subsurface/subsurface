#ifndef QTHELPER_H
#define QTHELPER_H

#include <QMultiMap>
#include <QString>
#include <stdint.h>
#include "dive.h"
#include <QTranslator>

// global pointers for our translation
extern QTranslator *qtTranslator, *ssrfTranslator;

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
	QMultiMap<QString, class DiveComputerNode> dcMap;
	QMultiMap<QString, class DiveComputerNode> dcWorkingMap;
};

QString weight_string(int weight_in_grams);
bool gpsHasChanged(struct dive* dive, struct dive *master, const QString &gps_text);

#endif // QTHELPER_H
