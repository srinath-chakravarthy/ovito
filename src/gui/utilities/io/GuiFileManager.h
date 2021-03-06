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

#ifndef __OVITO_GUI_FILE_MANAGER_H
#define __OVITO_GUI_FILE_MANAGER_H

#include <core/Core.h>
#include <core/utilities/io/FileManager.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/**
 * \brief The file manager provides transparent access to remote files.
 */
class GuiFileManager : public FileManager
{
	Q_OBJECT

public:

	/// \brief Shows a dialog which asks the user for the login credentials.
	/// \return True on success, false if user has canceled the operation.
	virtual bool askUserForCredentials(QUrl& url) override;

protected:
    
	/// This is a singleton class. No public instances allowed.
	GuiFileManager() : FileManager() {}

	friend class GuiApplication;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_GUI_FILE_MANAGER_H
