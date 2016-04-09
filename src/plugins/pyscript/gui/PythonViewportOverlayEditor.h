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

#ifndef __OVITO_PYTHON_VIEWPORT_OVERLAY_EDITOR_H
#define __OVITO_PYTHON_VIEWPORT_OVERLAY_EDITOR_H

#include <plugins/pyscript/PyScript.h>
#include <gui/properties/PropertiesEditor.h>

class QsciScintilla;

namespace PyScript { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace Ovito;

/**
 * \brief A properties editor for the PythonViewportOverlay class.
 */
class PythonViewportOverlayEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE PythonViewportOverlayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Is called when the current edit object has generated a change
	/// event or if a new object has been loaded into editor.
	void onContentsChanged(RefTarget* editObject);

	/// Is called when the user presses the 'Apply' button to commit the Python script.
	void onApplyChanges();

private:

	QsciScintilla* _codeEditor;
	QsciScintilla* _errorDisplay;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_PYTHON_VIEWPORT_OVERLAY_EDITOR_H
