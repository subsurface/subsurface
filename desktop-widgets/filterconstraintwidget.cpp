// SPDX-License-Identifier: GPL-2.0
#include "filterconstraintwidget.h"
#include "starwidget.h"
#include "core/pref.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "qt-models/cleanertablemodel.h" // for trashIcon()
#include "qt-models/filterconstraintmodel.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTimeEdit>

// Helper function to get enums through Qt's variants
template<typename T>
static T getEnum(const QModelIndex &idx, int role)
{
	return static_cast<T>(idx.data(role).value<int>());
}

// Helper function, which creates a combo box for a given model role
// or returns null if the model doesn't return a proper list.
static QComboBox *makeCombo(const QModelIndex &index, int role)
{
	QStringList list = index.data(role).value<QStringList>();
	if (list.isEmpty())
		return nullptr;
	QComboBox *res = new QComboBox;
	res->addItems(list);
	res->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	return res;
}

// Helper function, which creates a multiple choice list for a given model role
// or returns null if the model doesn't return a proper list.
static QListWidget *makeMultipleChoice(const QModelIndex &index, int role)
{
	QStringList list = index.data(role).value<QStringList>();
	if (list.isEmpty())
		return nullptr;
	QListWidget *res = new QListWidget;
	res->addItems(list);
	res->setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
	res->setSelectionMode(QAbstractItemView::ExtendedSelection);
	res->setFixedSize(res->sizeHintForColumn(0) + 2 * res->frameWidth(), res->sizeHintForRow(0) * res->count() + 2 * res->frameWidth());
	return res;
}

// Helper function to create a floating point spin box.
// Currently, this allows a large range of values.
// The limits should be adapted to the constraint in question.
static QDoubleSpinBox *makeSpinBox(int numDecimals)
{
	QDoubleSpinBox *res = new QDoubleSpinBox;
	res->setRange(-10000.0, 10000.0);
	res->setDecimals(numDecimals);
	res->setSingleStep(pow(10.0, -numDecimals));
	return res;
}

// Helper function to create a date edit widget
static QDateEdit *makeDateEdit()
{
	QDateEdit *res = new QDateEdit;
	res->setCalendarPopup(true);
	res->setTimeSpec(Qt::UTC);
	return res;
}

// Helper function to create a date edit widget
static QTimeEdit *makeTimeEdit()
{
	QTimeEdit *res = new QTimeEdit;
	res->setTimeSpec(Qt::UTC);
	return res;
}

// Helper function, which creates a label with a given string
// or returns null if the string is empty.
static QLabel *makeLabel(const QString &s)
{
	if (s.isEmpty())
		return nullptr;
	return new QLabel(s);
}

// Helper function, which sets the index of a combo box for a given
// model role, if the combo box is not null.
static void setIndex(QComboBox *w, const QModelIndex &index, int role)
{
	if (!w)
		return;
	w->setCurrentIndex(index.data(role).value<int>());
}

// Helper function to add a widget to a layout if it is non-null
static void addWidgetToLayout(QWidget *w, QHBoxLayout *l)
{
	if (w) {
		l->addWidget(w);
		l->setAlignment(w, Qt::AlignLeft);
	}
}

// Helper functions that show / hide non-null widgets
static void showWidget(QWidget *w)
{
	if (w)
		w->show();
}

// Helper functions that show / hide non-null widgets
static void hideWidget(QWidget *w)
{
	if (w)
		w->hide();
}

// Helper function to create datatimes from either a single date widget,
// or a combination of a date and a time widget.
static QDateTime makeDateTime(const QDateEdit *d, const QTimeEdit *t)
{
	if (!d)
		return QDateTime();
	if (!t)
		return d->dateTime();
	QDateTime res = d->dateTime();
	res.setTime(t->time());
	return res;
}

