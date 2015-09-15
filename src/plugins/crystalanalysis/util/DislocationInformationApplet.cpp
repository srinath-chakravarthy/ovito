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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/gui/actions/ViewportModeAction.h>
#include <core/gui/mainwin/MainWindow.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/animation/AnimationSettings.h>
#include <core/rendering/viewport/ViewportSceneRenderer.h>
#include <core/viewport/input/XFormModes.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include "DislocationInformationApplet.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, DislocationInformationApplet, UtilityApplet);

/******************************************************************************
* Shows the UI of the utility in the given RolloutContainer.
******************************************************************************/
void DislocationInformationApplet::openUtility(MainWindow* mainWindow, RolloutContainer* container, const RolloutInsertionParameters& rolloutParams)
{
	OVITO_ASSERT(_panel == nullptr);
	_mainWindow = mainWindow;

	// Create a rollout.
	_panel = new QWidget();
	container->addRollout(_panel, tr("Dislocation information"), rolloutParams.useAvailableSpace());

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(_panel);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	_inputMode = new DislocationInformationInputMode(this);
	ViewportModeAction* pickModeAction = new ViewportModeAction(_mainWindow, tr("Selection mode"), this, _inputMode);
	//layout->addWidget(pickModeAction->createPushButton());

	_infoDisplay = new QTextEdit(_panel);
	_infoDisplay->setReadOnly(true);
	_infoDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
#ifndef Q_OS_MACX
	_infoDisplay->setText(tr("Pick a dislocation line in the viewports. Hold down the CONTROL key to select multiple dislocations."));
#else
	_infoDisplay->setText(tr("Pick a dislocation line in the viewports. Hold down the COMMAND key to select multiple dislocations."));
#endif
	layout->addWidget(_infoDisplay, 1);

	_mainWindow->viewportInputManager()->pushInputMode(_inputMode);
}

/******************************************************************************
* Removes the UI of the utility from the rollout container.
******************************************************************************/
void DislocationInformationApplet::closeUtility(RolloutContainer* container)
{
	delete _panel;
}

/******************************************************************************
* Updates the display of atom properties.
******************************************************************************/
void DislocationInformationApplet::updateInformationDisplay()
{
	DataSet* dataset = _mainWindow->datasetContainer().currentSet();
	if(!dataset) return;

	QString infoText;
	QTextStream stream(&infoText, QIODevice::WriteOnly);

	for(auto& pickedDislocation : _inputMode->_pickedDislocations) {
		OVITO_ASSERT(pickedDislocation.objNode);
		const PipelineFlowState& flowState = pickedDislocation.objNode->evalPipeline(dataset->animationSettings()->time());
		DislocationNetworkObject* dislocationObj = flowState.findObject<DislocationNetworkObject>();
		if(!dislocationObj || pickedDislocation.segmentIndex >= dislocationObj->segments().size())
			continue;

		stream << QStringLiteral("<b>") << tr("Dislocation index") << QStringLiteral(" ") << (pickedDislocation.segmentIndex + 1) << QStringLiteral(":</b>");
		stream << QStringLiteral("<table border=\"0\">");

		DislocationSegment* segment = dislocationObj->segments()[pickedDislocation.segmentIndex];
		stream << tr("<tr><td>Segment Id:</td><td>%1</td></tr>").arg(segment->id);
		StructurePattern* structure = nullptr;
		Cluster* cluster = segment->burgersVector.cluster();
		PatternCatalog* patternCatalog = flowState.findObject<PatternCatalog>();
		if(patternCatalog != nullptr)
			structure = patternCatalog->structureById(cluster->structure);
		QString formattedBurgersVector = DislocationDisplay::formatBurgersVector(segment->burgersVector.localVec(), structure);
		stream << tr("<tr><td>True Burgers vector:</td><td>%1</td></tr>").arg(formattedBurgersVector);
		Vector3 transformedVector = segment->burgersVector.toSpatialVector();
		stream << tr("<tr><td>Spatial Burgers vector:</td><td>%1 %2 %3</td></tr>")
				.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
				.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
				.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
		stream << tr("<tr><td>Cluster Id:</td><td>%1</td></tr>").arg(segment->burgersVector.cluster()->id);
		if(structure) {
			stream << tr("<tr><td>Lattice structure:</td><td>%1</td></tr>").arg(structure->name());
			if(structure->symmetryType() == StructurePattern::CubicSymmetry) {
				static const Vector3 latticeVectors[] = {{1,0,0},{0,1,0},{0,0,1}};
				for(auto v = std::begin(latticeVectors); v != std::end(latticeVectors); ++v) {
					Vector3 transformedVector = ClusterVector(Vector3(1,0,0), cluster).toSpatialVector();
					stream << tr("<tr><td>Lattice vector [%1]:</td><td>%2 %3 %4</td></tr>")
							.arg(DislocationDisplay::formatBurgersVector(*v, structure))
							.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
							.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
							.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
				}
			}
			else if(structure->symmetryType() == StructurePattern::HexagonalSymmetry) {
				static const Vector3 latticeVectors[] = {{1,0,0},{0,1,0},{0,0,sqrt(4.0f/3.0f)}};
				for(auto v = std::begin(latticeVectors); v != std::end(latticeVectors); ++v) {
					Vector3 transformedVector = ClusterVector(Vector3(1,0,0), cluster).toSpatialVector();
					stream << tr("<tr><td>Lattice vector [%1]:</td><td>%2 %3 %4</td></tr>")
							.arg(DislocationDisplay::formatBurgersVector(*v, structure))
							.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
							.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
							.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
				}
			}
		}

		stream << QStringLiteral("</table><hr>");
	}

	if(_inputMode->_pickedDislocations.empty())
		infoText = tr("No dislocations selected.");

	_infoDisplay->setText(infoText);
}

