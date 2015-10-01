#include "preferences.h"
#include "mainwindow.h"
#include "models.h"
#include "divelocationmodel.h"
#include "prefs-macros.h"
#include "qthelper.h"
#include "subsurfacestartup.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QNetworkProxy>
#include <QNetworkCookieJar>

#include "subsurfacewebservices.h"

#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
#include "socialnetworks.h"
#include <QWebView>
#endif

PreferencesDialog *PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog(MainWindow::instance());
	return dialog;
}

PreferencesDialog::PreferencesDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);

#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	FacebookManager *fb = FacebookManager::instance();
	facebookWebView = new QWebView(this);
	ui.fbWebviewContainer->layout()->addWidget(facebookWebView);
	if (fb->loggedIn()) {
		facebookLoggedIn();
	} else {
		facebookDisconnect();
	}
	connect(facebookWebView, &QWebView::urlChanged, fb, &FacebookManager::tryLogin);
	connect(fb, &FacebookManager::justLoggedIn, this, &PreferencesDialog::facebookLoggedIn);
	connect(ui.fbDisconnect, &QPushButton::clicked, fb, &FacebookManager::logout);
	connect(fb, &FacebookManager::justLoggedOut, this, &PreferencesDialog::facebookDisconnect);
#endif

	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));

	//	connect(ui.defaultSetpoint, SIGNAL(valueChanged(double)), this, SLOT(defaultSetpointChanged(double)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
	loadSettings();
	setUiFromPrefs();
	rememberPrefs();
}

void PreferencesDialog::facebookLoggedIn()
{
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	ui.fbDisconnect->show();
	ui.fbWebviewContainer->hide();
	ui.fbWebviewContainer->setEnabled(false);
	ui.FBLabel->setText(tr("To disconnect Subsurface from your Facebook account, use the button below"));
#endif
}

void PreferencesDialog::facebookDisconnect()
{
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	// remove the connect/disconnect button
	// and instead add the login view
	ui.fbDisconnect->hide();
	ui.fbWebviewContainer->show();
	ui.fbWebviewContainer->setEnabled(true);
	ui.FBLabel->setText(tr("To connect to Facebook, please log in. This enables Subsurface to publish dives to your timeline"));
	if (facebookWebView) {
		facebookWebView->page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
		facebookWebView->setUrl(FacebookManager::instance()->connectUrl());
	}
#endif
}

void PreferencesDialog::showEvent(QShowEvent *event)
{
	setUiFromPrefs();
	rememberPrefs();
	QDialog::showEvent(event);
}

void PreferencesDialog::setUiFromPrefs()
{

}

void PreferencesDialog::restorePrefs()
{
	prefs = oldPrefs;
	setUiFromPrefs();
}

void PreferencesDialog::rememberPrefs()
{
	oldPrefs = prefs;
}

void PreferencesDialog::syncSettings()
{
}

void PreferencesDialog::loadSettings()
{
	// This code was on the mainwindow, it should belong nowhere, but since we didn't
	// correctly fixed this code yet ( too much stuff on the code calling preferences )
	// force this here.
	loadPreferences();
	QSettings s;
	QVariant v;
}

void PreferencesDialog::buttonClicked(QAbstractButton *button)
{
	switch (ui.buttonBox->standardButton(button)) {
	case QDialogButtonBox::Discard:
		restorePrefs();
		syncSettings();
		close();
		break;
	case QDialogButtonBox::Apply:
		syncSettings();
		break;
	case QDialogButtonBox::FirstButton:
		syncSettings();
		close();
		break;
	default:
		break; // ignore warnings.
	}
}
#undef SB

void PreferencesDialog::on_resetSettings_clicked()
{
	QSettings s;
	QMessageBox response(this);
	response.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	response.setDefaultButton(QMessageBox::Cancel);
	response.setWindowTitle(tr("Warning"));
	response.setText(tr("If you click OK, all settings of Subsurface will be reset to their default values. This will be applied immediately."));
	response.setWindowModality(Qt::WindowModal);

	int result = response.exec();
	if (result == QMessageBox::Ok) {
		copy_prefs(&default_prefs, &prefs);
		setUiFromPrefs();
		Q_FOREACH (QString key, s.allKeys()) {
			s.remove(key);
		}
		syncSettings();
		close();
	}
}

void PreferencesDialog::emitSettingsChanged()
{
	emit settingsChanged();
}
