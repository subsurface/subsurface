#include <QtDebug>
#include <QFileDialog>
#include <QShortcut>
#include "divelogimportdialog.h"
#include "mainwindow.h"
#include "ui_divelogimportdialog.h"
#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QFile>

static QString subsurface_mimedata = "subsurface/csvcolumns";

const DiveLogImportDialog::CSVAppConfig DiveLogImportDialog::CSVApps[CSVAPPS] = {
	// time, depth, temperature, po2, cns, ndl, tts, stopdepth, pressure
	{ "Manual Import", },
	{ "APD Log Viewer", 1, 2, 16, 7, 18, -1, -1, 19, -1, "Tab" },
	{ "XP5", 1, 2, 10, -1, -1, -1, -1, -1, -1, "Tab" },
	{ "SensusCSV", 10, 11, -1, -1, -1, -1, -1, -1, -1, "," },
	{ "Seabear CSV", 1, 2, 6, -1, -1, 3, 4, 5, 7, ";" },
	{ "SubsurfaceCSV", -1, -1, -1, -1, -1, -1, -1, -1, -1, "," },
	{ NULL, }
};

ColumnNameProvider::ColumnNameProvider(QObject *parent) : QAbstractListModel(parent)
{
	columnNames << tr("Dive #") << tr("Date") << tr("Time") << tr("Duration") << tr("Location") << tr("GPS") << tr("Weight") << tr("Cyl size") << tr("Start Pressure")
		    << tr("End Press") << tr("Max depth") << tr("Mean depth") << tr("Buddy") << tr("Notes") << tr("Tags") << tr("Air temp") << tr("Water temp")
		    << tr("O₂") << tr("He");
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

	event->acceptProposedAction();
	const QMimeData *mimeData = event->mimeData();
	if (mimeData->data(subsurface_mimedata).count()) {
		QVariant value = QString(mimeData->data(subsurface_mimedata));
		model()->setData(curr, value);
	}
}

ColumnNameResult::ColumnNameResult(QObject *parent) : QAbstractTableModel(parent)
{

}

bool ColumnNameResult::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() != 0)
		return false;
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
	if (role != Qt::DisplayRole)
		return QVariant();

	if (index.row() == 0) {
		return (columnNames[index.column()]);
	}
	return QVariant(columnValues[index.row() -1][index.column()]);
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
	for(int i = 0; i < first.count(); i++){
		columnNames.append(QString());
	}
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
	drag->setPixmap(pix);
	drag->setMimeData(mimeData);
	if (drag->exec() != Qt::IgnoreAction){
		// Do stuff here.
	}
}

DiveLogImportDialog::DiveLogImportDialog(QStringList fn, QWidget *parent) : QDialog(parent),
	selector(true),
	ui(new Ui::DiveLogImportDialog)
{
	ui->setupUi(this);
	fileNames = fn;
	column = 0;

	/* Add indexes of XSLTs requiring special handling to the list */
	specialCSV << 3;
	specialCSV << 5;

	for (int i = 0; !CSVApps[i].name.isNull(); ++i)
		ui->knownImports->addItem(CSVApps[i].name);

	ui->CSVSeparator->addItems( QStringList() << tr("Tab") << ";" << ",");
	ui->knownImports->setCurrentIndex(1);

	ColumnNameProvider *provider = new ColumnNameProvider(this);
	ui->avaliableColumns->setModel(provider);

	resultModel = new ColumnNameResult(this);
	ui->tableView->setModel(resultModel);

	loadFileContents();

	/* manually import CSV file */
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));

	connect(ui->CSVSeparator, SIGNAL(currentIndexChanged(int)), this, SLOT(loadFileContents()));
}

DiveLogImportDialog::~DiveLogImportDialog()
{
	delete ui;
}

void DiveLogImportDialog::loadFileContents() {
	QFile f(fileNames.first());
	QList<QStringList> fileColumns;
	QStringList currColumns;

	f.open(QFile::ReadOnly);
	int rows = 0;
	while (rows < 10 || !f.atEnd()) {
		QString currLine = f.readLine();
		QString separator = ui->CSVSeparator->currentText() == tr("Tab") ? "\t"
				: ui->CSVSeparator->currentText();
		currColumns = currLine.split(separator);
		fileColumns.append(currColumns);
		rows += 1;
	}
	resultModel->setColumnValues(fileColumns);
}

void DiveLogImportDialog::on_buttonBox_accepted()
{
	QStringList r = resultModel->result();
	if (ui->knownImports->currentText() != "Manual Import") {
		for (int i = 0; i < fileNames.size(); ++i) {
			if (ui->knownImports->currentText() == "Seabear CSV") {
				parse_seabear_csv_file(fileNames[i].toUtf8().data(),
					r.indexOf(tr("Time")),
					r.indexOf(tr("Max Depth")),
					r.indexOf(tr("Water temp")),
					r.indexOf(tr("PO₂")),
					r.indexOf(tr("CNS")),
					r.indexOf(tr("NDL")),
					r.indexOf(tr("TTS")),
					r.indexOf(tr("Stopped Depth")),
					r.indexOf(tr("Pressure")),
					ui->CSVSeparator->currentIndex(),
					specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv",
					ui->CSVUnits->currentIndex()
				);

				// Seabear CSV stores NDL and TTS in Minutes, not seconds
				struct dive *dive = dive_table.dives[dive_table.nr - 1];
				for(int s_nr = 0 ; s_nr <= dive->dc.samples ; s_nr++) {
					struct sample *sample = dive->dc.sample + s_nr;
					sample->ndl.seconds *= 60;
					sample->tts.seconds *= 60;
				}
			} else {
				parse_csv_file(fileNames[i].toUtf8().data(),
					r.indexOf(tr("Time")),
					r.indexOf(tr("Max Depth")),
					r.indexOf(tr("Water temp")),
					r.indexOf(tr("PO₂")),
					r.indexOf(tr("CNS")),
					r.indexOf(tr("NDL")),
					r.indexOf(tr("TTS")),
					r.indexOf(tr("Stopped Depth")),
					r.indexOf(tr("Pressure")),
					ui->CSVSeparator->currentIndex(),
					specialCSV.contains(ui->knownImports->currentIndex()) ? CSVApps[ui->knownImports->currentIndex()].name.toUtf8().data() : "csv",
					ui->CSVUnits->currentIndex()
				);
			}
		}
	} else {
		for (int i = 0; i < fileNames.size(); ++i) {
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
				r.indexOf(tr("Max depth")),
				r.indexOf(tr("Mean depth")),
				r.indexOf(tr("Buddy")),
				r.indexOf(tr("Notes")),
				r.indexOf(tr("Weight")),
				r.indexOf(tr("Tags")),
				r.indexOf(tr("Cyl size")),
				r.indexOf(tr("Start Pressure")),
				r.indexOf(tr("End Pressure")),
				r.indexOf(tr("O₂")),
				r.indexOf(tr("He")),
				r.indexOf(tr("Air Temp")),
				r.indexOf(tr("Water Temp"))
			);
		}
	}
	process_dives(true, false);
	MainWindow::instance()->refreshDisplay();
}
