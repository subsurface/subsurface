#include "simplewidgets.h"
#include "filtermodels.h"

#include <QProcess>
#include <QFileDialog>
#include <QShortcut>
#include <QCalendarWidget>
#include <QKeyEvent>
#include <QAction>
#include <QDesktopServices>
#include <QToolTip>

#include "file.h"
#include "mainwindow.h"
#include "helpers.h"
#include "libdivecomputer/parser.h"
#include "divelistview.h"
#include "display.h"
#include "profile-widget/profilewidget2.h"
#include "undocommands.h"

class MinMaxAvgWidgetPrivate {
public:
	QLabel *avgIco, *avgValue;
	QLabel *minIco, *minValue;
	QLabel *maxIco, *maxValue;

	MinMaxAvgWidgetPrivate(MinMaxAvgWidget *owner)
	{
		avgIco = new QLabel(owner);
		avgIco->setPixmap(QIcon(":/average").pixmap(16, 16));
		avgIco->setToolTip(QObject::tr("Average"));
		minIco = new QLabel(owner);
		minIco->setPixmap(QIcon(":/minimum").pixmap(16, 16));
		minIco->setToolTip(QObject::tr("Minimum"));
		maxIco = new QLabel(owner);
		maxIco->setPixmap(QIcon(":/maximum").pixmap(16, 16));
		maxIco->setToolTip(QObject::tr("Maximum"));
		avgValue = new QLabel(owner);
		minValue = new QLabel(owner);
		maxValue = new QLabel(owner);

		QGridLayout *formLayout = new QGridLayout();
		formLayout->addWidget(maxIco, 0, 0);
		formLayout->addWidget(maxValue, 0, 1);
		formLayout->addWidget(avgIco, 1, 0);
		formLayout->addWidget(avgValue, 1, 1);
		formLayout->addWidget(minIco, 2, 0);
		formLayout->addWidget(minValue, 2, 1);
		owner->setLayout(formLayout);
	}
};

double MinMaxAvgWidget::average() const
{
	return d->avgValue->text().toDouble();
}

double MinMaxAvgWidget::maximum() const
{
	return d->maxValue->text().toDouble();
}
double MinMaxAvgWidget::minimum() const
{
	return d->minValue->text().toDouble();
}

MinMaxAvgWidget::MinMaxAvgWidget(QWidget *parent) : d(new MinMaxAvgWidgetPrivate(this))
{
}

MinMaxAvgWidget::~MinMaxAvgWidget()
{
}

void MinMaxAvgWidget::clear()
{
	d->avgValue->setText(QString());
	d->maxValue->setText(QString());
	d->minValue->setText(QString());
}

void MinMaxAvgWidget::setAverage(double average)
{
	d->avgValue->setText(QString::number(average));
}

void MinMaxAvgWidget::setMaximum(double maximum)
{
	d->maxValue->setText(QString::number(maximum));
}
void MinMaxAvgWidget::setMinimum(double minimum)
{
	d->minValue->setText(QString::number(minimum));
}

void MinMaxAvgWidget::setAverage(const QString &average)
{
	d->avgValue->setText(average);
}

void MinMaxAvgWidget::setMaximum(const QString &maximum)
{
	d->maxValue->setText(maximum);
}

void MinMaxAvgWidget::setMinimum(const QString &minimum)
{
	d->minValue->setText(minimum);
}

void MinMaxAvgWidget::overrideMinToolTipText(const QString &newTip)
{
	d->minIco->setToolTip(newTip);
	d->minValue->setToolTip(newTip);
}

void MinMaxAvgWidget::overrideAvgToolTipText(const QString &newTip)
{
	d->avgIco->setToolTip(newTip);
	d->avgValue->setToolTip(newTip);
}

void MinMaxAvgWidget::overrideMaxToolTipText(const QString &newTip)
{
	d->maxIco->setToolTip(newTip);
	d->maxValue->setToolTip(newTip);
}

RenumberDialog *RenumberDialog::instance()
{
	static RenumberDialog *self = new RenumberDialog(MainWindow::instance());
	return self;
}

void RenumberDialog::renumberOnlySelected(bool selected)
{
	if (selected && amount_selected == 1)
		ui.groupBox->setTitle(tr("New number"));
	else
		ui.groupBox->setTitle(tr("New starting number"));
	selectedOnly = selected;
}

void RenumberDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		MainWindow::instance()->dive_list()->rememberSelection();
		// we remember a map from dive uuid to a pair of old number / new number
		QMap<int,QPair<int, int> > renumberedDives;
		int i;
		int newNr = ui.spinBox->value();
		struct dive *dive = NULL;
		for_each_dive (i, dive) {
			if (!selectedOnly || dive->selected)
				renumberedDives.insert(dive->id, QPair<int,int>(dive->number, newNr++));
		}
		UndoRenumberDives *undoCommand = new UndoRenumberDives(renumberedDives);
		MainWindow::instance()->undoStack->push(undoCommand);

		MainWindow::instance()->dive_list()->fixMessyQtModelBehaviour();
		mark_divelist_changed(true);
		MainWindow::instance()->dive_list()->restoreSelection();
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
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole && dc)
		add_event(dc, time, SAMPLE_EVENT_PO2, 0, (int)(1000.0 * ui.spinbox->value()), "SP change");
	mark_divelist_changed(true);
	MainWindow::instance()->graphics()->replot();
}

SetpointDialog::SetpointDialog(QWidget *parent) :
	QDialog(parent),
	dc(0)
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
			struct dive *dive;
			QList<int> affectedDives;
			for_each_dive (i, dive) {
				if (!dive->selected)
					continue;

				affectedDives.append(dive->id);
			}
			MainWindow::instance()->undoStack->push(new UndoShiftTime(affectedDives, amount));
			sort_table(&dive_table);
			mark_divelist_changed(true);
			MainWindow::instance()->dive_list()->rememberSelection();
			MainWindow::instance()->refreshDisplay();
			MainWindow::instance()->dive_list()->restoreSelection();
		}
	}
}

void ShiftTimesDialog::showEvent(QShowEvent *event)
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

ShiftTimesDialog::ShiftTimesDialog(QWidget *parent) :
	QDialog(parent),
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
	QDateTime dcDateTime = QDateTime();
	QStringList fileNames = QFileDialog::getOpenFileNames(this,
							      tr("Open image file"),
							      DiveListView::lastUsedImageDir(),
							      tr("Image files (*.jpg *.jpeg *.pnm *.tif *.tiff)"));
	if (fileNames.isEmpty())
		return;

	picture.load(fileNames.at(0));
	ui.displayDC->setEnabled(true);
	QGraphicsScene *scene = new QGraphicsScene(this);

	scene->addPixmap(picture.scaled(ui.DCImage->size()));
	ui.DCImage->setScene(scene);

	dcImageEpoch = picture_get_timestamp(fileNames.at(0).toUtf8().data());
	dcDateTime.setTime_t(dcImageEpoch - gettimezoneoffset(displayed_dive.when));
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

ShiftImageTimesDialog::ShiftImageTimesDialog(QWidget *parent, QStringList fileNames) :
	QDialog(parent),
	fileNames(fileNames),
	m_amount(0),
	matchAllImages(false)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.syncCamera, SIGNAL(clicked()), this, SLOT(syncCameraClicked()));
	connect(ui.timeEdit, SIGNAL(timeChanged(const QTime &)), this, SLOT(timeEditChanged(const QTime &)));
	connect(ui.matchAllImages, SIGNAL(toggled(bool)), this, SLOT(matchAllImagesToggled(bool)));
	dcImageEpoch = (time_t)0;
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
	timestamp_t timestamp;
	QDateTime time;
	bool allValid = true;
	ui.warningLabel->hide();
	ui.invalidLabel->hide();
	time.setTime_t(displayed_dive.when - gettimezoneoffset(displayed_dive.when));
	ui.invalidLabel->setText("Dive:" + time.toString() + "\n");

	Q_FOREACH (const QString &fileName, fileNames) {
		if (picture_check_valid(fileName.toUtf8().data(), m_amount))
			continue;

		// We've found invalid image
		timestamp = picture_get_timestamp(fileName.toUtf8().data());
		time.setTime_t(timestamp + m_amount - gettimezoneoffset(displayed_dive.when));
		ui.invalidLabel->setText(ui.invalidLabel->text() + fileName + " " + time.toString() + "\n");
		allValid = false;
	}

	if (!allValid){
		ui.warningLabel->show();
		ui.invalidLabel->show();
	}
}

