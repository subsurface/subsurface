// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/divelogimportdialog.h"
#include "desktop-widgets/mainwindow.h"
#include "commands/command.h"
#include "core/color.h"
#include "ui_divelogimportdialog.h"
#include <QShortcut>
#include <QDrag>
#include <QMimeData>
#include <QRegularExpression>
#include <QUndoStack>
#include <QPainter>
#include <QFile>
#include "core/filterpreset.h"
#include "core/qthelper.h"
#include "core/divesite.h"
#include "core/divelog.h"
#include "core/device.h"
#include "core/trip.h"
#include "core/import-csv.h"
#include "core/xmlparams.h"

static QString subsurface_mimedata = "subsurface/csvcolumns";
static QString subsurface_index = "subsurface/csvindex";

#define SILENCE_WARNING 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ""

struct CSVAppConfig {
	QString name;
	int time;
	int depth;
	int temperature;
	int po2;
	int sensor1;
	int sensor2;
	int sensor3;
	int cns;
	int ndl;
	int tts;
	int stopdepth;
	int pressure;
	int setpoint;
	QString separator;
};
static const CSVAppConfig CSVApps[] = {
	// time, depth, temperature, po2, sensor1, sensor2, sensor3, cns, ndl, tts, stopdepth, pressure, setpoint
	// indices are 0 based, -1 means the column doesn't exist
	{ "Manual import", SILENCE_WARNING },
	{ "APD Log Viewer - DC1", 0, 1, 15, 6, 3, 4, 5, 17, -1, -1, 18, -1, 2, "Tab" },
	{ "APD Log Viewer - DC2", 0, 1, 15, 6, 7, 8, 9, 17, -1, -1, 18, -1, 2, "Tab" },
	{ "DL7", 1, 2, -1, -1, -1, -1, -1, -1, -1, 8, -1, 10, -1, "|" },
	{ "XP5", 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "Tab" },
	{ "SensusCSV", 9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "," },
	{ "Seabear CSV", 0, 1, 5, -1, -1, -1, -1, -1, 2, 3, 4, 6, -1, ";" },
	{ "SubsurfaceCSV", -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "Tab" },
	{ "AV1", 0, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, " " },
	{ "Poseidon MkVI", 0, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "," },
};

enum Known {
	MANUAL,
	APD,
	APD2,
	DL7,
	XP5,
	SENSUS,
	SEABEAR,
	SUBSURFACE,
	AV1,
	POSEIDON
};

ColumnNameProvider::ColumnNameProvider(QObject *parent) : QAbstractListModel(parent)
{
	columnNames << tr("Dive #") << tr("Date") << tr("Time") << tr("Duration") << tr("Mode") << tr("Location") << tr("GPS") << tr("Weight") << tr("Cyl. size") << tr("Start pressure") <<
		       tr("End pressure") << tr("Max. depth") << tr("Avg. depth") << tr("Divemaster") << tr("Buddy") << tr("Suit") << tr("Notes") << tr("Tags") << tr("Air temp.") << tr("Water temp.") <<
		       tr("O₂") << tr("He") << tr("Sample time") << tr("Sample depth") << tr("Sample temperature") << tr("Sample pO₂") << tr("Sample CNS") << tr("Sample NDL") <<
		       tr("Sample TTS") << tr("Sample stopdepth") << tr("Sample pressure") <<
		       tr("Sample sensor1 pO₂") << tr("Sample sensor2 pO₂") << tr("Sample sensor3 pO₂") <<
		       tr("Sample setpoint") << tr("Visibility") << tr("Rating") << tr("Sample heartrate");
}

bool ColumnNameProvider::insertRows(int row, int, const QModelIndex&)
{
	beginInsertRows(QModelIndex(), row, row);
	columnNames.append(QString());
	endInsertRows();
	return true;
}

bool ColumnNameProvider::removeRows(int row, int, const QModelIndex&)
{
	beginRemoveRows(QModelIndex(), row, row);
	columnNames.removeAt(row);
	endRemoveRows();
	return true;
}

bool ColumnNameProvider::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::EditRole) {
		columnNames[index.row()] = value.toString();
	}
	dataChanged(index, index);
	return true;
}

QVariant ColumnNameProvider::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role != Qt::DisplayRole)
		return QVariant();

	return QVariant(columnNames[index.row()]);
}

int ColumnNameProvider::rowCount(const QModelIndex&) const
{
	return columnNames.count();
}

int ColumnNameProvider::mymatch(QString value) const
{
	QString searchString = value.toLower();
	QRegularExpression re(" \\(.*\\)");

	searchString.replace("\"", "").replace(re, "").replace(" ", "").replace(".", "").replace("\n","");
	for (int i = 0; i < columnNames.count(); i++) {
		QString name = columnNames.at(i).toLower();
		name.replace("\"", "").replace(" ", "").replace(".", "").replace("\n","");
		if (searchString == name.toLower())
			return i;
	}
	return -1;
}

