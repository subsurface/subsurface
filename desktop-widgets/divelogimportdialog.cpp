#include "divelogimportdialog.h"
#include "mainwindow.h"
#include "color.h"
#include "ui_divelogimportdialog.h"
#include <QShortcut>
#include <QDrag>
#include <QMimeData>

static QString subsurface_mimedata = "subsurface/csvcolumns";
static QString subsurface_index = "subsurface/csvindex";

const DiveLogImportDialog::CSVAppConfig DiveLogImportDialog::CSVApps[CSVAPPS] = {
	// time, depth, temperature, po2, sensor1, sensor2, sensor3, cns, ndl, tts, stopdepth, pressure, setpoint
	// indices are 0 based, -1 means the column doesn't exist
	{ "Manual import", },
	{ "APD Log Viewer - DC1", 0, 1, 15, 6, 3, 4, 5, 17, -1, -1, 18, -1, 2, "Tab" },
	{ "APD Log Viewer - DC2", 0, 1, 15, 6, 7, 8, 9, 17, -1, -1, 18, -1, 2, "Tab" },
	{ "XP5", 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "Tab" },
	{ "SensusCSV", 9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "," },
	{ "Seabear CSV", 0, 1, 5, -1, -1, -1, -1, -1, 2, 3, 4, 6, -1, ";" },
	{ "SubsurfaceCSV", -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "Tab" },
	{ NULL, }
};

enum Known {
	MANUAL,
	APD,
	APD2,
	XP5,
	SENSUS,
	SEABEAR,
	SUBSURFACE
};

ColumnNameProvider::ColumnNameProvider(QObject *parent) : QAbstractListModel(parent)
{
	columnNames << tr("Dive #") << tr("Date") << tr("Time") << tr("Duration") << tr("Location") << tr("GPS") << tr("Weight") << tr("Cyl. size") << tr("Start pressure") <<
		       tr("End pressure") << tr("Max. depth") << tr("Avg. depth") << tr("Divemaster") << tr("Buddy") << tr("Suit") << tr("Notes") << tr("Tags") << tr("Air temp.") << tr("Water temp.") <<
		       tr("O₂") << tr("He") << tr("Sample time") << tr("Sample depth") << tr("Sample temperature") << tr("Sample pO₂") << tr("Sample CNS") << tr("Sample NDL") <<
		       tr("Sample TTS") << tr("Sample stopdepth") << tr("Sample pressure") <<
		       tr("Sample sensor1 pO₂") << tr("Sample sensor2 pO₂") << tr("Sample sensor3 pO₂") <<
		       tr("Sample setpoint");
}

bool ColumnNameProvider::insertRows(int row, int count, const QModelIndex &parent)
{
	beginInsertRows(QModelIndex(), row, row);
	columnNames.append(QString());
	endInsertRows();
	return true;
}

bool ColumnNameProvider::removeRows(int row, int count, const QModelIndex &parent)
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

int ColumnNameProvider::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return columnNames.count();
}

int ColumnNameProvider::mymatch(QString value) const
{
	QString searchString = value.toLower();
	searchString.replace("\"", "").replace(" ", "").replace(".", "").replace("\n","");
	for (int i = 0; i < columnNames.count(); i++) {
		QString name = columnNames.at(i).toLower();
		name.replace("\"", "").replace(" ", "").replace(".", "").replace("\n","");
		if (searchString == name.toLower())
			return i;
	}
	return -1;
}



ColumnNameView::ColumnNameView(QWidget *parent)
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

