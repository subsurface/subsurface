#ifndef SMRTK2SSRFC_WINDOW_H
#define SMRTK2SSRFC_WINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFileInfo>

extern "C" void smartrak_import(const char *file, struct dive_table *divetable);

namespace Ui {
class Smrtk2ssrfcWindow;
}

class Smrtk2ssrfcWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit Smrtk2ssrfcWindow(QWidget *parent = 0);
	~Smrtk2ssrfcWindow();

private:
	Ui::Smrtk2ssrfcWindow *ui;
	QString lastUsedDir();
	QString filter();
	void updateLastUsedDir(const QString &s);
	void closeCurrentFile();

private
slots:
	void on_inputFilesButton_clicked();
	void on_outputFileButton_clicked();
	void on_importButton_clicked();
	void on_exitButton_clicked();
	void on_outputLine_textEdited();
	void on_inputLine_textEdited();
};

#endif // SMRTK2SSRFC_WINDOW_H
