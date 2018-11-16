#include "desktop-widgets/filterwidget2.h"
#include "desktop-widgets/simplewidgets.h"

#include <QDoubleSpinBox>

FilterWidget2::FilterWidget2(QWidget* parent)
: QWidget(parent)
, ui(new Ui::FilterWidget2())
{
	ui->setupUi(this);

	FilterData data;
	ui->minRating->setCurrentStars(data.minRating);
	ui->maxRating->setCurrentStars(data.maxRating);
	ui->minVisibility->setCurrentStars(data.minVisibility);
	ui->maxVisibility->setCurrentStars(data.maxVisibility);
	ui->minAirTemp->setValue(data.minAirTemp);
	ui->maxAirTemp->setValue(data.maxAirTemp);
	ui->minWaterTemp->setValue(data.minWaterTemp);
	ui->maxWaterTemp->setValue(data.maxWaterTemp);

	// TODO: unhide this when we discover how to search for equipment.
	ui->equipment->hide();
	ui->labelEquipment->hide();
	ui->invertFilter->hide();

	ui->to->setDate(data.to.date());

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
	FilterData data;

	data.validFilter = true;
	data.minVisibility = ui->minVisibility->currentStars();
	data.maxVisibility = ui->maxVisibility->currentStars();
	data.minRating = ui->minRating->currentStars();
	data.maxRating = ui->maxRating->currentStars();
	data.minWaterTemp = ui->minWaterTemp->value();
	data.maxWaterTemp = ui->maxWaterTemp->value();
	data.minAirTemp = ui->minAirTemp->value();
	data.maxWaterTemp = ui->maxWaterTemp->value();
	data.from = ui->from->dateTime();
	data.to = ui->to->dateTime();
	data.tags = ui->tags->text().split(",", QString::SkipEmptyParts);
	data.people = ui->people->text().split(",", QString::SkipEmptyParts);
	data.location = ui->location->text().split(",", QString::SkipEmptyParts);
	data.equipment = ui->equipment->text().split(",", QString::SkipEmptyParts);
	data.invertFilter = ui->invertFilter->isChecked();

	filterData = data;
	emit filterDataChanged(data);
}

void FilterWidget2::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	emit filterDataChanged(filterData);
}

void FilterWidget2::hideEvent(QHideEvent *event)
{
	QWidget::hideEvent(event);
	FilterData data;
	emit filterDataChanged(data);
}
