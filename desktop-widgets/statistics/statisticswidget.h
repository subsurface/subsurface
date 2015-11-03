#ifndef YEARLYSTATISTICSWIDGET_H
#define YEARLYSTATISTICSWIDGET_H

#include <QGraphicsView>

class YearlyStatisticsModel;
class QModelIndex;

class YearlyStatisticsWidget : public QGraphicsView {
	Q_OBJECT
public:
	YearlyStatisticsWidget(QWidget *parent = 0);
	void setModel(YearlyStatisticsModel *m);
protected:
	virtual void resizeEvent(QResizeEvent *event);
public slots:
	void modelRowsInserted(const QModelIndex& index, int first, int last);
	void modelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
private:
	YearlyStatisticsModel *m_model;
};

#endif