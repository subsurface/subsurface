/*
 * Copyright (c) 2013 Maximilian Güntner <maximilian.guentner@gmail.com>
 *
 * This file is subject to the terms and conditions of version 2 of
 * the GNU General Public License.  See the file gpl-2.0.txt in the main
 * directory of this archive for more details.
 *
 * Original License:
 *
 * This file is part of the Nepomuk widgets collection
 *  Copyright (c) 2013 Denis Steckelmacher <steckdenis@yahoo.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2.1 as published by the Free Software Foundation,
 * or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef GROUPEDLINEEDIT_H
#define GROUPEDLINEEDIT_H

#include <QPlainTextEdit>
#include <QStringList>

class GroupedLineEdit : public QPlainTextEdit {
	Q_OBJECT

public:
	explicit GroupedLineEdit(QWidget *parent = 0);
	virtual ~GroupedLineEdit();

	QString text() const;

	int cursorPosition() const;
	void setCursorPosition(int position);
	void setText(const QString &text);
	void clear();
	void selectAll();

	void removeAllBlocks();
	void addBlock(int start, int end);
	QStringList getBlockStringList();

	void addColor(QColor color);
	void removeAllColors();

	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;

signals:
	void editingFinished();

protected:
	virtual void paintEvent(QPaintEvent *e);
	virtual void keyPressEvent(QKeyEvent *e);

private:
	struct Private;
	Private *d;
};
#endif // GROUPEDLINEEDIT_H
