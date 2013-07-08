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

/** 
 * \file FileManager.h
 * \brief Contains the definition of the Ovito::FileManager class.
 */

#ifndef __OVITO_FILE_MANAGER_H
#define __OVITO_FILE_MANAGER_H

#include <core/Core.h>
#include <core/utilities/concurrent/Future.h>

namespace Ovito {

/**
 * \brief The file manager provides transparent access to remote file.
 */
class FileManager : public QObject
{
	Q_OBJECT

public:

	/// \brief Returns the one and only instance of this class.
	/// \return The predefined instance of the UnitsManager singleton class.
	inline static FileManager& instance() {
		OVITO_ASSERT_MSG(_instance != nullptr, "FileManager::instance", "Singleton object is not initialized yet.");
		return *_instance;
	}

	/// \brief makes a file available on this computer.
	/// \return A QFuture that will provide the local file name of the downloaded file.
	Future<QString> fetchUrl(const QUrl& url);

private:
    
	/// This is a singleton class. No public instances allowed.
	FileManager();

	/// Create the singleton instance of this class.
	static void initialize() { _instance = new FileManager(); }

	/// Deletes the singleton instance of this class.
	static void shutdown() { delete _instance; _instance = nullptr; }

	/// The singleton instance of this class.
	static FileManager* _instance;

	friend class Application;
};


};

#endif // __OVITO_FILE_MANAGER_H