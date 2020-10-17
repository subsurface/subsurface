// SPDX-License-Identifier: GPL-2.0
#ifndef DIVELOGIMPORTDIALOG_H
#define DIVELOGIMPORTDIALOG_H

#include <QDialog>
#include <QAbstractListModel>
#include <QListView>
#include <QDragLeaveEvent>
#include <QTableView>
#include <QAbstractTableModel>
#include <QStyledItemDelegate>

#include "core/dive.h"
#include "core/divelist.h"

struct xml_params;
namespace Ui {
	class DiveLogImportDialog;
}

class ColumnNameProvider : public QAbstractListModel {
	Q_OBJECT
public:
	ColumnNameProvider(QObject *parent);
	bool insertRows(int row, int count, const QModelIndex &parent);
	bool removeRows(int row, int count, const QModelIndex &parent);
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant data(const QModelIndex &index, int role) const;
	int rowCount(const QModelIndex &parent) const;
	int mymatch(QString value) const;
private:
	QStringList columnNames;
};

class ColumnNameResult : public QAbstractTableModel {
	Q_OBJECT
public:
	ColumnNameResult(QObject *parent);
	bool setData(const QModelIndex &index, const QVariant &value, int role);
	QVariant data(const QModelIndex &index, int role) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	void setColumnValues(QList<QStringList> columns);
	QStringList result() const;
	void swapValues(int firstIndex, int secondIndex);
private:
	QList<QStringList> columnValues;
	QStringList columnNames;
};

class ColumnNameView : public QListView {
	Q_OBJECT
public:
	ColumnNameView(QWidget *parent);
protected:
	void mousePressEvent(QMouseEvent *press);
	void dragLeaveEvent(QDragLeaveEvent *leave);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);
private:
};

class ColumnDropCSVView : public QTableView {
	Q_OBJECT
public:
	ColumnDropCSVView(QWidget *parent);
protected:
	void mousePressEvent(QMouseEvent *press);
	void dragLeaveEvent(QDragLeaveEvent *leave);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void dropEvent(QDropEvent *event);
private:
	QStringList columns;
};

class DiveLogImportDialog : public QDialog {
	Q_OBJECT

public:
	explicit DiveLogImportDialog(QStringList fn, QWidget *parent = 0);
	~DiveLogImportDialog();
	enum whatChanged { INITIAL, SEPARATOR, KNOWNTYPES };
private
slots:
	void on_buttonBox_accepted();
	void loadFileContentsSeperatorSelected(int value);
	void loadFileContentsKnownTypesSelected(int value);
	void loadFileContents(int value, enum whatChanged triggeredBy);
	void setup_csv_params(QStringList r, xml_params &params);
	void parseTxtHeader(QString fileName, xml_params &params);

private:
	bool selector;
	QStringList fileNames;
	Ui::DiveLogImportDialog *ui;
	QList<int> specialCSV;
	int column;
	ColumnNameResult *resultModel;
	QString delta;
	QString hw;
	bool txtLog;

};

class TagDragDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	TagDragDelegate(QObject *parent);
	QSize	sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
};

#endif // DIVELOGIMPORTDIALOG_H
