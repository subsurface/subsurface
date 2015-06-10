#ifndef GLOBE_H
#define GLOBE_H

#include <stdint.h>

#ifndef NO_MARBLE
#include <marble/MarbleWidget.h>
#include <marble/GeoDataCoordinates.h>

#include <QHash>

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
	GlobeGPS(QWidget *parent = 0);
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

signals:
	void coordinatesChanged();

public
slots:
	void repopulateLabels();
	void changeDiveGeoPosition(qreal lon, qreal lat, GeoDataCoordinates::Unit);
	void mouseClicked(qreal lon, qreal lat, GeoDataCoordinates::Unit);
	void fixZoom();
	void zoomOutForNoGPS();
	void prepareForGetDiveCoordinates();
	void endGetDiveCoordinates();
	void centerOnDiveSite(uint32_t uuid);
};

#else // NO_MARBLE
/* Dummy widget for when we don't have MarbleWidget */
#include <QLabel>

class GlobeGPS : public QLabel {
	Q_OBJECT
public:
	GlobeGPS(QWidget *parent);
	void reload();
	void repopulateLabels();
	void centerOnDiveSite(uint32_t uuid);
	void centerOnCurrentDive();
	bool eventFilter(QObject *, QEvent *);
public
slots:
	void prepareForGetDiveCoordinates();
	void endGetDiveCoordinates();
};

#endif // NO_MARBLE

extern "C" double getDistance(int lat1, int lon1, int lat2, int lon2);

#endif // GLOBE_H
