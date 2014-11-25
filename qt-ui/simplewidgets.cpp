#include "simplewidgets.h"
#include "filtermodels.h"

#include <QLabel>
#include <QProcess>
#include <QStringList>
#include <QDebug>
#include <QFileDialog>
#include <QShortcut>
#include <QCalendarWidget>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QToolBar>
#include "exif.h"
#include "dive.h"
#include "file.h"
#include "display.h"
#include "mainwindow.h"
#include "helpers.h"
#include "ui_filterwidget.h"
#include "libdivecomputer/parser.h"


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
		qDebug() << "Renumbering.";
		renumber_dives(ui.spinBox->value(), selectedOnly);
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
	time = second;
}

void SetpointDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		add_event(dc, time, SAMPLE_EVENT_PO2, 0, ui.spinbox->value(), "SP change");
	}
}

SetpointDialog::SetpointDialog(QWidget *parent) : QDialog(parent)
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
			shift_times(amount);
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

ShiftTimesDialog::ShiftTimesDialog(QWidget *parent) : QDialog(parent)
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
	struct memblock mem;
	EXIFInfo exiv;
	int retval;
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
	if (readfile(fileNames.at(0).toUtf8().data(), &mem) <= 0)
		return;
	retval = exiv.parseFrom((const unsigned char *)mem.buffer, (unsigned)mem.size);
	free(mem.buffer);
	if (retval != PARSE_EXIF_SUCCESS)
		return;
	dcImageEpoch = exiv.epoch();
	dcDateTime.setTime_t(dcImageEpoch);
	ui.dcTime->setDateTime(dcDateTime);
	connect(ui.dcTime, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(dcDateTimeChanged(const QDateTime &)));
}

void ShiftImageTimesDialog::dcDateTimeChanged(const QDateTime &newDateTime)
{
	if (!dcImageEpoch)
		return;
	setOffset(newDateTime.toTime_t() - dcImageEpoch);
}

ShiftImageTimesDialog::ShiftImageTimesDialog(QWidget *parent) : QDialog(parent), m_amount(0)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.syncCamera, SIGNAL(clicked()), this, SLOT(syncCameraClicked()));
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
	UI_FROM_COMPONENT(location);
	UI_FROM_COMPONENT(gps);
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
		COMPONENT_FROM_UI(location);
		COMPONENT_FROM_UI(gps);
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
#if QT_VERSION >= 0x050000
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(TagFilterModel::instance());
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
#if QT_VERSION >= 0x050000
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(BuddyFilterModel::instance());
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
#if QT_VERSION >= 0x050000
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(LocationFilterModel::instance());
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
#if QT_VERSION >= 0x050000
	ui.filterInternalList->setClearButtonEnabled(true);
#endif
	QSortFilterProxyModel *filter = new QSortFilterProxyModel();
	filter->setSourceModel(SuitsFilterModel::instance());
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
	ui = new Ui::FilterWidget2();
	ui->setupUi(this);

	QWidget *expandedWidget = new QWidget();
	QHBoxLayout *l = new QHBoxLayout();

	TagFilter *tagFilter = new TagFilter();
	int minimumHeight = tagFilter->ui.filterInternalList->height() +
			tagFilter->ui.verticalLayout->spacing() * tagFilter->ui.verticalLayout->count();

	QListView *dummyList = new QListView();
	QStringListModel *dummy = new QStringListModel(QStringList() << "Dummy Text");
	dummyList->setModel(dummy);

	connect(ui->close, SIGNAL(clicked(bool)), this, SLOT(closeFilter()));
	connect(ui->clear, SIGNAL(clicked(bool)), MultiFilterSortModel::instance(), SLOT(clearFilter()));
	connect(ui->maximize, SIGNAL(clicked(bool)), this, SLOT(adjustHeight()));

	l->addWidget(tagFilter);
	l->addWidget(new BuddyFilter());
	l->addWidget(new LocationFilter());
	l->addWidget(new SuitFilter());
	l->setContentsMargins(0, 0, 0, 0);
	l->setSpacing(0);
	expandedWidget->setLayout(l);

	ui->scrollArea->setWidget(expandedWidget);
	expandedWidget->resize(expandedWidget->width(), minimumHeight + dummyList->sizeHintForRow(0) * 5 );
	ui->scrollArea->setMinimumHeight(expandedWidget->height() + 5);

	connect(MultiFilterSortModel::instance(), SIGNAL(filterFinished()), this, SLOT(filterFinished()));
}

void MultiFilter::filterFinished()
{
	ui->filterText->setText(tr("Filter shows %1 (of %2) dives").arg(MultiFilterSortModel::instance()->divesDisplayed).arg(dive_table.nr));
}

void MultiFilter::adjustHeight()
{
	ui->scrollArea->setVisible(!ui->scrollArea->isVisible());
}

void MultiFilter::closeFilter()
{
	MultiFilterSortModel::instance()->clearFilter();
	hide();
}
