#include "desktop-widgets/filterwidget2.h"
#include "desktop-widgets/simplewidgets.h"
#include "core/qthelper.h"
#include "core/settings/qPrefUnit.h"

#include <QDoubleSpinBox>

FilterWidget2::FilterWidget2(QWidget* parent) : QWidget(parent)
{
	ui.setupUi(this);

	FilterData data;

	// Use default values to set minimum and maximum air and water temperature.
	ui.minAirTemp->setRange(data.minAirTemp, data.maxAirTemp);
	ui.maxAirTemp->setRange(data.minAirTemp, data.maxAirTemp);
	ui.minWaterTemp->setRange(data.minWaterTemp, data.maxWaterTemp);
	ui.maxWaterTemp->setRange(data.minWaterTemp, data.maxWaterTemp);

	ui.minRating->setCurrentStars(data.minRating);
	ui.maxRating->setCurrentStars(data.maxRating);
	ui.minVisibility->setCurrentStars(data.minVisibility);
	ui.maxVisibility->setCurrentStars(data.maxVisibility);
	ui.minAirTemp->setValue(data.minAirTemp);
	ui.maxAirTemp->setValue(data.maxAirTemp);
	ui.minWaterTemp->setValue(data.minWaterTemp);
	ui.maxWaterTemp->setValue(data.maxWaterTemp);
	ui.planned->setChecked(data.logged);
	ui.planned->setChecked(data.planned);

	// TODO: unhide this when we discover how to search for equipment.
	ui.equipment->hide();
	ui.labelEquipment->hide();
	ui.invertFilter->hide();

	ui.fromDate->setDisplayFormat(prefs.date_format);
	ui.fromTime->setDisplayFormat(prefs.time_format);

	ui.toDate->setDisplayFormat(prefs.date_format);
	ui.toTime->setDisplayFormat(prefs.time_format);
	ui.toDate->setDate(data.toDate.date());
	ui.toTime->setTime(data.toTime);

	// Initialize temperature fields to display correct unit.
	temperatureChanged();

	connect(ui.maxRating, &StarWidget::valueChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.minRating, &StarWidget::valueChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.maxVisibility, &StarWidget::valueChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.minVisibility, &StarWidget::valueChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.maxAirTemp, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.minAirTemp, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.maxWaterTemp, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.minWaterTemp, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.fromDate, &QDateTimeEdit::dateChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.fromTime, &QDateTimeEdit::timeChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.toDate, &QDateTimeEdit::dateChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.toTime, &QDateTimeEdit::timeChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.tags, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.people, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.location, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.logged, SIGNAL(stateChanged(int)), this, SLOT(updateLogged(int)));

	connect(ui.planned, SIGNAL(stateChanged(int)), this, SLOT(updatePlanned(int)));

	// Update temperature fields if user changes temperature-units in preferences.
	connect(qPrefUnits::instance(), &qPrefUnits::temperatureChanged, this, &FilterWidget2::temperatureChanged);
	connect(qPrefUnits::instance(), &qPrefUnits::unit_systemChanged, this, &FilterWidget2::temperatureChanged);

	// Update counts if dives were added / removed
	connect(MultiFilterSortModel::instance(), &MultiFilterSortModel::countsChanged,
		this, &FilterWidget2::countsChanged);
}

void FilterWidget2::temperatureChanged()
{
	QString temp = get_temp_unit();
	ui.minAirTemp->setSuffix(temp);
	ui.maxAirTemp->setSuffix(temp);
	ui.minWaterTemp->setSuffix(temp);
	ui.maxWaterTemp->setSuffix(temp);
}

void FilterWidget2::updateFilter()
{
	filterData.validFilter = true;
	filterData.minVisibility = ui.minVisibility->currentStars();
	filterData.maxVisibility = ui.maxVisibility->currentStars();
	filterData.minRating = ui.minRating->currentStars();
	filterData.maxRating = ui.maxRating->currentStars();
	filterData.minWaterTemp = ui.minWaterTemp->value();
	filterData.maxWaterTemp = ui.maxWaterTemp->value();
	filterData.minAirTemp = ui.minAirTemp->value();
	filterData.maxWaterTemp = ui.maxWaterTemp->value();
	filterData.fromDate = ui.fromDate->dateTime();
	filterData.fromTime = ui.fromTime->time();
	filterData.toDate = ui.toDate->dateTime();
	filterData.toTime = ui.toTime->time();
	filterData.tags = ui.tags->text().split(",", QString::SkipEmptyParts);
	filterData.people = ui.people->text().split(",", QString::SkipEmptyParts);
	filterData.location = ui.location->text().split(",", QString::SkipEmptyParts);
	filterData.equipment = ui.equipment->text().split(",", QString::SkipEmptyParts);
	filterData.invertFilter = ui.invertFilter->isChecked();
	filterData.logged = ui.logged->isChecked();
	filterData.planned = ui.planned->isChecked();

	filterDataChanged(filterData);
}

void FilterWidget2::updateLogged(int value) {
	if (value == Qt::Unchecked)
		ui.planned->setChecked(true);
	updateFilter();
}

void FilterWidget2::updatePlanned(int value) {
	if (value == Qt::Unchecked)
		ui.logged->setChecked(true);
	updateFilter();
}

void FilterWidget2::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	filterDataChanged(filterData);
}

void FilterWidget2::hideEvent(QHideEvent *event)
{
	QWidget::hideEvent(event);
	FilterData data;
	filterDataChanged(data);
}

void FilterWidget2::filterDataChanged(const FilterData &data)
{
	MultiFilterSortModel::instance()->filterDataChanged(data);
	countsChanged();
}

void FilterWidget2::countsChanged()
{
	ui.filterText->setText(tr("%L1/%L2 shown").arg(MultiFilterSortModel::instance()->divesDisplayed)
						  .arg(dive_table.nr));
}
