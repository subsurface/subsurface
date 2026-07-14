// SPDX-License-Identifier: GPL-2.0
#include "TabDiveEquipment.h"
#include "maintab.h"
#include "desktop-widgets/modeldelegates.h"
#include "core/dive.h"
#include "core/sample.h"
#include "core/selection.h"
#include "commands/command.h"

#include "qt-models/cylindermodel.h"
#include "qt-models/weightmodel.h"
#include "qt-models/suitcomponentmodel.h"

#include <QMessageBox>
#include <QSettings>
#include <QCompleter>

static bool ignoreHiddenFlag(int i)
{
	switch (i) {
	case CylindersModel::NUMBER:
	case CylindersModel::REMOVE:
	case CylindersModel::TYPE:
	case CylindersModel::WORKINGPRESS_INT:
	case CylindersModel::SIZE_INT:
		return true;
	default:
		return false;
	}
}

TabDiveEquipment::TabDiveEquipment(MainTab *parent) : TabBase(parent),
	cylindersModel(new CylindersModel(false, this)),
	weightModel(new WeightModel(this)),
	suitComponentModel(new SuitComponentModel(this))
{
	ui.setupUi(this);

	// This makes sure we only delete the models
	// after the destructor of the tables,
	// this is needed to save the column sizes.
	cylindersModel->setParent(ui.cylinders);
	weightModel->setParent(ui.weights);
	suitComponentModel->setParent(ui.suit_items);

	ui.cylinders->setModel(cylindersModel);
	ui.weights->setModel(weightModel);
	ui.suit_items->setModel(suitComponentModel);

	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &TabDiveEquipment::divesChanged);
	connect(ui.cylinders, &TableView::itemClicked, this, &TabDiveEquipment::editCylinderWidget);
	connect(ui.weights, &TableView::itemClicked, this, &TabDiveEquipment::editWeightWidget);
	connect(ui.suit_items, &TableView::itemClicked, this, &TabDiveEquipment::editSuitWidget);
	connect(cylindersModel, &CylindersModel::divesEdited, this, &TabDiveEquipment::divesEdited);
	connect(weightModel, &WeightModel::divesEdited, this, &TabDiveEquipment::divesEdited);
	connect(suitComponentModel, &SuitComponentModel::divesEdited, this, &TabDiveEquipment::divesEdited);
	connect(&diveListNotifier, &DiveListNotifier::diveComputerEdited, this, &TabDiveEquipment::diveComputerEdited);
	connect(&diveListNotifier, &DiveListNotifier::cylinderRemoved, this, &TabDiveEquipment::cylinderRemoved);

	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::TYPE, &tankInfoDelegate);
	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::USE, &tankUseDelegate);
	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::SENSORS, &sensorDelegate);
	ui.weights->view()->setItemDelegateForColumn(WeightModel::TYPE, &wsInfoDelegate);
	ui.suit_items->view()->setItemDelegateForColumn(SuitComponentModel::TYPE, &suitTypeDelegate);
	ui.suit_items->view()->setItemDelegateForColumn(SuitComponentModel::BRAND, &suitInfoDelegate);
	ui.suit_items->view()->setItemDelegateForColumn(SuitComponentModel::MODEL, &suitInfoDelegate);
	ui.suit_items->view()->setItemDelegateForColumn(SuitComponentModel::THICKNESS, &suitInfoDelegate);
	ui.suit_items->view()->setItemDelegateForColumn(SuitComponentModel::SIZE, &suitInfoDelegate);

	cylindersModel->setParent(ui.cylinders->view());
	weightModel->setParent(ui.weights->view());
	suitComponentModel->setParent(ui.suit_items->view());

	ui.cylinders->view()->setColumnHidden(CylindersModel::DEPTH, true);
	ui.cylinders->view()->setColumnHidden(CylindersModel::WORKINGPRESS_INT, true);
	ui.cylinders->view()->setColumnHidden(CylindersModel::SIZE_INT, true);

	ui.cylinders->view()->horizontalHeader()->setStretchLastSection(true);

	ui.cylinders->setTitle(tr("Cylinders"));
	ui.cylinders->setBtnToolTip(tr("Add cylinder"));
	connect(ui.cylinders, &TableView::addButtonClicked, this, &TabDiveEquipment::addCylinder_clicked);

	ui.weights->setTitle(tr("Weights"));
	ui.weights->setBtnToolTip(tr("Add weight system"));
	connect(ui.weights, &TableView::addButtonClicked, this, &TabDiveEquipment::addWeight_clicked);

	ui.weights->view()->horizontalHeader()->setStretchLastSection(true);

	ui.suit_items->setTitle(tr("Suit"));
	ui.suit_items->setBtnToolTip(tr("Add suit part"));
	connect(ui.suit_items, &TableView::addButtonClicked, this, &TabDiveEquipment::addSuit_clicked);
	ui.suit_items->view()->horizontalHeader()->setStretchLastSection(true);

	ui.cylinders->view()->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
	ui.weights->view()->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
	ui.suit_items->view()->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

	QAction *action = new QAction(tr("OK"), this);
	connect(action, &QAction::triggered, this, &TabDiveEquipment::closeWarning);
	ui.multiDiveWarningMessage->addAction(action);

	action = new QAction(tr("Undo"), this);
	connect(action, &QAction::triggered, Command::undoAction(this), &QAction::trigger);
	connect(action, &QAction::triggered, this, &TabDiveEquipment::closeWarning);
	ui.multiDiveWarningMessage->addAction(action);

	ui.multiDiveWarningMessage->hide();

	QSettings s;
	s.beginGroup("cylinders_dialog");
	for (int i = 0; i < CylindersModel::COLUMNS; i++) {
		if (ignoreHiddenFlag(i))
			continue;
		auto setting = s.value(QString("column%1_hidden").arg(i));
		bool hidden = setting.isValid() ? setting.toBool() : false;
		QAction *action = new QAction(cylindersModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString(), ui.cylinders->view());
		action->setCheckable(true);
		action->setData(i);
		action->setChecked(!hidden);
		connect(action, &QAction::triggered, this, &TabDiveEquipment::toggleTriggeredColumn);
		ui.cylinders->view()->horizontalHeader()->addAction(action);
		ui.cylinders->view()->setColumnHidden(i, hidden);
	}
	setCylinderColumnVisibility();
	ui.cylinders->view()->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);
	ui.weights->view()->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);
}

