#ifndef DEVICEDETAILS_H
#define DEVICEDETAILS_H

#include <QObject>
#include <QDateTime>
#include "libdivecomputer.h"

struct gas {
	int oxygen;
	int helium;
	int type;
	int depth;
};

struct setpoint {
	int sp;
	int depth;
};

class DeviceDetails : public QObject
{
	Q_OBJECT
public:
	explicit DeviceDetails(QObject *parent = 0);

	device_data_t *data() const;
	void setData(device_data_t *data);

	QString serialNo() const;
	void setSerialNo(const QString &serialNo);

	QString firmwareVersion() const;
	void setFirmwareVersion(const QString &firmwareVersion);

	QString customText() const;
	void setCustomText(const QString &customText);

	int brightness() const;
	void setBrightness(int brightness);

	int diveModeColor() const;
	void setDiveModeColor(int diveModeColor);

	int language() const;
	void setLanguage(int language);

	int dateFormat() const;
	void setDateFormat(int dateFormat);

	int lastDeco() const;
	void setLastDeco(int lastDeco);

	bool syncTime() const;
	void setSyncTime(bool syncTime);

	gas gas1() const;
	void setGas1(const gas &gas1);

	gas gas2() const;
	void setGas2(const gas &gas2);

	gas gas3() const;
	void setGas3(const gas &gas3);

	gas gas4() const;
	void setGas4(const gas &gas4);

	gas gas5() const;
	void setGas5(const gas &gas5);

	gas dil1() const;
	void setDil1(const gas &dil1);

	gas dil2() const;
	void setDil2(const gas &dil2);

	gas dil3() const;
	void setDil3(const gas &dil3);

	gas dil4() const;
	void setDil4(const gas &dil4);

	gas dil5() const;
	void setDil5(const gas &dil5);

	setpoint sp1() const;
	void setSp1(const setpoint &sp1);

	setpoint sp2() const;
	void setSp2(const setpoint &sp2);

	setpoint sp3() const;
	void setSp3(const setpoint &sp3);

	setpoint sp4() const;
	void setSp4(const setpoint &sp4);

	setpoint sp5() const;
	void setSp5(const setpoint &sp5);

	int ccrMode() const;
	void setCcrMode(int ccrMode);

	int diveMode() const;
	void setDiveMode(int diveMode);

	int decoType() const;
	void setDecoType(int decoType);

	int pp02Max() const;
	void setPp02Max(int pp02Max);

	int pp02Min() const;
	void setPp02Min(int pp02Min);

	int futureTTS() const;
	void setFutureTTS(int futureTTS);

	int gfLow() const;
	void setGfLow(int gfLow);

	int gfHigh() const;
	void setGfHigh(int gfHigh);

	int aGFLow() const;
	void setAGFLow(int aGFLow);

	int aGFHigh() const;
	void setAGFHigh(int aGFHigh);

	int aGFSelectable() const;
	void setAGFSelectable(int aGFSelectable);

	int saturation() const;
	void setSaturation(int saturation);

	int desaturation() const;
	void setDesaturation(int desaturation);

	int units() const;
	void setUnits(int units);

	int samplingRate() const;
	void setSamplingRate(int samplingRate);

	int salinity() const;
	void setSalinity(int salinity);

	int compassGain() const;
	void setCompassGain(int compassGain);

	int pressureSensorOffset() const;
	void setPressureSensorOffset(int pressureSensorOffset);

private:
	device_data_t *m_data;
	QString m_serialNo;
	QString m_firmwareVersion;
	QString m_customText;
	bool m_syncTime;
	gas m_gas1;
	gas m_gas2;
	gas m_gas3;
	gas m_gas4;
	gas m_gas5;
	gas m_dil1;
	gas m_dil2;
	gas m_dil3;
	gas m_dil4;
	gas m_dil5;
	setpoint m_sp1;
	setpoint m_sp2;
	setpoint m_sp3;
	setpoint m_sp4;
	setpoint m_sp5;
	int m_ccrMode;
	int m_diveMode;
	int m_decoType;
	int m_pp02Max;
	int m_pp02Min;
	int m_futureTTS;
	int m_gfLow;
	int m_gfHigh;
	int m_aGFLow;
	int m_aGFHigh;
	int m_aGFSelectable;
	int m_saturation;
	int m_desaturation;
	int m_lastDeco;
	int m_brightness;
	int m_units;
	int m_samplingRate;
	int m_salinity;
	int m_diveModeColor;
	int m_language;
	int m_dateFormat;
	int m_compassGain;
	int m_pressureSensorOffset;
};


#endif // DEVICEDETAILS_H
