#include "desktop-widgets/filterwidget.h"
#include "desktop-widgets/filterconstraintwidget.h"
#include "desktop-widgets/simplewidgets.h"
#include "desktop-widgets/mainwindow.h"
#include "commands/command.h"
#include "core/qthelper.h"
#include "core/divelist.h"
#include "core/settings/qPrefUnit.h"
#include "qt-models/filterpresetmodel.h"

FilterWidget::FilterWidget(QWidget* parent) :
	QWidget(parent),
	ignoreSignal(false),
	presetModified(false)
{
	ui.setupUi(this);

	QMenu *newConstraintMenu = new QMenu(this);
	QStringList constraintTypes = filter_constraint_type_list_translated();
	for (int i = 0; i < constraintTypes.size(); ++i) {
		filter_constraint_type type = filter_constraint_type_from_index(i);
		newConstraintMenu->addAction(constraintTypes[i], [this, type]() { addConstraint(type); });
	}
	ui.addConstraintButton->setMenu(newConstraintMenu);
	ui.addConstraintButton->setPopupMode(QToolButton::InstantPopup);
	ui.constraintTable->setColumnStretch(4, 1); // The fifth column is were the actual constraint resides - stretch that.

	ui.loadSetButton->setPopupMode(QToolButton::InstantPopup);

	ui.presetTable->setModel(FilterPresetModel::instance());
	ui.presetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui.presetTable->setSelectionMode(QAbstractItemView::SingleSelection);
	ui.presetTable->horizontalHeader()->setStretchLastSection(true);
	ui.presetTable->resizeColumnsToContents();

	ui.currentSet->setTextFormat(Qt::PlainText);

	connect(ui.clear, &QToolButton::clicked, this, &FilterWidget::clearFilter);
	connect(ui.close, &QToolButton::clicked, this, &FilterWidget::closeFilter);
	connect(ui.fullText, &QLineEdit::textChanged, this, &FilterWidget::filterChanged);
	connect(ui.fulltextStringMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FilterWidget::filterChanged);
	connect(ui.presetTable, &QTableView::clicked, this, &FilterWidget::presetClicked);
	connect(ui.presetTable->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FilterWidget::presetSelected);

	connect(&constraintModel, &FilterConstraintModel::rowsInserted, this, &FilterWidget::constraintAdded);
	connect(&constraintModel, &FilterConstraintModel::rowsRemoved, this, &FilterWidget::constraintRemoved);
	connect(&constraintModel, &FilterConstraintModel::dataChanged, this, &FilterWidget::constraintChanged);
	connect(&constraintModel, &FilterConstraintModel::modelReset, this, &FilterWidget::constraintsReset);

	// QDataWidgetMapper might be the more civilized way to keep the menus up to data.
	// For now, let's be blunt and fully reload the context menu if the presets list changes.
	// This gives us more flexibility in populating the menus.
	QAbstractItemModel *presetModel = FilterPresetModel::instance();
	connect(presetModel, &QAbstractItemModel::rowsInserted, this, &FilterWidget::updatePresetMenu);
	connect(presetModel, &QAbstractItemModel::rowsRemoved, this, &FilterWidget::updatePresetMenu);
	connect(presetModel, &QAbstractItemModel::dataChanged, this, &FilterWidget::updatePresetMenu);
	connect(presetModel, &QAbstractItemModel::modelReset, this, &FilterWidget::updatePresetMenu);

	clearFilter();
	updatePresetMenu();
}

FilterWidget::~FilterWidget()
{
}

void FilterWidget::updatePresetMenu()
{
	loadFilterPresetMenu.reset(new QMenu);
	QAbstractItemModel *model = FilterPresetModel::instance();
	int count = model->rowCount(QModelIndex());
	if (count == 0) {
		ui.loadSetButton->setEnabled(false);
		return;
	}
	ui.loadSetButton->setEnabled(true);
	for (int i = 0; i < count; ++i) {
		QModelIndex idx = model->index(i, FilterPresetModel::NAME);
		QString name = model->data(idx, Qt::DisplayRole).value<QString>();
		QAction *action = new QAction(loadFilterPresetMenu.get());
		action->setIconText(name);
		connect(action, &QAction::triggered, [this,i]() { selectPreset(i); });
		loadFilterPresetMenu->addAction(action);
	}
	ui.loadSetButton->setMenu(loadFilterPresetMenu.get());
}

void FilterWidget::selectPreset(int i)
{
	QAbstractItemModel *model = FilterPresetModel::instance();
	QItemSelectionModel *selectionModel = ui.presetTable->selectionModel();
	QModelIndex idx = model->index(i, 0);
	selectionModel->reset();
	selectionModel->select(idx, QItemSelectionModel::Select | QItemSelectionModel::Rows);
}

void FilterWidget::loadPreset(int index)
{
	ignoreSignal = true; // When reloading the filter UI, we get numerous constraintChanged signals. Ignore them.
	FilterData filter = filter_preset_get(index);
	setFilterData(filter);
	ignoreSignal = false;
	presetModified = false;
	updateFilter();
}

