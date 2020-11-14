// SPDX-License-Identifier: GPL-2.0
#include "TabDiveEquipment.h"
#include "maintab.h"
#include "desktop-widgets/simplewidgets.h" // For isGnome3Session()
#include "desktop-widgets/modeldelegates.h"
#include "commands/command.h"

#include "qt-models/cylindermodel.h"
#include "qt-models/weightmodel.h"

#include <QSettings>
#include <QCompleter>

TabDiveEquipment::TabDiveEquipment(QWidget *parent) : TabBase(parent),
	cylindersModel(new CylindersModelFiltered(this)),
	weightModel(new WeightModel(this))
{
	QCompleter *suitCompleter;
	ui.setupUi(this);

	// This makes sure we only delete the models
	// after the destructor of the tables,
	// this is needed to save the column sizes.
	cylindersModel->setParent(ui.cylinders);
	weightModel->setParent(ui.weights);

	ui.cylinders->setModel(cylindersModel);
	ui.weights->setModel(weightModel);

	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &TabDiveEquipment::divesChanged);
	connect(ui.cylinders, &TableView::itemClicked, this, &TabDiveEquipment::editCylinderWidget);
	connect(ui.weights, &TableView::itemClicked, this, &TabDiveEquipment::editWeightWidget);
	connect(cylindersModel->model(), &CylindersModel::divesEdited, this, &TabDiveEquipment::divesEdited);
	connect(weightModel, &WeightModel::divesEdited, this, &TabDiveEquipment::divesEdited);

	// Current display of things on Gnome3 looks like shit, so
	// let's fix that.
	if (isGnome3Session()) {
		QPalette p;
		p.setColor(QPalette::Window, QColor(Qt::white));
		ui.scrollArea->viewport()->setPalette(p);
	}

	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::TYPE, new TankInfoDelegate(this));
	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::USE, new TankUseDelegate(this));
	ui.weights->view()->setItemDelegateForColumn(WeightModel::TYPE, new WSInfoDelegate(this));
	ui.cylinders->view()->setColumnHidden(CylindersModel::DEPTH, true);

	ui.cylinders->setTitle(tr("Cylinders"));
	ui.cylinders->setBtnToolTip(tr("Add cylinder"));
	connect(ui.cylinders, &TableView::addButtonClicked, this, &TabDiveEquipment::addCylinder_clicked);

	ui.weights->setTitle(tr("Weights"));
	ui.weights->setBtnToolTip(tr("Add weight system"));
	connect(ui.weights, &TableView::addButtonClicked, this, &TabDiveEquipment::addWeight_clicked);

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
		if ((i == CylindersModel::REMOVE) || (i == CylindersModel::TYPE))
			continue;
		bool checked = s.value(QString("column%1_hidden").arg(i)).toBool();
		QAction *action = new QAction(cylindersModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString(), ui.cylinders->view());
		action->setCheckable(true);
		action->setData(i);
		action->setChecked(!checked);
		connect(action, &QAction::triggered, this, &TabDiveEquipment::toggleTriggeredColumn);
		ui.cylinders->view()->setColumnHidden(i, checked);
		ui.cylinders->view()->horizontalHeader()->addAction(action);
	}
	ui.cylinders->view()->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);
	ui.weights->view()->horizontalHeader()->setContextMenuPolicy(Qt::ActionsContextMenu);
	suitCompleter = new QCompleter(&suitModel, ui.suit);
	suitCompleter->setCaseSensitivity(Qt::CaseInsensitive);
	ui.suit->setCompleter(suitCompleter);
}

TabDiveEquipment::~TabDiveEquipment()
{
	QSettings s;
	s.beginGroup("cylinders_dialog");
	for (int i = 0; i < CylindersModel::COLUMNS; i++) {
		if ((i == CylindersModel::REMOVE) || (i == CylindersModel::TYPE))
			continue;
		s.setValue(QString("column%1_hidden").arg(i), ui.cylinders->view()->isColumnHidden(i));
	}
}

// This function gets called if a field gets updated by an undo command.
// Refresh the corresponding UI field.
void TabDiveEquipment::divesChanged(const QVector<dive *> &dives, DiveField field)
{
	// If the current dive is not in list of changed dives, do nothing
	if (!current_dive || !dives.contains(current_dive))
		return;

	if (field.suit)
		ui.suit->setText(QString(current_dive->suit));
}

void TabDiveEquipment::toggleTriggeredColumn()
{
	QAction *action = qobject_cast<QAction *>(sender());
	int col = action->data().toInt();
	QTableView *view = ui.cylinders->view();

	if (action->isChecked()) {
		view->showColumn(col);
		if (view->columnWidth(col) <= 15)
			view->setColumnWidth(col, 80);
	} else
		view->hideColumn(col);
}

void TabDiveEquipment::updateData()
{
	cylindersModel->updateDive(current_dive);
	weightModel->updateDive(current_dive);

	ui.cylinders->view()->hideColumn(CylindersModel::DEPTH);
	bool is_ccr = current_dive && get_dive_dc(current_dive, dc_number)->divemode == CCR;
	if (is_ccr)
		ui.cylinders->view()->showColumn(CylindersModel::USE);
	else
		ui.cylinders->view()->hideColumn(CylindersModel::USE);
	if (current_dive && current_dive->suit)
		ui.suit->setText(QString(current_dive->suit));
	else
		ui.suit->clear();
}

void TabDiveEquipment::clear()
{
	cylindersModel->clear();
	weightModel->clear();
	ui.suit->clear();
}

void TabDiveEquipment::addCylinder_clicked()
{
	divesEdited(Command::addCylinder(false));
}

void TabDiveEquipment::addWeight_clicked()
{
	divesEdited(Command::addWeight(false));
}

void TabDiveEquipment::editCylinderWidget(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	if (index.column() == CylindersModel::REMOVE)
		divesEdited(Command::removeCylinder(cylindersModel->mapToSource(index).row(), false));
	else
		ui.cylinders->edit(index);
}

void TabDiveEquipment::editWeightWidget(const QModelIndex &index)
{
	if (!index.isValid())
		return;

	if (index.column() == WeightModel::REMOVE)
		divesEdited(Command::removeWeight(index.row(), false));
	else
		ui.weights->edit(index);
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

void TabDiveEquipment::on_suit_editingFinished()
{
	if (!current_dive)
		return;
	divesEdited(Command::editSuit(ui.suit->text(), false));
}

void TabDiveEquipment::closeWarning()
{
	ui.multiDiveWarningMessage->hide();
}
