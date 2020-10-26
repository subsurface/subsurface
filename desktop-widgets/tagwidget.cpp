// SPDX-License-Identifier: GPL-2.0
#include "tagwidget.h"
#include "mainwindow.h"
#include "tab-widgets/maintab.h"
#include <QCompleter>
#include <QMimeData>

TagWidget::TagWidget(QWidget *parent) : GroupedLineEdit(parent), m_completer(NULL), lastFinishedTag(false)
{
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(reparse()));
	connect(this, SIGNAL(textChanged()), this, SLOT(reparse()));

	QColor textColor = palette().color(QPalette::Text);
	qreal h, s, l, a;
	textColor.getHslF(&h, &s, &l, &a);
	// I use dark themes
	if (l <= 0.3) { // very dark text. get a brigth background
		addColor(QColor(Qt::red).lighter(120));
		addColor(QColor(Qt::green).lighter(120));
		addColor(QColor(Qt::blue).lighter(120));
	} else if (l <= 0.6) { // moderated dark text. get a somewhat bright background
		addColor(QColor(Qt::red).lighter(60));
		addColor(QColor(Qt::green).lighter(60));
		addColor(QColor(Qt::blue).lighter(60));
	} else {
		addColor(QColor(Qt::red).darker(120));
		addColor(QColor(Qt::green).darker(120));
		addColor(QColor(Qt::blue).darker(120));
	} // light text. get a dark background.
	setTabChangesFocus(true);
	setFocusPolicy(Qt::StrongFocus);
}

void TagWidget::setCompleter(QCompleter *completer)
{
	m_completer = completer;
	m_completer->setWidget(this);
	connect(m_completer, SIGNAL(activated(QString)), this, SLOT(completionSelected(QString)));
	connect(m_completer, SIGNAL(highlighted(QString)), this, SLOT(completionHighlighted(QString)));
}

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY Qt::SkipEmptyParts
#else
#define SKIP_EMPTY QString::SkipEmptyParts
#endif

QPair<int, int> TagWidget::getCursorTagPosition()
{
	int i = 0, start = 0, end = 0;
	/* Parse string near cursor */
	i = cursorPosition();
	while (--i > 0) {
		if (text().at(i) == ',') {
			if (i > 0 && text().at(i - 1) != '\\') {
				i++;
				break;
			}
		}
	}
	start = i;
	while (++i < text().length()) {
		if (text().at(i) == ',') {
			if (i > 0 && text().at(i - 1) != '\\')
				break;
		}
	}
	end = i;
	if (start < 0 || end < 0) {
		start = 0;
		end = 0;
	}
	return qMakePair(start, end);
}

void TagWidget::highlight()
{
	removeAllBlocks();
	int lastPos = 0;
	const auto l = text().split(QChar(','), SKIP_EMPTY);
	for (const QString &s: l) {
		QString trimmed = s.trimmed();
		if (trimmed.isEmpty())
			continue;
		int start = text().indexOf(trimmed, lastPos);
		addBlock(start, trimmed.size() + start);
		lastPos = trimmed.size() + start;
	}
}

void TagWidget::reparse()
{
	highlight();
	QPair<int, int> pos = getCursorTagPosition();
	QString currentText;
	if (pos.first >= 0 && pos.second > 0)
		currentText = text().mid(pos.first, pos.second - pos.first).trimmed();

	if (m_completer) {
		m_completer->setCompletionPrefix(currentText);
		if (m_completer->completionCount() == 1) {
			if (m_completer->currentCompletion() == currentText) {
				QAbstractItemView *popup = m_completer->popup();
				if (popup)
					popup->hide();
			} else {
				m_completer->complete();
			}
		} else {
			m_completer->complete();
		}
	}
}

void TagWidget::completionSelected(const QString &completion)
{
	completionHighlighted(completion);
	emit textChanged();
}

