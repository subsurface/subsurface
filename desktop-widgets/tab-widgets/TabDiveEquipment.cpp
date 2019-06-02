// SPDX-License-Identifier: GPL-2.0
#include "TabDiveEquipment.h"
#include "maintab.h"
#include "desktop-widgets/mainwindow.h" // TODO: Only used temporarilly for edit mode changes
#include "desktop-widgets/simplewidgets.h" // For isGnome3Session()
#include "desktop-widgets/modeldelegates.h"

#include "profile-widget/profilewidget2.h"

#include "qt-models/cylindermodel.h"
#include "qt-models/weightmodel.h"

#include "core/divelist.h"
#include "core/subsurface-string.h"

#include <QSettings>
TabDiveEquipment::TabDiveEquipment(QWidget *parent) : TabBase(parent),
	cylindersModel(new CylindersModel(this)),
	weightModel(new WeightModel(this))
{
	ui.setupUi(this);

	// This makes sure we only delete the models
	// after the destructor of the tables,
	// this is needed to save the column sizes.
	cylindersModel->setParent(ui.cylinders);
	weightModel->setParent(ui.weights);

	ui.cylinders->setModel(cylindersModel);
	ui.weights->setModel(weightModel);

	connect(ui.cylinders->view(), &QTableView::clicked, this, &TabDiveEquipment::editCylinderWidget);
	connect(ui.weights->view(), &QTableView::clicked, this, &TabDiveEquipment::editWeightWidget);

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
	cylindersModel->updateDive();
	weightModel->updateDive();

	ui.cylinders->view()->hideColumn(CylindersModel::DEPTH);
	if (get_dive_dc(&displayed_dive, dc_number)->divemode == CCR)
		ui.cylinders->view()->showColumn(CylindersModel::USE);
	else
		ui.cylinders->view()->hideColumn(CylindersModel::USE);
}

void TabDiveEquipment::clear()
{
	cylindersModel->clear();
	weightModel->clear();
}

void TabDiveEquipment::addCylinder_clicked()
{
	MainWindow::instance()->mainTab->enableEdition();
	cylindersModel->add();
}

void TabDiveEquipment::addWeight_clicked()
{
	MainWindow::instance()->mainTab->enableEdition();
	weightModel->add();
}

void TabDiveEquipment::editCylinderWidget(const QModelIndex &index)
{
	if (cylindersModel->changed && !MainWindow::instance()->mainTab->isEditing()) {
		MainWindow::instance()->mainTab->enableEdition();
		return;
	}
	if (index.isValid() && index.column() != CylindersModel::REMOVE) {
		MainWindow::instance()->mainTab->enableEdition();
		ui.cylinders->edit(index);
	}
}

void TabDiveEquipment::editWeightWidget(const QModelIndex &index)
{
	MainWindow::instance()->mainTab->enableEdition();

	if (index.isValid() && index.column() != WeightModel::REMOVE)
		ui.weights->edit(index);
}

// tricky little macro to edit all the selected dives
// loop ove all DIVES and do WHAT.
#define MODIFY_DIVES(DIVES, WHAT)                            \
	do {                                                 \
		for (dive *mydive: DIVES) {                  \
			invalidate_dive_cache(mydive);       \
			WHAT;                                \
		}					     \
		mark_divelist_changed(true);                 \
	} while (0)

// Get the list of selected dives, but put the current dive at the last position of the vector
static QVector<dive *> getSelectedDivesCurrentLast()
{
	QVector<dive *> res;
	struct dive *d;
	int i;
	for_each_dive (i, d) {
		if (d->selected && d != current_dive)
			res.append(d);
	}
	res.append(current_dive);
	return res;
}

void TabDiveEquipment::acceptChanges()
{
	bool do_replot = false;

	// now check if something has changed and if yes, edit the selected dives that
	// were identical with the master dive shown (and mark the divelist as changed)
	struct dive *cd = current_dive;

	// Get list of selected dives, but put the current dive last;
	// this is required in case the invocation wants to compare things
	// to the original value in current_dive like it should
	QVector<dive *> selectedDives = getSelectedDivesCurrentLast();

	if (cylindersModel->changed) {
		mark_divelist_changed(true);
		MODIFY_DIVES(selectedDives,
			for (int i = 0; i < MAX_CYLINDERS; i++) {
				if (mydive != cd) {
					if (same_string(mydive->cylinder[i].type.description, cd->cylinder[i].type.description)) {
						// if we started out with the same cylinder description (for multi-edit) or if we do copt & paste
						// make sure that we have the same cylinder type and copy the gasmix, but DON'T copy the start
						// and end pressures (those are per dive after all)
						if (!same_string(mydive->cylinder[i].type.description, displayed_dive.cylinder[i].type.description)) {
							free((void*)mydive->cylinder[i].type.description);
							mydive->cylinder[i].type.description = copy_string(displayed_dive.cylinder[i].type.description);
						}
						mydive->cylinder[i].type.size = displayed_dive.cylinder[i].type.size;
						mydive->cylinder[i].type.workingpressure = displayed_dive.cylinder[i].type.workingpressure;
						mydive->cylinder[i].gasmix = displayed_dive.cylinder[i].gasmix;
						mydive->cylinder[i].cylinder_use = displayed_dive.cylinder[i].cylinder_use;
						mydive->cylinder[i].depth = displayed_dive.cylinder[i].depth;
					}
				}
			}
		);
		for (int i = 0; i < MAX_CYLINDERS; i++) {
			// copy the cylinder but make sure we have our own copy of the strings
			free((void*)cd->cylinder[i].type.description);
			cd->cylinder[i] = displayed_dive.cylinder[i];
			cd->cylinder[i].type.description = copy_string(displayed_dive.cylinder[i].type.description);
		}
		/* if cylinders changed we may have changed gas change events
		 * and sensor idx in samples as well
		 * - so far this is ONLY supported for a single selected dive */
		struct divecomputer *tdc = &current_dive->dc;
		struct divecomputer *sdc = &displayed_dive.dc;
		while(tdc && sdc) {
			free_events(tdc->events);
			copy_events(sdc, tdc);
			free(tdc->sample);
			copy_samples(sdc, tdc);
			tdc = tdc->next;
			sdc = sdc->next;
		}
		do_replot = true;
	}

	if (weightModel->changed) {
		mark_divelist_changed(true);
		MODIFY_DIVES(selectedDives,
			for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
				if (mydive != cd && (same_string(mydive->weightsystem[i].description, cd->weightsystem[i].description))) {
					mydive->weightsystem[i] = displayed_dive.weightsystem[i];
					mydive->weightsystem[i].description = copy_string(displayed_dive.weightsystem[i].description);
				}
			}
		);
		for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
			cd->weightsystem[i] = displayed_dive.weightsystem[i];
			cd->weightsystem[i].description = copy_string(displayed_dive.weightsystem[i].description);
		}
	}

	if (do_replot)
		MainWindow::instance()->graphics->replot();

	cylindersModel->changed = false;
	weightModel->changed = false;
}

void TabDiveEquipment::rejectChanges()
{
	cylindersModel->changed = false;
	weightModel->changed = false;
	cylindersModel->updateDive();
	weightModel->updateDive();
}