FilterConstraintWidget::FilterConstraintWidget(FilterConstraintModel *modelIn, const QModelIndex &index, QGridLayout *layoutIn) : QObject(layoutIn),
	layout(layoutIn),
	model(modelIn),
	row(index.row()),
	type(getEnum<filter_constraint_type>(index, FilterConstraintModel::TYPE_ROLE))
{
	rangeLayout.reset(new QHBoxLayout);
	rangeLayout->setAlignment(Qt::AlignLeft);

	trashButton.reset(new QPushButton(QIcon(trashIcon()), QString()));
	trashButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	trashButton->setToolTip(tr("Click to remove this constraint"));
	connect(trashButton.get(), &QPushButton::clicked, this, &FilterConstraintWidget::trash);

	typeLabel.reset(new QLabel(index.data(FilterConstraintModel::TYPE_DISPLAY_ROLE).value<QString>()));
	typeLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	QString unitString = index.data(FilterConstraintModel::UNIT_ROLE).value<QString>();
	negate.reset(makeCombo(index, FilterConstraintModel::NEGATE_COMBO_ROLE));
	connect(negate.get(), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterConstraintWidget::negateEdited);
	stringMode.reset(makeCombo(index, FilterConstraintModel::STRING_MODE_COMBO_ROLE));
	rangeMode.reset(makeCombo(index, FilterConstraintModel::RANGE_MODE_COMBO_ROLE));
	multipleChoice.reset(makeMultipleChoice(index, FilterConstraintModel::MULTIPLE_CHOICE_LIST_ROLE));
	bool isStarWidget = index.data(FilterConstraintModel::IS_STAR_WIDGET_ROLE).value<bool>();
	bool hasDateWidget = index.data(FilterConstraintModel::HAS_DATE_WIDGET_ROLE).value<bool>();
	bool hasTimeWidget = index.data(FilterConstraintModel::HAS_TIME_WIDGET_ROLE).value<bool>();
	bool hasSpinBox = rangeMode && !stringMode && !hasDateWidget && !hasTimeWidget && !isStarWidget && !multipleChoice;
	unitFrom.reset(makeLabel(unitString));
	unitTo.reset(makeLabel(unitString));
	if (stringMode) {
		string.reset(new QLineEdit);
		string->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	}
	int numDecimals = hasSpinBox ? index.data(FilterConstraintModel::NUM_DECIMALS_ROLE).value<int>() : 0;
	if (string)
		connect(string.get(), &QLineEdit::textEdited, this, &FilterConstraintWidget::stringEdited);

	if (multipleChoice)
		connect(multipleChoice.get(), &QListWidget::itemSelectionChanged, this, &FilterConstraintWidget::multipleChoiceEdited);

	if (stringMode)
		connect(stringMode.get(), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterConstraintWidget::stringModeEdited);

	if (rangeMode) {
		toLabel.reset(new QLabel(tr("and")));
		connect(rangeMode.get(), QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterConstraintWidget::rangeModeEdited);
	}

	if (hasSpinBox) {
		spinBoxFrom.reset(makeSpinBox(numDecimals));
		spinBoxTo.reset(makeSpinBox(numDecimals));
		connect(spinBoxFrom.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FilterConstraintWidget::fromEditedFloat);
		connect(spinBoxTo.get(), QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FilterConstraintWidget::toEditedFloat);
	}

	if (isStarWidget) {
		starFrom.reset(new StarWidget);
		starTo.reset(new StarWidget);
		connect(starFrom.get(), &StarWidget::valueChanged, this, &FilterConstraintWidget::fromEditedInt);
		connect(starTo.get(), &StarWidget::valueChanged, this, &FilterConstraintWidget::toEditedInt);
	}

	if (hasDateWidget) {
		dateFrom.reset(makeDateEdit());
		dateTo.reset(makeDateEdit());
		connect(dateFrom.get(), &QDateEdit::dateTimeChanged, this, &FilterConstraintWidget::fromEditedTimestamp);
		connect(dateTo.get(), &QDateEdit::dateTimeChanged, this, &FilterConstraintWidget::toEditedTimestamp);
	}

	if (hasTimeWidget) {
		timeFrom.reset(makeTimeEdit());
		timeTo.reset(makeTimeEdit());
		connect(timeFrom.get(), &QTimeEdit::dateTimeChanged, this, &FilterConstraintWidget::fromEditedTimestamp);
		connect(timeTo.get(), &QTimeEdit::dateTimeChanged, this, &FilterConstraintWidget::toEditedTimestamp);
	}

	addWidgetToLayout(string.get(), rangeLayout.get());
	addWidgetToLayout(starFrom.get(), rangeLayout.get());
	addWidgetToLayout(spinBoxFrom.get(), rangeLayout.get());
	addWidgetToLayout(dateFrom.get(), rangeLayout.get());
	addWidgetToLayout(timeFrom.get(), rangeLayout.get());
	addWidgetToLayout(unitFrom.get(), rangeLayout.get());
	addWidgetToLayout(toLabel.get(), rangeLayout.get());
	addWidgetToLayout(starTo.get(), rangeLayout.get());
	addWidgetToLayout(spinBoxTo.get(), rangeLayout.get());
	addWidgetToLayout(dateTo.get(), rangeLayout.get());
	addWidgetToLayout(timeTo.get(), rangeLayout.get());
	addWidgetToLayout(unitTo.get(), rangeLayout.get());
	rangeLayout->addStretch();

	// Update the widget if the settings changed to reflect new units.
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &FilterConstraintWidget::update);

	addToLayout();
	update();
}

FilterConstraintWidget::~FilterConstraintWidget()
{
}

