#include "desktop-widgets/filterwidget2.h"
#include "desktop-widgets/simplewidgets.h"

#include <QDoubleSpinBox>

FilterWidget2::FilterWidget2(QWidget* parent)
: QWidget(parent)
, ui(new Ui::FilterWidget2())
{
	ui->setupUi(this);
	ui->minRating->setCurrentStars(0);
	ui->maxRating->setCurrentStars(5);
	ui->minVisibility->setCurrentStars(0);
	ui->maxVisibility->setCurrentStars(5);

	connect(ui->maxRating, &StarWidget::valueChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui->minRating, &StarWidget::valueChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui->maxVisibility, &StarWidget::valueChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui->minVisibility, &StarWidget::valueChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui->maxAirTemp, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui->minAirTemp, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui->maxWaterTemp, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui->minWaterTemp, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui->from, &QDateTimeEdit::dateTimeChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui->to, &QDateTimeEdit::dateTimeChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui->tags, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui->people, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui->location, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);
}

void FilterWidget2::updateFilter()
{

}
