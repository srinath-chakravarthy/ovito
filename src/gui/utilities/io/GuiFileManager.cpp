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

#include <gui/GUI.h>
#include <gui/dialogs/RemoteAuthenticationDialog.h>
#include <core/app/Application.h>
#include "GuiFileManager.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO)

/******************************************************************************
* Shows a dialog which asks the user for the login credentials.
******************************************************************************/
bool GuiFileManager::askUserForCredentials(QUrl& url)
{
	if(Application::instance()->guiMode()) {

		// Ask for new username/password.
		RemoteAuthenticationDialog dialog(nullptr, tr("Remote authentication"),
				url.password().isEmpty() ?
				tr("<p>Please enter username and password to access the remote machine</p><p><b>%1</b></p>").arg(url.host()) :
				tr("<p>Authentication failed. Please enter the correct username and password to access the remote machine</p><p><b>%1</b></p>").arg(url.host()));

		dialog.setUsername(url.userName());
		dialog.setPassword(url.password());
		if(dialog.exec() == QDialog::Accepted) {
			url.setUserName(dialog.username());
			url.setPassword(dialog.password());
			return true;
		}
		return false;
	}
	else {
		return FileManager::askUserForCredentials(url);
	}
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
