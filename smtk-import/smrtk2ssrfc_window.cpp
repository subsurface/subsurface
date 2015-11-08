#include "smrtk2ssrfc_window.h"
#include "ui_smrtk2ssrfc_window.h"
#include "filtermodels.h"
#include "dive.h"
#include "divelist.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QDebug>

QStringList inputFiles;
QString outputFile;

Smrtk2ssrfcWindow::Smrtk2ssrfcWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::Smrtk2ssrfcWindow)
{
	ui->setupUi(this);
	ui->plainTextEdit->setDisabled(true);
	ui->progressBar->setDisabled(true);
	ui->statusBar->adjustSize();
}

Smrtk2ssrfcWindow::~Smrtk2ssrfcWindow()
{
	delete ui;
}

QString Smrtk2ssrfcWindow::lastUsedDir()
{
	QSettings settings;
	QString lastDir = QDir::homePath();

	settings.beginGroup("FileDialog");
	if (settings.contains("LastDir"))
		if (QDir::setCurrent(settings.value("LastDir").toString()))
			lastDir = settings.value("LastDir").toString();
	return lastDir;
}

void Smrtk2ssrfcWindow::updateLastUsedDir(const QString &dir)
{
	QSettings s;
	s.beginGroup("FileDialog");
	s.setValue("LastDir", dir);
}

void Smrtk2ssrfcWindow::on_inputFilesButton_clicked()
{
	inputFiles = QFileDialog::getOpenFileNames(this, tr("Open SmartTrak files"), lastUsedDir(),
		tr("SmartTrak files (*.slg *.SLG);;"
		   "All files (*)"));
	if (inputFiles.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(inputFiles[0]).dir().path());
	ui->inputLine->setText(inputFiles.join(" "));
	ui->progressBar->setEnabled(true);
}

void Smrtk2ssrfcWindow::on_outputFileButton_clicked()
{
	outputFile = QFileDialog::getSaveFileName(this, tr("Open Subsurface files"), lastUsedDir(),
		tr("Subsurface files (*.ssrf *SSRF *.xml *.XML);;"
		   "All files (*)"));
	if (outputFile.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(outputFile).dir().path());
	ui->outputLine->setText(outputFile);
}

void Smrtk2ssrfcWindow::on_importButton_clicked()
{
	if (inputFiles.isEmpty())
		return;

	QByteArray fileNamePtr;

	ui->plainTextEdit->setDisabled(false);
	ui->progressBar->setRange(0, inputFiles.size());
	for (int i = 0; i < inputFiles.size(); ++i) {
		ui->progressBar->setValue(i);
		fileNamePtr = QFile::encodeName(inputFiles.at(i));
		smartrak_import(fileNamePtr.data(), &dive_table);
		ui->plainTextEdit->appendPlainText(QString(get_error_string()));
	}
	ui->progressBar->setValue(inputFiles.size());
	save_dives_logic(outputFile.toUtf8().data(), false);
	ui->progressBar->setDisabled(true);
}

void Smrtk2ssrfcWindow::on_exitButton_clicked()
{
	this->close();
}

void Smrtk2ssrfcWindow::on_outputLine_textEdited()
{
	outputFile = ui->outputLine->text();
}

void Smrtk2ssrfcWindow::on_inputLine_textEdited()
{
	inputFiles = ui->inputLine->text().split(" ");
}
