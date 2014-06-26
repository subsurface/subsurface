#ifndef GLOBE_H
#define GLOBE_H
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
	GlobeGPS(QWidget *parent);
	void reload();
	void repopulateLabels();
	void centerOnCurrentDive();
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

public
slots:
	void changeDiveGeoPosition(qreal lon, qreal lat, GeoDataCoordinates::Unit);
	void mouseClicked(qreal lon, qreal lat, GeoDataCoordinates::Unit);
	void fixZoom();
	void zoomOutForNoGPS();
	void prepareForGetDiveCoordinates();
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
	void centerOnCurrentDive();
	bool eventFilter(QObject *, QEvent *);
public
slots:
	void prepareForGetDiveCoordinates();
};

#endif // NO_MARBLE
#endif // GLOBE_H
