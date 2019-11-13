// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/simplewidgets.h"
#include "qt-models/filtermodels.h"

#include <QProcess>
#include <QFileDialog>
#include <QShortcut>
#include <QCalendarWidget>
#include <QKeyEvent>
#include <QAction>
#include <QDesktopServices>
#include <QToolTip>
#include <QClipboard>

#include "core/file.h"
#include "core/divesite.h"
#include "desktop-widgets/mainwindow.h"
#include "core/qthelper.h"
#include "libdivecomputer/parser.h"
#include "desktop-widgets/divelistview.h"
#include "core/display.h"
#include "profile-widget/profilewidget2.h"
#include "commands/command.h"
#include "core/metadata.h"
#include "core/tag.h"
#include "core/divelist.h" // for mark_divelist_changed

double MinMaxAvgWidget::average() const
{
	return avgValue->text().toDouble();
}

double MinMaxAvgWidget::maximum() const
{
	return maxValue->text().toDouble();
}

double MinMaxAvgWidget::minimum() const
{
	return minValue->text().toDouble();
}

MinMaxAvgWidget::MinMaxAvgWidget(QWidget *parent) : QWidget(parent)
{
	avgIco = new QLabel(this);
	avgIco->setPixmap(QIcon(":value-average-icon").pixmap(16, 16));
	avgIco->setToolTip(gettextFromC::tr("Average"));
	minIco = new QLabel(this);
	minIco->setPixmap(QIcon(":value-minimum-icon").pixmap(16, 16));
	minIco->setToolTip(gettextFromC::tr("Minimum"));
	maxIco = new QLabel(this);
	maxIco->setPixmap(QIcon(":value-maximum-icon").pixmap(16, 16));
	maxIco->setToolTip(gettextFromC::tr("Maximum"));
	avgValue = new QLabel(this);
	minValue = new QLabel(this);
	maxValue = new QLabel(this);

	QGridLayout *formLayout = new QGridLayout;
	formLayout->addWidget(maxIco, 0, 0);
	formLayout->addWidget(maxValue, 0, 1);
	formLayout->addWidget(avgIco, 1, 0);
	formLayout->addWidget(avgValue, 1, 1);
	formLayout->addWidget(minIco, 2, 0);
	formLayout->addWidget(minValue, 2, 1);
	setLayout(formLayout);
}

void MinMaxAvgWidget::clear()
{
	avgValue->setText(QString());
	maxValue->setText(QString());
	minValue->setText(QString());
}

void MinMaxAvgWidget::setAverage(double average)
{
	avgValue->setText(QString::number(average));
}

void MinMaxAvgWidget::setMaximum(double maximum)
{
	maxValue->setText(QString::number(maximum));
}
void MinMaxAvgWidget::setMinimum(double minimum)
{
	minValue->setText(QString::number(minimum));
}

void MinMaxAvgWidget::setAverage(const QString &average)
{
	avgValue->setText(average);
}

void MinMaxAvgWidget::setMaximum(const QString &maximum)
{
	maxValue->setText(maximum);
}

void MinMaxAvgWidget::setMinimum(const QString &minimum)
{
	minValue->setText(minimum);
}

void MinMaxAvgWidget::overrideMinToolTipText(const QString &newTip)
{
	minIco->setToolTip(newTip);
	minValue->setToolTip(newTip);
}

void MinMaxAvgWidget::overrideAvgToolTipText(const QString &newTip)
{
	avgIco->setToolTip(newTip);
	avgValue->setToolTip(newTip);
}

void MinMaxAvgWidget::overrideMaxToolTipText(const QString &newTip)
{
	maxIco->setToolTip(newTip);
	maxValue->setToolTip(newTip);
}

void MinMaxAvgWidget::setAvgVisibility(bool visible)
{
	avgIco->setVisible(visible);
	avgValue->setVisible(visible);
}

RenumberDialog *RenumberDialog::instance()
{
	static RenumberDialog *self = new RenumberDialog(MainWindow::instance());
	return self;
}

void RenumberDialog::renumberOnlySelected(bool selected)
{
	if (selected && amount_selected == 1)
		ui.renumberText->setText(tr("New number"));
	else
		ui.renumberText->setText(tr("New starting number"));

	if (selected)
		ui.groupBox->setTitle(tr("Renumber selected dives"));
	else
		ui.groupBox->setTitle(tr("Renumber all dives"));

	selectedOnly = selected;
}

void RenumberDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		MainWindow::instance()->diveList->rememberSelection();
		// we remember a list from dive uuid to a new number
		QVector<QPair<dive *, int>> renumberedDives;
		int i;
		int newNr = ui.spinBox->value();
		struct dive *d;
		for_each_dive (i, d) {
			if (!selectedOnly || d->selected) {
				invalidate_dive_cache(d);
				renumberedDives.append({ d, newNr++ });
			}
		}
		Command::renumberDives(renumberedDives);
	}
}

