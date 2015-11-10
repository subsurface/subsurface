#ifndef ABSTRACTPREFERENCESWIDGET_H
#define ABSTRACTPREFERENCESWIDGET_H

#include <QIcon>
#include <QWidget>

class AbstractPreferencesWidget : public QWidget {
	Q_OBJECT
public:
	AbstractPreferencesWidget(const QString& name, const QIcon& icon, float positionHeight);
	QIcon icon() const;
	QString name() const;
	float positionHeight() const;

	/* gets the values from the preferences and should set the correct values in
	 * the interface */
	virtual void refreshSettings() = 0;

	/* gets the values from the interface and set in the preferences object. */
	virtual void syncSettings() = 0;

signals:
	void settingsChanged();

private:
	QIcon _icon;
	QString _name;
	float _positionHeight;
};
#endif
