// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/simplewidgets.h"
#include "qt-models/filtermodels.h"

#include <QProcess>
#include <QFileDialog>
#include <QShortcut>
#include <QKeyEvent>
#include <QAction>
#include <QDesktopServices>
#include <QToolTip>
#include <QClipboard>
#include <QCompleter>

#include "core/file.h"
#include "core/filterpreset.h"
#include "core/divesite.h"
#include "desktop-widgets/mainwindow.h"
#include "core/qthelper.h"
#include "libdivecomputer/parser.h"
#include "desktop-widgets/divelistview.h"
#include "core/selection.h"
#include "profile-widget/profilewidget2.h"
#include "commands/command.h"
#include "core/metadata.h"
#include "core/tag.h"

void RenumberDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		// we remember a list from dive uuid to a new number
		QVector<QPair<dive *, int>> renumberedDives;
		int i;
		int newNr = ui.spinBox->value();
		struct dive *d;
		for_each_dive (i, d) {
			if (!selectedOnly || d->selected)
				renumberedDives.append({ d, newNr++ });
		}
		Command::renumberDives(renumberedDives);
	}
}

RenumberDialog::RenumberDialog(bool selectedOnlyIn, QWidget *parent) : QDialog(parent), selectedOnly(selectedOnlyIn)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));

	if (selectedOnly && amount_selected == 1)
		ui.renumberText->setText(tr("New number"));
	else
		ui.renumberText->setText(tr("New starting number"));

	if (selectedOnly)
		ui.groupBox->setTitle(tr("Renumber selected dives"));
	else
		ui.groupBox->setTitle(tr("Renumber all dives"));
}

void SetpointDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
		Command::addEventSetpointChange(d, dcNr, time, pressure_t { (int)(1000.0 * ui.spinbox->value()) });
}

SetpointDialog::SetpointDialog(struct dive *dIn, int dcNrIn, int seconds) : QDialog(MainWindow::instance()),
	d(dIn), dcNr(dcNrIn), time(seconds < 0 ? 0 : seconds)
{
	ui.setupUi(this);
	connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &SetpointDialog::buttonClicked);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	connect(close, &QShortcut::activated, this, &QDialog::close);
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
	connect(quit, &QShortcut::activated, MainWindow::instance(), &QWidget::close);
}

void ShiftTimesDialog::buttonClicked(QAbstractButton *button)
{
	int amount;

	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
		if (ui.backwards->isChecked())
			amount *= -1;
		if (amount != 0)
			Command::shiftTime(dives, amount);
	}
}

void ShiftTimesDialog::changeTime()
{
	int amount;

	amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
	if (ui.backwards->isChecked())
		amount *= -1;

	ui.shiftedTime->setText(get_dive_date_string(amount + when));
}

ShiftTimesDialog::ShiftTimesDialog(std::vector<dive *> dives_in, QWidget *parent) : QDialog(parent),
	when(0), dives(std::move(dives_in))
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.timeEdit, SIGNAL(timeChanged(const QTime)), this, SLOT(changeTime()));
	connect(ui.backwards, SIGNAL(toggled(bool)), this, SLOT(changeTime()));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
	when = dives[0]->when;
	ui.currentTime->setText(get_dive_date_string(when));
	ui.shiftedTime->setText(get_dive_date_string(when));
}

void ShiftImageTimesDialog::syncCameraClicked()
{
	QPixmap picture;
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
							      tr("Open image file"),
							      DiveListView::lastUsedImageDir(),
							      QString("%1 (%2)").arg(tr("Image files"), imageExtensionFilters().join(" ")));
	if (fileNames.isEmpty())
		return;

	ui.timeEdit->setEnabled(false);
	ui.backwards->setEnabled(false);
	ui.forward->setEnabled(false);

	picture.load(fileNames.at(0));
	ui.displayDC->setEnabled(true);
	QGraphicsScene *scene = new QGraphicsScene(this);

	scene->addPixmap(picture.scaled(ui.DCImage->size()));
	ui.DCImage->setScene(scene);

	dcImageEpoch = picture_get_timestamp(qPrintable(fileNames.at(0)));
	QDateTime dcDateTime = QDateTime::fromSecsSinceEpoch(dcImageEpoch, Qt::UTC);
	ui.dcTime->setDateTime(dcDateTime);
	connect(ui.dcTime, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(dcDateTimeChanged(const QDateTime &)));
}

