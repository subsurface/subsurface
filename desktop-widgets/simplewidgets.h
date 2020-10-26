// SPDX-License-Identifier: GPL-2.0
#ifndef SIMPLEWIDGETS_H
#define SIMPLEWIDGETS_H

class MinMaxAvgWidgetPrivate;
class QAbstractButton;
class QNetworkReply;
class FilterModelBase;
struct dive;
struct dive_components;

#include "core/units.h"
#include <QWidget>
#include <QGroupBox>
#include <QDialog>
#include <QTextEdit>

#include "ui_renumber.h"
#include "ui_setpoint.h"
#include "ui_shifttimes.h"
#include "ui_shiftimagetimes.h"
#include "ui_urldialog.h"
#include "ui_divecomponentselection.h"
#include "ui_listfilter.h"
#include "ui_addfilterpreset.h"

class RenumberDialog : public QDialog {
	Q_OBJECT
public:
	explicit RenumberDialog(bool selectedOnly, QWidget *parent);
private
slots:
	void buttonClicked(QAbstractButton *button);

private:
	Ui::RenumberDialog ui;
	bool selectedOnly;
};

class SetpointDialog : public QDialog {
	Q_OBJECT
public:
	SetpointDialog(struct dive *d, int dcNr, int time);
private
slots:
	void buttonClicked(QAbstractButton *button);

private:
	Ui::SetpointDialog ui;
	struct dive *d;
	int dcNr;
	int time;
};

class ShiftTimesDialog : public QDialog {
	Q_OBJECT
public:
	explicit ShiftTimesDialog(QWidget *parent);
private
slots:
	void buttonClicked(QAbstractButton *button);
	void changeTime();

private:
	int64_t when;
	Ui::ShiftTimesDialog ui;
};

class ShiftImageTimesDialog : public QDialog {
	Q_OBJECT
public:
	explicit ShiftImageTimesDialog(QWidget *parent, QStringList fileNames);
	time_t amount() const;
	void setOffset(time_t offset);
	bool matchAll();
private
slots:
	void buttonClicked(QAbstractButton *button);
	void syncCameraClicked();
	void dcDateTimeChanged(const QDateTime &);
	void timeEditChanged(const QTime &time);
	void timeEditChanged();
	void updateInvalid();
	void matchAllImagesToggled(bool);

private:
	QStringList fileNames;
	QVector<timestamp_t> timestamps;
	Ui::ShiftImageTimesDialog ui;
	time_t m_amount;
	time_t dcImageEpoch;
	bool matchAllImages;
};

class URLDialog : public QDialog {
	Q_OBJECT
public:
	explicit URLDialog(QWidget *parent);
	QString url() const;
private:
	Ui::URLDialog ui;
};

class DiveComponentSelection : public QDialog {
	Q_OBJECT
public:
	explicit DiveComponentSelection(QWidget *parent, struct dive *target, struct dive_components *_what);
private
slots:
	void buttonClicked(QAbstractButton *button);

private:
	Ui::DiveComponentSelectionDialog ui;
	struct dive *targetDive;
	struct dive_components *what;
};

class AddFilterPresetDialog : public QDialog {
	Q_OBJECT
public:
	explicit AddFilterPresetDialog(const QString &defaultName, QWidget *parent);
	QString doit(); // returns name of filter preset or empty string if user cancelled the dialog
private
slots:
	void nameChanged(const QString &text);
private:
	Ui::AddFilterPresetDialog ui;
};

class TextHyperlinkEventFilter : public QObject {
	Q_OBJECT
public:
	explicit TextHyperlinkEventFilter(QTextEdit *txtEdit);

	bool eventFilter(QObject *target, QEvent *evt) override;

private:
	void handleUrlClick(const QString &urlStr);
	void handleUrlTooltip(const QString &urlStr, const QPoint &pos);
	bool stringMeetsOurUrlRequirements(const QString &maybeUrlStr);
	QString fromCursorTilWhitespace(QTextCursor *cursor, bool searchBackwards);
	QString tryToFormulateUrl(QTextCursor *cursor);

	QTextEdit const *const textEdit;
	QWidget const *const scrollView;

	Q_DISABLE_COPY(TextHyperlinkEventFilter)
};

bool isGnome3Session();

#endif // SIMPLEWIDGETS_H
