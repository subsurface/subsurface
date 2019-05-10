#include "desktop-widgets/filterwidget2.h"
#include "desktop-widgets/simplewidgets.h"
#include "desktop-widgets/mainwindow.h"
#include "core/qthelper.h"
#include "core/settings/qPrefUnit.h"

#include <QDoubleSpinBox>

FilterWidget2::FilterWidget2(QWidget* parent) :
	QWidget(parent),
	ignoreSignal(false)
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
	ui.equipmentMode->hide();
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

	connect(ui.maxAirTemp, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.minAirTemp, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.maxWaterTemp, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.minWaterTemp, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
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

	connect(ui.tagsMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.people, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.peopleMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.location, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.locationMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.suit, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.suitMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &FilterWidget2::updateFilter);

	connect(ui.dnotes, &QLineEdit::textChanged,
		this, &FilterWidget2::updateFilter);

	connect(ui.dnotesMode, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
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
	ui.suit->clear();
	ui.dnotes->clear();
	ui.equipment->clear();
	ui.tags->clear();
	ui.fromDate->setDate(filterData.fromDate.date());
	ui.fromTime->setTime(filterData.fromTime);
	ui.toDate->setDate(filterData.toDate.date());
	ui.toTime->setTime(filterData.toTime);
	ui.tagsMode->setCurrentIndex((int)filterData.tagsMode);
	ui.peopleMode->setCurrentIndex((int)filterData.peopleMode);
	ui.locationMode->setCurrentIndex((int)filterData.locationMode);
	ui.suitMode->setCurrentIndex((int)filterData.suitMode);
	ui.dnotesMode->setCurrentIndex((int)filterData.dnotesMode);
	ui.equipmentMode->setCurrentIndex((int)filterData.equipmentMode);

	ignoreSignal = false;

	filterDataChanged(filterData);
}

void FilterWidget2::closeFilter()
{
	MainWindow::instance()->setApplicationState(ApplicationState::Default);
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
	filterData.suit = ui.suit->text().split(",", QString::SkipEmptyParts);
	filterData.dnotes = ui.dnotes->text().split(",", QString::SkipEmptyParts);
	filterData.equipment = ui.equipment->text().split(",", QString::SkipEmptyParts);
	filterData.tagsMode = (FilterData::Mode)ui.tagsMode->currentIndex();
	filterData.peopleMode = (FilterData::Mode)ui.peopleMode->currentIndex();
	filterData.locationMode = (FilterData::Mode)ui.locationMode->currentIndex();
	filterData.suitMode = (FilterData::Mode)ui.suitMode->currentIndex();
	filterData.dnotesMode = (FilterData::Mode)ui.dnotesMode->currentIndex();
	filterData.equipmentMode = (FilterData::Mode)ui.equipmentMode->currentIndex();
	filterData.logged = ui.logged->isChecked();
	filterData.planned = ui.planned->isChecked();

	filterDataChanged(filterData);
}

void FilterWidget2::updateLogged(int value)
{
	if (value == Qt::Unchecked)
		ui.planned->setChecked(true);
	updateFilter();
}

void FilterWidget2::updatePlanned(int value)
{
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
}

void FilterWidget2::filterDataChanged(const FilterData &data)
{
	MultiFilterSortModel::instance()->filterDataChanged(data);
}

QString FilterWidget2::shownText()
{
	if (isActive())
		return tr("%L1/%L2 shown").arg(MultiFilterSortModel::instance()->divesDisplayed)
				.arg(dive_table.nr);
	else
		return tr("%L1 dives").arg(dive_table.nr);
}

bool FilterWidget2::isActive() const
{
	return filterData.validFilter;
}
