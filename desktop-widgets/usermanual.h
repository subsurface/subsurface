// SPDX-License-Identifier: GPL-2.0
#ifndef USERMANUAL_H
#define USERMANUAL_H

#ifdef USE_WEBENGINE
#include <QWebEngineView>
#include <QWebEnginePage>
#else
#include <QWebView>
#endif
#include "ui_searchbar.h"

class SearchBar : public QWidget{
	Q_OBJECT
public:
	SearchBar(QWidget *parent = 0);
signals:
	void searchTextChanged(const QString& s);
	void searchNext();
	void searchPrev();
protected:
	void setVisible(bool visible);
private slots:
	void enableButtons(const QString& s);
private:
	Ui::SearchBar ui;
};

#ifdef USE_WEBENGINE
class MyQWebEnginePage : public QWebEnginePage
{
	Q_OBJECT

public:
	MyQWebEnginePage(QObject* parent = 0);
	bool acceptNavigationRequest(const QUrl & url, QWebEnginePage::NavigationType type, bool);
};

class MyQWebEngineView : public QWebEngineView
{
	Q_OBJECT

public:
	MyQWebEngineView(QWidget* parent = 0);
	MyQWebEnginePage* page() const;
};
#endif


class UserManual : public QWidget {
	Q_OBJECT

public:
	explicit UserManual(QWidget *parent = 0);

#ifdef Q_OS_MAC
protected:
	void showEvent(QShowEvent *e);
	void hideEvent(QHideEvent *e);
	QAction *closeAction;
	QAction *filterAction;
#endif

private
slots:
	void searchTextChanged(const QString& s);
	void searchNext();
	void searchPrev();
#ifndef USE_WEBENGINE
	void linkClickedSlot(const QUrl& url);
#endif
private:
	SearchBar *searchBar;
	QString mLastText;
#ifdef USE_WEBENGINE
	QWebEngineView *userManual;
	void search(QString, QWebEnginePage::FindFlags);
#else
	QWebView *userManual;
	void search(QString, QWebPage::FindFlags);
#endif
};
#endif // USERMANUAL_H