void ColumnNameView::dragLeaveEvent(QDragLeaveEvent *leave)
{
	Q_UNUSED(leave);
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

ColumnDropCSVView::ColumnDropCSVView(QWidget *parent)
{
	setAcceptDrops(true);
}

void ColumnDropCSVView::dragLeaveEvent(QDragLeaveEvent *leave)
{
	Q_UNUSED(leave);
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

void ColumnNameResult::swapValues(int firstIndex, int secondIndex) {
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
	if (role == Qt::BackgroundColorRole)
		if (index.row() == 0)
			return QVariant(AIR_BLUE_TRANS);

	if (role != Qt::DisplayRole)
		return QVariant();

	if (index.row() == 0) {
		return (columnNames[index.column()]);
	}
	// make sure the element exists before returning it - this might get called before the
	// model is correctly set up again (e.g., when changing separators)
	if (columnValues.count() > index.row() - 1 && columnValues[index.row() - 1].count() > index.column())
		return QVariant(columnValues[index.row() - 1][index.column()]);
	else
		return QVariant();
}

int ColumnNameResult::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return columnValues.count() + 1; // +1 == the header.
}

int ColumnNameResult::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
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
	columnValues = columns;
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
	mimeData->setData(subsurface_index, QString::number(atClick.column()).toLocal8Bit());
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
	fileNames = fn;
	column = 0;
	delta = "0";
	hw = "";

	/* Add indexes of XSLTs requiring special handling to the list */
	specialCSV << SENSUS;
	specialCSV << SUBSURFACE;

	for (int i = 0; !CSVApps[i].name.isNull(); ++i)
		ui->knownImports->addItem(CSVApps[i].name);

	ui->CSVSeparator->addItems( QStringList() << tr("Tab") << "," << ";");

	loadFileContents(-1, INITIAL);

	/* manually import CSV file */
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
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

void DiveLogImportDialog::loadFileContents(int value, whatChanged triggeredBy)
{
	QFile f(fileNames.first());
	QList<QStringList> fileColumns;
	QStringList currColumns;
	QStringList headers;
	bool matchedSome = false;
	bool seabear = false;
	bool xp5 = false;
	bool apd = false;

	// reset everything
	ColumnNameProvider *provider = new ColumnNameProvider(this);
	ui->avaliableColumns->setModel(provider);
	ui->avaliableColumns->setItemDelegate(new TagDragDelegate(ui->avaliableColumns));
	resultModel = new ColumnNameResult(this);
	ui->tableView->setModel(resultModel);

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
	}

	// Special handling for APD Log Viewer
	if ((triggeredBy == KNOWNTYPES && (value == APD || value == APD2)) || (triggeredBy == INITIAL && fileNames.first().endsWith(".apd", Qt::CaseInsensitive))) {
		apd=true;
		firstLine = "Sample time\tSample depth\tSample setpoint\tSample sensor1 pO₂\tSample sensor2 pO₂\tSample sensor3 pO₂\tSample pO₂\t\t\t\t\t\t\t\t\tSample temperature\t\tSample CNS\tSample stopdepth";
		blockSignals(true);
		ui->CSVSeparator->setCurrentText(tr("Tab"));
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
		if (tabs > commas && tabs > semis)
			separator = "\t";
		else if (commas > tabs && commas > semis)
			separator = ",";
		else if (semis > tabs && semis > commas)
			separator = ";";
		if (ui->CSVSeparator->currentText() != separator) {
			blockSignals(true);
			ui->CSVSeparator->setCurrentText(separator);
			blockSignals(false);
			currColumns = firstLine.split(separator);
		}
	}
	if (triggeredBy == INITIAL || (triggeredBy == KNOWNTYPES && value == MANUAL) || triggeredBy == SEPARATOR) {
		// now try and guess the columns
		Q_FOREACH (QString columnText, currColumns) {
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
			} else {
				headers.append("");
			}
		}
		if (matchedSome) {
			ui->dragInstructions->setText(tr("Some column headers were pre-populated; please drag and drop the headers so they match the column they are in."));
			if (triggeredBy != KNOWNTYPES && !seabear && !xp5 && !apd) {
				blockSignals(true);
				ui->knownImports->setCurrentIndex(0); // <- that's "Manual import"
				blockSignals(false);
			}
		}
	}
	if (triggeredBy == KNOWNTYPES && value != MANUAL) {
		// an actual known type
		if (value == SUBSURFACE) {
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
			headers.replace(6, tr("Air temp."));
			headers.replace(7, tr("Water temp."));
			headers.replace(8, tr("Cyl. size"));
			headers.replace(9, tr("Start pressure"));
			headers.replace(10, tr("End pressure"));
			headers.replace(11, tr("O₂"));
			headers.replace(12, tr("He"));
			headers.replace(13, tr("Location"));
			headers.replace(14, tr("GPS"));
			headers.replace(15, tr("Divemaster"));
			headers.replace(16, tr("Buddy"));
			headers.replace(17, tr("Suit"));
			headers.replace(18, tr("Rating"));
			headers.replace(19, tr("Visibility"));
			headers.replace(20, tr("Notes"));
			headers.replace(21, tr("Weight"));
			headers.replace(22, tr("Tags"));

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
	}

	while (rows < 10 && !f.atEnd()) {
		QString currLine = f.readLine().trimmed();
		currColumns = currLine.split(separator);
		fileColumns.append(currColumns);
		rows += 1;
	}
	resultModel->setColumnValues(fileColumns);
	for (int i = 0; i < headers.count(); i++)
		if (!headers.at(i).isEmpty())
			resultModel->setData(resultModel->index(0, i),headers.at(i),Qt::EditRole);
}

char *intdup(int index)
{
	char tmpbuf[21];

	snprintf(tmpbuf, sizeof(tmpbuf) - 2, "%d", index);
	tmpbuf[20] = 0;
	return strdup(tmpbuf);
}

