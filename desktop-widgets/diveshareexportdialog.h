#ifndef DIVESHAREEXPORTDIALOG_H
#define DIVESHAREEXPORTDIALOG_H

#include <QDialog>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#define DIVESHARE_WEBSITE "dive-share.appspot.com"
#define DIVESHARE_BASE_URI "http://" DIVESHARE_WEBSITE

namespace Ui {
class DiveShareExportDialog;
}

class DiveShareExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DiveShareExportDialog(QWidget *parent = 0);
    ~DiveShareExportDialog();
    static DiveShareExportDialog *instance();
    void prepareDivesForUpload(bool);
private:
    Ui::DiveShareExportDialog *ui;
    bool exportSelected;
    QNetworkReply *reply;
private
slots:
    void UIDFromBrowser();
    void doUpload();
    void finishedSlot();
};

#endif // DIVESHAREEXPORTDIALOG_H
