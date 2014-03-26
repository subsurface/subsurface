#ifndef USERMANUAL_H
#define USERMANUAL_H

#include <QMainWindow>
#include <QWebPage>

namespace Ui
{
	class UserManual;
}

class UserManual : public QMainWindow {
	Q_OBJECT

public:
	explicit UserManual(QWidget *parent = 0);
	~UserManual();

private
slots:
	void showSearchPanel();
	void hideSearchPanel();
	void searchTextChanged(QString);
	void searchNext();
	void searchPrev();
	void linkClickedSlot(QUrl url);

private:
	Ui::UserManual *ui;
	void search(QString, QWebPage::FindFlags);
};
#endif // USERMANUAL_H
