#include "CylinderObjectHelper.h"
#include "../helpers.h"

static QString EMPTY_CYLINDER_STRING = QStringLiteral("");
CylinderObjectHelper::CylinderObjectHelper(cylinder_t *cylinder) :
	m_cyl(cylinder) {
}

CylinderObjectHelper::~CylinderObjectHelper() {
}

QString CylinderObjectHelper::description() const {
	if (!m_cyl->type.description)
		return QString(EMPTY_CYLINDER_STRING);
	else
		return QString(m_cyl->type.description);
}

QString CylinderObjectHelper::size() const {
	return get_volume_string(m_cyl->type.size, true);
}

QString CylinderObjectHelper::workingPressure() const {
	return get_pressure_string(m_cyl->type.workingpressure, true);
}

QString CylinderObjectHelper::startPressure() const {
	return get_pressure_string(m_cyl->start, true);
}

QString CylinderObjectHelper::endPressure() const {
	return get_pressure_string(m_cyl->end, true);
}

QString CylinderObjectHelper::gasMix() const {
	return get_gas_string(m_cyl->gasmix);
}
