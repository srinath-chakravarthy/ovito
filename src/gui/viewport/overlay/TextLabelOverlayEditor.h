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

#pragma once


#include <gui/GUI.h>
#include <gui/properties/PropertiesEditor.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(View) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the TextLabelOverlay class.
 */
class TextLabelOverlayEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE TextLabelOverlayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Updates the enabled/disabled status of the editor's controls.
	void updateEditorFields();

private:

	QLabel* _attributeNamesList;
	AutocompleteTextEdit* _textEdit;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


