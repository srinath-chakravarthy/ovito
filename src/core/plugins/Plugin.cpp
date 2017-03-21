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

#include <core/Core.h>
#include <core/plugins/Plugin.h>
#include <core/plugins/PluginManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(PluginSystem)

/******************************************************************************
* Constructor for the Plugin class.
******************************************************************************/
Plugin::Plugin(const QString& pluginId) : _pluginId(pluginId)
{
}

/******************************************************************************
* Destructor
******************************************************************************/
Plugin::~Plugin()
{
}

/******************************************************************************
* Finds the plugin class with the given name defined by the plugin.
******************************************************************************/
OvitoObjectType* Plugin::findClass(const QString& name) const
{
	for(OvitoObjectType* type : classes()) {
		if(type->name() == name || type->nameAlias() == name)
			return type;
	}
	return nullptr;
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