void ShiftImageTimesDialog::dcDateTimeChanged(const QDateTime &newDateTime)
{
	QDateTime newtime(newDateTime);
	if (!dcImageEpoch)
		return;
	newtime.setTimeSpec(Qt::UTC);

	m_amount = dateTimeToTimestamp(newtime) - dcImageEpoch;
	if (m_amount)
		updateInvalid();
}

void ShiftImageTimesDialog::matchAllImagesToggled(bool state)
{
	matchAllImages = state;
}

bool ShiftImageTimesDialog::matchAll()
{
	return matchAllImages;
}

ShiftImageTimesDialog::ShiftImageTimesDialog(QWidget *parent, QStringList fileNames) : QDialog(parent),
	fileNames(fileNames),
	m_amount(0),
	matchAllImages(false)
{
	ui.setupUi(this);
	ui.timeEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{0,6}:[0-5]\\d")));
	connect(ui.syncCamera, SIGNAL(clicked()), this, SLOT(syncCameraClicked()));
	connect(ui.timeEdit, &QLineEdit::textEdited, this, &ShiftImageTimesDialog::timeEdited);
	connect(ui.backwards, &QCheckBox::toggled, this, &ShiftImageTimesDialog::backwardsChanged);
	connect(ui.matchAllImages, SIGNAL(toggled(bool)), this, SLOT(matchAllImagesToggled(bool)));
	dcImageEpoch = (time_t)0;

	// Get times of all files. 0 means that the time couldn't be determined.
	int numFiles = fileNames.size();
	timestamps.resize(numFiles);
	for (int i = 0; i < numFiles; ++i)
		timestamps[i] = picture_get_timestamp(qPrintable(fileNames[i]));
	updateInvalid();
}

time_t ShiftImageTimesDialog::amount() const
{
	return m_amount;
}

void ShiftImageTimesDialog::setOffset(time_t offset)
{
	int sign = offset >= 0 ? 1 : -1;
	time_t value = sign * offset;
	ui.timeEdit->setText(QString("%1:%2").arg(value / 3600).arg((value % 3600) / 60, 2, 10, QLatin1Char('0')));
	if (offset >= 0)
		ui.forward->setChecked(true);
	else
		ui.backwards->setChecked(true);
}

void ShiftImageTimesDialog::updateInvalid()
{
	bool allValid = true;
	ui.warningLabel->hide();
	ui.invalidFilesText->hide();
	QDateTime time_first = QDateTime::fromSecsSinceEpoch(first_selected_dive()->when, Qt::UTC);
	QDateTime time_last = QDateTime::fromSecsSinceEpoch(last_selected_dive()->when, Qt::UTC);
	if (first_selected_dive() == last_selected_dive()) {
		ui.invalidFilesText->setPlainText(tr("Selected dive date/time") + ": " + time_first.toString());
	} else {
		ui.invalidFilesText->setPlainText(tr("First selected dive date/time") + ": " + time_first.toString());
		ui.invalidFilesText->append(tr("Last selected dive date/time") + ": " + time_last.toString());
	}
	ui.invalidFilesText->append(tr("\nFiles with inappropriate date/time") + ":");

	int numFiles = fileNames.size();
	for (int i = 0; i < numFiles; ++i) {
		if (picture_check_valid_time(timestamps[i], m_amount))
			continue;

		// We've found an invalid image
		time_first.setSecsSinceEpoch(timestamps[i] + m_amount);
		if (timestamps[i] == 0)
			ui.invalidFilesText->append(fileNames[i] + " - " + tr("No Exif date/time found"));
		else
			ui.invalidFilesText->append(fileNames[i] + " - " + time_first.toString());
		allValid = false;
	}

	if (!allValid) {
		ui.warningLabel->show();
		ui.invalidFilesText->show();
	}
}

