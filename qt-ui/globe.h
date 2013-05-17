#ifndef GLOBE_H
#define GLOBE_H

#include <marble/MarbleWidget.h>
#include <QHash>

namespace Marble{
	class GeoDataDocument;
}
class GlobeGPS : public Marble::MarbleWidget{
	Q_OBJECT
public:
	using Marble::MarbleWidget::centerOn;
	GlobeGPS(QWidget *parent);
	void reload();
	void centerOn(struct dive* dive);

private:
	Marble::GeoDataDocument *loadedDives;
	QStringList diveLocations;
	
};

#endif
