// SPDX-License-Identifier: GPL-2.0
#include "TabDiveExtraInfo.h"
#include "ui_TabDiveExtraInfo.h"
#include "core/dive.h"
#include "core/divemode.h"
#include "core/gettextfromc.h"
#include "core/selection.h"
#include "core/string-format.h"
#include "qt-models/divecomputerextradatamodel.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QSignalBlocker>

// Override the disabled-text colour so the combo remains readable in both
// light and dark themes when it is disabled (only one option available).
// Platform defaults often render disabled text as near-invisible grey.
static void setReadableWhenDisabled(QComboBox *combo)
{
	QPalette p = combo->palette();
	QColor activeText = p.color(QPalette::Active, QPalette::WindowText);
	p.setColor(QPalette::Disabled, QPalette::Text,       activeText);
	p.setColor(QPalette::Disabled, QPalette::ButtonText, activeText);
	combo->setPalette(p);
}

TabDiveExtraInfo::TabDiveExtraInfo(MainTab *parent) :
	TabBase(parent),
	ui(new Ui::TabDiveExtraInfo()),
	extraDataModel(new ExtraDataModel(this))
{
	ui->setupUi(this);
	ui->extraData->setModel(extraDataModel);
	ui->extraData->horizontalHeader()->setStretchLastSection(true);

	setReadableWhenDisabled(ui->modelCombo);
	setReadableWhenDisabled(ui->serialCombo);

	connect(ui->modelCombo, QOverload<int>::of(&QComboBox::activated),
		this, &TabDiveExtraInfo::onModelComboActivated);
	connect(ui->serialCombo, QOverload<int>::of(&QComboBox::activated),
		this, &TabDiveExtraInfo::onSerialComboActivated);
}

TabDiveExtraInfo::~TabDiveExtraInfo()
{
	delete ui;
}

// --- Private helpers --------------------------------------------------------

// Walks currentDivePtr once and returns a snapshot of every dive computer's
// index, trimmed model name, and trimmed serial.  All other helpers consume
// this list so that model/serial canonicalization lives in a single place.
QList<TabDiveExtraInfo::DCEntry> TabDiveExtraInfo::allDCs() const
{
	QList<DCEntry> entries;
	if (!currentDivePtr)
		return entries;
	const int numDCs = currentDivePtr->number_of_computers();
	entries.reserve(numDCs);
	for (int i = 0; i < numDCs; ++i) {
		const divecomputer *idc = currentDivePtr->get_dc(i);
		entries.append({ i,
			QString::fromStdString(idc->model).trimmed(),
			QString::fromStdString(idc->serial).trimmed() });
	}
	return entries;
}

// Returns all unique DC model names in dive order (O(n) via QSet lookup).
QStringList TabDiveExtraInfo::uniqueModelNames() const
{
	QStringList ordered;
	QSet<QString> seen;
	for (const DCEntry &e : allDCs()) {
		if (!seen.contains(e.model)) {
			seen.insert(e.model);
			ordered.append(e.model);
		}
	}
	return ordered;
}

// Returns the DC index (0-based) of the first dive computer with the given
// model name, or -1 if none matches.
int TabDiveExtraInfo::firstDCIndexForModel(const QString &model) const
{
	for (const DCEntry &e : allDCs()) {
		if (e.model == model)
			return e.index;
	}
	return -1;
}

// ----------------------------------------------------------------------------

