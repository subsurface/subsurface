#ifndef SIMPLEWIDGETS_H
#define SIMPLEWIDGETS_H

class MinMaxAvgWidgetPrivate;
class QAbstractButton;

#include <QWidget>
#include <QDialog>

#include "ui_renumber.h"
#include "ui_shifttimes.h"
#include "ui_shiftimagetimes.h"

class MinMaxAvgWidget : public QWidget{
	Q_OBJECT
	Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
	Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
	Q_PROPERTY(double average READ average WRITE setAverage)
public:
	MinMaxAvgWidget(QWidget *parent);
	~MinMaxAvgWidget();
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
	QScopedPointer<MinMaxAvgWidgetPrivate> d;
};

class RenumberDialog : public QDialog {
	Q_OBJECT
public:
	static RenumberDialog *instance();
private slots:
	void buttonClicked(QAbstractButton *button);
private:
	explicit RenumberDialog(QWidget *parent);
	Ui::RenumberDialog ui;
};

class ShiftTimesDialog : public QDialog {
	Q_OBJECT
public:
	static ShiftTimesDialog *instance();
private slots:
	void buttonClicked(QAbstractButton *button);
private:
	explicit ShiftTimesDialog(QWidget *parent);
	Ui::ShiftTimesDialog ui;
};

class ShiftImageTimesDialog : public QDialog {
	Q_OBJECT
public:
	explicit ShiftImageTimesDialog(QWidget *parent);
	int amount() const;
	void setOffset(int offset);
private slots:
	void buttonClicked(QAbstractButton *button);
private:
	Ui::ShiftImageTimesDialog ui;
	int m_amount;
};

bool isGnome3Session();

#endif // SIMPLEWIDGETS_H
