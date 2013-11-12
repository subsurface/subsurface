#include "tableview.h"
#include "models.h"

#include <QPushButton>
#include <QLayout>
#include <QFile>
#include <QTextStream>
#include <QSettings>

TableView::TableView(QWidget *parent) : QTableView(parent)
{
	ui.setupUi(this);
	QFile cssFile(":table-css");
	cssFile.open(QIODevice::ReadOnly);
	QTextStream reader(&cssFile);
	QString css = reader.readAll();
	ui.tableView->setStyleSheet(css);
	/* There`s mostly a need for a Mac fix here too. */
	if (qApp->style()->objectName() == "gtk+")
		ui.groupBox->layout()->setContentsMargins(0, 9, 0, 0);
	else
		ui.groupBox->layout()->setContentsMargins(0, 0, 0, 0);
	QIcon plusIcon(":plus");
	plusBtn = new QPushButton(plusIcon, QString(), ui.groupBox);
	plusBtn->setFlat(true);
	plusBtn->setToolTip(tr("Add Cylinder"));
	plusBtn->setIconSize(QSize(16,16));
	connect(plusBtn, SIGNAL(clicked(bool)), this, SIGNAL(addButtonClicked()));
}

TableView::~TableView()
{
	QSettings s;
	s.beginGroup(objectName());
	for (int i = 0; i < ui.tableView->model()->columnCount(); i++) {
		s.setValue(QString("colwidth%1").arg(i), ui.tableView->columnWidth(i));
	}
	s.endGroup();
	s.sync();
}

void TableView::setBtnToolTip(const QString& tooltip)
{
	plusBtn->setToolTip(tooltip);
}

void TableView::setTitle(const QString& title)
{
	ui.groupBox->setTitle(title);
}

void TableView::setModel(QAbstractItemModel *model){
	ui.tableView->setModel(model);
	connect(ui.tableView, SIGNAL(clicked(QModelIndex)), model, SLOT(remove(QModelIndex)));

	QSettings s;
	s.beginGroup(objectName());
	for (int i = 0; i < ui.tableView->model()->columnCount(); i++) {
		QVariant width = s.value(QString("colwidth%1").arg(i));
		if (width.isValid())
			ui.tableView->setColumnWidth(i, width.toInt());
		else
			ui.tableView->resizeColumnToContents(i);
	}
	s.endGroup();

	QFontMetrics metrics(defaultModelFont());
	ui.tableView->horizontalHeader()->setMinimumHeight(metrics.height() + 10);
}

void TableView::fixPlusPosition()
{
	plusBtn->setGeometry(ui.groupBox->contentsRect().width() - 30, 2, 24,24);
}

// We need to manually position the 'plus' on cylinder and weight.
void TableView::resizeEvent(QResizeEvent* event)
{
	fixPlusPosition();
	QWidget::resizeEvent(event);
}

void TableView::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	fixPlusPosition();
}

void TableView::edit(const QModelIndex& index){
	ui.tableView->edit(index);
}

QTableView *TableView::view(){
	return ui.tableView;
}
