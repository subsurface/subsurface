#include "textedit.h"

void TextEdit::focusOutEvent(QFocusEvent *ev)
{
	QTextEdit::focusOutEvent(ev);
	emit editingFinished();
}
