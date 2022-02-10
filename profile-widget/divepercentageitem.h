#ifndef DIVEPERCENTAGEITEM_H
#define DIVEPERCENTAGEITEM_H

#include <QGraphicsPixmapItem>

struct dive;
struct divecomputer;
struct plot_info;
class DiveCartesianAxis;

class DivePercentageItem : public QGraphicsPixmapItem {
public:
	DivePercentageItem(const DiveCartesianAxis &hAxis, const DiveCartesianAxis &vAxis);
	void replot(const dive *d, const divecomputer *dc, const plot_info &pi);
private:
	const DiveCartesianAxis &hAxis;
	const DiveCartesianAxis &vAxis;
};

#endif