int DiveLogImportDialog::setup_csv_params(QStringList r, char **params, int pnr)
{
	params[pnr++] = strdup("timeField");
	params[pnr++] = intdup(r.indexOf(tr("Sample time")));
	params[pnr++] = strdup("depthField");
	params[pnr++] = intdup(r.indexOf(tr("Sample depth")));
	params[pnr++] = strdup("tempField");
	params[pnr++] = intdup(r.indexOf(tr("Sample temperature")));
	params[pnr++] = strdup("po2Field");
	params[pnr++] = intdup(r.indexOf(tr("Sample pO₂")));
	params[pnr++] = strdup("o2sensor1Field");
	params[pnr++] = intdup(r.indexOf(tr("Sample sensor1 pO₂")));
	params[pnr++] = strdup("o2sensor2Field");
	params[pnr++] = intdup(r.indexOf(tr("Sample sensor2 pO₂")));
	params[pnr++] = strdup("o2sensor3Field");
	params[pnr++] = intdup(r.indexOf(tr("Sample sensor3 pO₂")));
	params[pnr++] = strdup("cnsField");
	params[pnr++] = intdup(r.indexOf(tr("Sample CNS")));
	params[pnr++] = strdup("ndlField");
	params[pnr++] = intdup(r.indexOf(tr("Sample NDL")));
	params[pnr++] = strdup("ttsField");
	params[pnr++] = intdup(r.indexOf(tr("Sample TTS")));
	params[pnr++] = strdup("stopdepthField");
	params[pnr++] = intdup(r.indexOf(tr("Sample stopdepth")));
	params[pnr++] = strdup("pressureField");
	params[pnr++] = intdup(r.indexOf(tr("Sample pressure")));
	params[pnr++] = strdup("setpointFiend");
	params[pnr++] = intdup(r.indexOf(tr("Sample setpoint")));
	params[pnr++] = strdup("separatorIndex");
	params[pnr++] = intdup(ui->CSVSeparator->currentIndex());
	params[pnr++] = strdup("units");
	params[pnr++] = intdup(ui->CSVUnits->currentIndex());
	if (hw.length()) {
		params[pnr++] = strdup("hw");
		params[pnr++] = strdup(hw.toUtf8().data());
	} else if (ui->knownImports->currentText().length() > 0) {
		params[pnr++] = strdup("hw");
		params[pnr++] = strdup(ui->knownImports->currentText().prepend("\"").append("\"").toUtf8().data());
	}
	params[pnr++] = NULL;

	return pnr;
}

