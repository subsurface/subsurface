#include "simplewidgets.h"

#include <QLabel>
#include <QLabel>
#include <QFormLayout>
#include <QIcon>
#include <QAbstractButton>
#include <QSpinBox>
#include <QButtonGroup>
#include <QDebug>
#include <QProcess>
#include <QStringList>
#include <QDebug>
#include <QTime>
#include <QFileDialog>
#include <QDateTime>
#include "exif.h"
#include "../dive.h"
#include "../file.h"
#include "mainwindow.h"
#include "helpers.h"

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

RenumberDialog *RenumberDialog::instance()
{
	static RenumberDialog *self = new RenumberDialog(MainWindow::instance());
	return self;
}

void RenumberDialog::buttonClicked(QAbstractButton *button)
{
	if (ui.buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
		qDebug() << "Renumbering.";
		renumber_dives(ui.spinBox->value());
	}
}

RenumberDialog::RenumberDialog(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
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

void ShiftTimesDialog::showEvent(QShowEvent * event)
{
	ui.timeEdit->setTime(QTime(0, 0, 0, 0));
	when = get_times();//get time of first selected dive
	ui.currentTime->setText(get_dive_date_string(when));
	ui.shiftedTime->setText(get_dive_date_string(when));
}

void ShiftTimesDialog::changeTime()
{
	int amount;

	amount = ui.timeEdit->time().hour() * 3600 + ui.timeEdit->time().minute() * 60;
	if (ui.backwards->isChecked())
		amount *= -1;

	ui.shiftedTime->setText (get_dive_date_string(amount+when));
}

ShiftTimesDialog::ShiftTimesDialog(QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.timeEdit, SIGNAL(timeChanged(const QTime)), this, SLOT(changeTime()));
	connect(ui.backwards, SIGNAL(toggled(bool)), this, SLOT(changeTime()));
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
							      tr("Open Image File"),
							      DiveListView::lastUsedImageDir(),
							      tr("Image Files (*.jpg *.jpeg *.pnm *.tif *.tiff)"));
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
	dcImageEpoch = epochFromExiv(&exiv);
	dcDateTime.setTime_t(dcImageEpoch);
	ui.dcTime->setDateTime(dcDateTime);
	connect(ui.dcTime, SIGNAL(dateTimeChanged(const QDateTime &)), this, SLOT(dcDateTimeChanged(const QDateTime &)));
}

time_t ShiftImageTimesDialog::epochFromExiv(EXIFInfo *exif)
{
	struct tm tm;
	int year, month, day, hour, min, sec;

	if (strlen(exif->DateTime.c_str()))
		sscanf(exif->DateTime.c_str(), "%d:%d:%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);
	else
		sscanf(exif->DateTimeOriginal.c_str(), "%d:%d:%d %d:%d:%d", &year, &month, &day, &hour, &min, &sec);
	tm.tm_year = year;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = min;
	tm.tm_sec = sec;
	return (utc_mktime(&tm));
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
