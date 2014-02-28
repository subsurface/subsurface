#ifndef SIMPLEWIDGETS_H
#define SIMPLEWIDGETS_H

class MinMaxAvgWidgetPrivate;
class QAbstractButton;

#include <QWidget>
#include <QDialog>

#include "ui_renumber.h"
#include "ui_shifttimes.h"
#include "ui_shiftimagetimes.h"
#include "exif.h"

class MinMaxAvgWidget : public QWidget {
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
	void setMinimum(const QString &minimum);
	void setMaximum(const QString &maximum);
	void setAverage(const QString &average);
	void clear();

private:
	QScopedPointer<MinMaxAvgWidgetPrivate> d;
};

class RenumberDialog : public QDialog {
	Q_OBJECT
public:
	static RenumberDialog *instance();
private
slots:
	void buttonClicked(QAbstractButton *button);

private:
	explicit RenumberDialog(QWidget *parent);
	Ui::RenumberDialog ui;
};

class ShiftTimesDialog : public QDialog {
	Q_OBJECT
public:
	static ShiftTimesDialog *instance();
private
slots:
	void buttonClicked(QAbstractButton *button);

private:
	explicit ShiftTimesDialog(QWidget *parent);
	Ui::ShiftTimesDialog ui;
};

class ShiftImageTimesDialog : public QDialog {
	Q_OBJECT
public:
	explicit ShiftImageTimesDialog(QWidget *parent);
	time_t amount() const;
	void setOffset(time_t offset);
	time_t epochFromExiv(EXIFInfo *exif);
private
slots:
	void buttonClicked(QAbstractButton *button);
	void syncCameraClicked();
	void dcDateTimeChanged(const QDateTime &);

private:
	Ui::ShiftImageTimesDialog ui;
	time_t m_amount;
	time_t dcImageEpoch;
};

bool isGnome3Session();

#endif // SIMPLEWIDGETS_H
