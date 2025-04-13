// SPDX-License-Identifier: GPL-2.0
#include <QDesktopServices>
#include <QShortcut>
#include <QFile>
#ifdef USE_WEBENGINE
# include <QWebEngineFindTextResult>
#endif

#include "desktop-widgets/usermanual.h"
#include "desktop-widgets/mainwindow.h"
#include "core/qthelper.h"

SearchBar::SearchBar(QWidget *parent): QWidget(parent)
{
	ui.setupUi(this);
	#if defined(Q_OS_MAC) || defined(Q_OS_WIN) || defined(Q_OS_UNIX)
	ui.findNext->setIcon(QIcon(":go-down-icon"));
	ui.findPrev->setIcon(QIcon(":go-up-icon"));
	ui.findClose->setIcon(QIcon(":window-close-icon"));
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

UserManual::UserManual(QWidget *parent) : QDialog(parent)
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

	setWindowTitle(tr("User manual"));
	setWindowIcon(QIcon(":subsurface-icon"));

#ifdef USE_WEBENGINE
	userManual = new QWebEngineView(this);
#else
	userManual = new QWebView(this);
#endif
	QString colorBack = palette().highlight().color().name(QColor::HexRgb);
	QString colorText = palette().highlightedText().color().name(QColor::HexRgb);
	userManual->setStyleSheet(QString(
#ifdef USE_WEBENGINE
				"QWebEngineView"
#else
				"QWebView"
#endif
				" { selection-background-color: %1; selection-color: %2; }")
		.arg(colorBack).arg(colorText));
#ifndef USE_WEBENGINE
	userManual->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
#endif
	QString searchPath = getSubsurfaceDataPath("Documentation");
	if (searchPath.size()) {
		// look for localized versions of the manual first
		QString lang = getUiLanguage();
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
#ifdef USE_WEBENGINE
			UserManualPage* page = new UserManualPage(userManual);
			page->setUrl(urlString);
			userManual->setPage(page);
#else
			userManual->setUrl(QUrl(urlString, QUrl::TolerantMode));
#endif
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

#ifdef USE_WEBENGINE
	resize(700, 500);
#endif
}

#ifdef USE_WEBENGINE
void UserManual::search(QString text, bool backward)
{
	QWebEnginePage::FindFlags flags = QFlag(0);
	if (backward)
		flags |= QWebEnginePage::FindBackward;
	if (text.length() == 0)
		searchBar->setStyleSheet("");
	else
		userManual->findText(text, flags,
			[this](const QWebEngineFindTextResult &result) {
				if (result.numberOfMatches() == 0)
					searchBar->setStyleSheet("QLineEdit{background: red;}");
				else
					searchBar->setStyleSheet("");
			});
}
#else
void UserManual::search(QString text, bool backward)
{
	QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;
	if (backward)
		flags |= QWebPage::FindBackward;
	if (userManual->findText(text, flags) || text.length() == 0) {
		searchBar->setStyleSheet("");
	} else {
		searchBar->setStyleSheet("QLineEdit{background: red;}");
	}
}
#endif

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
	search(mLastText, true);
}

void UserManual::linkClickedSlot(const QUrl& url)
{
	QDesktopServices::openUrl(url);
}

#ifdef Q_OS_MAC
void UserManual::showEvent(QShowEvent *e)
{
	MainWindow *m = MainWindow::instance();
	filterAction = m->findChild<QAction *>(QLatin1String("actionFilterTags"), Qt::FindDirectChildrenOnly);
	if (filterAction != nullptr)
		filterAction->setShortcut(QKeySequence());
	closeAction = m->findChild<QAction *>(QLatin1String("actionClose"), Qt::FindDirectChildrenOnly);
	if (closeAction != nullptr)
		closeAction->setShortcut(QKeySequence());
}

void UserManual::hideEvent(QHideEvent *e)
{
	if (closeAction != NULL)
		closeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
	if (filterAction != NULL)
		filterAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
	closeAction = filterAction = NULL;
}
#endif

#ifdef USE_WEBENGINE
bool UserManualPage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
	if (type == QWebEnginePage::NavigationTypeLinkClicked
			&& (url.scheme() == "http" || url.scheme() == "https")) {
		QDesktopServices::openUrl(url);
		return false;
	}
	return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}
#endif