TabDiveEquipment::~TabDiveEquipment()
{
	QSettings s;
	s.beginGroup("cylinders_dialog");
	for (int i = 0; i < CylindersModel::COLUMNS; i++) {
		if (ignoreHiddenFlag(i))
			continue;
		s.setValue(QString("column%1_hidden").arg(i), ui.cylinders->view()->isColumnHidden(i));
	}
}

void TabDiveEquipment::setCylinderColumnVisibility()
{
	bool showSensors = true;
	if (parent.currentDive && parent.currentDC >= 0) {
		showSensors = false;
		const struct divecomputer *dc = parent.currentDive->get_dc(parent.currentDC);
		for (const auto &sample: dc->samples) {
			auto pressure = std::find_if(std::begin(sample.pressure), std::end(sample.pressure), [](const pressure_t &pressure) {
				return pressure.mbar;
			});
			if (pressure != std::end(sample.pressure)) {
				showSensors = true;
				break;
			}
		}
	}

	if (showSensors) {
		QList<QAction *> actions = ui.cylinders->view()->horizontalHeader()->actions();
		auto action = std::find_if(actions.begin(), actions.end(), [](QAction *a) {
			return a->data().toInt() == CylindersModel::SENSORS;
		});
		if (action != actions.end())
			showSensors = (*action)->isChecked();
	}
	ui.cylinders->view()->setColumnHidden(CylindersModel::SENSORS, !showSensors);

	// This is needed as Qt sets the column width to 0 when hiding a column
	ui.cylinders->view()->setVisible(false); // This will cause the resize to include rows outside the current viewport
	ui.cylinders->view()->resizeColumnsToContents();
	ui.cylinders->view()->setVisible(true);
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveEquipment::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	if (!parent.includesCurrentDive(dives))
		return;

	if (field.suit)
		suitComponentModel->updateDive(parent.currentDive);
}

void TabDiveEquipment::toggleTriggeredColumn()
{
	QAction *action = qobject_cast<QAction *>(sender());
	int col = action->data().toInt();
	ui.cylinders->view()->setColumnHidden(col, !action->isChecked());

	setCylinderColumnVisibility();
}

void TabDiveEquipment::updateData(const std::vector<dive *> &, dive *currentDive, int currentDC)
{
	divecomputer *dc = currentDive->get_dc(currentDC);

	cylindersModel->updateDive(currentDive, currentDC);
	weightModel->updateDive(currentDive);
	suitComponentModel->updateDive(currentDive);
	sensorDelegate.setCurrentDc(dc);
	tankUseDelegate.setDiveDc(*currentDive, currentDC);

	setCylinderColumnVisibility();
}

void TabDiveEquipment::clear()
{
	cylindersModel->clear();
	weightModel->clear();
	suitComponentModel->updateDive(nullptr);
}

void TabDiveEquipment::addCylinder_clicked()
{
	divesEdited(Command::addCylinder(false));
}

void TabDiveEquipment::addWeight_clicked()
{
	divesEdited(Command::addWeight(false));
}

void TabDiveEquipment::addSuit_clicked()
{
	if (!parent.currentDive)
		return;
	suitComponentModel->add();
}

void TabDiveEquipment::editCylinderWidget(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	if (index.column() == CylindersModel::REMOVE)
		divesEdited(Command::removeCylinder(index.row(), false));
}

void TabDiveEquipment::editWeightWidget(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	if (index.column() == WeightModel::REMOVE)
		divesEdited(Command::removeWeight(index.row(), false));
}

void TabDiveEquipment::editSuitWidget(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	if (index.column() == SuitComponentModel::REMOVE)
		suitComponentModel->remove(index.row());
}

void TabDiveEquipment::divesEdited(int i)
{
	// No warning if only one dive was edited
	if (i <= 1)
		return;
	ui.multiDiveWarningMessage->setCloseButtonVisible(false);
	ui.multiDiveWarningMessage->setText(tr("Warning: edited %1 dives").arg(i));
	ui.multiDiveWarningMessage->show();
}

void TabDiveEquipment::diveComputerEdited(dive &dive, divecomputer &dc)
{
	if (parent.currentDive == &dive && parent.currentDive->get_dc(parent.currentDC) == &dc)
		dive.fixup_dive_dc(dc);
}

void TabDiveEquipment::cylinderRemoved(struct dive *dive, int)
{
	if (parent.currentDive == dive)
		for (auto &dc: dive->dcs)
			dive->fixup_dive_dc(dc);
}

void TabDiveEquipment::closeWarning()
{
	ui.multiDiveWarningMessage->hide();
}
