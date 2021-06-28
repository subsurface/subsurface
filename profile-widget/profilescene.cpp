// SPDX-License-Identifier: GPL-2.0
#include "profilescene.h"
#include "divecartesianaxis.h"
#include "core/pref.h"
#include "qt-models/diveplotdatamodel.h"

const static struct ProfileItemPos {
	struct Pos {
		QPointF on;
		QPointF off;
	};
	struct Axis : public Pos {
		QLineF shrinked;
		QLineF expanded;
		QLineF intermediate;
	};
	Axis depth;
	Axis partialPressure;
	Axis partialPressureTissue;
	Axis partialPressureWithTankBar;
	Axis percentage;
	Axis percentageWithTankBar;
	Axis time;
	Axis cylinder;
	Axis temperature;
	Axis temperatureAll;
	Axis heartBeat;
	Axis heartBeatWithTankBar;
	ProfileItemPos();
} itemPos;

ProfileScene::ProfileScene(double fontPrintScale) :
	dataModel(new DivePlotDataModel(this)),
	profileYAxis(new DepthAxis(fontPrintScale, *this)),
	gasYAxis(new PartialGasPressureAxis(*dataModel, fontPrintScale, *this)),
	temperatureAxis(new TemperatureAxis(fontPrintScale, *this)),
	timeAxis(new TimeAxis(fontPrintScale, *this)),
	cylinderPressureAxis(new DiveCartesianAxis(fontPrintScale, *this)),
	heartBeatAxis(new DiveCartesianAxis(fontPrintScale, *this)),
	percentageAxis(new DiveCartesianAxis(fontPrintScale, *this))
{
	setSceneRect(0, 0, 100, 100);
	setItemIndexMethod(QGraphicsScene::NoIndex);

	// Initialize axes. Perhaps part of this should be moved down to the axes code?
	profileYAxis->setOrientation(DiveCartesianAxis::TopToBottom);
	profileYAxis->setMinimum(0);
	profileYAxis->setTickInterval(M_OR_FT(10, 30));
	profileYAxis->setTickSize(0.5);
	profileYAxis->setLineSize(96);

	timeAxis->setLineSize(92);
	timeAxis->setTickSize(-0.5);

	gasYAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	gasYAxis->setTickInterval(1);
	gasYAxis->setTickSize(1);
	gasYAxis->setMinimum(0);
	gasYAxis->setFontLabelScale(0.7);
	gasYAxis->setLineSize(96);

#ifndef SUBSURFACE_MOBILE
	heartBeatAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	heartBeatAxis->setTickSize(0.2);
	heartBeatAxis->setTickInterval(10);
	heartBeatAxis->setFontLabelScale(0.7);
	heartBeatAxis->setLineSize(96);

	percentageAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	percentageAxis->setTickSize(0.2);
	percentageAxis->setTickInterval(10);
	percentageAxis->setFontLabelScale(0.7);
	percentageAxis->setLineSize(96);
#endif

	temperatureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	temperatureAxis->setTickSize(2);
	temperatureAxis->setTickInterval(300);

	cylinderPressureAxis->setOrientation(DiveCartesianAxis::BottomToTop);
	cylinderPressureAxis->setTickSize(2);
	cylinderPressureAxis->setTickInterval(30000);

	heartBeatAxis->setTextVisible(true);
	heartBeatAxis->setLinesVisible(true);
	percentageAxis->setTextVisible(true);
	percentageAxis->setLinesVisible(true);

	temperatureAxis->setTextVisible(false);
	temperatureAxis->setLinesVisible(false);
	cylinderPressureAxis->setTextVisible(false);
	cylinderPressureAxis->setLinesVisible(false);
	timeAxis->setLinesVisible(true);
	profileYAxis->setLinesVisible(true);
	gasYAxis->setZValue(timeAxis->zValue() + 1);

	// Add items to scene
	addItem(profileYAxis);
	addItem(gasYAxis);
	addItem(temperatureAxis);
	addItem(timeAxis);
	addItem(cylinderPressureAxis);
	addItem(percentageAxis);
	addItem(heartBeatAxis);
}

