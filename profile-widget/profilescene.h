// SPDX-License-Identifier: GPL-2.0
// Displays the dive profile. Used by the interactive profile widget
// and the printing/exporting code.
#ifndef PROFILESCENE_H
#define PROFILESCENE_H

#include "core/color.h"
#include "core/profile.h"

#include <QGraphicsScene>
#include <QPainter>
#include <memory>

class AbstractProfilePolygonItem;
class DiveCalculatedCeiling;
class DiveCalculatedTissue;
class DiveCartesianAxis;
class DiveEventItem;
class DiveGasPressureItem;
class DiveHeartrateItem;
class DiveMeanDepthItem;
class DivePercentageItem;
class DivePixmaps;
class DivePlannerPointsModel;
class DiveProfileItem;
class DiveReportedCeiling;
class DiveTemperatureItem;
class DiveTextItem;
class PartialPressureGasItem;
class TankItem;

class ProfileScene : public QGraphicsScene {
public:
	ProfileScene(double dpr, bool printMode, bool isGrayscale);
	~ProfileScene();

	void resize(QSizeF size);
	void clear();
	bool pointOnProfile(const QPointF &point) const;
	void anim(double fraction); // Called by the animation with 0.0-1.0 (start to stop).
				    // Can be compared with literal 1.0 to determine "end" state.

	// If a plannerModel is passed, the deco-information is taken from there.
	void plotDive(const struct dive *d, int dc, int animSpeed, DivePlannerPointsModel *plannerModel = nullptr, bool inPlanner = false,
		      bool keepPlotInfo = false, bool calcMax = true, double zoom = 1.0, double zoomedPosition = 0.0);

	void draw(QPainter *painter, const QRect &pos,
		  const struct dive *d, int dc,
		  DivePlannerPointsModel *plannerModel = nullptr, bool inPlanner = false);
	double calcZoomPosition(double zoom, double originalPos, double delta);
	const plot_info &getPlotInfo() const;
	int timeAt(QPointF pos) const;

	const struct dive *d;
	int dc;
private:
	using DataAccessor = double (*)(const plot_data &data);
	template<typename T, class... Args> T *createItem(const DiveCartesianAxis &vAxis, DataAccessor accessor, int z, Args&&... args);
	PartialPressureGasItem *createPPGas(DataAccessor accessor, color_index_t color, color_index_t colorAlert,
					    const double *thresholdSettingsMin, const double *thresholdSettingsMax);
	template <int ACT, int MAX> void addTissueItems(double dpr);
	void updateVisibility(bool diveHasHeartBeat, bool simplified); // Update visibility of non-interactive chart features according to preferences
	void updateAxes(bool diveHasHeartBeat, bool simplified); // Update axes according to preferences

	double dpr; // Device Pixel Ratio. A DPR of one corresponds to a "standard" PC screen.
	bool printMode;
	bool isGrayscale;
	bool empty; // The profile currently shows nothing.
	int maxtime;
	int maxdepth;

	struct plot_info plotInfo;
	QRectF profileRegion; // Region inside the axes, where the crosshair is painted in plan and edit mode.
	DiveCartesianAxis *profileYAxis;
	DiveCartesianAxis *gasYAxis;
	DiveCartesianAxis *temperatureAxis;
	DiveCartesianAxis *timeAxis;
	DiveCartesianAxis *cylinderPressureAxis;
	DiveCartesianAxis *heartBeatAxis;
	DiveCartesianAxis *percentageAxis;
	std::vector<AbstractProfilePolygonItem *> profileItems;
	DiveProfileItem *diveProfileItem;
	DiveTemperatureItem *temperatureItem;
	DiveMeanDepthItem *meanDepthItem;
	DiveGasPressureItem *gasPressureItem;
	std::vector<std::unique_ptr<DiveEventItem>> eventItems;
	DiveTextItem *diveComputerText;
	DiveReportedCeiling *reportedCeiling;
	PartialPressureGasItem *pn2GasItem;
	PartialPressureGasItem *pheGasItem;
	PartialPressureGasItem *po2GasItem;
	PartialPressureGasItem *o2SetpointGasItem;
	PartialPressureGasItem *ccrsensor1GasItem;
	PartialPressureGasItem *ccrsensor2GasItem;
	PartialPressureGasItem *ccrsensor3GasItem;
	PartialPressureGasItem *ocpo2GasItem;
	DiveCalculatedCeiling *diveCeiling;
	DiveTextItem *decoModelParameters;
	std::vector<DiveCalculatedTissue *> allTissues;
	DiveHeartrateItem *heartBeatItem;
	DivePercentageItem *percentageItem;
	TankItem *tankItem;
	std::shared_ptr<const DivePixmaps> pixmaps;
	std::vector<DiveCartesianAxis *> animatedAxes;
};

#endif