void DiveLogImportDialog::on_buttonBox_accepted()
{
	QStringList r = resultModel->result();
	if (ui->knownImports->currentText() != "Manual import") {
		for (int i = 0; i < fileNames.size(); ++i) {
			if (ui->knownImports->currentText() == "Seabear CSV") {
				char *params[40];
				int pnr = 0;

				params[pnr++] = strdup("timeField");
				params[pnr++] = intdup(r.indexOf(tr("Sample time")));
				params[pnr++] = strdup("depthField");
				params[pnr++] = intdup(r.indexOf(tr("Sample depth")));
				params[pnr++] = strdup("tempField");
				params[pnr++] = intdup(r.indexOf(tr("Sample temperature")));
				params[pnr++] = strdup("po2Field");
				params[pnr++] = intdup(r.indexOf(tr("Sample pO₂")));
				params[pnr++] = strdup("o2sensor1Field");
				params[pnr++] = intdup(r.indexOf(tr("Sample sensor1 pO₂")));
				params[pnr++] = strdup("o2sensor2Field");
				params[pnr++] = intdup(r.indexOf(tr("Sample sensor2 pO₂")));
				params[pnr++] = strdup("o2sensor3Field");
				params[pnr++] = intdup(r.indexOf(tr("Sample sensor3 pO₂")));
				params[pnr++] = strdup("cnsField");
				params[pnr++] = intdup(r.indexOf(tr("Sample CNS")));
				params[pnr++] = strdup("ndlField");
				params[pnr++] = intdup(r.indexOf(tr("Sample NDL")));
				params[pnr++] = strdup("ttsField");
				params[pnr++] = intdup(r.indexOf(tr("Sample TTS")));
				params[pnr++] = strdup("stopdepthField");
				params[pnr++] = intdup(r.indexOf(tr("Sample stopdepth")));
				params[pnr++] = strdup("pressureField");
				params[pnr++] = intdup(r.indexOf(tr("Sample pressure")));
				params[pnr++] = strdup("setpointFiend");
				params[pnr++] = intdup(-1);
				params[pnr++] = strdup("separatorIndex");
				params[pnr++] = intdup(ui->CSVSeparator->currentIndex());
				params[pnr++] = strdup("units");
				params[pnr++] = intdup(ui->CSVUnits->currentIndex());
				params[pnr++] = strdup("delta");
				params[pnr++] = strdup(delta.toUtf8().data());
				if (hw.length()) {
					params[pnr++] = strdup("hw");
					params[pnr++] = strdup(hw.toUtf8().data());
				}
				params[pnr++] = NULL;

				if (parse_seabear_csv_file(fileNames[i].toUtf8().data(),
							params, pnr - 1, "csv") < 0) {
					return;
				}
				// Seabear CSV stores NDL and TTS in Minutes, not seconds
				struct dive *dive = dive_table.dives[dive_table.nr - 1];
				for(int s_nr = 0 ; s_nr <= dive->dc.samples ; s_nr++) {
					struct sample *sample = dive->dc.sample + s_nr;
					sample->ndl.seconds *= 60;
					sample->tts.seconds *= 60;
				}
			} else {
				char *params[37];
				int pnr = 0;

				pnr = setup_csv_params(r, params, pnr);
				parse_csv_file(fileNames[i].toUtf8().data(), params, pnr - 1,
						specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv");
			}
		}
	} else {
		for (int i = 0; i < fileNames.size(); ++i) {
			if (r.indexOf(tr("Sample time")) < 0) {
				char *params[55];
				int pnr = 0;
				params[pnr++] = strdup("numberField");
				params[pnr++] = intdup(r.indexOf(tr("Dive #")));
				params[pnr++] = strdup("dateField");
				params[pnr++] = intdup(r.indexOf(tr("Date")));
				params[pnr++] = strdup("timeField");
				params[pnr++] = intdup(r.indexOf(tr("Time")));
				params[pnr++] = strdup("durationField");
				params[pnr++] = intdup(r.indexOf(tr("Duration")));
				params[pnr++] = strdup("locationField");
				params[pnr++] = intdup(r.indexOf(tr("Location")));
				params[pnr++] = strdup("gpsField");
				params[pnr++] = intdup(r.indexOf(tr("GPS")));
				params[pnr++] = strdup("maxDepthField");
				params[pnr++] = intdup(r.indexOf(tr("Max. depth")));
				params[pnr++] = strdup("meanDepthField");
				params[pnr++] = intdup(r.indexOf(tr("Avg. depth")));
				params[pnr++] = strdup("divemasterField");
				params[pnr++] = intdup(r.indexOf(tr("Divemaster")));
				params[pnr++] = strdup("buddyField");
				params[pnr++] = intdup(r.indexOf(tr("Buddy")));
				params[pnr++] = strdup("suitField");
				params[pnr++] = intdup(r.indexOf(tr("Suit")));
				params[pnr++] = strdup("notesField");
				params[pnr++] = intdup(r.indexOf(tr("Notes")));
				params[pnr++] = strdup("weightField");
				params[pnr++] = intdup(r.indexOf(tr("Weight")));
				params[pnr++] = strdup("tagsField");
				params[pnr++] = intdup(r.indexOf(tr("Tags")));
				params[pnr++] = strdup("separatorIndex");
				params[pnr++] = intdup(ui->CSVSeparator->currentIndex());
				params[pnr++] = strdup("units");
				params[pnr++] = intdup(ui->CSVUnits->currentIndex());
				params[pnr++] = strdup("datefmt");
				params[pnr++] = intdup(ui->DateFormat->currentIndex());
				params[pnr++] = strdup("durationfmt");
				params[pnr++] = intdup(ui->DurationFormat->currentIndex());
				params[pnr++] = strdup("cylindersizeField");
				params[pnr++] = intdup(r.indexOf(tr("Cyl. size")));
				params[pnr++] = strdup("startpressureField");
				params[pnr++] = intdup(r.indexOf(tr("Start pressure")));
				params[pnr++] = strdup("endpressureField");
				params[pnr++] = intdup(r.indexOf(tr("End pressure")));
				params[pnr++] = strdup("o2Field");
				params[pnr++] = intdup(r.indexOf(tr("O₂")));
				params[pnr++] = strdup("heField");
				params[pnr++] = intdup(r.indexOf(tr("He")));
				params[pnr++] = strdup("airtempField");
				params[pnr++] = intdup(r.indexOf(tr("Air temp.")));
				params[pnr++] = strdup("watertempField");
				params[pnr++] = intdup(r.indexOf(tr("Water temp.")));
				params[pnr++] = NULL;

				parse_manual_file(fileNames[i].toUtf8().data(), params, pnr - 1);
			} else {
				char *params[37];
				int pnr = 0;

				pnr = setup_csv_params(r, params, pnr);
				parse_csv_file(fileNames[i].toUtf8().data(), params, pnr - 1,
						specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv");
			}
		}
	}

	process_dives(true, false);
	MainWindow::instance()->refreshDisplay();
}

TagDragDelegate::TagDragDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

QSize	TagDragDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
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
