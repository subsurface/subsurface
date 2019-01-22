#include "desktop-widgets/filterwidget2.h"
#include "desktop-widgets/simplewidgets.h"
#include "desktop-widgets/mainwindow.h"
#include "core/qthelper.h"
#include "core/settings/qPrefUnit.h"

#include <QDoubleSpinBox>

FilterWidget2::FilterWidget2(QWidget* parent) : QWidget(parent), ignoreSignal(false)
{
	ui.setupUi(this);

	FilterData data;

	// Use default values to set minimum and maximum air and water temperature.
	ui.minAirTemp->setRange(data.minAirTemp, data.maxAirTemp);
	ui.maxAirTemp->setRange(data.minAirTemp, data.maxAirTemp);
	ui.minWaterTemp->setRange(data.minWaterTemp, data.maxWaterTemp);
	ui.maxWaterTemp->setRange(data.minWaterTemp, data.maxWaterTemp);

	// TODO: unhide this when we discover how to search for equipment.
	ui.equipment->hide();
	ui.equipmentNegate->hide();
	ui.labelEquipment->hide();

	ui.fromDate->setDisplayFormat(prefs.date_format);
	ui.fromTime->setDisplayFormat(prefs.time_format);

	ui.toDate->setDisplayFormat(prefs.date_format);
	ui.toTime->setDisplayFormat(prefs.time_format);

	// Initialize temperature fields to display correct unit.
	temperatureChanged();

	connect(ui.clear, &QToolButton::clicked,
		this, &FilterWidget2::clearFilter);

	connect(ui.close, &QToolButton::clicked,
		this, &FilterWidget2::closeFilter);

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

	connect(ui.tagsNegate, &QToolButton::toggled,
		this, &FilterWidget2::updateFilter);

	connect(ui.people, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.peopleNegate, &QToolButton::toggled,
		this, &FilterWidget2::updateFilter);

	connect(ui.location, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.locationNegate, &QToolButton::toggled,
		this, &FilterWidget2::updateFilter);

	connect(ui.logged, &QCheckBox::stateChanged,
		this, &FilterWidget2::updateLogged);

	connect(ui.planned, &QCheckBox::stateChanged,
		this, &FilterWidget2::updatePlanned);

	// Update temperature fields if user changes temperature-units in preferences.
	connect(qPrefUnits::instance(), &qPrefUnits::temperatureChanged,
		this, &FilterWidget2::temperatureChanged);

	connect(qPrefUnits::instance(), &qPrefUnits::unit_systemChanged,
		this, &FilterWidget2::temperatureChanged);

	// Update counts if dives were added / removed
	connect(MultiFilterSortModel::instance(), &MultiFilterSortModel::countsChanged,
		this, &FilterWidget2::countsChanged);

	// Reset all fields.
	clearFilter();
}

void FilterWidget2::clearFilter()
{
	ignoreSignal = true; // Prevent signals to force filter recalculation
	filterData = FilterData();
	ui.minRating->setCurrentStars(filterData.minRating);
	ui.maxRating->setCurrentStars(filterData.maxRating);
	ui.minVisibility->setCurrentStars(filterData.minVisibility);
	ui.maxVisibility->setCurrentStars(filterData.maxVisibility);
	ui.minAirTemp->setValue(filterData.minAirTemp);
	ui.maxAirTemp->setValue(filterData.maxAirTemp);
	ui.minWaterTemp->setValue(filterData.minWaterTemp);
	ui.maxWaterTemp->setValue(filterData.maxWaterTemp);
	ui.planned->setChecked(filterData.logged);
	ui.planned->setChecked(filterData.planned);
	ui.people->clear();
	ui.location->clear();
	ui.equipment->clear();
	ui.tags->clear();
	ui.fromDate->setDate(filterData.fromDate.date());
	ui.fromTime->setTime(filterData.fromTime);
	ui.toDate->setDate(filterData.toDate.date());
	ui.toTime->setTime(filterData.toTime);
	ui.tagsNegate->setChecked(filterData.tagsNegate);
	ui.peopleNegate->setChecked(filterData.peopleNegate);
	ui.locationNegate->setChecked(filterData.locationNegate);
	ui.equipmentNegate->setChecked(filterData.equipmentNegate);
	ignoreSignal = false;

	filterDataChanged(filterData);
}

void FilterWidget2::closeFilter()
{
	MainWindow::instance()->setApplicationState("Default");
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
	if (ignoreSignal)
		return;

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
	filterData.tagsNegate = ui.tagsNegate->isChecked();
	filterData.peopleNegate = ui.peopleNegate->isChecked();
	filterData.locationNegate = ui.locationNegate->isChecked();
	filterData.equipmentNegate = ui.equipmentNegate->isChecked();
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
	clearFilter();
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
