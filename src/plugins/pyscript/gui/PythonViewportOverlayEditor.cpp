///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2014) Alexander Stukowski
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

#include <plugins/pyscript/PyScript.h>
#include <plugins/pyscript/extensions/PythonViewportOverlay.h>
#include "PythonViewportOverlayEditor.h"

#ifndef signals
#define signals Q_SIGNALS
#endif
#ifndef slots
#define slots Q_SLOTS
#endif
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerpython.h>

namespace PyScript { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(PyScriptGui, PythonViewportOverlayEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(PythonViewportOverlay, PythonViewportOverlayEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void PythonViewportOverlayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Python script"), rolloutParams, "viewport_overlays.python_script.html");

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	int row = 0;

	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

	layout->addWidget(new QLabel(tr("Python script:")), row++, 0);
	_codeEditor = new QsciScintilla();
	_codeEditor->setEnabled(false);
	_codeEditor->setAutoIndent(true);
	_codeEditor->setTabWidth(4);
	_codeEditor->setFont(font);
	QsciLexerPython* lexer = new QsciLexerPython(_codeEditor);
	lexer->setDefaultFont(font);
	_codeEditor->setLexer(lexer);
	_codeEditor->setMarginsFont(font);
	_codeEditor->setMarginWidth(0, QFontMetrics(font).width(QString::number(123)));
	_codeEditor->setMarginWidth(1, 0);
	_codeEditor->setMarginLineNumbers(0, true);
	layout->addWidget(_codeEditor, row++, 0);

	QPushButton* applyButton = new QPushButton(tr("Apply changes"));
	layout->addWidget(applyButton, row++, 0);

	layout->addWidget(new QLabel(tr("Script output:")), row++, 0);
	_errorDisplay = new QsciScintilla();
	_errorDisplay->setTabWidth(_codeEditor->tabWidth());
	_errorDisplay->setFont(font);
	_errorDisplay->setReadOnly(true);
	_errorDisplay->setMarginWidth(1, 0);
	layout->addWidget(_errorDisplay, row++, 0);

	connect(this, &PropertiesEditor::contentsChanged, this, &PythonViewportOverlayEditor::onContentsChanged);
	connect(applyButton, &QPushButton::clicked, this, &PythonViewportOverlayEditor::onApplyChanges);
}

/******************************************************************************
* Is called when the current edit object has generated a change
* event or if a new object has been loaded into editor.
******************************************************************************/
void PythonViewportOverlayEditor::onContentsChanged(RefTarget* editObject)
{
	PythonViewportOverlay* overlay = static_object_cast<PythonViewportOverlay>(editObject);
	if(editObject) {
		_codeEditor->setText(overlay->script());
		_codeEditor->setEnabled(true);
		_errorDisplay->setText(overlay->scriptOutput());
	}
	else {
		_codeEditor->setEnabled(false);
		_codeEditor->clear();
		_errorDisplay->clear();
	}
}

/******************************************************************************
* Is called when the user presses the 'Apply' button to commit the Python script.
******************************************************************************/
void PythonViewportOverlayEditor::onApplyChanges()
{
	PythonViewportOverlay* overlay = static_object_cast<PythonViewportOverlay>(editObject());
	if(!overlay) return;

	undoableTransaction(tr("Change script"), [this, overlay]() {
		overlay->setScript(_codeEditor->text());
	});
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PythonViewportOverlayEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		PythonViewportOverlay* overlay = static_object_cast<PythonViewportOverlay>(editObject());
		if(overlay)
			_errorDisplay->setText(overlay->scriptOutput());
	}
	return PropertiesEditor::referenceEvent(source, event);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
