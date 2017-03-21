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

#pragma once


#include <gui/GUI.h>
#include <gui/plugins/autostart/GuiAutoStartObject.h>

namespace VRPlugin {

using namespace Ovito;

/**
 * \brief An auto-start object that is automatically invoked on application startup.
 */
class StartVRAction : public GuiAutoStartObject
{
public:

	/// \brief Default constructor.
	Q_INVOKABLE StartVRAction() {}

	/// \brief Is called when a new main window is created.
	virtual void registerActions(ActionManager& actionManager) override;

	/// \brief Is called when the main menu is created.
	virtual void addActionsToMenu(ActionManager& actionManager, QMenuBar* menuBar) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