void ShiftImageTimesDialog::timeEditChanged(const QTime &time)
{
	m_amount = time.hour() * 3600 + time.minute() * 60;
	if (ui.backwards->isChecked())
			m_amount *= -1;
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
	return ui.urlField->toPlainText();
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

DateWidget::DateWidget(QWidget *parent) : QWidget(parent),
	calendarWidget(new QCalendarWidget())
{
	setDate(QDate::currentDate());
	setMinimumSize(QSize(80, 64));
	setFocusPolicy(Qt::StrongFocus);
	calendarWidget->setWindowFlags(Qt::FramelessWindowHint);
	calendarWidget->setFirstDayOfWeek(getLocale().firstDayOfWeek());
	calendarWidget->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

	connect(calendarWidget, SIGNAL(activated(QDate)), calendarWidget, SLOT(hide()));
	connect(calendarWidget, SIGNAL(clicked(QDate)), calendarWidget, SLOT(hide()));
	connect(calendarWidget, SIGNAL(activated(QDate)), this, SLOT(setDate(QDate)));
	connect(calendarWidget, SIGNAL(clicked(QDate)), this, SLOT(setDate(QDate)));
	calendarWidget->installEventFilter(this);
}

bool DateWidget::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::FocusOut) {
		calendarWidget->hide();
		return true;
	}
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *ev = static_cast<QKeyEvent *>(event);
		if (ev->key() == Qt::Key_Escape) {
			calendarWidget->hide();
		}
	}
	return QObject::eventFilter(object, event);
}


void DateWidget::setDate(const QDate &date)
{
	mDate = date;
	update();
	emit dateChanged(mDate);
}

QDate DateWidget::date() const
{
	return mDate;
}

void DateWidget::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::EnabledChange) {
		update();
	}
}

#define DATEWIDGETWIDTH 80
void DateWidget::paintEvent(QPaintEvent *event)
{
	static QPixmap pix = QPixmap(":/calendar").scaled(DATEWIDGETWIDTH, 64);

	QPainter painter(this);

	painter.drawPixmap(QPoint(0, 0), isEnabled() ? pix : QPixmap::fromImage(grayImage(pix.toImage())));

	QString month = mDate.toString("MMM");
	QString year = mDate.toString("yyyy");
	QString day = mDate.toString("dd");

	QFont font = QFont("monospace", 10);
	QFontMetrics metrics = QFontMetrics(font);
	painter.setFont(font);
	painter.setPen(QPen(QBrush(Qt::white), 0));
	painter.setBrush(QBrush(Qt::white));
	painter.drawText(QPoint(6, metrics.height() + 1), month);
	painter.drawText(QPoint(DATEWIDGETWIDTH - metrics.width(year) - 6, metrics.height() + 1), year);

	font.setPointSize(14);
	metrics = QFontMetrics(font);
	painter.setPen(QPen(QBrush(Qt::black), 0));
	painter.setBrush(Qt::black);
	painter.setFont(font);
	painter.drawText(QPoint(DATEWIDGETWIDTH / 2 - metrics.width(day) / 2, 45), day);

	if (hasFocus()) {
		QStyleOptionFocusRect option;
		option.initFrom(this);
		option.backgroundColor = palette().color(QPalette::Background);
		style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &painter, this);
	}
}

void DateWidget::mousePressEvent(QMouseEvent *event)
{
	calendarWidget->setSelectedDate(mDate);
	calendarWidget->move(event->globalPos());
	calendarWidget->show();
	calendarWidget->raise();
	calendarWidget->setFocus();
}

void DateWidget::focusInEvent(QFocusEvent *event)
{
	setFocus();
	QWidget::focusInEvent(event);
}

void DateWidget::focusOutEvent(QFocusEvent *event)
{
	QWidget::focusOutEvent(event);
}

void DateWidget::keyPressEvent(QKeyEvent *event)
{
	if (event->key() == Qt::Key_Return ||
	    event->key() == Qt::Key_Enter ||
	    event->key() == Qt::Key_Space) {
		calendarWidget->move(mapToGlobal(QPoint(0, 64)));
		calendarWidget->show();
		event->setAccepted(true);
	} else {
		QWidget::keyPressEvent(event);
	}
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
	}
}

TagFilter::TagFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(tr("Tags: "));
#if QT_VERSION >= 0x050200
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(TagFilterModel::instance());
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(ui.filterInternalList, SIGNAL(textChanged(QString)), filter, SLOT(setFilterFixedString(QString)));
	ui.filterList->setModel(filter);
}

void TagFilter::showEvent(QShowEvent *event)
{
	MultiFilterSortModel::instance()->addFilterModel(TagFilterModel::instance());
	QWidget::showEvent(event);
}

void TagFilter::hideEvent(QHideEvent *event)
{
	MultiFilterSortModel::instance()->removeFilterModel(TagFilterModel::instance());
	QWidget::hideEvent(event);
}

BuddyFilter::BuddyFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(tr("Person: "));
	ui.label->setToolTip(tr("Searches for buddies and divemasters"));
#if QT_VERSION >= 0x050200
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(BuddyFilterModel::instance());
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(ui.filterInternalList, SIGNAL(textChanged(QString)), filter, SLOT(setFilterFixedString(QString)));
	ui.filterList->setModel(filter);
}

void BuddyFilter::showEvent(QShowEvent *event)
{
	MultiFilterSortModel::instance()->addFilterModel(BuddyFilterModel::instance());
	QWidget::showEvent(event);
}