ColumnNameView::ColumnNameView(QWidget*)
{
	setAcceptDrops(true);
	setDragEnabled(true);
}

void ColumnNameView::mousePressEvent(QMouseEvent *press)
{
	QModelIndex atClick = indexAt(press->pos());
	if (!atClick.isValid())
		return;

	QRect indexRect = visualRect(atClick);
	QPixmap pix(indexRect.width(), indexRect.height());
	pix.fill(QColor(0,0,0,0));
	render(&pix, QPoint(0, 0),QRegion(indexRect));

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;
	mimeData->setData(subsurface_mimedata, atClick.data().toByteArray());
	model()->removeRow(atClick.row());
	drag->setPixmap(pix);
	drag->setMimeData(mimeData);
	if (drag->exec() == Qt::IgnoreAction){
		model()->insertRow(model()->rowCount());
		QModelIndex idx = model()->index(model()->rowCount()-1, 0);
		model()->setData(idx, mimeData->data(subsurface_mimedata));
	}
}

void ColumnNameView::dragLeaveEvent(QDragLeaveEvent*)
{
}

void ColumnNameView::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void ColumnNameView::dragMoveEvent(QDragMoveEvent *event)
{
	QModelIndex curr = indexAt(event->pos());
	if (!curr.isValid() || curr.row() != 0)
		return;
	event->acceptProposedAction();
}

void ColumnNameView::dropEvent(QDropEvent *event)
{
	const QMimeData *mimeData = event->mimeData();
	if (mimeData->data(subsurface_mimedata).count()) {
		if (event->source() != this) {
			event->acceptProposedAction();
			QVariant value = QString(mimeData->data(subsurface_mimedata));
			model()->insertRow(model()->rowCount());
			model()->setData(model()->index(model()->rowCount()-1, 0), value);
		}
	}
}

ColumnDropCSVView::ColumnDropCSVView(QWidget*)
{
	setAcceptDrops(true);
}

void ColumnDropCSVView::dragLeaveEvent(QDragLeaveEvent*)
{
}

void ColumnDropCSVView::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void ColumnDropCSVView::dragMoveEvent(QDragMoveEvent *event)
{
	QModelIndex curr = indexAt(event->pos());
	if (!curr.isValid() || curr.row() != 0)
		return;
	event->acceptProposedAction();
}

void ColumnDropCSVView::dropEvent(QDropEvent *event)
{
	QModelIndex curr = indexAt(event->pos());
	if (!curr.isValid() || curr.row() != 0)
		return;

	const QMimeData *mimeData = event->mimeData();
	if (!mimeData->data(subsurface_mimedata).count())
		return;

	if (event->source() == this ) {
		int value_old = mimeData->data(subsurface_index).toInt();
		int value_new = curr.column();
		ColumnNameResult *m = qobject_cast<ColumnNameResult*>(model());
		m->swapValues(value_old, value_new);
		event->acceptProposedAction();
		return;
	}

	if (curr.data().toString().isEmpty()) {
		QVariant value = QString(mimeData->data(subsurface_mimedata));
		model()->setData(curr, value);
		event->acceptProposedAction();
	}
}

ColumnNameResult::ColumnNameResult(QObject *parent) : QAbstractTableModel(parent)
{

}

void ColumnNameResult::swapValues(int firstIndex, int secondIndex)
{
	QString one = columnNames[firstIndex];
	QString two = columnNames[secondIndex];
	setData(index(0, firstIndex), QVariant(two), Qt::EditRole);
	setData(index(0, secondIndex), QVariant(one), Qt::EditRole);
}

bool ColumnNameResult::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() != 0) {
		return false;
	}
	if (role == Qt::EditRole) {
		columnNames[index.column()] = value.toString();
		dataChanged(index, index);
	}
	return true;
}

QVariant ColumnNameResult::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	if (role == Qt::BackgroundRole)
		if (index.row() == 0)
			return QVariant(AIR_BLUE_TRANS);

	if (role != Qt::DisplayRole)
		return QVariant();

	if (index.row() == 0) {
		return columnNames[index.column()];
	}
	// make sure the element exists before returning it - this might get called before the
	// model is correctly set up again (e.g., when changing separators)
	if (columnValues.count() > index.row() - 1 && columnValues[index.row() - 1].count() > index.column())
		return QVariant(columnValues[index.row() - 1][index.column()]);
	else
		return QVariant();
}

int ColumnNameResult::rowCount(const QModelIndex&) const
{
	return columnValues.count() + 1; // +1 == the header.
}

int ColumnNameResult::columnCount(const QModelIndex&) const
{
	return columnNames.count();
}

QStringList ColumnNameResult::result() const
{
	return columnNames;
}

