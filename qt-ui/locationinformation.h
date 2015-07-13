#ifndef LOCATIONINFORMATION_H
#define LOCATIONINFORMATION_H

#include "ui_locationInformation.h"
#include <stdint.h>
#include <QAbstractListModel>

class LocationInformationWidget : public QGroupBox {
Q_OBJECT
public:
	enum mode{CREATE_DIVE_SITE, EDIT_DIVE_SITE};
	LocationInformationWidget(QWidget *parent = 0);
protected:
	void showEvent(QShowEvent *);

public slots:
	void acceptChanges();
	void rejectChanges();
	void updateGpsCoordinates();
	void editDiveSite(uint32_t uuid);
	void createDiveSite();
	void markChangedWidget(QWidget *w);
	void enableEdition();
	void resetState();
	void resetPallete();
	void on_diveSiteCoordinates_textChanged(const QString& text);
	void on_diveSiteDescription_textChanged(const QString& text);
	void on_diveSiteName_textChanged(const QString& text);
	void on_diveSiteNotes_textChanged();
private slots:
	void setCurrentDiveSiteByUuid(uint32_t uuid);
signals:
	void startEditDiveSite(uint32_t uuid);
	void endEditDiveSite();
	void informationManagementEnded();
	void coordinatesChanged();
	void startFilterDiveSite(uint32_t uuid);
	void stopFilterDiveSite();
private:
	struct dive_site *currentDs;
	Ui::LocationInformation ui;
	bool modified;
	QAction *closeAction, *acceptAction, *rejectAction;
	mode current_mode;
};


#include "ui_simpledivesiteedit.h"
class SimpleDiveSiteEditDialog : public QDialog {
Q_OBJECT
public:
	SimpleDiveSiteEditDialog(QWidget *parent);
	virtual ~SimpleDiveSiteEditDialog();
	bool changed_dive_site;
	bool eventFilter(QObject *obj, QEvent *ev);
public slots:
	void on_diveSiteName_editingFinished();
	void on_diveSiteCoordinates_editingFinished();
	void diveSiteDescription_editingFinished();
	void diveSiteNotes_editingFinished();
protected:
	void showEvent(QShowEvent *ev);
private:
	Ui::SimpleDiveSiteEditDialog *ui;

};

class LocationManagementEditHelper : public QObject {
Q_OBJECT
public:
	bool eventFilter(QObject *obj, QEvent *ev);
	void handleActivation(const QModelIndex& activated);
	void resetDiveSiteUuid();
	uint32_t diveSiteUuid() const;
private:
	uint32_t last_uuid;

};
#endif
