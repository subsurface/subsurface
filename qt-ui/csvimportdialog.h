#ifndef CSVIMPORTDIALOG_H
#define CSVIMPORTDIALOG_H

#include <QDialog>
#include <QModelIndex>
#include "../dive.h"
#include "../divelist.h"

namespace Ui {
class CSVImportDialog;
}

class CSVImportDialog : public QDialog
{
	Q_OBJECT

public:
	explicit CSVImportDialog(QWidget *parent = 0);
	~CSVImportDialog();

private slots:
	void on_buttonBox_accepted();
	void on_CSVFileSelector_clicked();
	void on_knownImports_currentIndexChanged(int index);
	void on_CSVFile_textEdited();
	void unknownImports(int);

private:
	void unknownImports();

	bool selector;
	Ui::CSVImportDialog *ui;

	struct CSVAppConfig {
		QString name;
		int time;
		int depth;
		int temperature;
		QString separator;
	};

#define CSVAPPS 4
	static const CSVAppConfig CSVApps[CSVAPPS];
};

#endif // CSVIMPORTDIALOG_H