void ColumnNameResult::setColumnValues(QList<QStringList> columns)
{
	if (rowCount() != 1) {
		beginRemoveRows(QModelIndex(), 1, rowCount()-1);
		columnValues.clear();
		endRemoveRows();
	}
	if (columnCount() != 0) {
		beginRemoveColumns(QModelIndex(), 0, columnCount()-1);
		columnNames.clear();
		endRemoveColumns();
	}

	QStringList first = columns.first();
	beginInsertColumns(QModelIndex(), 0, first.count()-1);
	for(int i = 0; i < first.count(); i++)
		columnNames.append(QString());

	endInsertColumns();

	beginInsertRows(QModelIndex(), 0, columns.count()-1);
	columnValues = std::move(columns);
	endInsertRows();
}

void ColumnDropCSVView::mousePressEvent(QMouseEvent *press)
{
	QModelIndex atClick = indexAt(press->pos());
	if (!atClick.isValid() || atClick.row())
		return;

	QRect indexRect = visualRect(atClick);
	QPixmap pix(indexRect.width(), indexRect.height());
	pix.fill(QColor(0,0,0,0));
	render(&pix, QPoint(0, 0),QRegion(indexRect));

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;
	mimeData->setData(subsurface_mimedata, atClick.data().toByteArray());
	mimeData->setData(subsurface_index, QString::number(atClick.column()).toUtf8());
	drag->setPixmap(pix);
	drag->setMimeData(mimeData);
	if (drag->exec() != Qt::IgnoreAction){
		QObject *target = drag->target();
		if (target->objectName() ==  "qt_scrollarea_viewport")
			target = target->parent();
		if (target != drag->source())
			model()->setData(atClick, QString());
	}
}

DiveLogImportDialog::DiveLogImportDialog(QStringList fn, QWidget *parent) : QDialog(parent),
	selector(true),
	ui(new Ui::DiveLogImportDialog)
{
	ui->setupUi(this);
	fileNames = std::move(fn);
	column = 0;
	delta = "0";
	hw = "";
	txtLog = false;

	/* Add indices of XSLTs requiring special handling to the list */
	specialCSV << SENSUS;
	specialCSV << SUBSURFACE;
	specialCSV << DL7;
	specialCSV << AV1;
	specialCSV << POSEIDON;

	for (const CSVAppConfig &conf: CSVApps)
		ui->knownImports->addItem(conf.name);

	ui->CSVSeparator->addItems( QStringList() << tr("Tab") << "," << ";" << "|");

	loadFileContents(-1, INITIAL);

	/* manually import CSV file */
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));

	connect(ui->CSVSeparator, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFileContentsSeperatorSelected(int)));
	connect(ui->knownImports, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFileContentsKnownTypesSelected(int)));
}

DiveLogImportDialog::~DiveLogImportDialog()
{
	delete ui;
}

void DiveLogImportDialog::loadFileContentsSeperatorSelected(int value)
{
	loadFileContents(value, SEPARATOR);
}

void DiveLogImportDialog::loadFileContentsKnownTypesSelected(int value)
{
	loadFileContents(value, KNOWNTYPES);
}

// Turn a "*.csv" or "*.txt" filename into a pair of both, "*.csv" and "*.txt".
// If the input wasn't either "*.csv" or "*.txt", then both returned strings
// are empty
static QPair<QString, QString> poseidonFileNames(const QString &fn)
{
	if (fn.endsWith(".csv", Qt::CaseInsensitive)) {
		QString txt = fn.left(fn.size() - 3) + "txt";
		return { fn, txt };
	} else if (fn.endsWith(".txt", Qt::CaseInsensitive)) {
		QString csv = fn.left(fn.size() - 3) + "csv";
		return { csv, fn };
	} else {
		return { QString(), QString() };
	}
}

