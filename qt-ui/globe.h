#ifndef GLOBE_H
#define GLOBE_H

#include <marble/MarbleWidget.h>
#include <marble/GeoDataCoordinates.h>
#include <marble/GeoDataDocument.h>

#include <QHash>

class KMessageWidget;
using namespace Marble;
struct dive;

class GlobeGPS : public MarbleWidget{
	Q_OBJECT
public:
	using MarbleWidget::centerOn;
	GlobeGPS(QWidget *parent);
	void reload();
	void repopulateLabels();
	void centerOn(struct dive* dive);
	void diveEditMode();
	bool eventFilter(QObject*, QEvent*);
protected:
	/* reimp */ void resizeEvent(QResizeEvent *event);
	/* reimp */ void mousePressEvent(QMouseEvent* event);
private:
	void prepareForGetDiveCoordinates(struct dive* dive);
	GeoDataDocument *loadedDives;
	struct dive* editingDiveCoords;
	KMessageWidget* messageWidget;
	QTimer *fixZoomTimer;
	int currentZoomLevel;

public slots:
	void changeDiveGeoPosition(qreal lon,qreal lat,GeoDataCoordinates::Unit);
	void mouseClicked(qreal lon, qreal lat, GeoDataCoordinates::Unit);
	void fixZoom();
};

#endif
