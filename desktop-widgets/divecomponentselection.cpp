// SPDX-License-Identifier: GPL-2.0
#include "divecomponentselection.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "core/format.h"
#include "core/qthelper.h" // for get_dive_date_string()
#include "core/selection.h"
#include "core/string-format.h"

#include <QClipboard>
#include <QShortcut>

template <typename T>
static void assign_paste_data(QCheckBox &checkbox, const T &src, std::optional<T> &dest)
{
	if (checkbox.isChecked())
		dest = src;
	else
		dest = {};
}

template <typename T>
static void set_checked(QCheckBox &checkbox, const std::optional<T> &v)
{
	checkbox.setChecked(v.has_value());
}

DiveComponentSelection::DiveComponentSelection(dive_paste_data &data, QWidget *parent) :
	QDialog(parent), data(data)
{
	ui.setupUi(this);
	set_checked(*ui.divesite, data.divesite);
	set_checked(*ui.diveguide, data.diveguide);
	set_checked(*ui.buddy, data.buddy);
	set_checked(*ui.rating, data.rating);
	set_checked(*ui.visibility, data.visibility);
	set_checked(*ui.notes, data.notes);
	set_checked(*ui.suit, data.suit);
	set_checked(*ui.tags, data.tags);
	set_checked(*ui.cylinders, data.cylinders);
	set_checked(*ui.weights, data.weights);
	set_checked(*ui.number, data.number);
	set_checked(*ui.when, data.when);
	connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &DiveComponentSelection::buttonClicked);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	connect(close, &QShortcut::activated, this, &DiveComponentSelection::close);
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
	connect(quit, &QShortcut::activated, parent, &QWidget::close);
}

void DiveComponentSelection::buttonClicked(QAbstractButton *button)
{
	if (current_dive && ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		// Divesite is special, as we store the uuid not the pointer, since the
		// user may delete the divesite after copying.
		assign_paste_data(*ui.divesite, current_dive->dive_site ? current_dive->dive_site->uuid : uint32_t(), data.divesite);
		assign_paste_data(*ui.diveguide, current_dive->diveguide, data.diveguide);
		assign_paste_data(*ui.buddy, current_dive->buddy, data.buddy);
		assign_paste_data(*ui.rating, current_dive->rating, data.rating);
		assign_paste_data(*ui.visibility, current_dive->visibility, data.visibility);
		assign_paste_data(*ui.notes, current_dive->notes, data.notes);
		assign_paste_data(*ui.suit, current_dive->suit, data.suit);
		assign_paste_data(*ui.tags, current_dive->tags, data.tags);
		assign_paste_data(*ui.cylinders, current_dive->cylinders, data.cylinders);
		assign_paste_data(*ui.weights, current_dive->weightsystems, data.weights);
		assign_paste_data(*ui.number, current_dive->number, data.number);
		assign_paste_data(*ui.when, current_dive->when, data.when);

		std::string text;
		if (data.divesite && current_dive->dive_site)
			text += tr("Dive site: ").toStdString() + current_dive->dive_site->name + '\n';
		if (data.diveguide)
			text += tr("Dive guide: ").toStdString() + current_dive->diveguide + '\n';
		if (data.buddy)
			text += tr("Buddy: ").toStdString() + current_dive->buddy + '\n';
		if (data.rating)
			text += tr("Rating: ").toStdString() + std::string(current_dive->rating, '*') + '\n';
		if (data.visibility)
			text += tr("Visibility: ").toStdString() + std::string(current_dive->visibility, '*') + '\n';
		if (data.wavesize)
			text += tr("Wave size: ").toStdString() + std::string(current_dive->wavesize, '*') + '\n';
		if (data.current)
			text += tr("Current: ").toStdString() + std::string(current_dive->current, '*') + '\n';
		if (data.surge)
			text += tr("Surge: ").toStdString() + std::string(current_dive->surge, '*') + '\n';
		if (data.chill)
			text += tr("Chill: ").toStdString() + std::string(current_dive->chill, '*') + '\n';
		if (data.notes)
			text += tr("Notes:\n").toStdString() + current_dive->notes + '\n';
		if (data.suit)
			text += tr("Suit: ").toStdString() + current_dive->suit + '\n';
		if (data.tags)
			text += tr("Tags: ").toStdString() + taglist_get_tagstring(current_dive->tags) + '\n';
		if (data.cylinders)
			text += tr("Cylinders:\n").toStdString() + formatGas(current_dive).toStdString();
		if (data.weights)
			text += tr("Weights:\n").toStdString() + formatWeightList(current_dive).toStdString();
		if (data.number)
			text += tr("Dive number: ").toStdString() + casprintf_loc("%d", current_dive->number) + '\n';
		if (data.when)
			text += tr("Date / time: ").toStdString() + get_dive_date_string(current_dive->when).toStdString() + '\n';
		QApplication::clipboard()->setText(QString::fromStdString(text));
	}
}
