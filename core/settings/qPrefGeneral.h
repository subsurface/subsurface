// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFGENERAL_H
#define QPREFGENERAL_H

#include <QObject>


class qPrefGeneral : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool     auto_recalculate_thumbnails
				READ	autoRecalculateThumbnails
				WRITE	setAutoRecalculateThumbnails
				NOTIFY  autoRecalculateThumbnailsChanged);
	Q_PROPERTY(QString	default_cylinder
				READ	defaultCylinder
				WRITE	setDefaultCylinder
				NOTIFY  defaultCylinderChanged);
	Q_PROPERTY(QString	default_filename
				READ	defaultFilename
				WRITE	setDefaultFilename
				NOTIFY  defaultFilenameChanged);
	Q_PROPERTY(short	default_file_behavior
				READ	defaultFileBehavior
				WRITE	setDefaultFileBehavior
				NOTIFY  defaultFileBehaviorChanged);
	Q_PROPERTY(int		default_setpoint
				READ	defaultSetPoint
				WRITE	setDefaultSetPoint
				NOTIFY  defaultSetPointChanged);
	Q_PROPERTY(int		o2consumption
				READ	o2Consumption
				WRITE	setO2Consumption
				NOTIFY  o2ConsumptionChanged);
	Q_PROPERTY(int		pscr_ratio
				READ	pscrRatio
				WRITE	setPscrRatio
				NOTIFY  pscrRatioChanged);
	Q_PROPERTY(bool		use_default_file
				READ	useDefaultFile
				WRITE	setUseDefaultFile
				NOTIFY  useDefaultFileChanged);

public:
	qPrefGeneral(QObject *parent = NULL) : QObject(parent) {};
	~qPrefGeneral() {};
	static qPrefGeneral *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	bool autoRecalculateThumbnails() const;
	const QString defaultCylinder() const;
	const QString defaultFilename() const;
	short defaultFileBehavior() const;
	int defaultSetPoint() const;
	int o2Consumption() const;
	int pscrRatio() const;
	bool useDefaultFile() const;

public slots:
	void setAutoRecalculateThumbnails(bool value);
	void setDefaultCylinder(const QString& value);
	void setDefaultFilename(const QString& value);
	void setDefaultFileBehavior(short value);
	void setDefaultSetPoint(int value);
	void setO2Consumption(int value);
	void setPscrRatio(int value);
	void setUseDefaultFile(bool value);

signals:
	void autoRecalculateThumbnailsChanged(bool value);
	void defaultCylinderChanged(const QString& value);
	void defaultFilenameChanged(const QString& value);
	void defaultFileBehaviorChanged(short value);
	void defaultSetPointChanged(int value);
	void o2ConsumptionChanged(int value);
	void pscrRatioChanged(int value);
	void useDefaultFileChanged(bool value);

private:
	const QString group = QStringLiteral("GeneralSettings");
	static qPrefGeneral *m_instance;

	void diskAutoRecalculateThumbnails(bool doSync);
	void diskDefaultCylinder(bool doSync);
	void diskDefaultFilename(bool doSync);
	void diskDefaultFileBehavior(bool doSync);
	void diskDefaultSetPoint(bool doSync);
	void diskO2Consumption(bool doSync);
	void diskPscrRatio(bool doSync);
	void diskUseDefaultFile(bool doSync);
};

#endif
