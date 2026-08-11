// SPDX-License-Identifier: GPL-2.0
// AI-generated (Claude)
#include "suuntogasassigndialog.h"
#include "core/dive.h"
#include "core/gettext.h"
#include "core/qthelper.h"

#include <cmath>

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

SuuntoGasAssignDialog::SuuntoGasAssignDialog(std::vector<suunto_unresolved_gas> unresolved, QWidget *parent) : QDialog(parent),
	rows(std::move(unresolved))
{
	setWindowTitle(tr("Confirm gas mixes"));

	auto *layout = new QVBoxLayout(this);
	auto *label = new QLabel(tr("These dives switch gases mid-dive, but the imported log doesn't say "
				     "what was in each tank. Enter the gas mix and cylinder for each gas below "
				     "(left as air in an 11.1 L / 200 bar tank if you skip a row)."), this);
	label->setWordWrap(true);
	layout->addWidget(label);

	table = new QTableWidget(static_cast<int>(rows.size()), 6, this);
	table->setHorizontalHeaderLabels(QStringList() << tr("Dive") << tr("Gas") << tr("O2 %") << tr("He %")
							<< tr("Size (L)") << tr("Working pressure (bar)"));
	table->verticalHeader()->setVisible(false);
	table->setSelectionMode(QAbstractItemView::NoSelection);

	for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
		const suunto_unresolved_gas &row = rows[i];

		auto *diveItem = new QTableWidgetItem(get_short_dive_date_string(row.d->when));
		diveItem->setFlags(diveItem->flags() & ~Qt::ItemIsEditable);
		table->setItem(i, 0, diveItem);

		auto *gasItem = new QTableWidgetItem(tr("Gas %1").arg(row.cylinder_idx));
		gasItem->setFlags(gasItem->flags() & ~Qt::ItemIsEditable);
		table->setItem(i, 1, gasItem);

		auto *o2 = new QDoubleSpinBox(table);
		o2->setRange(1.0, 100.0);
		o2->setValue(21.0);
		o2->setSuffix(tr(" %"));
		table->setCellWidget(i, 2, o2);

		auto *he = new QDoubleSpinBox(table);
		he->setRange(0.0, 99.0);
		he->setValue(0.0);
		he->setSuffix(tr(" %"));
		table->setCellWidget(i, 3, he);

		auto *size = new QDoubleSpinBox(table);
		size->setRange(0.1, 50.0);
		size->setValue(11.1);
		size->setSuffix(tr(" L"));
		table->setCellWidget(i, 4, size);

		auto *workingPressure = new QDoubleSpinBox(table);
		workingPressure->setRange(1.0, 500.0);
		workingPressure->setValue(200.0);
		workingPressure->setSuffix(tr(" bar"));
		table->setCellWidget(i, 5, workingPressure);
	}
	table->resizeColumnsToContents();
	table->horizontalHeader()->setStretchLastSection(true);
	layout->addWidget(table);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(buttons, &QDialogButtonBox::accepted, this, &SuuntoGasAssignDialog::applyAndAccept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
}

void SuuntoGasAssignDialog::applyAndAccept()
{
	for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
		const suunto_unresolved_gas &row = rows[i];
		cylinder_t *cyl = row.d->get_or_create_cylinder(row.cylinder_idx);

		auto *o2 = qobject_cast<QDoubleSpinBox *>(table->cellWidget(i, 2));
		auto *he = qobject_cast<QDoubleSpinBox *>(table->cellWidget(i, 3));
		auto *size = qobject_cast<QDoubleSpinBox *>(table->cellWidget(i, 4));
		auto *workingPressure = qobject_cast<QDoubleSpinBox *>(table->cellWidget(i, 5));
		if (o2)
			cyl->gasmix.o2.permille = lrint(o2->value() * 10.0);
		if (he)
			cyl->gasmix.he.permille = lrint(he->value() * 10.0);
		if (size)
			cyl->type.size.mliter = lrint(size->value() * 1000.0);
		if (workingPressure)
			cyl->type.workingpressure.mbar = lrint(workingPressure->value() * 1000.0);
	}
	accept();
}
