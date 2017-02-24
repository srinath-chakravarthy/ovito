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
#include <core/plugins/autostart/AutoStartObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(PluginSystem)

/**
 * \brief Base class that allows plugins to execute code on application startup.
 *
 * Derive a sub-class from this class if you want to perform something on application startup.
 */
class OVITO_GUI_EXPORT GuiAutoStartObject : public AutoStartObject
{
protected:

	/// \brief The default constructor.
	GuiAutoStartObject() {}

public:

	/// \brief Is called when a new main window is created.
	virtual void registerActions(ActionManager& actionManager) {}

	/// \brief Is called when the main menu is created.
	virtual void addActionsToMenu(ActionManager& actionManager, QMenuBar* menuBar) {}

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


