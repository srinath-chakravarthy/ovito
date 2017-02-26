///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <gui/dialogs/HistoryFileDialog.h>
#include "ObjectScriptEditor.h"

#ifndef signals
#define signals Q_SIGNALS
#endif
#ifndef slots
#define slots Q_SLOTS
#endif
#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexerpython.h>

namespace PyScript {

/******************************************************************************
* Constructs the editor frame.
******************************************************************************/
ObjectScriptEditor::ObjectScriptEditor(QWidget* parentWidget, RefTarget* scriptableObject) :
	QMainWindow(parentWidget, (Qt::WindowFlags)(Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint))
{
	// Create the central editor component.
	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	_codeEditor = new QsciScintilla();
	_codeEditor->setMarginLineNumbers(1, true);
	_codeEditor->setAutoIndent(true);
	_codeEditor->setTabWidth(4);
	_codeEditor->setFont(font);
	_codeEditor->setUtf8(true);
	QsciLexerPython* lexer = new QsciLexerPython(_codeEditor);
	lexer->setDefaultFont(font);
	_codeEditor->setLexer(lexer);
	_codeEditor->setMarginsFont(font);
	_codeEditor->setMarginWidth(0, QFontMetrics(font).width(QString::number(123)));
	_codeEditor->setMarginWidth(1, 0);
	_codeEditor->setMarginLineNumbers(0, true);
	setCentralWidget(_codeEditor);

	// Create the output pane.
	_outputWindow = new QsciScintilla();
	_outputWindow->setTabWidth(_codeEditor->tabWidth());
	_outputWindow->setFont(font);
	_outputWindow->setReadOnly(true);
	_outputWindow->setMarginWidth(1, 0);
	_outputWindow->setPaper(Qt::white);
	_outputWindow->setUtf8(true);
	QDockWidget* outputDockWidget = new QDockWidget(tr("Script output:"), this);
	outputDockWidget->setObjectName("ScriptOutput");
	outputDockWidget->setWidget(_outputWindow);
	outputDockWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
	addDockWidget(Qt::BottomDockWidgetArea, outputDockWidget);

	QToolBar* toolBar = addToolBar(tr("Script Editor"));
	QAction* commitAction = toolBar->addAction(QIcon(":/pyscript/icons/run_script.png"), tr("Commit and run script"), this, SLOT(onCommitScript()));
	commitAction->setShortcut(Qt::CTRL + Qt::Key_E);
	commitAction->setText(commitAction->text() + QStringLiteral(" [") + commitAction->shortcut().toString() + QStringLiteral("]"));
	toolBar->addSeparator();
	toolBar->addAction(QIcon(":/pyscript/icons/file_open.png"), tr("Load script from disk"), this, SLOT(onLoadScriptFromFile()));
	toolBar->addAction(QIcon(":/pyscript/icons/file_save_as.png"), tr("Save script to disk"), this, SLOT(onSaveScriptToFile()));
	toolBar->addSeparator();
	_undoAction = toolBar->addAction(QIcon(":/pyscript/icons/edit_undo.png"), tr("Undo"));
	_redoAction = toolBar->addAction(QIcon(":/pyscript/icons/edit_redo.png"), tr("Redo"));
	_undoAction->setEnabled(false);
	_redoAction->setEnabled(false);

	// Disable context menu in toolbar.
	setContextMenuPolicy(Qt::NoContextMenu);

    // Delete window when it is being closed by the user.
    setAttribute(Qt::WA_DeleteOnClose);

	// Make the input widget active.
	_codeEditor->setFocus();

	// Use a default window size.
	resize(800, 600);

	// Restore window layout.
	//QSettings settings;
	//settings.beginGroup("scripting/editor");
	//QVariant state = settings.value("state");
	//if(state.canConvert<QByteArray>())
	//	restoreState(state.toByteArray());

	connect(&_scriptableObject, &RefTargetListenerBase::notificationEvent, this, &ObjectScriptEditor::onNotificationEvent);
	_scriptableObject.setTarget(scriptableObject);
	setWindowTitle(scriptableObject ? scriptableObject->objectTitle() : tr("Script editor"));

	connect(_codeEditor, &QsciScintilla::textChanged, [this]() {
		_undoAction->setEnabled(_codeEditor->isUndoAvailable());
		_redoAction->setEnabled(_codeEditor->isRedoAvailable());
	});
	connect(_undoAction, &QAction::triggered, _codeEditor, &QsciScintilla::undo);
	connect(_redoAction, &QAction::triggered, _codeEditor, &QsciScintilla::redo);
	connect(_codeEditor, &QsciScintilla::modificationChanged, [this](bool m) {
		QString baseTitle = _scriptableObject.target() ? _scriptableObject.target()->objectTitle() : tr("Script editor");
		if(m)
			setWindowTitle(baseTitle + QStringLiteral(" *"));
		else
			setWindowTitle(baseTitle);
	});
}

/******************************************************************************
* Returns an existing editor window for the given object if there is one.
******************************************************************************/
ObjectScriptEditor* ObjectScriptEditor::findEditorForObject(RefTarget* scriptableObject)
{
	for(QWidget* w : QApplication::topLevelWidgets()) {
		if(ObjectScriptEditor* e = qobject_cast<ObjectScriptEditor*>(w)) {
			if(e->_scriptableObject.target() == scriptableObject)
				return e;
		}
	}
	return nullptr;
}	

/******************************************************************************
* Is called when the scriptable object generates an event.
******************************************************************************/
void ObjectScriptEditor::onNotificationEvent(ReferenceEvent* event)
{
	if(event->type() == ReferenceEvent::TargetDeleted) {
		// Close editor window when object is being deleted.
		this->deleteLater();
	}
	else if(event->type() == ReferenceEvent::TargetChanged) {
		// Update editor when object has been assigned a new script.
		updateEditorContents();
		updateOutputWindow();
	}
	else if(event->type() == ReferenceEvent::ObjectStatusChanged) {
		updateOutputWindow();
	}
}

/******************************************************************************
* Compiles/runs the current script.
******************************************************************************/
void ObjectScriptEditor::onCommitScript()
{
	if(_scriptableObject.target()) {
		setObjectScript(_scriptableObject.target(), _codeEditor->text());
	}
}

/******************************************************************************
* Replaces the editor contents with the script from the owning object.
******************************************************************************/
void ObjectScriptEditor::updateEditorContents()
{
	if(_scriptableObject.target()) {
		_codeEditor->setEnabled(true);
		const QString& script = getObjectScript(_scriptableObject.target());
		if(script != _codeEditor->text()) {
			_codeEditor->setText(script);
			_undoAction->setEnabled(false);
			_redoAction->setEnabled(false);
		}
		_codeEditor->setModified(false);
	}
	else {
		_codeEditor->setModified(false);
		_codeEditor->setEnabled(false);
		_codeEditor->setText(QString());
	}
}

/******************************************************************************
* Replaces the output window contents with the script output cached by the owning object.
******************************************************************************/
void ObjectScriptEditor::updateOutputWindow()
{
	if(_scriptableObject.target()) {
		_outputWindow->setText(getOutputText(_scriptableObject.target()));
	}
	else {
		_outputWindow->setText(QString());
	}
}

/******************************************************************************
* Is called when the window is shown.
******************************************************************************/
void ObjectScriptEditor::showEvent(QShowEvent* event)
{
	QMainWindow::showEvent(event);
	updateEditorContents();
	updateOutputWindow();
}

/******************************************************************************
* Is called when the user closes the window.
******************************************************************************/
void ObjectScriptEditor::closeEvent(QCloseEvent* event)
{
	if(_scriptableObject.target() && _codeEditor->isModified()) {
		QMessageBox::StandardButton btn = QMessageBox::question(this, tr("Save changes"), tr("The script has been modified. Do you want to commit the changes before closing the editor window?"),
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
		if(btn == QMessageBox::Cancel) {
			event->ignore();
			return;
		}
		else if(btn == QMessageBox::Yes) {
			onCommitScript();
		}
	}

	// Save window layout.
	//QSettings settings;
	//settings.beginGroup("scripting/editor");
	//settings.setValue("state", saveState());
}

/******************************************************************************
* Lets the user load a script file into the editor.
******************************************************************************/
void ObjectScriptEditor::onLoadScriptFromFile()
{
	if(!_scriptableObject.target()) return; 

	UndoableTransaction::handleExceptions(_scriptableObject.target()->dataset()->undoStack(), tr("Load script"), [this]() {
		HistoryFileDialog fileDialog("script", this, tr("Load script file"));
		fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
		fileDialog.setFileMode(QFileDialog::ExistingFile);
		QStringList filters;
		filters << "Python scripts (*.py)" << "Any files (*)";
		fileDialog.setNameFilters(filters);
		if(fileDialog.exec()) {
			QFile file(fileDialog.selectedFiles().front());
			if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
				throw Exception(tr("Failed to open file '%1' for reading: %2").arg(fileDialog.selectedFiles().front()).arg(file.errorString()));				
			setObjectScript(_scriptableObject.target(), QString::fromUtf8(file.readAll()));
		}
	});
}

/******************************************************************************
* Lets the user save the current script to a file.
******************************************************************************/
void ObjectScriptEditor::onSaveScriptToFile()
{
	if(!_scriptableObject.target()) return; 

	HistoryFileDialog fileDialog("script", this, tr("Save script to file"));
	fileDialog.setAcceptMode(QFileDialog::AcceptSave);
	QStringList filters;
	filters << "Python scripts (*.py)" << "Any files (*)";
	fileDialog.setNameFilters(filters);
	if(fileDialog.exec()) {
		QString filename = fileDialog.selectedFiles().front();
		QFile file(filename);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			QMessageBox::critical(this, tr("I/O Error"), tr("Failed to open file '%1' for writing: %2").arg(filename).arg(file.errorString()));
			return;
		}
		if(!_codeEditor->write(&file)) {
			QMessageBox::critical(this, tr("I/O Error"), tr("Failed to write file '%1': %2").arg(filename).arg(file.errorString()));
			return;
		}
	}
}

};