void ShiftImageTimesDialog::timeEdited(const QString &timeText)
{
	// simplistic indication of whether the string is valid
	if (ui.timeEdit->hasAcceptableInput()) {
		ui.timeEdit->setStyleSheet("");
		// parse based on the same reg exp used to validate...
		QRegularExpression re("(\\d{0,6}):(\\d\\d)");
		QRegularExpressionMatch match = re.match(timeText);
		if (match.hasMatch()) {
			time_t hours = match.captured(1).toInt();
			time_t minutes = match.captured(2).toInt();
			m_amount = (ui.backwards->isChecked() ? -1 : 1) * (3600 * hours + 60 * minutes);
			updateInvalid();
		}
	} else {
		ui.timeEdit->setStyleSheet("QLineEdit { color: red;}");
	}
}

void ShiftImageTimesDialog::backwardsChanged(bool)
{
	// simply use the timeEdit slot to deal with the sign change
	timeEdited(ui.timeEdit->text());
}

URLDialog::URLDialog(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

QString URLDialog::url() const
{
	return ui.urlField->toPlainText();
}

#define COMPONENT_FROM_UI(_component) what->_component = ui._component->isChecked()
#define UI_FROM_COMPONENT(_component) ui._component->setChecked(what->_component)

DiveComponentSelection::DiveComponentSelection(QWidget *parent, struct dive *target, struct dive_components *_what) : targetDive(target)
{
	ui.setupUi(this);
	what = _what;
	UI_FROM_COMPONENT(divesite);
	UI_FROM_COMPONENT(diveguide);
	UI_FROM_COMPONENT(buddy);
	UI_FROM_COMPONENT(rating);
	UI_FROM_COMPONENT(visibility);
	UI_FROM_COMPONENT(notes);
	UI_FROM_COMPONENT(suit);
	UI_FROM_COMPONENT(tags);
	UI_FROM_COMPONENT(cylinders);
	UI_FROM_COMPONENT(weights);
	UI_FROM_COMPONENT(number);
	UI_FROM_COMPONENT(when);
	connect(ui.buttonBox, &QDialogButtonBox::clicked, this, &DiveComponentSelection::buttonClicked);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	connect(close, &QShortcut::activated, this, &DiveComponentSelection::close);
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
	connect(quit, &QShortcut::activated, parent, &QWidget::close);
}

void DiveComponentSelection::buttonClicked(QAbstractButton *button)
{
	if (current_dive && ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		COMPONENT_FROM_UI(divesite);
		COMPONENT_FROM_UI(diveguide);
		COMPONENT_FROM_UI(buddy);
		COMPONENT_FROM_UI(rating);
		COMPONENT_FROM_UI(visibility);
		COMPONENT_FROM_UI(notes);
		COMPONENT_FROM_UI(suit);
		COMPONENT_FROM_UI(tags);
		COMPONENT_FROM_UI(cylinders);
		COMPONENT_FROM_UI(weights);
		COMPONENT_FROM_UI(number);
		COMPONENT_FROM_UI(when);
		selective_copy_dive(current_dive, targetDive, *what, true);
		QClipboard *clipboard = QApplication::clipboard();
		QTextStream text;
		QString cliptext;
		text.setString(&cliptext);
		if (what->divesite && current_dive->dive_site)
			text << tr("Dive site: ") << current_dive->dive_site->name << "\n";
		if (what->diveguide)
			text << tr("Dive guide: ") << current_dive->diveguide << "\n";
		if (what->buddy)
			text << tr("Buddy: ") << current_dive->buddy << "\n";
		if (what->rating)
			text << tr("Rating: ") + QString("*").repeated(current_dive->rating) << "\n";
		if (what->visibility)
			text << tr("Visibility: ") + QString("*").repeated(current_dive->visibility) << "\n";
		if (what->notes)
			text << tr("Notes:\n") << current_dive->notes << "\n";
		if (what->suit)
			text << tr("Suit: ") << current_dive->suit << "\n";
		if (what-> tags) {
			text << tr("Tags: ");
			tag_entry *entry = current_dive->tag_list;
			while (entry) {
				text << entry->tag->name << " ";
				entry = entry->next;
			}
			text << "\n";
		}
		if (what->cylinders) {
			int cyl;
			text << tr("Cylinders:\n");
			for (cyl = 0; cyl < current_dive->cylinders.nr; cyl++) {
				if (is_cylinder_used(current_dive, cyl))
					text << get_cylinder(current_dive, cyl)->type.description << " " << gasname(get_cylinder(current_dive, cyl)->gasmix) << "\n";
			}
		}
		if (what->weights) {
			int w;
			text << tr("Weights:\n");
			for (w = 0; w < current_dive->weightsystems.nr; w++) {
				weightsystem_t ws = current_dive->weightsystems.weightsystems[w];
				text << ws.description << ws.weight.grams / 1000 << "kg\n";
			}
		}
		if (what->number)
			text << tr("Dive number: ") << current_dive->number << "\n";
		if (what->when)
			text << tr("Date / time: ") << get_dive_date_string(current_dive->when) << "\n";
		clipboard->setText(cliptext);
	}
}

AddFilterPresetDialog::AddFilterPresetDialog(const QString &defaultName, QWidget *parent)
{
	ui.setupUi(this);
	ui.name->setText(defaultName);
	connect(ui.name, &QLineEdit::textChanged, this, &AddFilterPresetDialog::nameChanged);
	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &AddFilterPresetDialog::accept);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &AddFilterPresetDialog::reject);
	nameChanged(ui.name->text());

	// Create a completer so that the user can easily overwrite existing presets.
	QStringList presets;
	int count = filter_presets_count();
	presets.reserve(count);
	for (int i = 0; i < count; ++i)
		presets.push_back(filter_preset_name_qstring(i));
	QCompleter *completer = new QCompleter(presets, this);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	ui.name->setCompleter(completer);
}

