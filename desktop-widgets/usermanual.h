// SPDX-License-Identifier: GPL-2.0
#ifndef USERMANUAL_H
#define USERMANUAL_H

#include <QWebEngineView>
#include <QWebEnginePage>
#include <QDialog>
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


class UserManual : public QDialog {
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
private:
	SearchBar *searchBar;
	QString mLastText;
	QWebEngineView *userManual;
	void search(QString, QWebEnginePage::FindFlags);
};
#endif // USERMANUAL_H
