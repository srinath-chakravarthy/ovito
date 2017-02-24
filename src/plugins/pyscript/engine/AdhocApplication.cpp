///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <core/dataset/DataSetContainer.h>
#include <core/plugins/PluginManager.h>
#include "AdhocApplication.h"

namespace PyScript {

/******************************************************************************
* Initializes the application object.
******************************************************************************/
bool AdhocApplication::initialize()
{
	if(!Application::initialize())
		return false;

	// Initialize application state.
	PluginManager::initialize();

	// Create a DataSetContainer and a default DataSet.
	_datasetContainer = new DataSetContainer();
	_datasetContainer->setParent(this);
	_datasetContainer->setCurrentSet(new DataSet());

#if defined(Q_OS_LINUX)
	// On Unix/Linux, use headless mode if no X server is available.
	if(!qEnvironmentVariableIsEmpty("DISPLAY"))
		_headlessMode = false;
#elif defined(Q_OS_OSX) || defined(Q_OS_WIN)
	// On Windows and macOS, there is always an OpenGL implementation available for background rendering.
	_headlessMode = false;
#endif

	return true;
}

}	// End of namespace
