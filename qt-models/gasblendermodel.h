// SPDX-License-Identifier: GPL-2.0
#ifndef GasBlenderModel_H
#define GasBlenderModel_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QVariantList>

class GasBlenderModel : public QObject {
	Q_OBJECT
public:
	explicit GasBlenderModel(QObject *parent = nullptr);
	
	// Optional: Singleton instance if you need to register it globally for QML
	static GasBlenderModel *instance();

	Q_INVOKABLE QString calculateGasInfo(const QString &cylinderType, int o2_permille, int he_permille);
	
	Q_INVOKABLE QString calculateCheapestBlend(const QString &target_amount, const QString &target_start_mix, 
				int target_start_pressure, const QString &target_end_mix, int target_end_pressure, 
				const QVariantList &available_gases, bool simple_blend);
 
};

#endif // GasBlenderModel_H