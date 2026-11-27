// SPDX-License-Identifier: GPL-2.0
// AI-generated (GPT-6 Sol)
#include "testregressionrendering.h"
#include "stats/regressionitem.h"
#include "stats/statsaxis.h"
#include "stats/statsview.h"

#include <QTest>
#include <limits>

class RegressionProbe : public RegressionItem {
public:
	using RegressionItem::RegressionItem;
	QImage image() const { return img ? *img : QImage(); }
};

static int opaquePixels(const QImage &image)
{
	int count = 0;
	for (int y = 0; y < image.height(); ++y)
		for (int x = 0; x < image.width(); ++x)
			count += image.pixelColor(x, y).alpha() != 0;
	return count;
}

void TestRegressionRendering::twoPointsAndFeatures()
{
	StatsView view;
	auto x = view.createChartItem<ValueAxis>(QString(), 0.0, 10.0, 0, true);
	auto y = view.createChartItem<ValueAxis>(QString(), 0.0, 10.0, 0, false);
	x->setSize(200);
	y->setSize(200);
	x->setPos(QPointF(10, 220));
	y->setPos(QPointF(10, 220));
	regression_data data { 1.0, 0.0, 0.0, 1.0, 50.0, 5.0, 2 };
	auto item = view.createChartItem<RegressionProbe>(data, &*x, &*y);
	item->updatePosition();
	QVERIFY(opaquePixels(item->image()) > 0); // The default enables both features.
	item->setFeatures(true, false);
	QVERIFY(opaquePixels(item->image()) > 0);
	item->setFeatures(false, true);
	QCOMPARE(opaquePixels(item->image()), 0); // No confidence estimate for n=2.
	item->setFeatures(false, false);
	QCOMPARE(opaquePixels(item->image()), 0);
	item->setFeatures(true, true);
	QVERIFY(opaquePixels(item->image()) > 0);
	x->setSize(120);
	y->setSize(120);
	x->setPos(QPointF(10, 140));
	y->setPos(QPointF(10, 140));
	item->updatePosition();
	QVERIFY(opaquePixels(item->image()) > 0);
	QVERIFY(item->image().width() < 200);
	view.emergencyShutdown();
}

void TestRegressionRendering::confidenceAndClipping()
{
	StatsView view;
	const double timestamp = 1700000000.0;
	auto x = view.createChartItem<ValueAxis>(QString(), timestamp, timestamp + 10.0, 0, true);
	auto y = view.createChartItem<ValueAxis>(QString(), 0.0, 10.0, 0, false);
	x->setSize(160);
	y->setSize(160);
	x->setPos(QPointF(20, 180));
	y->setPos(QPointF(20, 180));
	regression_data data { 1.0, -timestamp, 2.0, 0.8, 50.0, timestamp + 5.0, 4 };
	auto item = view.createChartItem<RegressionProbe>(data, &*x, &*y);
	item->setFeatures(false, true);
	QVERIFY(opaquePixels(item->image()) > 0);
	QVERIFY(item->image().width() <= 162);
	item->setFeatures(true, false);
	QVERIFY(opaquePixels(item->image()) > 0);
	regression_data horizontal { 0.0, 5.0, 0.0, 1.0, 50.0, timestamp + 5.0, 4 };
	auto flat = view.createChartItem<RegressionProbe>(horizontal, &*x, &*y);
	flat->updatePosition();
	QVERIFY(opaquePixels(flat->image()) > 0);
	flat->setFeatures(false, true);
	QVERIFY(opaquePixels(flat->image()) > 0);
	regression_data clipped { 2.0, -2.0 * timestamp - 5.0, 2.0, 0.8, 50.0, timestamp + 5.0, 4 };
	auto steep = view.createChartItem<RegressionProbe>(clipped, &*x, &*y);
	steep->setFeatures(true, false);
	QVERIFY(opaquePixels(steep->image()) > 0);
	QVERIFY(steep->image().width() <= 162);
	steep->setFeatures(false, false);
	QCOMPARE(opaquePixels(steep->image()), 0);
	view.emergencyShutdown();
}

// AI-generated (GPT-6 Sol): cover rounding at both clipped endpoints.
void TestRegressionRendering::exactClippingBoundary()
{
	StatsView view;
	auto x = view.createChartItem<ValueAxis>(QString(), 0.0, 100.0, 0, true);
	auto y = view.createChartItem<ValueAxis>(QString(), 0.0, 100.0, 0, false);
	x->setSize(100);
	y->setSize(100);
	x->setRange(0.0, 100.0);
	y->setRange(0.0, 100.0);
	x->setPos(QPointF(0, 100));
	y->setPos(QPointF(0, 100));
	regression_data entering { -0.49, 101.0, 0.0, 1.0, 50.0, 50.0, 2 };
	auto first = view.createChartItem<RegressionProbe>(entering, &*x, &*y);
	first->updatePosition(); // Screen endpoints (0,-1) and (100,48).
	QVERIFY(opaquePixels(first->image()) > 0);
	regression_data leaving { 0.49, 52.0, 0.0, 1.0, 50.0, 50.0, 2 };
	auto last = view.createChartItem<RegressionProbe>(leaving, &*x, &*y);
	last->updatePosition(); // Screen endpoints (0,48) and (100,-1).
	QVERIFY(opaquePixels(last->image()) > 0);
	regression_data outside { 0.0, 101.0, 0.0, 1.0, 50.0, 50.0, 2 };
	auto hidden = view.createChartItem<RegressionProbe>(outside, &*x, &*y);
	hidden->updatePosition();
	QCOMPARE(opaquePixels(hidden->image()), 0);
	view.emergencyShutdown();
}

void TestRegressionRendering::invalidGeometry()
{
	StatsView view;
	auto x = view.createChartItem<ValueAxis>(QString(), 0.0, 10.0, 0, true);
	auto y = view.createChartItem<ValueAxis>(QString(), 0.0, 10.0, 0, false);
	x->setSize(100);
	y->setSize(100);
	x->setPos(QPointF(0, 100));
	y->setPos(QPointF(0, 100));
	regression_data bad { std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0, 1.0, 1.0, 1.0, 4 };
	auto item = view.createChartItem<RegressionProbe>(bad, &*x, &*y);
	item->updatePosition();
	QCOMPARE(opaquePixels(item->image()), 0);
	regression_data valid { 1.0, 0.0, 0.0, 1.0, 50.0, 5.0, 2 };
	auto resized = view.createChartItem<RegressionProbe>(valid, &*x, &*y);
	resized->updatePosition();
	QVERIFY(opaquePixels(resized->image()) > 0);
	x->setPos(QPointF(std::numeric_limits<double>::quiet_NaN(), 100));
	resized->updatePosition();
	QCOMPARE(opaquePixels(resized->image()), 0);
	view.emergencyShutdown();
}

QTEST_MAIN(TestRegressionRendering)
