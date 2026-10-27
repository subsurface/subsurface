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

#include <QMessageBox>
#include <QSettings>
#include <QCompleter>

// Set once the user has dismissed the one-off hint about the "Tank sensor"
// column. This is purely a "have we told them already" flag, never a
// judgement about whether their current sensor assignment makes sense.
static const QString tankSensorHintDismissedKey = QStringLiteral("tank_sensor_hint_dismissed");

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
	connect(cylindersModel, &CylindersModel::divesEdited, this, &TabDiveEquipment::divesEdited);
	connect(weightModel, &WeightModel::divesEdited, this, &TabDiveEquipment::divesEdited);
	connect(&diveListNotifier, &DiveListNotifier::diveComputerEdited, this, &TabDiveEquipment::diveComputerEdited);
	connect(&diveListNotifier, &DiveListNotifier::cylinderRemoved, this, &TabDiveEquipment::cylinderRemoved);

	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::TYPE, &tankInfoDelegate);
	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::USE, &tankUseDelegate);
	ui.cylinders->view()->setItemDelegateForColumn(CylindersModel::SENSORS, &sensorDelegate);
	ui.weights->view()->setItemDelegateForColumn(WeightModel::TYPE, &wsInfoDelegate);
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

	QAction *action = new QAction(tr("OK"), this);
	connect(action, &QAction::triggered, this, &TabDiveEquipment::closeWarning);
	ui.multiDiveWarningMessage->addAction(action);

	action = new QAction(tr("Undo"), this);
	connect(action, &QAction::triggered, Command::undoAction(this), &QAction::trigger);
	connect(action, &QAction::triggered, this, &TabDiveEquipment::closeWarning);
	ui.multiDiveWarningMessage->addAction(action);

	ui.multiDiveWarningMessage->hide();

	ui.tankSensorHintMessage->setMessageType(KMessageWidget::Information);
	ui.tankSensorHintMessage->setText(tr("Tank pressure sensors are assigned to cylinders in the \"Tank sensor\" column."));
	action = new QAction(tr("Show column"), this);
	connect(action, &QAction::triggered, this, &TabDiveEquipment::showSensorColumnFromHint);
	ui.tankSensorHintMessage->addAction(action);
	connect(ui.tankSensorHintMessage, &KMessageWidget::hideAnimationFinished, this, &TabDiveEquipment::sensorHintDismissed);
	ui.tankSensorHintMessage->hide();

	QSettings s;
	s.beginGroup("cylinders_dialog");
	for (int i = 0; i < CylindersModel::COLUMNS; i++) {
		if (ignoreHiddenFlag(i))
			continue;
		// The SENSORS column is also hidden automatically when the displayed
		// dive has no tank-pressure samples (see setCylinderColumnVisibility()).
		// That data-driven hide must not be confused with the user's explicit
		// preference, so it is stored under its own settings key. This also
		// means that any pre-existing "column15_hidden" value, which may have
		// been poisoned by the automatic hide, is simply ignored from now on.
		QString key = i == CylindersModel::SENSORS ? QString("column%1_user_hidden").arg(i)
							     : QString("column%1_hidden").arg(i);
		auto setting = s.value(key);
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
	suitCompleter = new QCompleter(&suitModel, ui.suit);
	suitCompleter->setCaseSensitivity(Qt::CaseInsensitive);
	ui.suit->setCompleter(suitCompleter);
}

TabDiveEquipment::~TabDiveEquipment()
{
	QSettings s;
	s.beginGroup("cylinders_dialog");
	QList<QAction *> actions = ui.cylinders->view()->horizontalHeader()->actions();
	for (int i = 0; i < CylindersModel::COLUMNS; i++) {
		if (ignoreHiddenFlag(i))
			continue;
		if (i == CylindersModel::SENSORS) {
			// Do not persist isColumnHidden() here: for this column it reflects
			// whether the currently selected dive happens to have tank-pressure
			// samples, not the user's preference. The preference is tracked by
			// the checked state of the header context-menu action instead,
			// which only ever changes in toggleTriggeredColumn().
			auto action = std::find_if(actions.begin(), actions.end(), [](QAction *a) {
				return a->data().toInt() == CylindersModel::SENSORS;
			});
			bool hidden = action != actions.end() ? !(*action)->isChecked() : ui.cylinders->view()->isColumnHidden(i);
			s.setValue(QString("column%1_user_hidden").arg(i), hidden);
			continue;
		}
		s.setValue(QString("column%1_hidden").arg(i), ui.cylinders->view()->isColumnHidden(i));
	}
}

void TabDiveEquipment::setCylinderColumnVisibility()
{
	bool showSensors = true;
	bool hasPressureSamples = false;
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
		hasPressureSamples = showSensors;
	}

	// One-off, purely informational pointer to where cylinder-to-sensor
	// assignment happens. It is keyed only on whether the displayed dive
	// has tank-pressure samples, never on the contents of the assignment
	// itself: an unusual-looking mapping (more sensors than cylinders,
	// sensors on cylinders the diver doesn't breathe, or unassigned
	// sensors) is routine for buddy or student monitoring and is not an
	// error to flag.
	if (hasPressureSamples && !QSettings().value(tankSensorHintDismissedKey, false).toBool())
		ui.tankSensorHintMessage->show();
	else
		ui.tankSensorHintMessage->hide();

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
		ui.suit->setText(QString::fromStdString(parent.currentDive->suit));
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
	sensorDelegate.setCurrentDc(dc);
	tankUseDelegate.setDiveDc(*currentDive, currentDC);

	setCylinderColumnVisibility();

	if (!currentDive->suit.empty())
		ui.suit->setText(QString::fromStdString(currentDive->suit));
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

	if (index.column() == CylindersModel::REMOVE) {
		divesEdited(Command::removeCylinder(index.row(), false));
	} else if (index.column() != CylindersModel::NUMBER) {
		ui.cylinders->edit(index);
	}
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

void TabDiveEquipment::on_suit_editingFinished()
{
	if (!parent.currentDive)
		return;
	divesEdited(Command::editSuit(ui.suit->text(), false));
}

void TabDiveEquipment::closeWarning()
{
	ui.multiDiveWarningMessage->hide();
}

void TabDiveEquipment::showSensorColumnFromHint()
{
	QList<QAction *> actions = ui.cylinders->view()->horizontalHeader()->actions();
	auto action = std::find_if(actions.begin(), actions.end(), [](QAction *a) {
		return a->data().toInt() == CylindersModel::SENSORS;
	});
	if (action != actions.end())
		(*action)->setChecked(true);
	ui.cylinders->view()->setColumnHidden(CylindersModel::SENSORS, false);
	setCylinderColumnVisibility();
	ui.tankSensorHintMessage->animatedHide();
}

void TabDiveEquipment::sensorHintDismissed()
{
	QSettings().setValue(tankSensorHintDismissedKey, true);
}
