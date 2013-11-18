#include "simplewidgets.h"

#include <QLabel>
#include <QLabel>
#include <QFormLayout>
#include <QIcon>
#include <QAbstractButton>
#include <QSpinBox>
#include <QButtonGroup>
#include <QDebug>
#include <QProcess>
#include <QStringList>
#include <QDebug>

#include "../dive.h"
#include "mainwindow.h"

class MinMaxAvgWidgetPrivate{
public:
	QLabel *avgIco, *avgValue;
	QLabel *minIco, *minValue;
	QLabel *maxIco, *maxValue;

	MinMaxAvgWidgetPrivate(MinMaxAvgWidget *owner){
		avgIco = new QLabel(owner);
		avgIco->setPixmap(QIcon(":/average").pixmap(16,16));
		avgIco->setToolTip(QObject::tr("Average"));
		minIco = new QLabel(owner);
		minIco->setPixmap(QIcon(":/minimum").pixmap(16,16));
		minIco->setToolTip(QObject::tr("Minimum"));
		maxIco = new QLabel(owner);
		maxIco->setPixmap(QIcon(":/maximum").pixmap(16,16));
		maxIco->setToolTip(QObject::tr("Maximum"));
		avgValue = new QLabel(owner);
		minValue = new QLabel(owner);
		maxValue = new QLabel(owner);

		QGridLayout *formLayout = new QGridLayout();
		formLayout->addWidget(maxIco, 0, 0);
		formLayout->addWidget(maxValue, 0, 1);
		formLayout->addWidget(avgIco, 1, 0);
		formLayout->addWidget(avgValue, 1, 1);
		formLayout->addWidget(minIco, 2, 0);
		formLayout->addWidget(minValue, 2, 1);
		owner->setLayout(formLayout);
	}
};

double MinMaxAvgWidget::average() const
{
	return d->avgValue->text().toDouble();
}

double MinMaxAvgWidget::maximum() const
{
	return d->maxValue->text().toDouble();
}
double MinMaxAvgWidget::minimum() const
{
	return d->minValue->text().toDouble();
}

MinMaxAvgWidget::MinMaxAvgWidget(QWidget* parent)
: d(new MinMaxAvgWidgetPrivate(this)){
}

void MinMaxAvgWidget::clear()
{
	d->avgValue->setText(QString());
	d->maxValue->setText(QString());
	d->minValue->setText(QString());
}

void MinMaxAvgWidget::setAverage(double average)
{
	d->avgValue->setText(QString::number(average));
}

void MinMaxAvgWidget::setMaximum(double maximum)
{
	d->maxValue->setText(QString::number(maximum));
}
void MinMaxAvgWidget::setMinimum(double minimum)
{
	d->minValue->setText(QString::number(minimum));
}

void MinMaxAvgWidget::setAverage(const QString& average)
{
	d->avgValue->setText(average);
}

void MinMaxAvgWidget::setMaximum(const QString& maximum)
{
	d->maxValue->setText(maximum);
}

void MinMaxAvgWidget::setMinimum(const QString& minimum)
{
	d->minValue->setText(minimum);
}

RenumberDialog* RenumberDialog::instance()
{
	static RenumberDialog* self = new RenumberDialog();
	return self;
}

void RenumberDialog::buttonClicked(QAbstractButton* button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole){
		qDebug() << "Renumbering.";
		renumber_dives(ui.spinBox->value());
	}
}

RenumberDialog::RenumberDialog(): QDialog()
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

ShiftTimesDialog* ShiftTimesDialog::instance()
{
	static ShiftTimesDialog* self = new ShiftTimesDialog();
	return self;
}

void ShiftTimesDialog::buttonClicked(QAbstractButton* button)
{
	int amount;

	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole){
		amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
		if (ui.backwards->isChecked())
			amount *= -1;
		if (amount != 0) {
			// DANGER, DANGER - this could get our dive_table unsorted...
			shift_times(amount);
			sort_table(&dive_table);
			mark_divelist_changed(TRUE);
			mainWindow()->dive_list()->rememberSelection();
			mainWindow()->refreshDisplay();
			mainWindow()->dive_list()->restoreSelection();
		}
	}
}

ShiftTimesDialog::ShiftTimesDialog(): QDialog()
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

bool isGnome3Session()
{
#if defined(QT_OS_WIW) || defined(QT_OS_MAC)
	return false;
#else
	if (qApp->style()->objectName() != "gtk+")
		return false;
	QProcess p;
	p.start("pidof", QStringList() << "gnome-shell" );
	p.waitForFinished(-1);
	QString p_stdout = p.readAllStandardOutput();
	return !p_stdout.isEmpty();
#endif
}