void TagWidget::completionHighlighted(const QString &completion)
{
	QPair<int, int> pos = getCursorTagPosition();
	setText(text().remove(pos.first, pos.second - pos.first).insert(pos.first, completion));
	setCursorPosition(pos.first + completion.length());
}

void TagWidget::setCursorPosition(int position)
{
	blockSignals(true);
	GroupedLineEdit::setCursorPosition(position);
	blockSignals(false);
}

void TagWidget::setText(const QString &text)
{
	blockSignals(true);
	GroupedLineEdit::setText(text);
	blockSignals(false);
	highlight();
}

void TagWidget::clear()
{
	blockSignals(true);
	GroupedLineEdit::clear();
	blockSignals(false);
}

void TagWidget::keyPressEvent(QKeyEvent *e)
{
	QPair<int, int> pos;
	QAbstractItemView *popup;
	bool finishedTag = false;
	switch (e->key()) {
	case Qt::Key_Escape:
		pos = getCursorTagPosition();
		if (pos.first >= 0 && pos.second > 0) {
			setText(text().remove(pos.first, pos.second - pos.first));
			setCursorPosition(pos.first);
		}
		popup = m_completer->popup();
		if (popup)
			popup->hide();
		return;
	case Qt::Key_Return:
	case Qt::Key_Enter:
	case Qt::Key_Tab:
		/*
		 * Fake the QLineEdit behaviour by simply
		 * closing the QAbstractViewitem
		 */
		if (m_completer) {
			popup = m_completer->popup();
			if (popup)
				popup->hide();
		}
		finishedTag = true;
		break;
	case Qt::Key_Comma: { /* if this is the last key, and the previous string is empty, ignore the comma. */
		QString temp = text();
		if (temp.split(QChar(',')).last().trimmed().isEmpty()){
			e->ignore();
			return;
		}
	  }
	}
	if (e->key() == Qt::Key_Tab && lastFinishedTag) {		    // if we already end in comma, go to next/prev field
		MainWindow::instance()->mainTab->nextInputField(e);         // by sending the key event to the MainTab widget
	} else if (e->key() == Qt::Key_Tab || e->key() == Qt::Key_Return) { // otherwise let's pretend this is a comma instead
		QKeyEvent fakeEvent(e->type(), Qt::Key_Comma, e->modifiers(), QString(","));
		keyPressEvent(&fakeEvent);
	} else {
		GroupedLineEdit::keyPressEvent(e);
	}
	lastFinishedTag = finishedTag;
}

void TagWidget::wheelEvent(QWheelEvent *event)
{
	if (hasFocus()) {
		GroupedLineEdit::wheelEvent(event);
	}
}

// Since we capture enter / return / tab, we never send an editingFinished() signal.
// Therefore, override the focusOutEvent()
void TagWidget::focusOutEvent(QFocusEvent *ev)
{
	GroupedLineEdit::focusOutEvent(ev);
	emit editingFinished();
}

// Implement simple drag and drop: text dropped onto the widget
// will be added as a new tag at the end. This overrides
// Qt's implementation which resulted in weird UI behavior,
// as the user may succeed in scrolling the view port.
static void handleDragEvent(QDragMoveEvent *e)
{
	if (e->mimeData()->hasFormat(QStringLiteral("text/plain")))
		e->acceptProposedAction();
}

void TagWidget::dragEnterEvent(QDragEnterEvent *e)
{
	handleDragEvent(e);
}

void TagWidget::dragMoveEvent(QDragMoveEvent *e)
{
	handleDragEvent(e);
}

void TagWidget::dragLeaveEvent(QDragLeaveEvent *)
{
}

void TagWidget::dropEvent(QDropEvent *e)
{
	e->acceptProposedAction();

	QString newTag = e->mimeData()->text().trimmed();
	if (newTag.isEmpty())
		return;

	QString s = text().trimmed();
	if (!s.isEmpty())
		s += ", ";
	s += newTag;
	setText(s);
	emit editingFinished();
}
