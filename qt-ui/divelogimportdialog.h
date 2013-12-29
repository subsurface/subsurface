#ifndef DIVELOGIMPORTDIALOG_H
#define DIVELOGIMPORTDIALOG_H

#include <QDialog>
#include <QModelIndex>
#include "../dive.h"
#include "../divelist.h"

namespace Ui {
class DiveLogImportDialog;
}

class DiveLogImportDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DiveLogImportDialog(QWidget *parent = 0);
	~DiveLogImportDialog();

private slots:
	void on_buttonBox_accepted();
	void on_CSVFileSelector_clicked();
	void on_knownImports_currentIndexChanged(int index);
	void on_CSVFile_textEdited();
	void unknownImports(int);
	void unknownImports(bool);

	void on_DiveLogFileSelector_clicked();
	void on_DiveLogFile_editingFinished();

private:
	void unknownImports();

	bool selector;
	Ui::DiveLogImportDialog *ui;

	struct CSVAppConfig {
		QString name;
		int time;
		int depth;
		int temperature;
		int po2;
		int cns;
		int stopdepth;
		QString separator;
	};

#define CSVAPPS 4
	static const CSVAppConfig CSVApps[CSVAPPS];
};

#endif // DIVELOGIMPORTDIALOG_H
