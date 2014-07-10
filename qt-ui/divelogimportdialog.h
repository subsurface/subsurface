#ifndef DIVELOGIMPORTDIALOG_H
#define DIVELOGIMPORTDIALOG_H

#include <QDialog>

#include "../dive.h"
#include "../divelist.h"

namespace Ui {
	class DiveLogImportDialog;
}

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

#define CSVAPPS 6
	static const CSVAppConfig CSVApps[CSVAPPS];
};

#endif // DIVELOGIMPORTDIALOG_H