ProfileScene::~ProfileScene()
{
}

static bool ppGraphsEnabled()
{
	return prefs.pp_graphs.po2 || prefs.pp_graphs.pn2 || prefs.pp_graphs.phe;
}

void ProfileScene::updateAxes()
{
	profileYAxis->setPos(itemPos.depth.on);

#ifndef SUBSURFACE_MOBILE
	gasYAxis->update();	// Initialize ticks of partial pressure graph
	if ((prefs.percentagegraph||prefs.hrgraph) && ppGraphsEnabled()) {
		profileYAxis->animateChangeLine(itemPos.depth.shrinked);
		temperatureAxis->setPos(itemPos.temperatureAll.on);
		temperatureAxis->animateChangeLine(itemPos.temperature.shrinked);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.shrinked);

		if (prefs.tankbar) {
			percentageAxis->setPos(itemPos.percentageWithTankBar.on);
			percentageAxis->animateChangeLine(itemPos.percentageWithTankBar.expanded);
			heartBeatAxis->setPos(itemPos.heartBeatWithTankBar.on);
			heartBeatAxis->animateChangeLine(itemPos.heartBeatWithTankBar.expanded);
		} else {
			percentageAxis->setPos(itemPos.percentage.on);
			percentageAxis->animateChangeLine(itemPos.percentage.expanded);
			heartBeatAxis->setPos(itemPos.heartBeat.on);
			heartBeatAxis->animateChangeLine(itemPos.heartBeat.expanded);
		}
		gasYAxis->setPos(itemPos.partialPressureTissue.on);
		gasYAxis->animateChangeLine(itemPos.partialPressureTissue.expanded);
	} else if (ppGraphsEnabled() || prefs.hrgraph || prefs.percentagegraph) {
		profileYAxis->animateChangeLine(itemPos.depth.intermediate);
		temperatureAxis->setPos(itemPos.temperature.on);
		temperatureAxis->animateChangeLine(itemPos.temperature.intermediate);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.intermediate);
		if (prefs.tankbar) {
			percentageAxis->setPos(itemPos.percentageWithTankBar.on);
			percentageAxis->animateChangeLine(itemPos.percentageWithTankBar.expanded);
			gasYAxis->setPos(itemPos.partialPressureWithTankBar.on);
			gasYAxis->animateChangeLine(itemPos.partialPressureWithTankBar.expanded);
			heartBeatAxis->setPos(itemPos.heartBeatWithTankBar.on);
			heartBeatAxis->animateChangeLine(itemPos.heartBeatWithTankBar.expanded);
		} else {
			gasYAxis->setPos(itemPos.partialPressure.on);
			gasYAxis->animateChangeLine(itemPos.partialPressure.expanded);
			percentageAxis->setPos(itemPos.percentage.on);
			percentageAxis->animateChangeLine(itemPos.percentage.expanded);
			heartBeatAxis->setPos(itemPos.heartBeat.on);
			heartBeatAxis->animateChangeLine(itemPos.heartBeat.expanded);
		}
	} else {
#else
	{
#endif
		profileYAxis->animateChangeLine(itemPos.depth.expanded);
		if (prefs.tankbar) {
			temperatureAxis->setPos(itemPos.temperatureAll.on);
		} else {
			temperatureAxis->setPos(itemPos.temperature.on);
		}
		temperatureAxis->animateChangeLine(itemPos.temperature.expanded);
		cylinderPressureAxis->animateChangeLine(itemPos.cylinder.expanded);
	}

	timeAxis->setPos(itemPos.time.on);
	timeAxis->setLine(itemPos.time.expanded);

	cylinderPressureAxis->setPos(itemPos.cylinder.on);
}

bool ProfileScene::isPointOutOfBoundaries(const QPointF &point) const
{
	double xpos = timeAxis->valueAt(point);
	double ypos = profileYAxis->valueAt(point);
	return xpos > timeAxis->maximum() ||
	       xpos < timeAxis->minimum() ||
	       ypos > profileYAxis->maximum() ||
	       ypos < profileYAxis->minimum();
}

