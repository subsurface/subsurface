// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFGAS_H
#define QPREFGAS_H

#include <QObject>


class qPrefGas : public QObject {
	Q_OBJECT
	Q_PROPERTY(double	phe_threshold
				READ	pheThreshold
				WRITE	setPheThreshold
				NOTIFY  pheThresholdChanged);
	Q_PROPERTY(double	pn2_threshold
				READ	pn2Threshold
				WRITE	setPn2Threshold
				NOTIFY  pn2ThresholdChanged);
	Q_PROPERTY(double	po2_threshold_max
				READ	po2ThresholdMax
				WRITE	setPo2ThresholdMax
				NOTIFY  po2ThresholdMaxChanged);
	Q_PROPERTY(double	po2_threshold_min
				READ	po2ThresholdMin
				WRITE	setPo2ThresholdMin
				NOTIFY  po2ThresholdMinChanged);
	Q_PROPERTY(bool		show_phe
				READ	showPhe
				WRITE	setShowPhe
				NOTIFY	showPheChanged);
	Q_PROPERTY(bool		show_pn2
				READ	showPn2
				WRITE	setShowPn2
				NOTIFY	showPn2Changed);
	Q_PROPERTY(bool		show_po2
				READ	showPo2
				WRITE	setShowPo2
				NOTIFY	showPo2Changed);

public:
	qPrefGas(QObject *parent = NULL) : QObject(parent) {};
	~qPrefGas() {};
	static qPrefGas *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	double pheThreshold() const;
	double pn2Threshold() const;
	double po2ThresholdMax() const;
	double po2ThresholdMin() const;
	bool showPhe() const;
	bool showPn2() const;
	bool showPo2() const;

public slots:
	void setPheThreshold(double value);
	void setPn2Threshold(double value);
	void setPo2ThresholdMin(double value);
	void setPo2ThresholdMax(double value);
	void setShowPhe(bool value);
	void setShowPn2(bool value);
	void setShowPo2(bool value);

signals:
	void pheThresholdChanged(double value);
	void pn2ThresholdChanged(double value);
	void po2ThresholdMaxChanged(double value);
	void po2ThresholdMinChanged(double value);
	void showPheChanged(bool value);
	void showPn2Changed(bool value);
	void showPo2Changed(bool value);

private:
	const QString group = QStringLiteral("TecDetails");
	static qPrefGas *m_instance;

	void diskPheThreshold(bool doSync);
	void diskPn2Threshold(bool doSync);
	void diskPo2ThresholdMin(bool doSync);
	void diskPo2ThresholdMax(bool doSync);
	void diskShowPhe(bool doSync);
	void diskShowPn2(bool doSync);
	void diskShowPo2(bool doSync);
};

#endif
