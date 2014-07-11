#ifndef USERMANUAL_H
#define USERMANUAL_H

#include <QWebView>

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

class UserManual : public QWidget {
	Q_OBJECT

public:
	explicit UserManual(QWidget *parent = 0);
private
slots:
	void searchTextChanged(const QString& s);
	void searchNext();
	void searchPrev();
	void linkClickedSlot(const QUrl& url);
private:
	QWebView *userManual;
	SearchBar *searchBar;
	QString mLastText;
	void search(QString, QWebPage::FindFlags);
};
#endif // USERMANUAL_H