/******************************************************************************
* Handles the mouse up events for a Viewport.
******************************************************************************/
void DislocationInformationInputMode::mouseReleaseEvent(Viewport* vp, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		if(!event->modifiers().testFlag(Qt::ControlModifier))
			_pickedDislocations.clear();
		PickResult pickResult;
		if(pickDislocationSegment(vp, event->pos(), pickResult)) {
			// Don't select the same dislocation twice.
			bool alreadySelected = false;
			for(auto p = _pickedDislocations.begin(); p != _pickedDislocations.end(); ++p) {
				if(p->objNode == pickResult.objNode && p->segmentIndex == pickResult.segmentIndex) {
					alreadySelected = true;
					_pickedDislocations.erase(p);
					break;
				}
			}
			if(!alreadySelected)
				_pickedDislocations.push_back(pickResult);
		}
		_applet->updateInformationDisplay();
		vp->dataset()->viewportConfig()->updateViewports();
	}
	ViewportInputMode::mouseReleaseEvent(vp, event);
}

/******************************************************************************
* Determines the dislocation segment under the mouse cursor.
******************************************************************************/
bool DislocationInformationInputMode::pickDislocationSegment(Viewport* vp, const QPoint& pos, PickResult& result) const
{
	result.objNode = nullptr;
	result.displayObj = nullptr;

	ViewportPickResult vpPickResult = vp->pick(pos);

	// Check if user has clicked on something.
	if(vpPickResult.valid) {

		// Check if that was a dislocation.
		DislocationPickInfo* pickInfo = dynamic_object_cast<DislocationPickInfo>(vpPickResult.pickInfo);
		if(pickInfo) {
			int segmentIndex = pickInfo->segmentIndexFromSubObjectID(vpPickResult.subobjectId);
			if(segmentIndex >= 0 && segmentIndex < pickInfo->dislocationObj()->segments().size()) {

				// Save reference to the picked segment.
				result.objNode = vpPickResult.objectNode;
				result.segmentIndex = segmentIndex;
				result.displayObj = pickInfo->displayObject();

				return true;
			}
		}
	}

	return false;
}

/******************************************************************************
* Handles the mouse move event for the given viewport.
******************************************************************************/
void DislocationInformationInputMode::mouseMoveEvent(Viewport* vp, QMouseEvent* event)
{
	// Change mouse cursor while hovering over a dislocation.
	PickResult pickResult;
	if(pickDislocationSegment(vp, event->pos(), pickResult))
		setCursor(SelectionMode::selectionCursor());
	else
		setCursor(QCursor());

	ViewportInputMode::mouseMoveEvent(vp, event);
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void DislocationInformationInputMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer)
{
	ViewportInputMode::renderOverlay3D(vp, renderer);

	for(const auto& pickedDislocation : _pickedDislocations) {

		const PipelineFlowState& flowState = pickedDislocation.objNode->evalPipeline(vp->dataset()->animationSettings()->time());
		DislocationNetworkObject* dislocationObj = flowState.findObject<DislocationNetworkObject>();
		if(!dislocationObj)
			continue;

		pickedDislocation.displayObj->renderOverlayMarker(vp->dataset()->animationSettings()->time(), dislocationObj, flowState, pickedDislocation.segmentIndex, renderer, pickedDislocation.objNode);
	}
}

/******************************************************************************
* Computes the bounding box of the 3d visual viewport overlay rendered by the input mode.
******************************************************************************/
Box3 DislocationInformationInputMode::overlayBoundingBox(Viewport* vp, ViewportSceneRenderer* renderer)
{
	Box3 bbox = ViewportInputMode::overlayBoundingBox(vp, renderer);
	//for(const auto& pickedParticle : _pickedParticles)
	//	bbox.addBox(selectionMarkerBoundingBox(vp, pickedParticle));
	return bbox;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
