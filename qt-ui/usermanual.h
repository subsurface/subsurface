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

class UserManual : public QWebView {
	Q_OBJECT

public:
	explicit UserManual(QWidget *parent = 0);
	~UserManual();
private
slots:
	void searchTextChanged(const QString& s);
	void searchNext();
	void searchPrev();
	void linkClickedSlot(QUrl url);

private:
	SearchBar *searchBar;
	QString mLastText;
	void search(QString, QWebPage::FindFlags);
};
#endif // USERMANUAL_H
