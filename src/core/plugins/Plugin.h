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

#pragma once


#include <core/Core.h>
#include <core/object/OvitoObject.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(PluginSystem)

/**
 * \brief Represents a plugin that is loaded at runtime.
 */
class OVITO_CORE_EXPORT Plugin : public QObject
{
	Q_OBJECT

public:

	/// \brief Destructor
	virtual ~Plugin();

	/// \brief Returns the unique identifier of the plugin.
	const QString& pluginId() const { return _pluginId; }

	/// \brief Finds the plugin class with the given name defined by the plugin.
	/// \param name The class name.
	/// \return The descriptor for the plugin class with the given name or \c NULL
	///         if no such class is defined by the plugin.
	/// \sa classes()
	OvitoObjectType* findClass(const QString& name) const;

	/// \brief Returns whether the plugin's dynamic library has been loaded.
	/// \sa loadPlugin()
	bool isLoaded() const { return true; }

	/// \brief Loads the plugin's dynamic link library into memory.
	/// \throw Exception if an error occurs.
	///
	/// This method may load other plugins first if this plugin
	/// depends on them.
	/// \sa isLoaded()
	void loadPlugin() {}

	/// \brief Returns all classes defined by the plugin.
	/// \sa findClass()
	const QVector<OvitoObjectType*>& classes() const { return _classes; }

protected:

	/// \brief Constructor.
	Plugin(const QString& pluginId);

	/// \brief Adds a class to the list of plugin classes.
	void registerClass(OvitoObjectType* clazz) { _classes.push_back(clazz); }

private:

	/// The unique identifier of the plugin.
	QString _pluginId;

	/// The classes provided by the plugin.
	QVector<OvitoObjectType*> _classes;

	friend class PluginManager;
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


