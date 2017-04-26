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
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <gui/rendering/ViewportSceneRenderer.h>
#include <gui/actions/ViewportModeAction.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/viewport/ViewportWindow.h>
#include <core/viewport/Viewport.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/animation/AnimationSettings.h>
#include "DislocationInformationApplet.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(DislocationInformationApplet, UtilityApplet);

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

	QString cellColor1 = " style=\"background-color: #CCC;\"";
	QString cellColor2 = " style=\"background-color: #EEE;\"";

	for(auto& pickedDislocation : _inputMode->_pickedDislocations) {
		OVITO_ASSERT(pickedDislocation.objNode);
		const PipelineFlowState& flowState = pickedDislocation.objNode->evaluatePipelineImmediately(PipelineEvalRequest(dataset->animationSettings()->time(), false));
		DislocationNetworkObject* dislocationObj = flowState.findObject<DislocationNetworkObject>();
		if(!dislocationObj || pickedDislocation.segmentIndex >= dislocationObj->segments().size())
			continue;

		stream << QStringLiteral("<b>") << tr("Dislocation") << QStringLiteral(" ") << (pickedDislocation.segmentIndex + 1) << QStringLiteral(":</b>");
		stream << QStringLiteral("<table border=\"0\">");

		int row = 0;
		DislocationSegment* segment = dislocationObj->segments()[pickedDislocation.segmentIndex];
		stream << tr("<tr%1><td>Segment Id:</td><td>%2</td></tr>").arg((row++ % 2) ? cellColor1 : cellColor2).arg(segment->id);
		StructurePattern* structure = nullptr;
		Cluster* cluster = segment->burgersVector.cluster();
		PatternCatalog* patternCatalog = flowState.findObject<PatternCatalog>();
		if(patternCatalog != nullptr)
			structure = patternCatalog->structureById(cluster->structure);
		QString formattedBurgersVector = DislocationDisplay::formatBurgersVector(segment->burgersVector.localVec(), structure);
		stream << tr("<tr%1><td>True Burgers vector:</td><td>%2</td></tr>").arg((row++ % 2) ? cellColor1 : cellColor2).arg(formattedBurgersVector);
		Vector3 transformedVector = segment->burgersVector.toSpatialVector();
		stream << tr("<tr%1><td>Spatial Burgers vector:</td><td>%2 %3 %4</td></tr>")
				.arg((row++ % 2) ? cellColor1 : cellColor2)
				.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
				.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
				.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
		stream << tr("<tr%1><td>Length:</td><td>%2</td></tr>").arg((row++ % 2) ? cellColor1 : cellColor2).arg(segment->calculateLength());
		stream << tr("<tr%1><td>Cluster Id:</td><td>%2</td></tr>").arg((row++ % 2) ? cellColor1 : cellColor2).arg(segment->burgersVector.cluster()->id);
		if(structure) {
			stream << tr("<tr%1><td>Crystal structure:</td><td>%2</td></tr>").arg((row++ % 2) ? cellColor1 : cellColor2).arg(structure->name());
			if(structure->symmetryType() == StructurePattern::CubicSymmetry) {
				static const Vector3 latticeVectors[] = {{1,0,0},{0,1,0},{0,0,1}};
				for(auto v = std::begin(latticeVectors); v != std::end(latticeVectors); ++v) {
					Vector3 transformedVector = ClusterVector(*v, cluster).toSpatialVector();
					stream << tr("<tr%1><td>Lattice vector %2:</td><td>%3 %4 %5</td></tr>")
							.arg((row++ % 2) ? cellColor1 : cellColor2)
							.arg(DislocationDisplay::formatBurgersVector(*v, structure).replace(QStringLiteral(" "), QStringLiteral("&nbsp;")))
							.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
							.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
							.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
				}
			}
			else if(structure->symmetryType() == StructurePattern::HexagonalSymmetry) {
				static const Vector3 latticeVectors[] = {
							Vector3(-sqrt(1.0f/8.0f),-sqrt(3.0f/8.0f),0),
							Vector3(-sqrt(1.0f/8.0f),sqrt(3.0f/8.0f),0),
							Vector3(0,0,sqrt(4.0f/3.0f))
				};
				for(auto v = std::begin(latticeVectors); v != std::end(latticeVectors); ++v) {
					Vector3 transformedVector = ClusterVector(*v, cluster).toSpatialVector();
					stream << tr("<tr%1><td>Lattice vector %2:</td><td>%3 %4 %5</td></tr>")
							.arg((row++ % 2) ? cellColor1 : cellColor2)
							.arg(DislocationDisplay::formatBurgersVector(*v, structure).replace(QStringLiteral(" "), QStringLiteral("&nbsp;")))
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
void DislocationInformationInputMode::mouseReleaseEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton) {
		if(!event->modifiers().testFlag(Qt::ControlModifier))
			_pickedDislocations.clear();
		PickResult pickResult;
		if(pickDislocationSegment(vpwin, event->pos(), pickResult)) {
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
		vpwin->viewport()->dataset()->viewportConfig()->updateViewports();
	}
	ViewportInputMode::mouseReleaseEvent(vpwin, event);
}

/******************************************************************************
* Determines the dislocation segment under the mouse cursor.
******************************************************************************/
bool DislocationInformationInputMode::pickDislocationSegment(ViewportWindow* vpwin, const QPoint& pos, PickResult& result) const
{
	result.objNode = nullptr;
	result.displayObj = nullptr;

	ViewportPickResult vpPickResult = vpwin->pick(pos);

	// Check if user has clicked on something.
	if(vpPickResult) {

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
void DislocationInformationInputMode::mouseMoveEvent(ViewportWindow* vpwin, QMouseEvent* event)
{
	// Change mouse cursor while hovering over a dislocation.
	PickResult pickResult;
	if(pickDislocationSegment(vpwin, event->pos(), pickResult))
		setCursor(SelectionMode::selectionCursor());
	else
		setCursor(QCursor());

	ViewportInputMode::mouseMoveEvent(vpwin, event);
}

/******************************************************************************
* Lets the input mode render its overlay content in a viewport.
******************************************************************************/
void DislocationInformationInputMode::renderOverlay3D(Viewport* vp, ViewportSceneRenderer* renderer)
{
	ViewportInputMode::renderOverlay3D(vp, renderer);

	for(const auto& pickedDislocation : _pickedDislocations) {

		const PipelineFlowState& flowState = pickedDislocation.objNode->evaluatePipelineImmediately(PipelineEvalRequest(vp->dataset()->animationSettings()->time(), true));
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
