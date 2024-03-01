// SPDX-License-Identifier: GPL-2.0
#ifndef DIVETEXTITEM_H
#define DIVETEXTITEM_H

#include <QGraphicsPixmapItem>

class QBrush;

/* A Line Item that has animated-properties. */
class DiveTextItem : public QGraphicsPixmapItem {
public:
	// Note: vertical centring is based on the actual rendered text, not on the font metrics.
	// This is fine for placing text in the "tankbar", but it will look disastrous when
	// placing text items next to each other. This may have to be fixed.
	DiveTextItem(double dpr, double scale, int alignFlags, QGraphicsItem *parent);
	void set(const QString &text, const QBrush &brush);
	const QString &text();
	static double fontHeight(double dpr, double scale);
	static std::pair<double, double> getLabelSize(double dpr, double scale, const QString &label);
	double width() const;
	double height() const;

private:
	int internalAlignFlags;
	QString internalText;
	double dpr;
	double scale;
};

#endif // DIVETEXTITEM_H