RenumberDialog::RenumberDialog(QWidget *parent) : QDialog(parent), selectedOnly(false)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

SetpointDialog *SetpointDialog::instance()
{
	static SetpointDialog *self = new SetpointDialog(MainWindow::instance());
	return self;
}

void SetpointDialog::setpointData(struct divecomputer *divecomputer, int second)
{
	dc = divecomputer;
	time = second < 0 ? 0 : second;
}

void SetpointDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole && dc) {
		add_event(dc, time, SAMPLE_EVENT_PO2, 0, (int)(1000.0 * ui.spinbox->value()),
			QT_TRANSLATE_NOOP("gettextFromC", "SP change"));
		invalidate_dive_cache(current_dive);
	}
	mark_divelist_changed(true);
	MainWindow::instance()->graphics->replot();
}

SetpointDialog::SetpointDialog(QWidget *parent) : QDialog(parent),
	dc(0), time(0)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

ShiftTimesDialog *ShiftTimesDialog::instance()
{
	static ShiftTimesDialog *self = new ShiftTimesDialog(MainWindow::instance());
	return self;
}

void ShiftTimesDialog::buttonClicked(QAbstractButton *button)
{
	int amount;

	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
		if (ui.backwards->isChecked())
			amount *= -1;
		if (amount != 0) {
			// DANGER, DANGER - this could get our dive_table unsorted...
			int i;
			struct dive *d;
			QVector<dive *> affectedDives;
			for_each_dive (i, d) {
				if (d->selected)
					affectedDives.append(d);
			}
			Command::shiftTime(affectedDives, amount);
		}
	}
}

void ShiftTimesDialog::showEvent(QShowEvent*)
{
	ui.timeEdit->setTime(QTime(0, 0, 0, 0));
	when = get_times(); //get time of first selected dive
	ui.currentTime->setText(get_dive_date_string(when));
	ui.shiftedTime->setText(get_dive_date_string(when));
}

void ShiftTimesDialog::changeTime()
{
	int amount;

	amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
	if (ui.backwards->isChecked())
		amount *= -1;

	ui.shiftedTime->setText(get_dive_date_string(amount + when));
}

ShiftTimesDialog::ShiftTimesDialog(QWidget *parent) : QDialog(parent),
	when(0)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.timeEdit, SIGNAL(timeChanged(const QTime)), this, SLOT(changeTime()));
	connect(ui.backwards, SIGNAL(toggled(bool)), this, SLOT(changeTime()));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void ShiftImageTimesDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		m_amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
		if (ui.backwards->isChecked())
			m_amount *= -1;
	}
}

void ShiftImageTimesDialog::syncCameraClicked()
{
	QPixmap picture;
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
							      tr("Open image file"),
							      DiveListView::lastUsedImageDir(),
							      tr("Image files") + " (*.jpg *.jpeg)");
	if (fileNames.isEmpty())
		return;

	picture.load(fileNames.at(0));
	ui.displayDC->setEnabled(true);
	QGraphicsScene *scene = new QGraphicsScene(this);

	scene->addPixmap(picture.scaled(ui.DCImage->size()));
	ui.DCImage->setScene(scene);

	dcImageEpoch = picture_get_timestamp(qPrintable(fileNames.at(0)));
	QDateTime dcDateTime = QDateTime::fromTime_t(dcImageEpoch, Qt::UTC);
	ui.dcTime->setDateTime(dcDateTime);
	connect(ui.dcTime, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(dcDateTimeChanged(const QDateTime &)));
}

void ShiftImageTimesDialog::dcDateTimeChanged(const QDateTime &newDateTime)
{
	QDateTime newtime(newDateTime);
	if (!dcImageEpoch)
		return;
	newtime.setTimeSpec(Qt::UTC);
	setOffset(newtime.toTime_t() - dcImageEpoch);
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
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.syncCamera, SIGNAL(clicked()), this, SLOT(syncCameraClicked()));
	connect(ui.timeEdit, SIGNAL(timeChanged(const QTime &)), this, SLOT(timeEditChanged(const QTime &)));
	connect(ui.backwards, SIGNAL(toggled(bool)), this, SLOT(timeEditChanged()));
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
	if (offset >= 0) {
		ui.forward->setChecked(true);
	} else {
		ui.backwards->setChecked(true);
		offset *= -1;
	}
	ui.timeEdit->setTime(QTime(offset / 3600, (offset % 3600) / 60, offset % 60));
}

