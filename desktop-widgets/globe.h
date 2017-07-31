// SPDX-License-Identifier: GPL-2.0
#ifndef GLOBE_H
#define GLOBE_H

#include <stdint.h>

#ifndef NO_MARBLE
#include <marble/MarbleWidget.h>
#include <marble/GeoDataCoordinates.h>
#include <marble/GeoDataStyle.h>

#include <QHash>
#include <QSharedPointer>

namespace Marble{
	class GeoDataDocument;
}

class KMessageWidget;
using namespace Marble;
struct dive;

class GlobeGPS : public MarbleWidget {
	Q_OBJECT
public:
	using MarbleWidget::centerOn;
	static GlobeGPS *instance();
	void reload();
	bool eventFilter(QObject *, QEvent *);

protected:
	/* reimp */ void resizeEvent(QResizeEvent *event);
	/* reimp */ void mousePressEvent(QMouseEvent *event);
	/* reimp */ void contextMenuEvent(QContextMenuEvent *);

private:
	GeoDataDocument *loadedDives;
	KMessageWidget *messageWidget;
	QTimer *fixZoomTimer;
	int currentZoomLevel;
	bool needResetZoom;
	bool editingDiveLocation;
	bool doubleClick;
	QSharedPointer<GeoDataStyle> otherSite;
	QSharedPointer<GeoDataStyle> currentSite;
	GlobeGPS(QWidget *parent = 0);

signals:
	void coordinatesChanged();

public
slots:
	void repopulateLabels();
	void changeDiveGeoPosition(qreal lon, qreal lat, GeoDataCoordinates::Unit);
	void mouseClicked(qreal lon, qreal lat, GeoDataCoordinates::Unit);
	void fixZoom(bool now = false);
	void zoomOutForNoGPS();
	void prepareForGetDiveCoordinates();
	void endGetDiveCoordinates();
	void centerOnDiveSite(struct dive_site *ds);
	void centerOnIndex(const QModelIndex& idx);
};

#endif // NO_MARBLE

extern "C" double getDistance(int lat1, int lon1, int lat2, int lon2);

#endif // GLOBE_H
