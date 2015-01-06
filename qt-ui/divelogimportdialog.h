#ifndef DIVELOGIMPORTDIALOG_H
#define DIVELOGIMPORTDIALOG_H

#include <QDialog>
#include <QAbstractListModel>
#include <QListView>
#include <QDragLeaveEvent>
#include <QTableView>

#include "../dive.h"
#include "../divelist.h"

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
private:
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
	int currentDraggedIndex;
};

class ColumnDropCSVView : public QTableView {
	Q_OBJECT
public:
	ColumnDropCSVView(QWidget *parent);
protected:
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
	explicit DiveLogImportDialog(QStringList *fn, QWidget *parent = 0);
	~DiveLogImportDialog();

private
slots:
	void on_buttonBox_accepted();
	void on_knownImports_currentIndexChanged(int index);
	void unknownImports();

private:
	bool selector;
	QStringList fileNames;
	Ui::DiveLogImportDialog *ui;
	QList<int> specialCSV;
	int column;

	struct CSVAppConfig {
		QString name;
		int time;
		int depth;
		int temperature;
		int po2;
		int cns;
		int ndl;
		int tts;
		int stopdepth;
		int pressure;
		QString separator;
	};

#define CSVAPPS 7
	static const CSVAppConfig CSVApps[CSVAPPS];
};

#endif // DIVELOGIMPORTDIALOG_H