void FilterConstraintWidget::addToLayout()
{
	// Careful: this must mirror the code in removeFromLayout() or weird things will happen.

	// Note: we add 2 to row, because the first and second row of the filter layout
	// are reserved for the title line and the fulltext widget!
	int layoutRow = row + 2;
	layout->addWidget(trashButton.get(), layoutRow, 0);
	if (!stringMode && !rangeMode && !multipleChoice) {
		// If there are no string or range modes, we rearrange to "negate / type" to get
		// a layout of the type "is not planned" or "is not logged", where the subject is
		// implicitly "dive". This presumes SVO grammar, but so does the rest of the layout.
		layout->addWidget(negate.get(), layoutRow, 1, Qt::AlignLeft);
		layout->addWidget(typeLabel.get(), layoutRow, 2, Qt::AlignLeft);
	} else {
		layout->addWidget(negate.get(), layoutRow, 2, Qt::AlignLeft);
		layout->addWidget(typeLabel.get(), layoutRow, 1, Qt::AlignLeft);
	}
	if (stringMode)
		layout->addWidget(stringMode.get(), layoutRow, 3, Qt::AlignLeft);
	else if (rangeMode)
		layout->addWidget(rangeMode.get(), layoutRow, 3, Qt::AlignLeft);
	else if (multipleChoice)
		layout->addWidget(multipleChoice.get(), layoutRow, 3, 1, 2, Qt::AlignLeft); // column span 2
	if (!multipleChoice)
		layout->addLayout(rangeLayout.get(), layoutRow, 4, Qt::AlignLeft);
}

void FilterConstraintWidget::removeFromLayout()
{
	// Careful: this must mirror the code in addToLayout() or weird things will happen.
	layout->removeWidget(trashButton.get());
	layout->removeWidget(negate.get());
	layout->removeWidget(typeLabel.get());
	if (stringMode)
		layout->removeWidget(stringMode.get());
	else if (rangeMode)
		layout->removeWidget(rangeMode.get());
	else if (multipleChoice)
		layout->removeWidget(multipleChoice.get());
	if (!multipleChoice)
		layout->removeItem(rangeLayout.get());
}

void FilterConstraintWidget::update()
{
	// The user might have changed the date and/or time format. Let's update the widgets.
	if (dateFrom)
		dateFrom->setDisplayFormat(prefs.date_format);
	if (dateTo)
		dateTo->setDisplayFormat(prefs.date_format);
	if (timeFrom)
		timeFrom->setDisplayFormat(prefs.time_format);
	if (timeTo)
		timeTo->setDisplayFormat(prefs.time_format);

	QModelIndex idx = model->index(row, 0);
	setIndex(negate.get(), idx, FilterConstraintModel::NEGATE_INDEX_ROLE);
	setIndex(stringMode.get(), idx, FilterConstraintModel::STRING_MODE_INDEX_ROLE);
	setIndex(rangeMode.get(), idx, FilterConstraintModel::RANGE_MODE_INDEX_ROLE);
	if (string)
		string->setText(idx.data(FilterConstraintModel::STRING_ROLE).value<QString>());
	if (starFrom)
		starFrom->setCurrentStars(idx.data(FilterConstraintModel::INTEGER_FROM_ROLE).value<int>());
	if (starTo)
		starTo->setCurrentStars(idx.data(FilterConstraintModel::INTEGER_TO_ROLE).value<int>());
	if (spinBoxFrom)
		spinBoxFrom->setValue(idx.data(FilterConstraintModel::FLOAT_FROM_ROLE).value<double>());
	if (spinBoxTo)
		spinBoxTo->setValue(idx.data(FilterConstraintModel::FLOAT_TO_ROLE).value<double>());
	if (dateFrom) {
		QDateTime dateTime = idx.data(FilterConstraintModel::TIMESTAMP_FROM_ROLE).value<QDateTime>();
		dateFrom->setDateTime(dateTime);
		if (timeFrom)
			timeFrom->setDateTime(dateTime);
	} else if (timeFrom) {
		timeFrom->setTime(idx.data(FilterConstraintModel::TIME_FROM_ROLE).value<QTime>());
	}
	if (dateTo) {
		QDateTime dateTime = idx.data(FilterConstraintModel::TIMESTAMP_TO_ROLE).value<QDateTime>();
		dateTo->setDateTime(dateTime);
		if (timeTo)
			timeTo->setDateTime(dateTime);
	} else if (timeTo) {
		timeTo->setTime(idx.data(FilterConstraintModel::INTEGER_TO_ROLE).value<QTime>());
	}
	if (multipleChoice) {
		uint64_t bits = idx.data(FilterConstraintModel::MULTIPLE_CHOICE_ROLE).value<uint64_t>();
		for (int i = 0; i < multipleChoice->count(); ++i)
			multipleChoice->item(i)->setSelected(bits & (1ULL << i));
	}

	// Update the unit strings in case the locale was changed
	if (unitFrom || unitTo) {
		QString unitString = idx.data(FilterConstraintModel::UNIT_ROLE).value<QString>();
		unitFrom->setText(unitString);
		unitTo->setText(unitString);
	}

	if (rangeMode) {
		switch(getEnum<filter_constraint_range_mode>(idx, FilterConstraintModel::RANGE_MODE_ROLE)) {
		case FILTER_CONSTRAINT_LESS:
			hideWidget(toLabel.get());
			hideWidget(starFrom.get());
			hideWidget(spinBoxFrom.get());
			hideWidget(dateFrom.get());
			hideWidget(timeFrom.get());
			hideWidget(unitFrom.get());
			showWidget(starTo.get());
			showWidget(spinBoxTo.get());
			showWidget(dateTo.get());
			showWidget(timeTo.get());
			showWidget(unitTo.get());
			break;
		case FILTER_CONSTRAINT_EQUAL:
		case FILTER_CONSTRAINT_GREATER:
			hideWidget(toLabel.get());
			showWidget(starFrom.get());
			showWidget(spinBoxFrom.get());
			showWidget(dateFrom.get());
			showWidget(timeFrom.get());
			showWidget(unitFrom.get());
			hideWidget(starTo.get());
			hideWidget(spinBoxTo.get());
			hideWidget(dateTo.get());
			hideWidget(timeTo.get());
			hideWidget(unitTo.get());
			break;
		case FILTER_CONSTRAINT_RANGE:
			showWidget(toLabel.get());
			showWidget(starFrom.get());
			showWidget(spinBoxFrom.get());
			showWidget(dateFrom.get());
			showWidget(timeFrom.get());
			showWidget(unitFrom.get());
			showWidget(starTo.get());
			showWidget(spinBoxTo.get());
			showWidget(dateTo.get());
			showWidget(timeTo.get());
			showWidget(unitTo.get());
			break;
		}
	}
}

