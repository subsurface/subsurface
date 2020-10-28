// SPDX-License-Identifier: GPL-2.0
#include "statsview.h"
#include <QQuickItem>
#include <QChart>
#include <QBarSet>
#include <QBarSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>

static const QUrl urlStatsView = QUrl(QStringLiteral("qrc:/qml/statsview.qml"));

StatsView::StatsView(QWidget *parent) : QQuickWidget(parent)
{
	setResizeMode(QQuickWidget::SizeRootObjectToView);
	setSource(urlStatsView);
}

StatsView::~StatsView()
{
}

// Convenience templates that turn a series-type into a series-id.
// TODO: Qt should provide that, no? Let's inspect the docs more closely...
template<typename Series> constexpr int series_type_id();
template<> constexpr int series_type_id<QtCharts::QBarSeries>()
{ return QtCharts::QAbstractSeries::SeriesTypeBar; }

// Helper function to add a series to a QML ChartView.
// Sadly, accessing QML from C++ is maliciously cumbersome.
template<typename Type>
Type *StatsView::addSeries(const QString &name, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis)
{
	QtCharts::QAbstractSeries *abstract_series = addSeriesHelper(name, series_type_id<Type>(), xAxis, yAxis);
	Type *res = qobject_cast<Type *>(abstract_series);
	if (!res)
		qWarning("Couldn't cast abstract series to %s", typeid(Type).name());

	return res;
}

QtCharts::QAbstractSeries *StatsView::addSeriesHelper(const QString &name, int type, QtCharts::QAbstractAxis *xAxis, QtCharts::QAbstractAxis *yAxis)
{
	using QtCharts::QAbstractSeries;
	using QtCharts::QAbstractAxis;

	QQuickItem *chart = rootObject();
	QAbstractSeries *abstract_series;
	if (!QMetaObject::invokeMethod(chart, "createSeries", Qt::AutoConnection,
				       Q_RETURN_ARG(QAbstractSeries *, abstract_series),
				       Q_ARG(int, type),
				       Q_ARG(QString, name),
				       Q_ARG(QAbstractAxis *, xAxis),
				       Q_ARG(QAbstractAxis *, yAxis))) {
		qWarning("Couldn't call createSeries()");
		return nullptr;
	}

	return abstract_series;
}

template <typename T>
T *StatsView::makeAxis()
{
	T *res = new T;
	axes.emplace_back(res);
	return res;
}

void StatsView::reset()
{
	QQuickItem *chart = rootObject();
	QMetaObject::invokeMethod(chart, "removeAllSeries", Qt::AutoConnection);
	axes.clear();
}

void StatsView::plot()
{
	reset();
}
