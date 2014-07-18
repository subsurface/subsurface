#include <QDesktopServices>
#include <QShortcut>
#include <QFile>
#include <QDebug>

#include "usermanual.h"

#include "helpers.h"

SearchBar::SearchBar(QWidget *parent): QWidget(parent)
{
	ui.setupUi(this);
	#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
	ui.findNext->setIcon(QIcon(":icons/subsurface/32x32/actions/go-down.png"));
	ui.findPrev->setIcon(QIcon(":icons/subsurface/32x32/actions/go-up.png"));
	ui.findClose->setIcon(QIcon(":icons/subsurface/32x32/actions/window-close.png"));
	#endif

	connect(ui.findNext, SIGNAL(pressed()), this, SIGNAL(searchNext()));
	connect(ui.findPrev, SIGNAL(pressed()), this, SIGNAL(searchPrev()));
	connect(ui.searchEdit, SIGNAL(textChanged(QString)), this, SIGNAL(searchTextChanged(QString)));
	connect(ui.searchEdit, SIGNAL(textChanged(QString)), this, SLOT(enableButtons(QString)));
	connect(ui.findClose, SIGNAL(pressed()), this, SLOT(hide()));
}

void SearchBar::setVisible(bool visible)
{
	QWidget::setVisible(visible);
	ui.searchEdit->setFocus();
}

void SearchBar::enableButtons(const QString &s)
{
	ui.findPrev->setEnabled(s.length());
	ui.findNext->setEnabled(s.length());
}

UserManual::UserManual(QWidget *parent) : QWidget(parent)
{
	QShortcut *closeKey = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(closeKey, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quitKey = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quitKey, SIGNAL(activated()), qApp, SLOT(quit()));

	QAction *actionShowSearch = new QAction(this);
	actionShowSearch->setShortcut(Qt::CTRL + Qt::Key_F);
	actionShowSearch->setShortcutContext(Qt::WindowShortcut);
	addAction(actionShowSearch);

	QAction *actionHideSearch = new QAction(this);
	actionHideSearch->setShortcut(Qt::Key_Escape);
	actionHideSearch->setShortcutContext(Qt::WindowShortcut);
	addAction(actionHideSearch);

	setWindowTitle(tr("User Manual"));
	setWindowIcon(QIcon(":/subsurface-icon"));


	userManual = new QWebView(this);
	userManual->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
	QString searchPath = getSubsurfaceDataPath("Documentation");
	if (searchPath.size()) {
		// look for localized versions of the manual first
		QString lang = uiLanguage(NULL);
		QString prefix = searchPath.append("/user-manual");
		QFile manual(prefix + "_" + lang + ".html");
		if (!manual.exists())
			manual.setFileName(prefix + "_" + lang.left(2) + ".html");
		if (!manual.exists())
			manual.setFileName(prefix + ".html");
		if (!manual.exists()) {
			userManual->setHtml(tr("Cannot find the Subsurface manual"));
		} else {
			QString urlString = QString("file:///") + manual.fileName();
			userManual->setUrl(QUrl(urlString, QUrl::TolerantMode));
		}
	} else {
		userManual->setHtml(tr("Cannot find the Subsurface manual"));
	}

	searchBar = new SearchBar(this);
	searchBar->hide();
	connect(actionShowSearch, SIGNAL(triggered(bool)), searchBar, SLOT(show()));
	connect(actionHideSearch, SIGNAL(triggered(bool)), searchBar, SLOT(hide()));
	connect(userManual, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClickedSlot(QUrl)));
	connect(searchBar, SIGNAL(searchTextChanged(QString)), this, SLOT(searchTextChanged(QString)));
	connect(searchBar, SIGNAL(searchNext()), this, SLOT(searchNext()));
	connect(searchBar, SIGNAL(searchPrev()), this, SLOT(searchPrev()));

	QVBoxLayout *vboxLayout = new QVBoxLayout();
	userManual->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
	vboxLayout->addWidget(userManual);
	vboxLayout->addWidget(searchBar);
	setLayout(vboxLayout);
}

void UserManual::search(QString text, QWebPage::FindFlags flags = 0)
{
	if (userManual->findText(text, QWebPage::FindWrapsAroundDocument | flags) || text.length() == 0) {
		searchBar->setStyleSheet("");
	} else {
		searchBar->setStyleSheet("QLineEdit{background: red;}");
	}
}

void UserManual::searchTextChanged(const QString& text)
{
	mLastText = text;
	search(text);
}

void UserManual::searchNext()
{
	search(mLastText);
}

void UserManual::searchPrev()
{
	search(mLastText, QWebPage::FindBackward);
}

void UserManual::linkClickedSlot(const QUrl& url)
{
	QDesktopServices::openUrl(url);
}