void ShiftImageTimesDialog::updateInvalid()
{
	bool allValid = true;
	ui.warningLabel->hide();
	ui.invalidFilesText->hide();
	QDateTime time_first = QDateTime::fromTime_t(first_selected_dive()->when, Qt::UTC);
	QDateTime time_last = QDateTime::fromTime_t(last_selected_dive()->when, Qt::UTC);
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
		time_first.setTime_t(timestamps[i] + m_amount);
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

void ShiftImageTimesDialog::timeEditChanged(const QTime &time)
{
	QDateTimeEdit::Section timeEditSection = ui.timeEdit->currentSection();
	ui.timeEdit->setEnabled(false);
	m_amount = time.hour() * 3600 + time.minute() * 60;
	if (ui.backwards->isChecked())
		m_amount *= -1;
	updateInvalid();
	ui.timeEdit->setEnabled(true);
	ui.timeEdit->setFocus();
	ui.timeEdit->setSelectedSection(timeEditSection);
}

void ShiftImageTimesDialog::timeEditChanged()
{
	if ((m_amount > 0) == ui.backwards->isChecked())
		m_amount *= -1;
	if (m_amount)
		updateInvalid();
}

URLDialog::URLDialog(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

QString URLDialog::url() const
{
	return ui.urlField->text();
}

bool isGnome3Session()
{
#if defined(QT_OS_WIW) || defined(QT_OS_MAC)
	return false;
#else
	if (qApp->style()->objectName() != "gtk+")
		return false;
	QProcess p;
	p.start("pidof", QStringList() << "gnome-shell");
	p.waitForFinished(-1);
	QString p_stdout = p.readAllStandardOutput();
	return !p_stdout.isEmpty();
#endif
}

#define COMPONENT_FROM_UI(_component) what->_component = ui._component->isChecked()
#define UI_FROM_COMPONENT(_component) ui._component->setChecked(what->_component)

DiveComponentSelection::DiveComponentSelection(QWidget *parent, struct dive *target, struct dive_components *_what) : targetDive(target)
{
	ui.setupUi(this);
	what = _what;
	UI_FROM_COMPONENT(divesite);
	UI_FROM_COMPONENT(divemaster);
	UI_FROM_COMPONENT(buddy);
	UI_FROM_COMPONENT(rating);
	UI_FROM_COMPONENT(visibility);
	UI_FROM_COMPONENT(notes);
	UI_FROM_COMPONENT(suit);
	UI_FROM_COMPONENT(tags);
	UI_FROM_COMPONENT(cylinders);
	UI_FROM_COMPONENT(weights);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void DiveComponentSelection::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		COMPONENT_FROM_UI(divesite);
		COMPONENT_FROM_UI(divemaster);
		COMPONENT_FROM_UI(buddy);
		COMPONENT_FROM_UI(rating);
		COMPONENT_FROM_UI(visibility);
		COMPONENT_FROM_UI(notes);
		COMPONENT_FROM_UI(suit);
		COMPONENT_FROM_UI(tags);
		COMPONENT_FROM_UI(cylinders);
		COMPONENT_FROM_UI(weights);
		selective_copy_dive(&displayed_dive, targetDive, *what, true);
		QClipboard *clipboard = QApplication::clipboard();
		QTextStream text;
		QString cliptext;
		text.setString(&cliptext);
		if (what->divesite && displayed_dive.dive_site)
			text << tr("Dive site: ") << displayed_dive.dive_site->name << "\n";
		if (what->divemaster)
			text << tr("Dive master: ") << displayed_dive.divemaster << "\n";
		if (what->buddy)
			text << tr("Buddy: ") << displayed_dive.buddy << "\n";
		if (what->rating)
			text << tr("Rating: ") + QString("*").repeated(displayed_dive.rating) << "\n";
		if (what->visibility)
			text << tr("Visibility: ") + QString("*").repeated(displayed_dive.visibility) << "\n";
		if (what->notes)
			text << tr("Notes:\n") << displayed_dive.notes << "\n";
		if (what->suit)
			text << tr("Suit: ") << displayed_dive.suit << "\n";
		if (what-> tags) {
			text << tr("Tags: ");
			tag_entry *entry = displayed_dive.tag_list;
			while (entry) {
				text << entry->tag->name << " ";
				entry = entry->next;
			}
			text << "\n";
		}
		if (what->cylinders) {
			int cyl;
			text << tr("Cylinders:\n");
			for (cyl = 0; cyl < displayed_dive.cylinders.nr; cyl++) {
				if (is_cylinder_used(&displayed_dive, cyl))
					text << get_cylinder(&displayed_dive, cyl)->type.description << " " << gasname(get_cylinder(&displayed_dive, cyl)->gasmix) << "\n";
			}
		}
		if (what->weights) {
			int w;
			text << tr("Weights:\n");
			for (w = 0; w < displayed_dive.weightsystems.nr; w++) {
				weightsystem_t ws = displayed_dive.weightsystems.weightsystems[w];
				text << ws.description << ws.weight.grams / 1000 << "kg\n";
			}
		}
		clipboard->setText(cliptext);
	}
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

	return stringMeetsOurUrlRequirements(maybeUrlStr) ? maybeUrlStr : QString::null;
}

QString TextHyperlinkEventFilter::fromCursorTilWhitespace(QTextCursor *cursor, const bool searchBackwards)
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
		QStringList list = grownText.split(QRegExp("\\s"), QString::SkipEmptyParts);
		if (!list.isEmpty()) {
			result = list[0];
		}
	}

	return result;
}
