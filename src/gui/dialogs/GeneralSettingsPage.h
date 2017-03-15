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


#include <gui/GUI.h>
#include <gui/dialogs/ApplicationSettingsDialog.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * Page of the application settings dialog, which hosts general program options.
 */
class OVITO_GUI_EXPORT GeneralSettingsPage : public ApplicationSettingsDialogPage
{
public:

	/// Default constructor.
	Q_INVOKABLE GeneralSettingsPage() : ApplicationSettingsDialogPage() {}

	/// \brief Creates the widget.
	virtual void insertSettingsDialogPage(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

	/// \brief Lets the settings page to save all values entered by the user.
	/// \param settingsDialog The settings dialog box.
	virtual bool saveValues(ApplicationSettingsDialog* settingsDialog, QTabWidget* tabWidget) override;

private:

	QCheckBox* _useQtFileDialog;
	QCheckBox* _enableMRUModifierList;
	QCheckBox* _overrideGLContextSharing;
	QComboBox* _contextSharingMode;
	QCheckBox* _overrideUseOfPointSprites;
	QComboBox* _pointSpriteMode;
	QCheckBox* _overrideUseOfGeometryShaders;
	QComboBox* _geometryShaderMode;
#if !defined(OVITO_BUILD_APPSTORE_VERSION)
	QCheckBox* _enableUpdateChecks;
	QCheckBox* _enableUsageStatistics;
#endif

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


