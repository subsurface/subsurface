#ifndef MODELDELEGATES_H
#define MODELDELEGATES_H

#include <QStyledItemDelegate>
#include <QComboBox>
class QPainter;

class DiveListDelegate : public QStyledItemDelegate {
public:
	explicit DiveListDelegate(QObject *parent = 0)
	    : QStyledItemDelegate(parent)
	{
	}
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class StarWidgetsDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit StarWidgetsDelegate(QWidget *parent = 0);
	virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	const QSize& starSize() const;

private:
	QWidget *parentWidget;
	QSize minStarSize;
};

class ComboBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit ComboBoxDelegate(QAbstractItemModel *model, QObject *parent = 0);
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;
	virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual bool eventFilter(QObject *object, QEvent *event);
public
slots:
	void testActivation(const QString &currString = QString());
	void testActivation(const QModelIndex &currIndex);
	//HACK: try to get rid of this in the future.
	void fakeActivation();
	void fixTabBehavior();
	virtual void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint) = 0;

protected:
	QAbstractItemModel *model;
};

class TankInfoDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit TankInfoDelegate(QObject *parent = 0);
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
public
slots:
	void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint);
};

class TankUseDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit TankUseDelegate(QObject *parent = 0);
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual void setEditorData(QWidget * editor, const QModelIndex & index) const;
};

class WSInfoDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit WSInfoDelegate(QObject *parent = 0);
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
public
slots:
	void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint);
};

class AirTypesDelegate : public ComboBoxDelegate {
	Q_OBJECT
public:
	explicit AirTypesDelegate(QObject *parent = 0);
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
public
slots:
	void revertModelData(QWidget *widget, QAbstractItemDelegate::EndEditHint hint);
};

/* ProfilePrintDelagate:
 * this delegate is used to modify the look of the table that is printed
 * bellow profiles.
 */
class ProfilePrintDelegate : public QStyledItemDelegate {
public:
	explicit ProfilePrintDelegate(QObject *parent = 0);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class SpinBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	SpinBoxDelegate(int min, int max, int step, QObject *parent = 0);
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
	int min;
	int max;
	int step;
};

class DoubleSpinBoxDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	DoubleSpinBoxDelegate(double min, double max, double step, QObject *parent = 0);
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
private:
	double min;
	double max;
	double step;
};

class HTMLDelegate : public ProfilePrintDelegate {
	Q_OBJECT
public:
	explicit HTMLDelegate(QObject *parent = 0);
	virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};
#endif // MODELDELEGATES_H
