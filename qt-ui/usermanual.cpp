#include <QDesktopServices>

#include "usermanual.h"
#include "ui_usermanual.h"

#include "../helpers.h"

UserManual::UserManual(QWidget *parent) : QMainWindow(parent),
	ui(new Ui::UserManual)
{
	ui->setupUi(this);

	QAction *actionShowSearch = new QAction(this);
	actionShowSearch->setShortcut(Qt::CTRL + Qt::Key_F);
	actionShowSearch->setShortcutContext(Qt::WindowShortcut);
	addAction(actionShowSearch);

	QAction *actionHideSearch = new QAction(this);
	actionHideSearch->setShortcut(Qt::Key_Escape);
	actionHideSearch->setShortcutContext(Qt::WindowShortcut);
	addAction(actionHideSearch);

	setWindowTitle(tr("User Manual"));

	ui->webView->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
	QString searchPath = getSubsurfaceDataPath("Documentation");
	if (searchPath != "") {
		QUrl url(searchPath.append("/user-manual.html"));
		ui->webView->setUrl(url);
	} else {
		ui->webView->setHtml(tr("Cannot find the Subsurface manual"));
	}
	ui->searchPanel->setParent(this);
	ui->searchPanel->hide();

	connect(actionShowSearch, SIGNAL(triggered(bool)), this, SLOT(showSearchPanel()));
	connect(actionHideSearch, SIGNAL(triggered(bool)), this, SLOT(hideSearchPanel()));
	connect(ui->webView, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClickedSlot(QUrl)));
	connect(ui->searchEdit, SIGNAL(textChanged(QString)), this, SLOT(searchTextChanged(QString)));
	connect(ui->findNext, SIGNAL(clicked()), this, SLOT(searchNext()));
	connect(ui->findPrev, SIGNAL(clicked()), this, SLOT(searchPrev()));
}

void UserManual::showSearchPanel()
{
	ui->searchPanel->show();
	ui->searchEdit->setFocus();
	ui->searchEdit->selectAll();
}

void UserManual::hideSearchPanel()
{
	ui->searchPanel->hide();
}

void UserManual::search(QString text, QWebPage::FindFlags flags = 0)
{
	if (ui->webView->findText(text, QWebPage::FindWrapsAroundDocument | flags) || text.length() == 0) {
		ui->searchEdit->setStyleSheet("");
	} else {
		ui->searchEdit->setStyleSheet("QLineEdit{background: red;}");
	}
}

void UserManual::searchTextChanged(QString text)
{
	bool hasText = text.length() > 0;

	ui->findPrev->setEnabled(hasText);
	ui->findNext->setEnabled(hasText);

	search(text);
}

void UserManual::searchNext()
{
	search(ui->searchEdit->text());
}

void UserManual::searchPrev()
{
	search(ui->searchEdit->text(), QWebPage::FindBackward);
}

void UserManual::linkClickedSlot(QUrl url)
{
	QDesktopServices::openUrl(url);
}

UserManual::~UserManual()
{
	delete ui;
}
