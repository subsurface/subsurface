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
	void prepareForGetDiveCoordinates(struct dive* dive);
public:
	using MarbleWidget::centerOn;
	GlobeGPS(QWidget *parent);
	void reload();
	void centerOn(struct dive* dive);
	void resizeEvent(QResizeEvent *event);

protected:
	virtual void mousePressEvent(QMouseEvent* event);

private:
	GeoDataDocument *loadedDives;
	struct dive* editingDiveCoords;
	KMessageWidget* messageWidget;

public Q_SLOTS:
	void changeDiveGeoPosition(qreal lon,qreal lat,GeoDataCoordinates::Unit);
	void mouseClicked(qreal lon, qreal lat, GeoDataCoordinates::Unit);
};

#endif
