#include "tagwidget.h"
#include <QPair>
#include <QDebug>
#include <QAbstractItemView>
#include <QSettings>
#include <QFont>

TagWidget::TagWidget(QWidget *parent) : GroupedLineEdit(parent), m_completer(NULL)
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
	} else if (l <= 0.6) { // moderated dark text. get a somewhat brigth background
		addColor(QColor(Qt::red).lighter(60));
		addColor(QColor(Qt::green).lighter(60));
		addColor(QColor(Qt::blue).lighter(60));
	} else {
		addColor(QColor(Qt::red).darker(120));
		addColor(QColor(Qt::green).darker(120));
		addColor(QColor(Qt::blue).darker(120));
	} // light text. get a dark background.
	setFocusPolicy(Qt::StrongFocus);
}

void TagWidget::setCompleter(QCompleter *completer)
{
	m_completer = completer;
	m_completer->setWidget(this);
	connect(m_completer, SIGNAL(activated(QString)), this, SLOT(completionSelected(QString)));
	connect(m_completer, SIGNAL(highlighted(QString)), this, SLOT(completionSelected(QString)));
}

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

enum ParseState {
	FINDSTART,
	FINDEND
};

void TagWidget::highlight()
{
	int i = 0, start = 0, end = 0;
	ParseState state = FINDEND;
	removeAllBlocks();

	while (i < text().length()) {
		if (text().at(i) == ',') {
			if (state == FINDSTART) {
				/* Detect empty tags */
			} else if (state == FINDEND) {
				/* Found end of tag */
				if (i > 1) {
					if (text().at(i - 1) != '\\') {
						addBlock(start, end);
						state = FINDSTART;
					}
				} else {
					state = FINDSTART;
				}
			}
		} else if (text().at(i) == ' ') {
			/* Handled */
		} else {
			/* Found start of tag */
			if (state == FINDSTART) {
				state = FINDEND;
				start = i;
			} else if (state == FINDEND) {
				end = i;
			}
		}
		i++;
	}
	if (state == FINDEND) {
		if (end < start)
			end = text().length() - 1;
		if (text().length() > 0)
			addBlock(start, end);
	}
}

void TagWidget::reparse()
{
	highlight();
	QPair<int, int> pos = getCursorTagPosition();
	QString currentText;
	if (pos.first >= 0 && pos.second > 0)
		currentText = text().mid(pos.first, pos.second - pos.first).trimmed();
	else
		currentText = "";
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

void TagWidget::completionSelected(QString completion)
{
	QPair<int, int> pos;
	pos = getCursorTagPosition();
	if (pos.first >= 0 && pos.second > 0) {
		setText(text().remove(pos.first, pos.second - pos.first).insert(pos.first, completion));
		setCursorPosition(pos.first + completion.length());
	} else {
		setText(completion.append(", "));
		setCursorPosition(text().length());
	}
	emit(textChanged());
}

void TagWidget::setCursorPosition(int position)
{
	blockSignals(true);
	GroupedLineEdit::setCursorPosition(position);
	blockSignals(false);
}

void TagWidget::setText(QString text)
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
	switch (e->key()) {
	case Qt::Key_Return:
	case Qt::Key_Enter:
	case Qt::Key_Tab:
		/*
		 * Fake the QLineEdit behaviour by simply
		 * closing the QAbstractViewitem
		 */
		if (m_completer) {
			QAbstractItemView *popup = m_completer->popup();
			if (popup)
				popup->hide();
		}
	}
	if (e->key() == Qt::Key_Tab) { // let's pretend this is a comma instead
		QKeyEvent fakeEvent(e->type(), Qt::Key_Comma, e->modifiers(), QString(","));
		GroupedLineEdit::keyPressEvent(&fakeEvent);
	} else {
		GroupedLineEdit::keyPressEvent(e);
	}
}

void TagWidget::wheelEvent(QWheelEvent *event)
{
	if (hasFocus()) {
		GroupedLineEdit::wheelEvent(event);
	}
}