void TabDiveExtraInfo::populateSerialCombo(const QString &modelName, int selectedDCIdx)
{
	QSignalBlocker block(ui->serialCombo);
	ui->serialCombo->clear();

	if (!currentDivePtr)
		return;

	// Collect only the DCs that belong to the requested model.
	QList<DCEntry> group;
	for (const DCEntry &e : allDCs()) {
		if (e.model == modelName)
			group.append(e);
	}

	// Count how many times each serial appears in the group so we can
	// detect duplicates (including the empty-string case).
	QHash<QString, int> serialCount;
	for (const DCEntry &e : group)
		++serialCount[e.serial];

	int comboIndexToSelect = 0;
	for (int n = 0; n < group.size(); ++n) {
		const DCEntry &e = group[n];
		QString label;
		if (e.serial.isEmpty())
			label = tr("(no serial) #%1").arg(n + 1);
		else if (serialCount.value(e.serial) > 1)
			label = tr("%1 (#%2)").arg(e.serial).arg(n + 1);
		else
			label = e.serial;
		ui->serialCombo->addItem(label, e.index); // item data = DC index
		if (e.index == selectedDCIdx)
			comboIndexToSelect = ui->serialCombo->count() - 1;
	}

	ui->serialCombo->setCurrentIndex(comboIndexToSelect);
	// Only enable the serial combo when there are multiple DCs with the same model
	ui->serialCombo->setEnabled(ui->serialCombo->count() > 1);
}

void TabDiveExtraInfo::updateData(const std::vector<dive *> &, dive *currentDive, int currentDC)
{
	if (!currentDive) {
		clear();
		return;
	}

	currentDivePtr = currentDive;

	const divecomputer *dc = currentDive->get_dc(currentDC);

	// --- Populate model combo ---
	{
		QSignalBlocker block(ui->modelCombo);
		ui->modelCombo->clear();

		const QStringList models = uniqueModelNames();
		for (const QString &model : models)
			ui->modelCombo->addItem(model);

		const QString currentModel = QString::fromStdString(dc->model).trimmed();
		const int modelIdx = ui->modelCombo->findText(currentModel);
		if (modelIdx >= 0)
			ui->modelCombo->setCurrentIndex(modelIdx);

		// Only enable the model combo when there are multiple distinct model names
		ui->modelCombo->setEnabled(models.size() > 1);
	}

	// --- Populate serial combo for the current model ---
	populateSerialCombo(QString::fromStdString(dc->model).trimmed(), currentDC);

	// --- Remaining read-only fields ---
	ui->firmware->setText(QString::fromStdString(dc->fw_version).trimmed());
	ui->date->setText(get_dive_date_string(dc->when));
	ui->duration->setText(get_dive_duration_string(dc->duration.seconds, tr("h"), tr("min"), tr("sec"),
			" ", dc->divemode == FREEDIVE));
	if (dc->divemode >= 0 && dc->divemode < NUM_DIVEMODE)
		ui->diveMode->setText(gettextFromC::tr(divemode_text_ui[dc->divemode]));
	else
		ui->diveMode->clear();

	extraDataModel->updateDiveComputer(dc);
	ui->extraData->setVisible(false); // This causes the resize to include rows outside the current viewport
	ui->extraData->resizeColumnsToContents();
	ui->extraData->setVisible(true);
}

void TabDiveExtraInfo::clear()
{
	currentDivePtr = nullptr;

	QSignalBlocker blockModel(ui->modelCombo);
	QSignalBlocker blockSerial(ui->serialCombo);

	ui->modelCombo->clear();
	ui->modelCombo->setEnabled(false);
	ui->serialCombo->clear();
	ui->serialCombo->setEnabled(false);
	ui->firmware->clear();
	ui->date->clear();
	ui->duration->clear();
	ui->diveMode->clear();
	extraDataModel->clear();
}

void TabDiveExtraInfo::onModelComboActivated(int index)
{
	if (!currentDivePtr)
		return;

	const QString modelName = ui->modelCombo->itemText(index);
	const int firstDC = firstDCIndexForModel(modelName);
	if (firstDC < 0)
		return;

	// Repopulate the serial combo for the newly selected model
	populateSerialCombo(modelName, firstDC);

	// The serial combo now shows the first DC for that model; emit its DC index
	const int selectedDC = ui->serialCombo->currentData().toInt();
	emit dcChangeRequested(selectedDC);
}

void TabDiveExtraInfo::onSerialComboActivated(int index)
{
	if (!currentDivePtr)
		return;

	const int dcIndex = ui->serialCombo->itemData(index).toInt();
	emit dcChangeRequested(dcIndex);
}