void DiveLogImportDialog::loadFileContents(int value, whatChanged triggeredBy)
{
	QList<QStringList> fileColumns;
	QStringList currColumns;
	QStringList headers;
	bool matchedSome = false;
	bool seabear = false;
	bool xp5 = false;
	bool apd = false;
	bool dl7 = false;
	bool poseidon = false;

	// reset everything
	ColumnNameProvider *provider = new ColumnNameProvider(this);
	ui->avaliableColumns->setModel(provider);
	ui->avaliableColumns->setItemDelegate(new TagDragDelegate(ui->avaliableColumns));
	resultModel = new ColumnNameResult(this);
	ui->tableView->setModel(resultModel);

	// Poseidon MkVI is special: it is made up of a .csv *and* a .txt file.
	// If the user specified one, we'll try to check for the other.
	QString fileName = fileNames.first();
	QPair<QString, QString> pair = poseidonFileNames(fileName);
	if (!pair.second.isEmpty()) {
		QFile f_txt(pair.second);
		f_txt.open(QFile::ReadOnly);
		QString firstLine = f_txt.readLine();
		if (firstLine.startsWith("MkVI_Config ")) {
			poseidon = true;
			fileName = pair.first; // Read data from CSV
			headers.append("Time");
			headers.append("Depth");
			blockSignals(true);
			ui->knownImports->setCurrentText("Poseidon MkVI");
			blockSignals(false);
		}
	}

	QFile f(fileName);
	f.open(QFile::ReadOnly);
	QString firstLine = f.readLine();
	if (firstLine.contains("SEABEAR")) {
		seabear = true;

		/*
		 * Parse header - currently only interested in sample
		 * interval and hardware version. If we have old format
		 * the interval value is missing from the header.
		 */

		while ((firstLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
			if (firstLine.contains("//Hardware Version: ")) {
				hw = firstLine.replace(QString::fromLatin1("//Hardware Version: "), QString::fromLatin1("\"Seabear ")).trimmed().append("\"");
				break;
			}
		}

		/*
		 * Note that we scan over the "Log interval" on purpose
		 */

		while ((firstLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
			if (firstLine.contains("//Log interval: "))
				delta = firstLine.remove(QString::fromLatin1("//Log interval: ")).trimmed().remove(QString::fromLatin1(" s"));
		}

		/*
		 * Parse CSV fields
		 */

		firstLine = f.readLine().trimmed();

		currColumns = firstLine.split(';');
		Q_FOREACH (QString columnText, currColumns) {
			if (columnText == "Time") {
				headers.append("Sample time");
			} else if (columnText == "Depth") {
				headers.append("Sample depth");
			} else if (columnText == "Temperature") {
				headers.append("Sample temperature");
			} else if (columnText == "NDT") {
				headers.append("Sample NDL");
			} else if (columnText == "TTS") {
				headers.append("Sample TTS");
			} else if (columnText == "pO2_1") {
				headers.append("Sample sensor1 pO₂");
			} else if (columnText == "pO2_2") {
				headers.append("Sample sensor2 pO₂");
			} else if (columnText == "pO2_3") {
				headers.append("Sample sensor3 pO₂");
			} else if (columnText == "Ceiling") {
				headers.append("Sample ceiling");
			} else if (columnText == "Tank pressure") {
				headers.append("Sample pressure");
			} else {
				// We do not know about this value
				qDebug() << "Seabear import found an un-handled field: " << columnText;
				headers.append("");
			}
		}

		firstLine = headers.join(";");
		blockSignals(true);
		ui->knownImports->setCurrentText("Seabear CSV");
		blockSignals(false);
	} else if (firstLine.contains("Tauchgangs-Nr.:")) {
		xp5 = true;
		//"Abgelaufene Tauchzeit (Std:Min.)\tTiefe\tStickstoff Balkenanzeige\tSauerstoff Balkenanzeige\tAufstiegsgeschwindigkeit\tRestluftzeit\tRestliche Tauchzeit\tDekompressionszeit (Std:Min)\tDekostopp-Tiefe\tTemperatur\tPO2\tPressluftflasche\tLesen des Druckes\tStatus der Verbindung\tTauchstatus";
		firstLine = "Sample time\tSample depth\t\t\t\t\t\t\t\tSample temperature\t";
		blockSignals(true);
		ui->knownImports->setCurrentText("XP5");
		blockSignals(false);
	} else if (firstLine.contains("FSH")) {
		QString units = "Metric";
		dl7 = true;
		while ((firstLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
			/* DL7 actually defines individual units (e.g. depth, temperature,
			 * pressure, etc.) and there are quite a few other options as well,
			 * but let's use metric unless depth unit is clearly Imperial. */

			if (firstLine.contains("ThFt")) {
				units = "Imperial";
			}
		}
		firstLine = "|Sample time|Sample depth||||||Sample temperature||Sample pressure";
		blockSignals(true);
		ui->knownImports->setCurrentText("DL7");
		ui->CSVUnits->setCurrentText(units);
		blockSignals(false);
	} else if (firstLine.contains("Life Time Dive")) {
		txtLog = true;

		while ((firstLine = f.readLine().trimmed()).length() >= 0 && !f.atEnd()) {
			if (firstLine.contains("Dive Profile")) {
				f.readLine();
				break;
			}
		}
		firstLine = f.readLine().trimmed();
	}

	// Special handling for APD Log Viewer
	if ((triggeredBy == KNOWNTYPES && (value == APD || value == APD2)) || (triggeredBy == INITIAL && fileNames.first().endsWith(".apd", Qt::CaseInsensitive))) {
		QString apdseparator;
		int tabs = firstLine.count('\t');
		int commas = firstLine.count(',');
		if (tabs > commas)
			apdseparator = "\t";
		else
			apdseparator = ",";

		apd=true;

		firstLine = tr("Sample time") + apdseparator + tr("Sample depth") + apdseparator + tr("Sample setpoint") + apdseparator + tr("Sample sensor1 pO₂") + apdseparator + tr("Sample sensor2 pO₂") + apdseparator + tr("Sample sensor3 pO₂") + apdseparator + tr("Sample pO₂") + apdseparator + "" + apdseparator + "" + apdseparator + "" + apdseparator + "" + apdseparator + "" + apdseparator + "" + apdseparator + "" + apdseparator + "" + apdseparator + tr("Sample temperature") + apdseparator + "" + apdseparator + tr("Sample CNS") + apdseparator + tr("Sample stopdepth");
		blockSignals(true);
		ui->CSVSeparator->setCurrentText(apdseparator);
		if (triggeredBy == INITIAL && fileNames.first().contains(".apd", Qt::CaseInsensitive))
			ui->knownImports->setCurrentText("APD Log Viewer - DC1");
		blockSignals(false);
	}

	QString separator = ui->CSVSeparator->currentText() == tr("Tab") ? "\t" : ui->CSVSeparator->currentText();
	currColumns = firstLine.split(separator);
	if (triggeredBy == INITIAL) {
		// guess the separator
		int tabs = firstLine.count('\t');
		int commas = firstLine.count(',');
		int semis = firstLine.count(';');
		int pipes = firstLine.count('|');
		if (tabs > commas && tabs > semis && tabs > pipes)
			separator = "\t";
		else if (commas > tabs && commas > semis && commas > pipes)
			separator = ",";
		else if (pipes > tabs && pipes > commas && pipes > semis)
			separator = "|";
		else if (semis > tabs && semis > commas && semis > pipes)
			separator = ";";
		if (ui->CSVSeparator->currentText() != separator) {
			blockSignals(true);
			ui->CSVSeparator->setCurrentText(separator);
			blockSignals(false);
			currColumns = firstLine.split(separator);
		}
	}
	if (triggeredBy == INITIAL || (triggeredBy == KNOWNTYPES && value == MANUAL) || triggeredBy == SEPARATOR) {
		int count = -1;
		QString line = f.readLine().trimmed();
		QStringList columns;
		if (line.length() > 0)
			columns = line.split(separator);
		// now try and guess the columns
		Q_FOREACH (QString columnText, currColumns) {
			count++;
			/*
			 * We have to skip the conversion of 2 to ₂ for APD Log
			 * viewer as that would mess up the sensor numbering. We
			 * also know that the column headers do not need this
			 * conversion.
			 */
			if (apd == false) {
				columnText.replace("\"", "");
				columnText.replace("number", "#", Qt::CaseInsensitive);
				columnText.replace("2", "₂", Qt::CaseInsensitive);
				columnText.replace("cylinder", "cyl.", Qt::CaseInsensitive);
			}
			int idx = provider->mymatch(columnText.trimmed());
			if (idx >= 0) {
				QString foundHeading = provider->data(provider->index(idx, 0), Qt::DisplayRole).toString();
				provider->removeRow(idx);
				headers.append(foundHeading);
				matchedSome = true;
				if (foundHeading == QString::fromLatin1("Date") && columns.count() >= count) {
					QString date = columns.at(count);
					if (date.contains('-')) {
						ui->DateFormat->setCurrentText("yyyy-mm-dd");

					} else if (date.contains('/')) {
						ui->DateFormat->setCurrentText("mm/dd/yyyy");
					}
				} else if (foundHeading == QString::fromLatin1("Time") && columns.count() >= count) {
					QString time = columns.at(count);
					if (time.contains(':')) {
						ui->DurationFormat->setCurrentText("Minutes:seconds");

					}
				}
			} else {
				headers.append("");
			}
		}
		if (matchedSome) {
			ui->dragInstructions->setText(tr("Some column headers were pre-populated; please drag and drop the headers so they match the column they are in."));
			if (triggeredBy != KNOWNTYPES && !seabear && !xp5 && !apd && !dl7) {
				blockSignals(true);
				ui->knownImports->setCurrentIndex(0); // <- that's "Manual import"
				blockSignals(false);
			}
		}
	}
	if (triggeredBy == KNOWNTYPES && value != MANUAL) {
		// an actual known type
		if (value == SUBSURFACE || value == APD || value == APD2) {
			/*
			 * Subsurface CSV file needs separator detection
			 * as we used to default to comma but switched
			 * to tab.
			 */
			int tabs = firstLine.count('\t');
			int commas = firstLine.count(',');
			if (tabs > commas)
				separator = "Tab";
			else
				separator = ",";
		} else {
			separator = CSVApps[value].separator;
		}

		if (ui->CSVSeparator->currentText() != separator || separator == "Tab") {
			ui->CSVSeparator->blockSignals(true);
			ui->CSVSeparator->setCurrentText(separator);
			ui->CSVSeparator->blockSignals(false);
			if (separator == "Tab")
				separator = "\t";
			currColumns = firstLine.split(separator);
		}
		// now set up time, depth, temperature, po2, cns, ndl, tts, stopdepth, pressure, setpoint
		for (int i = 0; i < currColumns.count(); i++)
			headers.append("");
		if (CSVApps[value].time > -1 && CSVApps[value].time < currColumns.count())
			headers.replace(CSVApps[value].time, tr("Sample time"));
		if (CSVApps[value].depth > -1 && CSVApps[value].depth < currColumns.count())
			headers.replace(CSVApps[value].depth, tr("Sample depth"));
		if (CSVApps[value].temperature > -1 && CSVApps[value].temperature < currColumns.count())
			headers.replace(CSVApps[value].temperature, tr("Sample temperature"));
		if (CSVApps[value].po2 > -1 && CSVApps[value].po2 < currColumns.count())
			headers.replace(CSVApps[value].po2, tr("Sample pO₂"));
		if (CSVApps[value].sensor1 > -1 && CSVApps[value].sensor1 < currColumns.count())
			headers.replace(CSVApps[value].sensor1, tr("Sample sensor1 pO₂"));
		if (CSVApps[value].sensor2 > -1 && CSVApps[value].sensor2 < currColumns.count())
			headers.replace(CSVApps[value].sensor2, tr("Sample sensor2 pO₂"));
		if (CSVApps[value].sensor3 > -1 && CSVApps[value].sensor3 < currColumns.count())
			headers.replace(CSVApps[value].sensor3, tr("Sample sensor3 pO₂"));
		if (CSVApps[value].cns > -1 && CSVApps[value].cns < currColumns.count())
			headers.replace(CSVApps[value].cns, tr("Sample CNS"));
		if (CSVApps[value].ndl > -1 && CSVApps[value].ndl < currColumns.count())
			headers.replace(CSVApps[value].ndl, tr("Sample NDL"));
		if (CSVApps[value].tts > -1 && CSVApps[value].tts < currColumns.count())
			headers.replace(CSVApps[value].tts, tr("Sample TTS"));
		if (CSVApps[value].stopdepth > -1 && CSVApps[value].stopdepth < currColumns.count())
			headers.replace(CSVApps[value].stopdepth, tr("Sample stopdepth"));
		if (CSVApps[value].pressure > -1 && CSVApps[value].pressure < currColumns.count())
			headers.replace(CSVApps[value].pressure, tr("Sample pressure"));
		if (CSVApps[value].setpoint > -1 && CSVApps[value].setpoint < currColumns.count())
			headers.replace(CSVApps[value].setpoint, tr("Sample setpoint"));

		/* Show the Subsurface CSV column headers */
		if (value == SUBSURFACE && currColumns.count() >= 23) {
			headers.replace(0, tr("Dive #"));
			headers.replace(1, tr("Date"));
			headers.replace(2, tr("Time"));
			headers.replace(3, tr("Duration"));
			headers.replace(4, tr("Max. depth"));
			headers.replace(5, tr("Avg. depth"));
			headers.replace(6, tr("Mode"));
			headers.replace(7, tr("Air temp."));
			headers.replace(8, tr("Water temp."));
			headers.replace(9, tr("Cyl. size"));
			headers.replace(10, tr("Start pressure"));
			headers.replace(11, tr("End pressure"));
			headers.replace(12, tr("O₂"));
			headers.replace(13, tr("He"));
			headers.replace(14, tr("Location"));
			headers.replace(15, tr("GPS"));
			headers.replace(16, tr("Divemaster"));
			headers.replace(17, tr("Buddy"));
			headers.replace(18, tr("Suit"));
			headers.replace(19, tr("Rating"));
			headers.replace(20, tr("Visibility"));
			headers.replace(21, tr("Notes"));
			headers.replace(22, tr("Weight"));
			headers.replace(23, tr("Tags"));

			blockSignals(true);
			ui->CSVSeparator->setCurrentText(separator);
			ui->DateFormat->setCurrentText("yyyy-mm-dd");
			ui->DurationFormat->setCurrentText("Minutes:seconds");
			blockSignals(false);
		}
	}

	f.reset();
	int rows = 0;

	/* Skipping the header of Seabear and XP5 CSV files. */
	if (seabear || xp5) {
		/*
		 * First set of data on Seabear CSV file is metadata
		 * that is separated by an empty line (windows line
		 * termination might be encountered.
		 */
		while (strlen(f.readLine()) > 3 && !f.atEnd());
		/*
		 * Next we have description of the fields and two dummy
		 * lines. Separated again with an empty line from the
		 * actual data.
		 */
		while (strlen(f.readLine()) > 3 && !f.atEnd());
	} else if (dl7) {
		while ((firstLine = f.readLine().trimmed()).length() > 0 && !f.atEnd()) {
			if (firstLine.contains("ZDP")) {
				firstLine = f.readLine().trimmed();
				break;
			}
		}
	} else if (txtLog) {
		while ((firstLine = f.readLine().trimmed()).length() >= 0 && !f.atEnd()) {
			if (firstLine.contains("Dive Profile")) {
				firstLine = f.readLine().trimmed();
				break;
			}
		}
	}

	while (rows < 10 && !f.atEnd()) {
		QString currLine = f.readLine().trimmed();
		currColumns = currLine.split(separator);
		// For Poseidon, read only columns where the second value is 8 (=depth)
		if (poseidon) {
			if (currColumns.size() < 3 || currColumns[1] != "8")
				continue;
			currColumns.removeAt(1);
		}
		fileColumns.append(currColumns);
		rows += 1;
	}

	if (rows > 0)
		resultModel->setColumnValues(std::move(fileColumns));
	for (int i = 0; i < headers.count(); i++) {
		if (!headers.at(i).isEmpty())
			resultModel->setData(resultModel->index(0, i),headers.at(i),Qt::EditRole);
	}
}

void DiveLogImportDialog::setup_csv_params(QStringList r, xml_params &params)
{
	xml_params_add_int(&params, "dateField", r.indexOf(tr("Date")));
	xml_params_add_int(&params, "datefmt", ui->DateFormat->currentIndex());
	xml_params_add_int(&params, "starttimeField", r.indexOf(tr("Time")));
	xml_params_add_int(&params, "numberField", r.indexOf(tr("Dive #")));
	xml_params_add_int(&params, "timeField", r.indexOf(tr("Sample time")));
	xml_params_add_int(&params, "depthField", r.indexOf(tr("Sample depth")));
	xml_params_add_int(&params, "tempField", r.indexOf(tr("Sample temperature")));
	xml_params_add_int(&params, "po2Field", r.indexOf(tr("Sample pO₂")));
	xml_params_add_int(&params, "o2sensor1Field", r.indexOf(tr("Sample sensor1 pO₂")));
	xml_params_add_int(&params, "o2sensor2Field", r.indexOf(tr("Sample sensor2 pO₂")));
	xml_params_add_int(&params, "o2sensor3Field", r.indexOf(tr("Sample sensor3 pO₂")));
	xml_params_add_int(&params, "cnsField", r.indexOf(tr("Sample CNS")));
	xml_params_add_int(&params, "ndlField", r.indexOf(tr("Sample NDL")));
	xml_params_add_int(&params, "ttsField", r.indexOf(tr("Sample TTS")));
	xml_params_add_int(&params, "stopdepthField", r.indexOf(tr("Sample stopdepth")));
	xml_params_add_int(&params, "pressureField", r.indexOf(tr("Sample pressure")));
	xml_params_add_int(&params, "heartBeat", r.indexOf(tr("Sample heartrate")));
	xml_params_add_int(&params, "setpointField", r.indexOf(tr("Sample setpoint")));
	xml_params_add_int(&params, "visibilityField", r.indexOf(tr("Visibility")));
	xml_params_add_int(&params, "ratingField", r.indexOf(tr("Rating")));
	xml_params_add_int(&params, "separatorIndex", ui->CSVSeparator->currentIndex());
	xml_params_add_int(&params, "units", ui->CSVUnits->currentIndex());
	if (hw.length())
		xml_params_add(&params, "hw", qPrintable(hw));
	else if (ui->knownImports->currentText().length() > 0)
		xml_params_add(&params, "hw", qPrintable(ui->knownImports->currentText().prepend("\"").append("\"")));
}

void DiveLogImportDialog::parseTxtHeader(QString fileName, xml_params &params)
{
	QFile f(fileName);
	QString date;
	QString time;
	QString line;

	f.open(QFile::ReadOnly);
	while ((line = f.readLine().trimmed()).length() >= 0 && !f.atEnd()) {
		if (line.contains("Dive Profile")) {
			f.readLine();
			break;
		} else if (line.contains("Dive Date: ")) {
			date = line.replace(QString::fromLatin1("Dive Date: "), QString::fromLatin1(""));

			if (date.contains('-')) {
				QStringList fmtDate = date.split('-');
				date = fmtDate[0] + fmtDate[1] + fmtDate[2];
			} else if (date.contains('/')) {
				QStringList fmtDate = date.split('/');
				date = fmtDate[2] + fmtDate[0] + fmtDate[1];
			} else {
				QStringList fmtDate = date.split('.');
				date = fmtDate[2] + fmtDate[1] + fmtDate[0];
			}
		} else if (line.contains("Elapsed Dive Time: ")) {
			// Skipping dive duration for now
		} else if (line.contains("Dive Time: ")) {
			time = line.replace(QString::fromLatin1("Dive Time: "), QString::fromLatin1(""));

			if (time.contains(':')) {
				QStringList fmtTime = time.split(':');
				time = fmtTime[0] + fmtTime[1];

			}
		}
	}
	f.close();

	xml_params_add(&params, "date", qPrintable(date));
	xml_params_add(&params, "time", qPrintable(time));
}

void DiveLogImportDialog::on_buttonBox_accepted()
{
	struct divelog log;
	QStringList r = resultModel->result();
	if (ui->knownImports->currentText() != "Manual import") {
		for (int i = 0; i < fileNames.size(); ++i) {
			if (ui->knownImports->currentText() == "Seabear CSV") {
				parse_seabear_log(qPrintable(fileNames[i]), &log);
			} else if (ui->knownImports->currentText() == "Poseidon MkVI") {
				QPair<QString, QString> pair = poseidonFileNames(fileNames[i]);
				parse_txt_file(qPrintable(pair.second), qPrintable(pair.first), &log);
			} else {
				xml_params params;

				if (txtLog) {
					parseTxtHeader(fileNames[i], params);
				} else {
					QRegularExpression apdRe("^.*[/\\][0-9a-zA-Z]*_([0-9]{6})_([0-9]{6})\\.apd\\z");
					QRegularExpressionMatch match = apdRe.match(fileNames[i]);
					if (match.hasMatch()) {
						xml_params_add(&params, "date", qPrintable("20" + match.captured(1)));
						xml_params_add(&params, "time", qPrintable("1" + match.captured(2)));
					}
				}
				setup_csv_params(r, params);
				parse_csv_file(qPrintable(fileNames[i]), &params,
						specialCSV.contains(ui->knownImports->currentIndex()) ? qPrintable(CSVApps[ui->knownImports->currentIndex()].name) : "csv",
						&log);
			}
		}
	} else {
		for (int i = 0; i < fileNames.size(); ++i) {
			if (r.indexOf(tr("Sample time")) < 0) {
				xml_params params;
				xml_params_add_int(&params, "numberField", r.indexOf(tr("Dive #")));
				xml_params_add_int(&params, "dateField", r.indexOf(tr("Date")));
				xml_params_add_int(&params, "timeField", r.indexOf(tr("Time")));
				xml_params_add_int(&params, "durationField", r.indexOf(tr("Duration")));
				xml_params_add_int(&params, "modeField", r.indexOf(tr("Mode")));
				xml_params_add_int(&params, "locationField", r.indexOf(tr("Location")));
				xml_params_add_int(&params, "gpsField", r.indexOf(tr("GPS")));
				xml_params_add_int(&params, "maxDepthField", r.indexOf(tr("Max. depth")));
				xml_params_add_int(&params, "meanDepthField", r.indexOf(tr("Avg. depth")));
				xml_params_add_int(&params, "divemasterField", r.indexOf(tr("Divemaster")));
				xml_params_add_int(&params, "buddyField", r.indexOf(tr("Buddy")));
				xml_params_add_int(&params, "suitField", r.indexOf(tr("Suit")));
				xml_params_add_int(&params, "notesField", r.indexOf(tr("Notes")));
				xml_params_add_int(&params, "weightField", r.indexOf(tr("Weight")));
				xml_params_add_int(&params, "tagsField", r.indexOf(tr("Tags")));
				xml_params_add_int(&params, "separatorIndex", ui->CSVSeparator->currentIndex());
				xml_params_add_int(&params, "units", ui->CSVUnits->currentIndex());
				xml_params_add_int(&params, "datefmt", ui->DateFormat->currentIndex());
				xml_params_add_int(&params, "durationfmt", ui->DurationFormat->currentIndex());
				xml_params_add_int(&params, "cylindersizeField", r.indexOf(tr("Cyl. size")));
				xml_params_add_int(&params, "startpressureField", r.indexOf(tr("Start pressure")));
				xml_params_add_int(&params, "endpressureField", r.indexOf(tr("End pressure")));
				xml_params_add_int(&params, "o2Field", r.indexOf(tr("O₂")));
				xml_params_add_int(&params, "heField", r.indexOf(tr("He")));
				xml_params_add_int(&params, "airtempField", r.indexOf(tr("Air temp.")));
				xml_params_add_int(&params, "watertempField", r.indexOf(tr("Water temp.")));
				xml_params_add_int(&params, "visibilityField", r.indexOf(tr("Visibility")));
				xml_params_add_int(&params, "ratingField", r.indexOf(tr("Rating")));

				parse_manual_file(qPrintable(fileNames[i]), &params, &log);
			} else {
				xml_params params;

				if (txtLog) {
					parseTxtHeader(fileNames[i], params);
				} else {
					QRegularExpression apdRe("\\A^.*[/\\][0-9a-zA-Z]*_([0-9]{6})_([0-9]{6})\\.apd\\z");
					QRegularExpressionMatch match = apdRe.match(fileNames[i]);
					if (match.hasMatch()) {
						xml_params_add(&params, "date", qPrintable("20" + match.captured(1)));
						xml_params_add(&params, "time", qPrintable("1" + match.captured(2)));
					}

				}
				setup_csv_params(r, params);
				parse_csv_file(qPrintable(fileNames[i]), &params,
						specialCSV.contains(ui->knownImports->currentIndex()) ? qPrintable(CSVApps[ui->knownImports->currentIndex()].name) : "csv",
						&log);
			}
		}
	}

	QString source = fileNames.size() == 1 ? fileNames[0] : tr("multiple files");
	Command::importDives(&log, IMPORT_MERGE_ALL_TRIPS, source);
}

TagDragDelegate::TagDragDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

QSize TagDragDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QSize originalSize = QStyledItemDelegate::sizeHint(option, index);
	return originalSize + QSize(5,5);
}

void TagDragDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	painter->save();
	painter->setRenderHints(QPainter::Antialiasing);
	painter->setBrush(QBrush(AIR_BLUE_TRANS));
	painter->drawRoundedRect(option.rect.adjusted(2,2,-2,-2), 5, 5);
	painter->restore();
	QStyledItemDelegate::paint(painter, option, index);
}
