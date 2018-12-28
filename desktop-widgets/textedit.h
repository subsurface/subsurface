// SPDX-License-Identifier: GPL-2.0
#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>

// QTextEdit does not possess a signal that fires when the user finished
// editing the text. This subclass therefore overrides the focusOutEvent
// and sends the textEdited signal.
class TextEdit : public QTextEdit {
	Q_OBJECT
public:
	using QTextEdit::QTextEdit; // Make constructors of QTextEdit available
signals:
	void editingFinished();
private:
	void focusOutEvent(QFocusEvent *ev) override;
};

#endif
