// SPDX-License-Identifier: GPL-2.0
#include "smrtk2ssrfc_window.h"
#include "ui_smrtk2ssrfc_window.h"
#include "qt-models/filtermodels.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divelist.h"
#include "core/errorhelper.h"
#include "core/settings/qPrefDisplay.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>

QStringList inputFiles;
QString outputFile;
QString error_buf;

void getErrorFromC(std::string buf)
{
	error_buf = QString::fromStdString(std::move(buf));
}

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

static QString lastUsedDir()
{
	QString lastDir = QDir::homePath();

	if (QDir(qPrefDisplay::lastDir()).exists())
		lastDir = qPrefDisplay::lastDir();
	return lastDir;
}

void Smrtk2ssrfcWindow::updateLastUsedDir(const QString &dir)
{
	qPrefDisplay::set_lastDir(dir);
}

void Smrtk2ssrfcWindow::on_inputFilesButton_clicked()
{
	inputFiles = QFileDialog::getOpenFileNames(this, tr("Open SmartTrak files"), lastUsedDir(),
		tr("SmartTrak files") + " (*.slg);;" + tr("All files") + " (*.*)");
	if (inputFiles.isEmpty())
		return;
	updateLastUsedDir(QFileInfo(inputFiles[0]).dir().path());
	ui->inputLine->setText(inputFiles.join(" "));
	ui->progressBar->setEnabled(true);
}

void Smrtk2ssrfcWindow::on_outputFileButton_clicked()
{
	outputFile = QFileDialog::getSaveFileName(this, tr("Open Subsurface files"), lastUsedDir(),
		tr("Subsurface files") + " (*.ssrf *.xml);;" + tr("All files") + " (*.*)");
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
	set_error_cb(&getErrorFromC);
	for (int i = 0; i < inputFiles.size(); ++i) {
		ui->progressBar->setValue(i);
		fileNamePtr = QFile::encodeName(inputFiles.at(i));
		smartrak_import(fileNamePtr.data(), &divelog);
		ui->plainTextEdit->appendPlainText(error_buf);
	}
	ui->progressBar->setValue(inputFiles.size());
	save_dives_logic(qPrintable(outputFile), false, false);
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