void AddFilterPresetDialog::nameChanged(const QString &text)
{
	QString trimmed = text.trimmed();
	bool isEmpty = trimmed.isEmpty();
	bool exists = !isEmpty && filter_preset_id(trimmed) >= 0;
	ui.duplicateWarning->setVisible(exists);
	ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!isEmpty);
}

QString AddFilterPresetDialog::doit()
{
	if (exec() == QDialog::Accepted)
		return ui.name->text().trimmed();
	return QString();
}

TextHyperlinkEventFilter::TextHyperlinkEventFilter(QTextEdit *txtEdit) : QObject(txtEdit),
	textEdit(txtEdit),
	scrollView(textEdit->viewport())
{
	// If you install the filter on textEdit, you fail to capture any clicks.
	// The clicks go to the viewport. http://stackoverflow.com/a/31582977/10278
	textEdit->viewport()->installEventFilter(this);
}

bool TextHyperlinkEventFilter::eventFilter(QObject *target, QEvent *evt)
{
	if (target != scrollView)
		return false;

	if (evt->type() != QEvent::MouseButtonPress &&
	    evt->type() != QEvent::ToolTip)
		return false;

	// --------------------

	// Note: Qt knows that on Mac OSX, ctrl (and Control) are the command key.
	const bool isCtrlClick = evt->type() == QEvent::MouseButtonPress &&
				 static_cast<QMouseEvent *>(evt)->modifiers() & Qt::ControlModifier &&
				 static_cast<QMouseEvent *>(evt)->button() == Qt::LeftButton;

	const bool isTooltip = evt->type() == QEvent::ToolTip;

	QString urlUnderCursor;

	if (isCtrlClick || isTooltip) {
		QTextCursor cursor = isCtrlClick ?
					     textEdit->cursorForPosition(static_cast<QMouseEvent *>(evt)->pos()) :
					     textEdit->cursorForPosition(static_cast<QHelpEvent *>(evt)->pos());

		urlUnderCursor = tryToFormulateUrl(&cursor);
	}

	if (isCtrlClick) {
		handleUrlClick(urlUnderCursor);
	}

	if (isTooltip) {
		handleUrlTooltip(urlUnderCursor, static_cast<QHelpEvent *>(evt)->globalPos());
	}

	// 'return true' would mean that all event handling stops for this event.
	// 'return false' lets Qt continue propagating the event to the target.
	// Since our URL behavior is meant as 'additive' and not necessarily mutually
	// exclusive with any default behaviors, it seems ok to return false to
	// avoid unintentially hijacking any 'normal' event handling.
	return false;
}