ProfileItemPos::ProfileItemPos()
{
	// Scene is *always* (double) 100 / 100.
	// TODO: The relative scaling doesn't work well with large or small
	// profiles and needs to be fixed.

	//Depth Axis Config
	depth.on.setX(3);
	depth.on.setY(3);
	depth.off.setX(-2);
	depth.off.setY(3);
	depth.expanded.setP1(QPointF(0, 0));
#ifndef SUBSURFACE_MOBILE
	depth.expanded.setP2(QPointF(0, 85));
#else
	depth.expanded.setP2(QPointF(0, 65));
#endif
	depth.shrinked.setP1(QPointF(0, 0));
	depth.shrinked.setP2(QPointF(0, 55));
	depth.intermediate.setP1(QPointF(0, 0));
	depth.intermediate.setP2(QPointF(0, 65));

	// Time Axis Config
	time.on.setX(3);
#ifndef SUBSURFACE_MOBILE
	time.on.setY(95);
#else
	time.on.setY(89.5);
#endif
	time.off.setX(3);
	time.off.setY(110);
	time.expanded.setP1(QPointF(0, 0));
	time.expanded.setP2(QPointF(94, 0));

	// Partial Gas Axis Config
	partialPressure.on.setX(97);
#ifndef SUBSURFACE_MOBILE
	partialPressure.on.setY(75);
#else
	partialPressure.on.setY(70);
#endif
	partialPressure.off.setX(110);
	partialPressure.off.setY(63);
	partialPressure.expanded.setP1(QPointF(0, 0));
#ifndef SUBSURFACE_MOBILE
	partialPressure.expanded.setP2(QPointF(0, 19));
#else
	partialPressure.expanded.setP2(QPointF(0, 20));
#endif
	partialPressureWithTankBar = partialPressure;
	partialPressureWithTankBar.expanded.setP2(QPointF(0, 17));
	partialPressureTissue = partialPressure;
	partialPressureTissue.on.setX(97);
	partialPressureTissue.on.setY(65);
	partialPressureTissue.expanded.setP2(QPointF(0, 16));

	// cylinder axis config
	cylinder.on.setX(3);
	cylinder.on.setY(20);
	cylinder.off.setX(-10);
	cylinder.off.setY(20);
	cylinder.expanded.setP1(QPointF(0, 15));
	cylinder.expanded.setP2(QPointF(0, 50));
	cylinder.shrinked.setP1(QPointF(0, 0));
	cylinder.shrinked.setP2(QPointF(0, 20));
	cylinder.intermediate.setP1(QPointF(0, 0));
	cylinder.intermediate.setP2(QPointF(0, 20));

	// Temperature axis config
	temperature.on.setX(3);
	temperature.off.setX(-10);
	temperature.off.setY(40);
	temperature.expanded.setP1(QPointF(0, 20));
	temperature.expanded.setP2(QPointF(0, 33));
	temperature.shrinked.setP1(QPointF(0, 2));
	temperature.shrinked.setP2(QPointF(0, 12));
#ifndef SUBSURFACE_MOBILE
	temperature.on.setY(60);
	temperatureAll.on.setY(51);
	temperature.intermediate.setP1(QPointF(0, 2));
	temperature.intermediate.setP2(QPointF(0, 12));
#else
	temperature.on.setY(51);
	temperatureAll.on.setY(47);
	temperature.intermediate.setP1(QPointF(0, 2));
	temperature.intermediate.setP2(QPointF(0, 12));
#endif

	// Heart rate axis config
	heartBeat.on.setX(3);
	heartBeat.on.setY(82);
	heartBeat.expanded.setP1(QPointF(0, 0));
	heartBeat.expanded.setP2(QPointF(0, 10));
	heartBeatWithTankBar = heartBeat;
	heartBeatWithTankBar.expanded.setP2(QPointF(0, 7));

	// Percentage axis config
	percentage.on.setX(3);
	percentage.on.setY(80);
	percentage.expanded.setP1(QPointF(0, 0));
	percentage.expanded.setP2(QPointF(0, 15));
	percentageWithTankBar = percentage;
	percentageWithTankBar.expanded.setP2(QPointF(0, 11.9));
}
