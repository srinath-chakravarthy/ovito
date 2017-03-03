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
#include <gui/properties/PropertiesEditor.h>
#include <core/viewport/ViewportConfiguration.h>

namespace VRPlugin {

using namespace Ovito;	
/*
 * \brief The UI component for the VRSettingsObject class.
 */
class VRSettingsObjectEditor : public PropertiesEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE VRSettingsObjectEditor() {}

protected:
	
	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private Q_SLOTS:	

	/// Disables rendering of the normal viewports.
	void disableViewportRendering(bool disable) {
		_viewportSuspender.reset(disable ? new ViewportSuspender(dataset()) : nullptr);
	}
	
private:

	/// Used to disable viewport rendering.
	std::unique_ptr<ViewportSuspender> _viewportSuspender;

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
