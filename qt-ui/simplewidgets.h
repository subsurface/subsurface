#ifndef SIMPLEWIDGETS_H
#define SIMPLEWIDGETS_H

class MinMaxAvgWidgetPrivate;
#include <QWidget>

class MinMaxAvgWidget : public QWidget{
	Q_OBJECT
	Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
	Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
	Q_PROPERTY(double average READ average WRITE setAverage)
public:
	MinMaxAvgWidget(QWidget *parent);
	double minimum() const;
	double maximum() const;
	double average() const;
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setAverage(double average);
	void setMinimum(const QString& minimum);
	void setMaximum(const QString& maximum);
	void setAverage(const QString& average);
	void clear();
private:
	MinMaxAvgWidgetPrivate *d;
};

#endif