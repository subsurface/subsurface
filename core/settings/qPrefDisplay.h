// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFDISPLAY_H
#define QPREFDISPLAY_H


#include <QObject>


class qPrefDisplay : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString	divelist_font
				READ	divelistFont
				WRITE	setDivelistFont
				NOTIFY  diveListFontChanged);
	Q_PROPERTY(double	font_size
				READ	fontSize
				WRITE	setFontSize
				NOTIFY  fontSizeChanged);
	Q_PROPERTY(bool		invalid_dives
				READ	displayInvalidDives
				WRITE	setDisplayInvalidDives
				NOTIFY  displayInvalidDivesChanged);
	Q_PROPERTY(bool		show_developer
				READ	showDeveloper
				WRITE	setShowDeveloper
				NOTIFY  showDeveloperChanged);
	Q_PROPERTY(QString	theme
				READ	theme
				WRITE	setTheme
				NOTIFY  themeChanged);

public:
	qPrefDisplay(QObject *parent = NULL) : QObject(parent) {};
	~qPrefDisplay() {};
	static qPrefDisplay *instance();


	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);

public:
	const QString divelistFont() const;
	double fontSize() const;
	bool displayInvalidDives() const;
	bool showDeveloper() const;
	const QString theme() const;

public slots:
	void setDivelistFont(const QString& value);
	void setFontSize(double value);
	void setDisplayInvalidDives(bool value);
	void setShowDeveloper(bool value);
	void setTheme(const QString& value);

signals:
	void diveListFontChanged(const QString& value);
	void fontSizeChanged(double value);
	void displayInvalidDivesChanged(bool value);
	void showDeveloperChanged(bool value);
	void themeChanged(const QString& value);

private:
	const QString group = QStringLiteral("Display");
	static qPrefDisplay *m_instance;

	// functions to load/sync variable with disk
	void diskDivelistFont(bool doSync);
	void diskFontSize(bool doSync);
	void diskDisplayInvalidDives(bool doSync);
	void diskShowDeveloper(bool doSync);
	void diskTheme(bool doSync);
};
#endif
