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

#pragma once


#include <core/Core.h>
#include "Plugin.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(PluginSystem)

/**
 * \brief Loads and manages the installed plugins.
 */
class OVITO_CORE_EXPORT PluginManager : public QObject
{
	Q_OBJECT

public:

	/// Create the singleton instance of this class.
	static void initialize() {
		_instance = new PluginManager();
		_instance->registerLoadedPluginClasses();
	}

	/// Deletes the singleton instance of this class.
	static void shutdown() { delete _instance; _instance = nullptr; }

	/// \brief Returns the one and only instance of this class.
	/// \return The predefined instance of the PluginManager singleton class.
	inline static PluginManager& instance() {
		OVITO_ASSERT_MSG(_instance != nullptr, "PluginManager::instance", "Singleton object is not initialized yet.");
		return *_instance;
	}

	/// Searches the plugin directories for installed plugins and loads them.
	void loadAllPlugins();

	/// \brief Returns the plugin with a given identifier.
	/// \param pluginId The identifier of the plugin to return.
	/// \return The plugin with the given identifier or \c NULL if no such plugin is installed.
	Plugin* plugin(const QString& pluginId);

	/// \brief Returns the list of installed plugins.
	/// \return The list of all installed plugins.
	const QVector<Plugin*>& plugins() const { return _plugins; }

	/// \brief Returns all installed plugin classes derived from the given type.
	/// \param superClass Specifies the base class from which all returned classes should be derived.
	/// \param skipAbstract If \c true only non-abstract classes are returned.
	/// \return A list that contains all requested classes.
	QVector<OvitoObjectType*> listClasses(const OvitoObjectType& superClass, bool skipAbstract = true);

	/// \brief Registers a new plugin with the manager.
	/// \param plugin The plugin to be registered.
	/// \throw Exception when the plugin ID is not unique.
	/// \note The PluginManager becomes the owner of the Plugin class instance and will
	///       delete it on application shutdown.
	void registerPlugin(Plugin* plugin);

	/// Registers all classes of already loaded plugins.
	void registerLoadedPluginClasses();

	/// \brief Returns the list of directories containing the Ovito plugins.
	QList<QDir> pluginDirs();

	/// \brief Destructor that unloads all plugins.
	~PluginManager();

private:

	/////////////////////////////////// Plugins ////////////////////////////////////

	/// The list of installed plugins.
	QVector<Plugin*> _plugins;

	/////////////////////////// Maintenance ////////////////////////////////

	/// Private constructor.
	/// This is a singleton class; no public instances are allowed.
	PluginManager();

	/// The position in the global linked list of native object types up to which classes have already been registered.
	NativeOvitoObjectType* _lastRegisteredClass = nullptr;

	/// The singleton instance of this class.
	static PluginManager* _instance;

	friend class Plugin;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


