#include "qthelper.h"

DiveComputerList::DiveComputerList()
{

}

DiveComputerList::~DiveComputerList()
{

}

bool DiveComputerNode::operator == (const DiveComputerNode &a) const {
	return this->model == a.model &&
			this->deviceId == a.deviceId &&
			this->firmware == a.firmware &&
			this->serialNumber == a.serialNumber &&
			this->nickName == a.nickName;
}

bool DiveComputerNode::operator !=(const DiveComputerNode &a) const {
	return !(*this == a);
}

bool DiveComputerNode::changesValues(const DiveComputerNode &b) const
{
	if (this->model != b.model || this->deviceId != b.deviceId) {
		qDebug("DiveComputerNodes were not for the same DC");
		return false;
	}
	return (b.firmware != "" && this->firmware != b.firmware) ||
			(b.serialNumber != "" && this->serialNumber != b.serialNumber) ||
			(b.nickName != "" && this->nickName != b.nickName);
}

const DiveComputerNode *DiveComputerList::getExact(QString m, uint32_t d)
{
	if (dcMap.contains(m)) {
		QList<DiveComputerNode> values = dcMap.values(m);
		for (int i = 0; i < values.size(); i++)
			if (values.at(i).deviceId == d)
				return &values.at(i);
	}
	return NULL;
}

const DiveComputerNode *DiveComputerList::get(QString m)
{
	if (dcMap.contains(m)) {
		QList<DiveComputerNode> values = dcMap.values(m);
		return &values.at(0);
	}
	return NULL;
}

void DiveComputerList::addDC(QString m, uint32_t d, QString n, QString s, QString f)
{
	if (m == "" || d == 0)
		return;
	const DiveComputerNode *existNode = this->getExact(m, d);
	DiveComputerNode newNode(m, d, s, f, n);
	if (existNode) {
		if (newNode.changesValues(*existNode)) {
			if (n != "" && existNode->nickName != n)
				qDebug("new nickname %s for DC model %s deviceId 0x%x", n.toUtf8().data(), m.toUtf8().data(), d);
			if (f != "" && existNode->firmware != f)
				qDebug("new firmware version %s for DC model %s deviceId 0x%x", f.toUtf8().data(), m.toUtf8().data(), d);
			if (s != "" && existNode->serialNumber != s)
				qDebug("new serial number %s for DC model %s deviceId 0x%x", s.toUtf8().data(), m.toUtf8().data(), d);
		} else {
			return;
		}
		dcMap.remove(m, *existNode);
	}
	dcMap.insert(m, newNode);
}

void DiveComputerList::rmDC(QString m, uint32_t d)
{
	const DiveComputerNode *existNode = this->getExact(m, d);
	dcMap.remove(m, *existNode);
}
