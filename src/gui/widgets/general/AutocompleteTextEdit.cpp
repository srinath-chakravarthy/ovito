///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2015) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <core/Core.h>
#include "AutocompleteTextEdit.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Widgets)

/******************************************************************************
* Constructor.
******************************************************************************/
AutocompleteTextEdit::AutocompleteTextEdit(QWidget* parent) : QPlainTextEdit(parent),
		_wordSplitter("(?:(?<![\\w\\.])(?=[\\w\\.])|(?<=[\\w\\.])(?![\\w\\.]))")
{
	_wordListModel = new QStringListModel(this);
	_completer = new QCompleter(this);
	_completer->setCompletionMode(QCompleter::PopupCompletion);
	_completer->setCaseSensitivity(Qt::CaseInsensitive);
	_completer->setModel(_wordListModel);
	_completer->setWidget(this);
	connect(_completer, (void (QCompleter::*)(const QString&))&QCompleter::activated, this, &AutocompleteTextEdit::onComplete);
}

/******************************************************************************
* Inserts a complete word into the text field.
******************************************************************************/
void AutocompleteTextEdit::onComplete(const QString& completion)
{
	QStringList tokens = getTokenList();
	int pos = 0;
	for(QString& token : tokens) {
		pos += token.length();
		if(pos >= textCursor().position()) {
			int oldLen = token.length();
			token = completion;
			setPlainText(tokens.join(QString()));
			QTextCursor cursor = textCursor();
			cursor.setPosition(pos - oldLen + completion.length());
			setTextCursor(cursor);
			break;
		}
	}
}

/******************************************************************************
* Creates a list of tokens from the current text string.
******************************************************************************/
QStringList AutocompleteTextEdit::getTokenList() const
{
	// Split text at word boundaries. Consider '.' a word character.
	return toPlainText().split(_wordSplitter);
}

/******************************************************************************
* Handles key-press events.
******************************************************************************/
void AutocompleteTextEdit::keyPressEvent(QKeyEvent* event)
{
	if(_completer->popup()->isVisible()) {
		if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return ||
				event->key() == Qt::Key_Escape || event->key() == Qt::Key_Tab) {
	                event->ignore();
	                return;
		}
	}
	else if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        Q_EMIT editingFinished();
        return;
    }

	QPlainTextEdit::keyPressEvent(event);

	QStringList tokens = getTokenList();
	if(tokens.empty())
		return;
	int pos = 0;
	QString completionPrefix;
	for(const QString& token : tokens) {
		pos += token.length();
		if(pos >= textCursor().position()) {
			completionPrefix = token.trimmed();
			break;
		}
	}

	if(completionPrefix != _completer->completionPrefix()) {
		_completer->setCompletionPrefix(completionPrefix);
		_completer->popup()->setCurrentIndex(_completer->completionModel()->index(0,0));
	}
	if(completionPrefix.isEmpty() == false && !_wordListModel->stringList().contains(completionPrefix)) {
		QRect cr = cursorRect();
		cr.setWidth(_completer->popup()->sizeHintForColumn(0)
				+ _completer->popup()->verticalScrollBar()->sizeHint().width());
		_completer->complete(cr);
	}
	else
		_completer->popup()->hide();
}

/******************************************************************************
* Handles keyboard focus lost events.
******************************************************************************/
void AutocompleteTextEdit::focusOutEvent(QFocusEvent* event)
{
	if(event->reason() != Qt::PopupFocusReason || !(QApplication::activePopupWidget() && QApplication::activePopupWidget()->parentWidget() == this)) {
		Q_EMIT editingFinished();
	}
	QPlainTextEdit::focusOutEvent(event);
}

/******************************************************************************
* Returns the preferred size of the widget.
******************************************************************************/
QSize AutocompleteTextEdit::sizeHint() const
{
	QFontMetrics m(font());
	int lineHeight = m.lineSpacing();
	int numLines = 3;
	return QSize(QPlainTextEdit::sizeHint().width(), numLines * lineHeight);
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