void BuddyFilter::hideEvent(QHideEvent *event)
{
	MultiFilterSortModel::instance()->removeFilterModel(BuddyFilterModel::instance());
	QWidget::hideEvent(event);
}

LocationFilter::LocationFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(tr("Location: "));
#if QT_VERSION >= 0x050200
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(LocationFilterModel::instance());
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(ui.filterInternalList, SIGNAL(textChanged(QString)), filter, SLOT(setFilterFixedString(QString)));
	ui.filterList->setModel(filter);
}

void LocationFilter::showEvent(QShowEvent *event)
{
	MultiFilterSortModel::instance()->addFilterModel(LocationFilterModel::instance());
	QWidget::showEvent(event);
}

void LocationFilter::hideEvent(QHideEvent *event)
{
	MultiFilterSortModel::instance()->removeFilterModel(LocationFilterModel::instance());
	QWidget::hideEvent(event);
}

SuitFilter::SuitFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);
	ui.label->setText(tr("Suits: "));
#if QT_VERSION >= 0x050200
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(SuitsFilterModel::instance());
	filter->setFilterCaseSensitivity(Qt::CaseInsensitive);
	connect(ui.filterInternalList, SIGNAL(textChanged(QString)), filter, SLOT(setFilterFixedString(QString)));
	ui.filterList->setModel(filter);
}

void SuitFilter::showEvent(QShowEvent *event)
{
	MultiFilterSortModel::instance()->addFilterModel(SuitsFilterModel::instance());
	QWidget::showEvent(event);
}

void SuitFilter::hideEvent(QHideEvent *event)
{
	MultiFilterSortModel::instance()->removeFilterModel(SuitsFilterModel::instance());
	QWidget::hideEvent(event);
}

MultiFilter::MultiFilter(QWidget *parent) : QWidget(parent)
{
	ui.setupUi(this);

	QWidget *expandedWidget = new QWidget();
	QHBoxLayout *l = new QHBoxLayout();

	TagFilter *tagFilter = new TagFilter(this);
	int minimumHeight = tagFilter->ui.filterInternalList->height() +
			tagFilter->ui.verticalLayout->spacing() * tagFilter->ui.verticalLayout->count();

	QListView *dummyList = new QListView();
	QStringListModel *dummy = new QStringListModel(QStringList() << "Dummy Text");
	dummyList->setModel(dummy);

	connect(ui.close, SIGNAL(clicked(bool)), this, SLOT(closeFilter()));
	connect(ui.clear, SIGNAL(clicked(bool)), MultiFilterSortModel::instance(), SLOT(clearFilter()));
	connect(ui.maximize, SIGNAL(clicked(bool)), this, SLOT(adjustHeight()));

	l->addWidget(tagFilter);
	l->addWidget(new BuddyFilter());
	l->addWidget(new LocationFilter());
	l->addWidget(new SuitFilter());
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	expandedWidget->setLayout(l);

	ui.scrollArea->setWidget(expandedWidget);
	expandedWidget->resize(expandedWidget->width(), minimumHeight + dummyList->sizeHintForRow(0) * 5 );
	ui.scrollArea->setMinimumHeight(expandedWidget->height() + 5);

	connect(MultiFilterSortModel::instance(), SIGNAL(filterFinished()), this, SLOT(filterFinished()));
}

void MultiFilter::filterFinished()
{
	ui.filterText->setText(tr("Filter shows %1 (of %2) dives").arg(MultiFilterSortModel::instance()->divesDisplayed).arg(dive_table.nr));
}

void MultiFilter::adjustHeight()
{
	ui.scrollArea->setVisible(!ui.scrollArea->isVisible());
}

void MultiFilter::closeFilter()
{
	MultiFilterSortModel::instance()->clearFilter();
	hide();
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
		const QString ctrlKeyName = QKeySequence(Qt::CTRL).toString();
		// ctrlKeyName comes with a trailing '+', as in: 'Ctrl+'
		QToolTip::showText(pos, tr("%1click to visit %2").arg(ctrlKeyName).arg(urlStr));
	}
}

bool TextHyperlinkEventFilter::stringMeetsOurUrlRequirements(const QString &maybeUrlStr)
{
	QUrl url(maybeUrlStr, QUrl::StrictMode);
	return url.isValid() && (!url.scheme().isEmpty());
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

	do {
		result = grownText; // this is a no-op on the first visit.

		if (searchBackwards) {
			movedOk = cursor->movePosition(QTextCursor::PreviousWord, QTextCursor::KeepAnchor);
		} else {
			movedOk = cursor->movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
		}

		grownText = cursor->selectedText();
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