void TextHyperlinkEventFilter::handleUrlClick(const QString &urlStr)
{
	if (!urlStr.isEmpty()) {
		QUrl url(urlStr, QUrl::StrictMode);
		QDesktopServices::openUrl(url);
	}
}

void TextHyperlinkEventFilter::handleUrlTooltip(const QString &urlStr, const QPoint &pos)
{
	if (urlStr.isEmpty()) {
		QToolTip::hideText();
	} else {
		// per Qt docs, QKeySequence::toString does localization "tr()" on strings like Ctrl.
		// Note: Qt knows that on Mac OSX, ctrl (and Control) are the command key.
		const QString ctrlKeyName = QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText);
		// ctrlKeyName comes with a trailing '+', as in: 'Ctrl+'
		QToolTip::showText(pos, tr("%1click to visit %2").arg(ctrlKeyName).arg(urlStr));
	}
}

bool TextHyperlinkEventFilter::stringMeetsOurUrlRequirements(const QString &maybeUrlStr)
{
	QUrl url(maybeUrlStr, QUrl::StrictMode);
	return url.isValid() && (!url.scheme().isEmpty()) && ((!url.authority().isEmpty()) || (!url.path().isEmpty()));
}

QString TextHyperlinkEventFilter::tryToFormulateUrl(QTextCursor *cursor)
{
	// tryToFormulateUrl exists because WordUnderCursor will not
	// treat "http://m.abc.def" as a word.

	// tryToFormulateUrl invokes fromCursorTilWhitespace two times (once
	// with a forward moving cursor and once in the backwards direction) in
	// order to expand the selection to try to capture a complete string
	// like "http://m.abc.def"

	// loosely inspired by advice here: http://stackoverflow.com/q/19262064/10278

	cursor->select(QTextCursor::WordUnderCursor);
	QString maybeUrlStr = cursor->selectedText();

	const bool soFarSoGood = !maybeUrlStr.simplified().replace(" ", "").isEmpty();

	if (soFarSoGood && !stringMeetsOurUrlRequirements(maybeUrlStr)) {
		// If we don't yet have a full url, try to expand til we get one.  Note:
		// after requesting WordUnderCursor, empirically (all platforms, in
		// Qt5), the 'anchor' is just past the end of the word.

		QTextCursor cursor2(*cursor);
		QString left = fromCursorTilWhitespace(cursor, true /*searchBackwards*/);
		QString right = fromCursorTilWhitespace(&cursor2, false);
		maybeUrlStr = left + right;
	}

	return stringMeetsOurUrlRequirements(maybeUrlStr) ? std::move(maybeUrlStr) : QString();
}

QString TextHyperlinkEventFilter::fromCursorTilWhitespace(QTextCursor *cursor, bool searchBackwards)
{
	// fromCursorTilWhitespace calls cursor->movePosition repeatedly, while
	// preserving the original 'anchor' (qt terminology) of the cursor.
	// We widen the selection with 'movePosition' until hitting any whitespace.

	QString result;
	QString grownText;
	QString noSpaces;
	bool movedOk = false;
	int oldSize = -1;

	do {
		result = grownText; // this is a no-op on the first visit.

		if (searchBackwards) {
			movedOk = cursor->movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
		} else {
			movedOk = cursor->movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
		}

		grownText = cursor->selectedText();
		if (grownText.size() == oldSize)
			movedOk = false;
		oldSize = grownText.size();
		noSpaces = grownText.simplified().replace(" ", "");
	} while (grownText == noSpaces && movedOk);

	// while growing the selection forwards, we have an extra step to do:
	if (!searchBackwards) {
		/*
		  The cursor keeps jumping to the start of the next word.
		  (for example) in the string "mn.abcd.edu is the spot" you land at
		  m,a,e,i (the 'i' in 'is). if we stop at e, then we only capture
		  "mn.abcd." for the url (wrong). So we have to go to 'i', to
		  capture "mn.abcd.edu " (with trailing space), and then clean it up.
		*/
		QStringList list = grownText.split(QRegularExpression("\\s"), SKIP_EMPTY);
		if (!list.isEmpty()) {
			result = list[0];
		}
	}

	return result;
}
