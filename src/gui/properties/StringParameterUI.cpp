///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
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

#include <gui/GUI.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/widgets/general/AutocompleteTextEdit.h>
#include <core/dataset/UndoStack.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Params)

// Gives the class run-time type information.
IMPLEMENT_OVITO_OBJECT(Gui, StringParameterUI, PropertyParameterUI);

/******************************************************************************
* Constructor for a Qt property.
******************************************************************************/
StringParameterUI::StringParameterUI(QObject* parentEditor, const char* propertyName) :
	PropertyParameterUI(parentEditor, propertyName), _textBox(nullptr)
{
	// Create UI widget.
	_textBox = new QLineEdit();
	connect(static_cast<QLineEdit*>(_textBox.data()), &QLineEdit::editingFinished, this, &StringParameterUI::updatePropertyValue);
}

/******************************************************************************
* Constructor for a PropertyField property.
******************************************************************************/
StringParameterUI::StringParameterUI(QObject* parentEditor, const PropertyFieldDescriptor& propField) :
	PropertyParameterUI(parentEditor, propField), _textBox(nullptr)
{
	// Create UI widget.
	_textBox = new QLineEdit();
	connect(static_cast<QLineEdit*>(_textBox.data()), &QLineEdit::editingFinished, this, &StringParameterUI::updatePropertyValue);
}

/******************************************************************************
* Destructor.
******************************************************************************/
StringParameterUI::~StringParameterUI()
{
	// Release GUI widget.
	delete _textBox;
}

/******************************************************************************
* Replaces the text box managed by this ParameterUI.
* The ParameterUI becomes the owner of the new text box and the old widget is deleted.
******************************************************************************/
void StringParameterUI::setTextBox(QWidget* textBox)
{
	OVITO_ASSERT(textBox != nullptr);
	delete _textBox;
	_textBox = textBox;
	if(qobject_cast<QLineEdit*>(textBox))
		connect(static_cast<QLineEdit*>(textBox), &QLineEdit::editingFinished, this, &StringParameterUI::updatePropertyValue);
	else if(qobject_cast<AutocompleteTextEdit*>(textBox))
		connect(static_cast<AutocompleteTextEdit*>(textBox), &AutocompleteTextEdit::editingFinished, this, &StringParameterUI::updatePropertyValue);
	updateUI();
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to. 
******************************************************************************/
void StringParameterUI::resetUI()
{
	PropertyParameterUI::resetUI();	
	
	if(textBox()) {
		if(editObject()) {
			textBox()->setEnabled(isEnabled());
		}
		else {
			textBox()->setEnabled(false);
			if(qobject_cast<QLineEdit*>(textBox()))
				static_cast<QLineEdit*>(textBox())->clear();
			else if(qobject_cast<QTextEdit*>(textBox()))
				static_cast<QTextEdit*>(textBox())->clear();
			else if(qobject_cast<QPlainTextEdit*>(textBox()))
				static_cast<QPlainTextEdit*>(textBox())->clear();
		}
	}
}

/******************************************************************************
* This method is called when a new editable object has been assigned to the properties owner this
* parameter UI belongs to. 
******************************************************************************/
void StringParameterUI::updateUI()
{
	PropertyParameterUI::updateUI();	
	
	if(textBox() && editObject()) {
		QVariant val;
		if(isQtPropertyUI()) {
			val = editObject()->property(propertyName());
			OVITO_ASSERT_MSG(val.isValid() && val.canConvert(QVariant::String), "StringParameterUI::updateUI()", QString("The object class %1 does not define a property with the name %2 that can be cast to string type.").arg(editObject()->metaObject()->className(), QString(propertyName())).toLocal8Bit().constData());
			if(!val.isValid() || !val.canConvert(QVariant::String)) {
				editObject()->throwException(tr("The object class %1 does not define a property with the name %2 that can be cast to string type.").arg(editObject()->metaObject()->className(), QString(propertyName())));
			}
		}
		else if(isPropertyFieldUI()) {
			val = editObject()->getPropertyFieldValue(*propertyField());
			OVITO_ASSERT(val.isValid());
		}
		if(qobject_cast<QLineEdit*>(textBox()))
			static_cast<QLineEdit*>(textBox())->setText(val.toString());
		else if(qobject_cast<QTextEdit*>(textBox()))
			static_cast<QTextEdit*>(textBox())->setPlainText(val.toString());
		else if(qobject_cast<QPlainTextEdit*>(textBox())) {
			QString newText = val.toString();
			if(static_cast<QPlainTextEdit*>(textBox())->toPlainText() != newText)
				static_cast<QPlainTextEdit*>(textBox())->setPlainText(newText);
		}
	}
}

/******************************************************************************
* Sets the enabled state of the UI.
******************************************************************************/
void StringParameterUI::setEnabled(bool enabled)
{
	if(enabled == isEnabled()) return;
	PropertyParameterUI::setEnabled(enabled);
	if(textBox())
		textBox()->setEnabled(editObject() != NULL && isEnabled());
}

/******************************************************************************
* Takes the value entered by the user and stores it in the property field 
* this property UI is bound to.
******************************************************************************/
void StringParameterUI::updatePropertyValue()
{
	QString text;
	if(qobject_cast<QLineEdit*>(textBox()))
		text = static_cast<QLineEdit*>(textBox())->text();
	else if(qobject_cast<QTextEdit*>(textBox()))
		text = static_cast<QTextEdit*>(textBox())->toPlainText();
	else if(qobject_cast<QPlainTextEdit*>(textBox()))
		text = static_cast<QPlainTextEdit*>(textBox())->toPlainText();
	else
		return;
	if(editObject()) {
		undoableTransaction(tr("Change parameter"), [this,text]() {
			if(isQtPropertyUI()) {
				if(!editObject()->setProperty(propertyName(), text)) {
					OVITO_ASSERT_MSG(false, "StringParameterUI::updatePropertyValue()", QString("The value of property %1 of object class %2 could not be set.").arg(QString(propertyName()), editObject()->metaObject()->className()).toLocal8Bit().constData());
				}
			}
			else if(isPropertyFieldUI()) {
				editObject()->setPropertyFieldValue(*propertyField(), text);
			}
			Q_EMIT valueEntered();
		});
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