void FilterConstraintWidget::stringEdited(const QString &s)
{
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, s, FilterConstraintModel::STRING_ROLE);
}

void FilterConstraintWidget::multipleChoiceEdited()
{
	// Turn selected items into bit-field
	uint64_t bits = 0;
	for (const QModelIndex &idx: multipleChoice->selectionModel()->selectedIndexes()) {
		int row = idx.row();
		if (row >= 64) {
			qWarning("FilterConstraint: multiple-choice with more than 64 entries not supported");
			continue;
		}
		bits |= 1ULL << row;
	}
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, qulonglong(bits), FilterConstraintModel::MULTIPLE_CHOICE_ROLE);
}

void FilterConstraintWidget::fromEditedInt(int i)
{
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, i, FilterConstraintModel::INTEGER_FROM_ROLE);
}

void FilterConstraintWidget::toEditedInt(int i)
{
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, i, FilterConstraintModel::INTEGER_TO_ROLE);
}

void FilterConstraintWidget::fromEditedFloat(double f)
{
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, f, FilterConstraintModel::FLOAT_FROM_ROLE);
}

void FilterConstraintWidget::toEditedFloat(double f)
{
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, f, FilterConstraintModel::FLOAT_TO_ROLE);
}

void FilterConstraintWidget::fromEditedTimestamp(const QDateTime &datetime)
{
	QModelIndex idx = model->index(row, 0);
	if (!dateFrom && timeFrom)
		model->setData(idx, timeFrom->time(), FilterConstraintModel::TIME_FROM_ROLE);
	else
		model->setData(idx, makeDateTime(dateFrom.get(), timeFrom.get()), FilterConstraintModel::TIMESTAMP_FROM_ROLE);
}

void FilterConstraintWidget::toEditedTimestamp(const QDateTime &datetime)
{
	QModelIndex idx = model->index(row, 0);
	if (!dateTo && timeTo)
		model->setData(idx, timeTo->time(), FilterConstraintModel::TIME_TO_ROLE);
	else
		model->setData(idx, makeDateTime(dateTo.get(), timeTo.get()), FilterConstraintModel::TIMESTAMP_TO_ROLE);
}

void FilterConstraintWidget::negateEdited(int i)
{
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, i, FilterConstraintModel::NEGATE_INDEX_ROLE);
}

void FilterConstraintWidget::rangeModeEdited(int i)
{
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, i, FilterConstraintModel::RANGE_MODE_INDEX_ROLE);
	update(); // Range mode may change the shown widgets
}

void FilterConstraintWidget::stringModeEdited(int i)
{
	QModelIndex idx = model->index(row, 0);
	model->setData(idx, i, FilterConstraintModel::STRING_MODE_INDEX_ROLE);
}

void FilterConstraintWidget::moveToRow(int rowIn)
{
	removeFromLayout();
	row = rowIn;
	addToLayout();
}

void FilterConstraintWidget::trash()
{
	model->deleteConstraint(row);
}
