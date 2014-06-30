#include <QShortcut>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>

#include "usersurvey.h"
#include "ui_usersurvey.h"
#include "ssrf-version.h"
#include "subsurfacewebservices.h"

#include "helpers.h"
#include "subsurfacesysinfo.h"

UserSurvey::UserSurvey(QWidget *parent) : QDialog(parent),
	ui(new Ui::UserSurvey)
{
	QString osArch, arch;
	ui->setupUi(this);
	QShortcut *closeKey = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(closeKey, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quitKey = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quitKey, SIGNAL(activated()), parent, SLOT(close()));

	// fill in the system data
	QString sysInfo = QString("Subsurface %1").arg(VERSION_STRING);
	os = QString("ssrfVers=%1").arg(VERSION_STRING);
	sysInfo.append(tr("\nOperating System: %1").arg(SubsurfaceSysInfo::prettyOsName()));
	os.append(QString("&prettyOsName=%1").arg(SubsurfaceSysInfo::prettyOsName()));
	arch = SubsurfaceSysInfo::cpuArchitecture();
	sysInfo.append(tr("\nCPU Architecture: %1").arg(arch));
	os.append(QString("&appCpuArch=%1").arg(arch));
	if (arch == "i386") {
		osArch = SubsurfaceSysInfo::osArch();
		if (!osArch.isEmpty()) {
			sysInfo.append(tr("\nOS CPU Architecture %1").arg(osArch));
			os.append(QString("&osCpuArch=%1").arg(osArch));
		}
	}
	sysInfo.append(tr("\nLanguage: %1").arg(uiLanguage(NULL)));
	os.append(QString("&uiLang=%1").arg(uiLanguage(NULL)));
	ui->system->setPlainText(sysInfo);
	manager = SubsurfaceWebServices::manager();
	connect(manager, SIGNAL(finished(QNetworkReply *)), SLOT(requestReceived(QNetworkReply *)));
}

UserSurvey::~UserSurvey()
{
	delete ui;
}

#define ADD_OPTION(_name) values.append(ui->_name->isChecked() ? "&" #_name "=1" : "&" #_name "=0")

void UserSurvey::on_buttonBox_accepted()
{
	// now we need to collect the data and submit it
	QString values = os;
	ADD_OPTION(recreational);
	ADD_OPTION(tech);
	ADD_OPTION(planning);
	ADD_OPTION(download);
	ADD_OPTION(divecomputer);
	ADD_OPTION(manual);
	ADD_OPTION(companion);
	values.append(QString("&suggestion=%1").arg(ui->suggestions->toPlainText()));
	UserSurveyServices uss(this);
	uss.sendSurvey(values);
	hide();
}

void UserSurvey::on_buttonBox_rejected()
{
	QMessageBox response(this);
	response.setText(tr("Should we ask you later?"));
	response.addButton(tr("Don't ask me again"), QMessageBox::RejectRole);
	response.addButton(tr("Ask Later"), QMessageBox::AcceptRole);
	response.setWindowTitle(tr("Ask again?")); // Not displayed on MacOSX as described in Qt API
	response.setIcon(QMessageBox::Question);
	response.setWindowModality(Qt::WindowModal);
	switch (response.exec()) {
	case QDialog::Accepted:
		// nothing to do here, we'll just ask again the next time they start
		break;
	case QDialog::Rejected:
		QSettings s;
		s.beginGroup("UserSurvey");
		s.setValue("SurveyDone", "declined");
		break;
	}
	hide();
}

void UserSurvey::requestReceived(QNetworkReply *reply)
{
	QMessageBox msgbox;
	QString msgTitle = tr("Submit User Survey.");
	QString msgText = tr("<h3>Subsurface was unable to submit the user survey.</h3>");


	if (reply->error() != QNetworkReply::NoError) {
		//Network Error
		msgText = msgText + tr("<br/><b>The following error occurred:</b><br/>") + reply->errorString()
				+ tr("<br/><br/><b>Please check your internet connection.</b>");
	} else {
		//No network error
		QString response(reply->readAll());
		QString responseBody = response.split("\"").at(1);

		msgbox.setIcon(QMessageBox::Information);

		if (responseBody == "OK") {
			msgText = tr("Survey successfully submitted.");
			QSettings s;
			s.beginGroup("UserSurvey");
			s.setValue("SurveyDone", "submitted");
		} else {
			msgText = tr("There was an error while trying to check for updates.<br/><br/>%1").arg(responseBody);
			msgbox.setIcon(QMessageBox::Warning);
		}
	}

	msgbox.setWindowTitle(msgTitle);
	msgbox.setText(msgText);
	msgbox.setTextFormat(Qt::RichText);
	msgbox.exec();
	reply->deleteLater();
}
