#include <QShortcut>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>

#include "usersurvey.h"
#include "ui_usersurvey.h"
#include "ssrf-version.h"

#include "helpers.h"
#include "subsurfacesysinfo.h"
UserSurvey::UserSurvey(QWidget *parent) : QDialog(parent),
	ui(new Ui::UserSurvey)
{
	ui->setupUi(this);
	QShortcut *closeKey = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(closeKey, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quitKey = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quitKey, SIGNAL(activated()), parent, SLOT(close()));

	// fill in the system data
	ui->system->append(tr("Subsurface %1").arg(VERSION_STRING));
	ui->system->append(tr("Operating System: %1").arg(SubsurfaceSysInfo::prettyOsName()));
	ui->system->append(tr("CPU Architecture: %1").arg(SubsurfaceSysInfo::cpuArchitecture()));
	ui->system->append(tr("Language: %1").arg(uiLanguage(NULL)));
}

UserSurvey::~UserSurvey()
{
	delete ui;
}

void UserSurvey::on_buttonBox_accepted()
{
	// now we need to collect the data and submit it
	QSettings s;
	s.beginGroup("UserSurvey");
	s.setValue("SurveyDone", "submitted");
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
