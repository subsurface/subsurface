// SPDX-License-Identifier: GPL-2.0
#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include "statsstate.h"
#include "statsselection.h"
#include "qt-quick/chartview.h"

#include <memory>
#include <QImage>
#include <QPainter>

struct dive;
struct StatsBinner;
struct StatsBin;
struct StatsState;
struct StatsVariable;

class StatsSeries;
class CategoryAxis;
class ChartRectLineItem;
class ChartTextItem;
class CountAxis;
class HistogramAxis;
class HistogramMarker;
class QuartileMarker;
class RegressionItem;
class StatsAxis;
class StatsGrid;
class StatsTheme;
class Legend;

enum class ChartSubType : int;
enum class StatsOperation : int;
enum class ChartSortMode : int;

class StatsView : public ChartView {
	Q_OBJECT
public:
	StatsView();
	StatsView(QQuickItem *parent);
	~StatsView();

	void plot(const StatsState &state);
	void updateFeatures(const StatsState &state);	// Updates the visibility of chart features, such as legend, regression, etc.
	void restrictToSelection();
	void unrestrict();
	int restrictionCount() const;			// <0: no restriction
	void setTheme(bool dark);			// Chart must be replot for theme to become effective.
	const StatsTheme &getCurrentTheme() const;

	void replotIfVisible();
	void divesSelected(const QVector<dive *> &dives);
private:
	void plotAreaChanged(const QSizeF &size) override;
	void reset(); // clears all series and axes
	void resetPointers() override;
	void setAxes(StatsAxis *x, StatsAxis *y);
	void plotBarChart(const std::vector<dive *> &dives,
			  ChartSubType subType, ChartSortMode sortMode,
			  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			  const StatsVariable *valueVariable, const StatsBinner *valueBinner);
	void plotValueChart(const std::vector<dive *> &dives,
			    ChartSubType subType, ChartSortMode sortMode,
			    const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
			    const StatsVariable *valueVariable, StatsOperation valueAxisOperation);
	void plotDiscreteCountChart(const std::vector<dive *> &dives,
				    ChartSubType subType, ChartSortMode sortMode,
				    const StatsVariable *categoryVariable, const StatsBinner *categoryBinner);
	void plotPieChart(const std::vector<dive *> &dives, ChartSortMode sortMode,
			  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner);
	void plotDiscreteBoxChart(const std::vector<dive *> &dives,
				  const StatsVariable *categoryVariable, const StatsBinner *categoryBinner, const StatsVariable *valueVariable);
	void plotDiscreteScatter(const std::vector<dive *> &dives,
				 const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				 const StatsVariable *valueVariable);
	void plotHistogramCountChart(const std::vector<dive *> &dives,
				     ChartSubType subType,
				     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner);
	void plotHistogramValueChart(const std::vector<dive *> &dives,
				     ChartSubType subType,
				     const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				     const StatsVariable *valueVariable, StatsOperation valueAxisOperation);
	void plotHistogramStackedChart(const std::vector<dive *> &dives,
				       ChartSubType subType,
				       const StatsVariable *categoryVariable, const StatsBinner *categoryBinner,
				       const StatsVariable *valueVariable, const StatsBinner *valueBinner);
	void plotHistogramBoxChart(const std::vector<dive *> &dives,
				   const StatsVariable *categoryVariable, const StatsBinner *categoryBinner, const StatsVariable *valueVariable);
	void plotScatter(const std::vector<dive *> &dives, const StatsVariable *categoryVariable, const StatsVariable *valueVariable);
	void setTitle(const QString &);
	void updateTitlePos(); // After resizing, set title to correct position
	void plotChart();
	void updateFeatures(); // Updates the visibility of chart features, such as legend, regression, etc.

	template <typename T, class... Args>
	T *createSeries(Args&&... args);

	template <typename T, class... Args>
	T *createAxis(const QString &title, Args&&... args);

	template<typename T>
	CategoryAxis *createCategoryAxis(const QString &title, const StatsBinner &binner,
					 const std::vector<T> &bins, bool isHorizontal);
	template<typename T>
	HistogramAxis *createHistogramAxis(const QString &title, const StatsBinner &binner,
					   const std::vector<T> &bins, bool isHorizontal);
	CountAxis *createCountAxis(int maxVal, bool isHorizontal);

	// Helper functions to add feature to the chart
	void addLineMarker(double pos, double low, double high, const QPen &pen, bool isHorizontal);

	StatsState state;
	const StatsTheme *currentTheme;
	std::vector<std::unique_ptr<StatsSeries>> series;
	std::unique_ptr<StatsGrid> grid;
	std::vector<ChartItemPtr<QuartileMarker>> quartileMarkers;
	ChartItemPtr<HistogramMarker> medianMarker, meanMarker;
	StatsSeries *highlightedSeries;
	StatsAxis *xAxis, *yAxis;
	ChartItemPtr<ChartTextItem> title;
	ChartItemPtr<Legend> legend;
	Legend *draggedItem;
	ChartItemPtr<RegressionItem> regressionItem;
	ChartItemPtr<ChartRectLineItem> selectionRect;
	QPointF dragStartMouse, dragStartItem;
	SelectionModifier selectionModifier;
	std::vector<dive *> oldSelection;
	bool restrictDives;
	std::vector<dive *> restrictedDives;	// sorted by pointer for quick lookup.

	void hoverEnterEvent(QHoverEvent *event) override;
	void hoverMoveEvent(QHoverEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
};

#endif
