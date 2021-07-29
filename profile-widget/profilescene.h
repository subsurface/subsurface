	// SPDX-License-Identifier: GPL-2.0
	// Displays the dive profile. Used by the interactive profile widget
	// and the printing/exporting code.
#ifndef PROFILESCENE_H
#define PROFILESCENE_H

#include <QGraphicsScene>

class DivePlotDataModel;

class DepthAxis;
class DiveCartesianAxis;
class PartialGasPressureAxis;
class TemperatureAxis;
class TimeAxis;

class ProfileScene : public QGraphicsScene {
public:
	ProfileScene(double fontPrintScale);
	~ProfileScene();

	void updateAxes(); // Update axes according to preferences
	bool isPointOutOfBoundaries(const QPointF &point) const;

	int animSpeed;
private:
	friend class ProfileWidget2; // For now, give the ProfileWidget full access to the objects on the scene
	double fontPrintScale;
	bool printMode;
	bool isGrayscale;

	DivePlotDataModel *dataModel;
	DepthAxis *profileYAxis;
	PartialGasPressureAxis *gasYAxis;
	TemperatureAxis *temperatureAxis;
	TimeAxis *timeAxis;
	DiveCartesianAxis *cylinderPressureAxis;
	DiveCartesianAxis *heartBeatAxis;
	DiveCartesianAxis *percentageAxis;
};

#endif
