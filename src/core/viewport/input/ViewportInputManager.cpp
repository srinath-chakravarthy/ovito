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

#include <core/Core.h>
#include <core/viewport/input/ViewportInputManager.h>
#include <core/viewport/input/ViewportInputMode.h>
#include <core/dataset/DataSetContainer.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/gui/mainwin/MainWindow.h>

namespace Ovito {

/******************************************************************************
* Initializes the viewport input manager.
******************************************************************************/
ViewportInputManager::ViewportInputManager(MainWindow* mainWindow) : QObject(mainWindow)
{
	class DefaultInputMode : public ViewportInputMode {
	public:
		DefaultInputMode(QObject* parent) : ViewportInputMode(parent) {}
		InputModeType modeType() override { return ExclusiveMode; }
	};
	_defaultMode = new DefaultInputMode(this);
	_zoomMode = new ZoomMode(this);
	_panMode = new PanMode(this);
	_orbitMode = new OrbitMode(this);
	_fovMode = new FOVMode(this);
	_pickOrbitCenterMode = new PickOrbitCenterMode(this);

	// Reset the viewport input manager when a new scene has been loaded.
	connect(&mainWindow->datasetContainer(), &DataSetContainer::dataSetChanged, this, &ViewportInputManager::reset);
}

/******************************************************************************
* Returns the currently active ViewportInputMode that handles the mouse events in viewports.
******************************************************************************/
ViewportInputMode* ViewportInputManager::activeMode()
{
	if(_inputModeStack.empty()) return nullptr;
	return _inputModeStack.back();
}

/******************************************************************************
* Pushes a mode onto the stack and makes it active.
******************************************************************************/
void ViewportInputManager::pushInputMode(ViewportInputMode* newMode, bool temporary)
{
    OVITO_CHECK_POINTER(newMode);

    ViewportInputMode* oldMode = activeMode();
	if(newMode == oldMode) return;

	if(oldMode) {
		if(newMode->modeType() == ViewportInputMode::ExclusiveMode) {
			// Remove all existing input modes from the stack before activating the exclusive mode.
			while(_inputModeStack.size() > 1)
				removeInputMode(activeMode());
			oldMode = activeMode();
			if(oldMode == newMode) return;
			OVITO_ASSERT(oldMode->_manager == this);
			oldMode->_manager = nullptr;
			_inputModeStack.clear();
		}
		else if(newMode->modeType() == ViewportInputMode::NormalMode) {
			// Remove all non-exclusive handlers from the stack before activating the new mode.
			while(_inputModeStack.size() > 1 && activeMode()->modeType() != ViewportInputMode::ExclusiveMode)
				removeInputMode(activeMode());
			oldMode = activeMode();
			if(oldMode == newMode) return;
			if(oldMode->modeType() != ViewportInputMode::ExclusiveMode) {
				_inputModeStack.pop_back();
				OVITO_ASSERT(oldMode->_manager == this);
				oldMode->_manager = nullptr;
			}
		}
		else if(newMode->modeType() == ViewportInputMode::TemporaryMode) {
			// Remove all temporary handlers from the stack before activating the new mode.
			if(oldMode->modeType() == ViewportInputMode::TemporaryMode) {
				_inputModeStack.pop_back();
				OVITO_ASSERT(oldMode->_manager == this);
				oldMode->_manager = nullptr;
			}
		}
	}

	// Put new handler on the stack.
	OVITO_ASSERT(newMode->_manager == nullptr);
	newMode->_manager = this;
	_inputModeStack.push_back(newMode);

	if(oldMode)
		oldMode->deactivated();
	newMode->activated(temporary);

	// Update viewports if the old or the new mode displays overlays.
	if((oldMode && oldMode->hasOverlay()) || newMode->hasOverlay()) {
		DataSet* dataset = mainWindow()->datasetContainer().currentSet();
		if(dataset && dataset->viewportConfig()) {
			dataset->viewportConfig()->updateViewports();
		}
	}

	Q_EMIT inputModeChanged(oldMode, newMode);
}

/******************************************************************************
* Removes a mode from the stack and deactivates it if it is currently active.
******************************************************************************/
void ViewportInputManager::removeInputMode(ViewportInputMode* mode)
{
	OVITO_CHECK_POINTER(mode);

	int index = _inputModeStack.indexOf(mode);
	if(index < 0) return;

	OVITO_ASSERT(mode->_manager == this);
	mode->_manager = nullptr;

	if(index == _inputModeStack.size() - 1) {
		_inputModeStack.remove(index);
		mode->deactivated();
		if(!_inputModeStack.empty())
			activeMode()->activated(false);

		// Update viewports if the old or the new mode displays overlays.
		if((!_inputModeStack.empty() && _inputModeStack.back()->hasOverlay()) || mode->hasOverlay()) {
			DataSet* dataset = mainWindow()->datasetContainer().currentSet();
			if(dataset && dataset->viewportConfig()) {
				dataset->viewportConfig()->updateViewports();
			}
		}

		Q_EMIT inputModeChanged(mode, activeMode());
	}
	else {
		_inputModeStack.remove(index);
	}
}

/******************************************************************************
* Resets the input mode stack to its initial state on application startup.
******************************************************************************/
void ViewportInputManager::reset()
{
	// Remove all input modes from the stack.
	while(activeMode())
		removeInputMode(activeMode());

	// Activate default mode
	pushInputMode(_defaultMode);
}

};
