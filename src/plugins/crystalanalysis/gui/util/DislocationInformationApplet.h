///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <gui/plugins/utility/UtilityApplet.h>
#include <gui/viewport/input/ViewportInputMode.h>
#include <gui/viewport/input/ViewportInputManager.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

class DislocationInformationInputMode;		// defined below

/**
 * \brief This utility lets the user select particles in the viewports and lists their properties and distances.
 */
class DislocationInformationApplet : public UtilityApplet
{
public:

	/// Constructor.
	Q_INVOKABLE DislocationInformationApplet() : UtilityApplet(), _panel(nullptr) {}

	/// Shows the UI of the utility in the given RolloutContainer.
	virtual void openUtility(MainWindow* mainWindow, RolloutContainer* container, const RolloutInsertionParameters& rolloutParams = RolloutInsertionParameters()) override;

	/// Removes the UI of the utility from the RolloutContainer.
	virtual void closeUtility(RolloutContainer* container) override;

public Q_SLOTS:

	/// Updates the display of particle properties.
	void updateInformationDisplay();

private:

	MainWindow* _mainWindow;
	QTextEdit* _infoDisplay;
	QWidget* _panel;
	DislocationInformationInputMode* _inputMode;

	Q_CLASSINFO("DisplayName", "Inspect dislocations");

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * Viewport input mode that lets the user pick a dislocation.
 */
class DislocationInformationInputMode : public ViewportInputMode
{
public:

	/// Constructor.
	DislocationInformationInputMode(DislocationInformationApplet* applet) : ViewportInputMode(applet),
		_applet(applet) {}

	/// Returns the activation behavior of this input mode.
	virtual InputModeType modeType() override { return ExclusiveMode; }

	/// Handles the mouse up events for a Viewport.
	virtual void mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

	/// \brief Handles the mouse move event for the given viewport.
	virtual void mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event) override;

	/// \brief Lets the input mode render its overlay content in a viewport.
	virtual void renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer) override;

	/// \brief Indicates whether this input mode renders into the viewports.
	virtual bool hasOverlay() override { return true; }

	/// Computes the bounding box of the 3d visual viewport overlay rendered by the input mode.
	virtual Box3 overlayBoundingBox(Viewport* vp, ViewportSceneRenderer* renderer) override;

private:

	struct PickResult {

		/// The index of the picked dislocation segment.
		int segmentIndex;

		/// The scene node that contains the picked segment.
		OORef<ObjectNode> objNode;

		/// The display object that rendered the picked segment.
		OORef<DislocationDisplay> displayObj;
	};

	/// Determines the dislocation segment under the mouse cursor.
	bool pickDislocationSegment(ViewportWindow* vpwin, const QPoint& pos, PickResult& result) const;

	/// The applet.
	DislocationInformationApplet* _applet;

	/// The selected dislocations whose properties are being displayed.
	std::deque<PickResult> _pickedDislocations;

	friend class DislocationInformationApplet;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


