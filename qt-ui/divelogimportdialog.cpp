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
	// time, depth, temperature, po2, cns, ndl, tts, stopdepth, pressure
	// indices are 0 based, -1 means the column doesn't exist
	{ "Manual import", },
	{ "APD Log Viewer", 0, 1, 15, 6, 17, -1, -1, 18, -1, "Tab" },
	{ "XP5", 0, 1, 9, -1, -1, -1, -1, -1, -1, "Tab" },
	{ "SensusCSV", 9, 10, -1, -1, -1, -1, -1, -1, -1, "," },
	{ "Seabear CSV", 0, 1, 5, -1, -1, 2, 3, 4, 6, ";" },
	{ "SubsurfaceCSV", -1, -1, -1, -1, -1, -1, -1, -1, -1, "Tab" },
	{ NULL, }
};

ColumnNameProvider::ColumnNameProvider(QObject *parent) : QAbstractListModel(parent)
{
	columnNames << tr("Dive #") << tr("Date") << tr("Time") << tr("Duration") << tr("Location") << tr("GPS") << tr("Weight") << tr("Cyl. size") << tr("Start pressure") <<
		       tr("End pressure") << tr("Max. depth") << tr("Avg. depth") << tr("Divemaster") << tr("Buddy") << tr("Suit") << tr("Notes") << tr("Tags") << tr("Air temp.") << tr("Water temp.") <<
		       tr("O₂") << tr("He") << tr("Sample time") << tr("Sample depth") << tr("Sample temperature") << tr("Sample pO₂") << tr("Sample CNS") << tr("Sample NDL") <<
		       tr("Sample TTS") << tr("Sample stopdepth") << tr("Sample pressure");
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

	/* Add indexes of XSLTs requiring special handling to the list */
	specialCSV << 3;
	specialCSV << 5;

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
		 * interval, or if we have old format (if interval value
		 * is missing from the header).
		 */

		while ((firstLine = f.readLine()).length() > 3 && !f.atEnd()) {
			if (firstLine.contains("//Log interval: "))
				delta = firstLine.remove(QString::fromLatin1("//Log interval: ")).trimmed();
		}

		/*
		 * Parse CSV fields
		 * The pO2 values from CCR diving are ignored later on.
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
				headers.append("Sample pO2_1");
			} else if (columnText == "pO2_2") {
				headers.append("Sample pO2_2");
			} else if (columnText == "pO2_3") {
				headers.append("Sample pO2_3");
			} else if (columnText == "Ceiling") {
				headers.append("Sample ceiling");
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
	if ((triggeredBy == KNOWNTYPES && value == 1) || (triggeredBy == INITIAL && fileNames.first().endsWith(".apd", Qt::CaseInsensitive))) {
		apd=true;
		firstLine = "Sample time\tSample depth\t\t\t\t\tSample pO₂\t\t\t\t\t\t\t\t\tSample temperature\t\tSample CNS\tSample stopdepth";
		blockSignals(true);
		ui->CSVSeparator->setCurrentText(tr("Tab"));
		if (triggeredBy == INITIAL && fileNames.first().contains(".apd", Qt::CaseInsensitive))
			ui->knownImports->setCurrentText("APD Log Viewer");
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
	if (triggeredBy == INITIAL || (triggeredBy == KNOWNTYPES && value == 0) || triggeredBy == SEPARATOR) {
		// now try and guess the columns
		Q_FOREACH (QString columnText, currColumns) {
			columnText.replace("\"", "");
			columnText.replace("number", "#", Qt::CaseInsensitive);
			columnText.replace("2", "₂", Qt::CaseInsensitive);
			columnText.replace("cylinder", "cyl.", Qt::CaseInsensitive);
			int idx = provider->mymatch(columnText);
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
	if (triggeredBy == KNOWNTYPES && value != 0) {
		// an actual known type
		if (value == 5) {
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
		// now set up time, depth, temperature, po2, cns, ndl, tts, stopdepth, pressure
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

void DiveLogImportDialog::on_buttonBox_accepted()
{
	QStringList r = resultModel->result();
	if (ui->knownImports->currentText() != "Manual import") {
		for (int i = 0; i < fileNames.size(); ++i) {
			if (ui->knownImports->currentText() == "Seabear CSV") {
				if (parse_seabear_csv_file(fileNames[i].toUtf8().data(),
						       r.indexOf(tr("Sample time")),
						       r.indexOf(tr("Sample depth")),
						       r.indexOf(tr("Sample temperature")),
						       r.indexOf(tr("Sample pO₂")),
						       r.indexOf(tr("Sample CNS")),
						       r.indexOf(tr("Sample NDL")),
						       r.indexOf(tr("Sample TTS")),
						       r.indexOf(tr("Sample stopdepth")),
						       r.indexOf(tr("Sample pressure")),
						       ui->CSVSeparator->currentIndex(),
						       specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv",
						       ui->CSVUnits->currentIndex(),
						       delta.toUtf8().data()
						       ) < 0) {
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
				parse_csv_file(fileNames[i].toUtf8().data(),
					       r.indexOf(tr("Sample time")),
					       r.indexOf(tr("Sample depth")),
					       r.indexOf(tr("Sample temperature")),
					       r.indexOf(tr("Sample pO₂")),
					       r.indexOf(tr("Sample CNS")),
					       r.indexOf(tr("Sample NDL")),
					       r.indexOf(tr("Sample TTS")),
					       r.indexOf(tr("Sample stopdepth")),
					       r.indexOf(tr("Sample pressure")),
					       ui->CSVSeparator->currentIndex(),
					       specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv",
					       ui->CSVUnits->currentIndex()
					       );
			}
		}
	} else {
		for (int i = 0; i < fileNames.size(); ++i) {
			if (r.indexOf(tr("Sample time")) < 0)
				parse_manual_file(fileNames[i].toUtf8().data(),
						ui->CSVSeparator->currentIndex(),
						ui->CSVUnits->currentIndex(),
						ui->DateFormat->currentIndex(),
						ui->DurationFormat->currentIndex(),
						r.indexOf(tr("Dive #")),
						r.indexOf(tr("Date")),
						r.indexOf(tr("Time")),
						r.indexOf(tr("Duration")),
						r.indexOf(tr("Location")),
						r.indexOf(tr("GPS")),
						r.indexOf(tr("Max. depth")),
						r.indexOf(tr("Avg. depth")),
						r.indexOf(tr("Divemaster")),
						r.indexOf(tr("Buddy")),
						r.indexOf(tr("Suit")),
						r.indexOf(tr("Notes")),
						r.indexOf(tr("Weight")),
						r.indexOf(tr("Tags")),
						r.indexOf(tr("Cyl. size")),
						r.indexOf(tr("Start pressure")),
						r.indexOf(tr("End pressure")),
						r.indexOf(tr("O₂")),
						r.indexOf(tr("He")),
						r.indexOf(tr("Air temp.")),
						r.indexOf(tr("Water temp."))
							);
			else
				parse_csv_file(fileNames[i].toUtf8().data(),
					       r.indexOf(tr("Sample time")),
					       r.indexOf(tr("Sample depth")),
					       r.indexOf(tr("Sample temperature")),
					       r.indexOf(tr("Sample pO₂")),
					       r.indexOf(tr("Sample CNS")),
					       r.indexOf(tr("Sample NDL")),
					       r.indexOf(tr("Sample TTS")),
					       r.indexOf(tr("Sample stopdepth")),
					       r.indexOf(tr("Sample pressure")),
					       ui->CSVSeparator->currentIndex(),
					       specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv",
					       ui->CSVUnits->currentIndex()
					       );
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