void FilterWidget::constraintAdded(const QModelIndex &parent, int first, int last)
{
	if (parent.isValid() || last < first)
		return; // We only support one level
	constraintWidgets.reserve(constraintWidgets.size() + 1 + last - first);
	for (int i = last + 1; i < (int)constraintWidgets.size(); ++i)
		constraintWidgets[i]->moveToRow(i);
	for (int i = first; i <= last; ++i) {
		QModelIndex idx = constraintModel.index(i, 0);
		constraintWidgets.emplace(constraintWidgets.begin() + i, new FilterConstraintWidget(&constraintModel, idx, ui.constraintTable));
	}
	filterChanged();
}

void FilterWidget::constraintRemoved(const QModelIndex &parent, int first, int last)
{
	if (parent.isValid() || last < first)
		return; // We only support one level
	constraintWidgets.erase(constraintWidgets.begin() + first, constraintWidgets.begin() + last + 1);
	for (int i = first; i < (int)constraintWidgets.size(); ++i)
		constraintWidgets[i]->moveToRow(i);
	filterChanged();
}

void FilterWidget::presetClicked(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	if (index.column() == FilterPresetModel::REMOVE)
		Command::removeFilterPreset(index.row());
}

void FilterWidget::presetSelected(const QItemSelection &selected, const QItemSelection &)
{
	if (selected.indexes().isEmpty())
		return clearFilter();
	const QModelIndex index = selected.indexes()[0];
	if (!index.isValid())
		return clearFilter();
	loadPreset(index.row());
}

void FilterWidget::constraintChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
	// Note: this may appear strange, but we don't update the widget if we get
	// a constraint-changed signal from the model. The reason being that the user
	// is currently editing the constraint and we don't want to bother them by
	// overwriting strings with canonicalized data.
	filterChanged();
}

void FilterWidget::constraintsReset()
{
	constraintWidgets.clear();
	int count = constraintModel.rowCount(QModelIndex());
	for (int i = 0; i < count; ++i) {
		QModelIndex idx = constraintModel.index(i, 0);
		constraintWidgets.emplace_back(new FilterConstraintWidget(&constraintModel, idx, ui.constraintTable));
	}
	updateFilter();
}

void FilterWidget::clearFilter()
{
	ignoreSignal = true; // Prevent signals to force filter recalculation (TODO: check if necessary)
	presetModified = false;
	ui.presetTable->selectionModel()->reset(); // Note: we use reset(), because that doesn't emit signals.
	ui.fulltextStringMode->setCurrentIndex((int)StringFilterMode::STARTSWITH);
	ui.fullText->clear();
	ui.presetTable->clearSelection();
	ignoreSignal = false;
	constraintModel.reload({}); // Causes a filter reload
}

void FilterWidget::closeFilter()
{
	MainWindow::instance()->setApplicationState(MainWindow::ApplicationState::Default);
}

FilterData FilterWidget::createFilterData() const
{
	FilterData filterData;
	filterData.fulltextStringMode = (StringFilterMode)ui.fulltextStringMode->currentIndex();
	filterData.fullText = ui.fullText->text();
	filterData.constraints = constraintModel.getConstraints();
	return filterData;
}

void FilterWidget::setFilterData(const FilterData &filterData)
{
	ui.fulltextStringMode->setCurrentIndex((int)filterData.fulltextStringMode);
	ui.fullText->setText(filterData.fullText.originalQuery);
	constraintModel.reload(filterData.constraints);
}

void FilterWidget::filterChanged()
{
	presetModified = true;
	updateFilter();
}

void FilterWidget::updateFilter()
{
	if (ignoreSignal)
		return;

	FilterData filterData = createFilterData();
	DiveFilter::instance()->setFilter(filterData);
	updatePresetLabel();
}

int FilterWidget::selectedPreset() const
{
	QModelIndexList selection = ui.presetTable->selectionModel()->selectedRows();
	return selection.size() >= 1 ? selection[0].row() : -1;
}

void FilterWidget::updatePresetLabel()
{
	int presetId = selectedPreset();
	QString text;
	if (presetId >= 0) {
		text = filter_preset_name_qstring(presetId);
		if (presetModified)
			text += " (" + tr("modified") + ")";
	}
	ui.currentSet->setText(text);
}

void FilterWidget::on_addSetButton_clicked()
{
	// If there is a selected item, suggest that to the user.
	// Thus, if the user selects an item and modify the filter,
	// they can simply overwrite the preset.
	int presetId = selectedPreset();
	QString selectedPreset = presetId >= 0 ? filter_preset_name_qstring(presetId) : QString();

	AddFilterPresetDialog dialog(selectedPreset, this);
	QString name = dialog.doit();
	if (name.isEmpty())
		return;
	int idx = filter_preset_id(name);
	if (idx >= 0)
		Command::editFilterPreset(idx, createFilterData());
	else
		Command::createFilterPreset(name, createFilterData());
	presetModified = false;
	updatePresetLabel();
}

void FilterWidget::showEvent(QShowEvent *event)
{
	QWidget::showEvent(event);
	ui.fullText->setFocus();
	updateFilter();
}

void FilterWidget::hideEvent(QHideEvent *event)
{
	QWidget::hideEvent(event);
}

void FilterWidget::addConstraint(filter_constraint_type type)
{
	constraintModel.addConstraint(type);
}
