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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/rendering/SceneRenderer.h>
#include <core/gui/properties/VariantComboBoxParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/ColorParameterUI.h>
#include <core/gui/properties/BooleanGroupBoxParameterUI.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include "DislocationDisplay.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, DislocationDisplay, DisplayObject);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, DislocationDisplayEditor, PropertiesEditor);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, DislocationPickInfo, ObjectPickInfo);
SET_OVITO_OBJECT_EDITOR(DislocationDisplay, DislocationDisplayEditor);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, _lineWidth, "LineWidth", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, _shadingMode, "ShadingMode", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, _burgersVectorWidth, "BurgersVectorWidth", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, _burgersVectorScaling, "BurgersVectorScaling", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, _burgersVectorColor, "BurgersVectorColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, _showBurgersVectors, "ShowBurgersVectors", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, _lineWidth, "Dislocation line width");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, _shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, _burgersVectorWidth, "Burgers vector width");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, _burgersVectorScaling, "Burgers vector scaling");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, _burgersVectorColor, "Burgers vector color");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, _showBurgersVectors, "Show Burgers vectors");
SET_PROPERTY_FIELD_UNITS(DislocationDisplay, _lineWidth, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(DislocationDisplay, _burgersVectorWidth, WorldParameterUnit);

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationDisplay::DislocationDisplay(DataSet* dataset) : DisplayObject(dataset),
	_lineWidth(1.0f), _shadingMode(ArrowPrimitive::NormalShading),
	_burgersVectorWidth(0.6f), _burgersVectorScaling(3.0f),
	_burgersVectorColor(0.7, 0.7, 0.7),
	_showBurgersVectors(false)
{
	INIT_PROPERTY_FIELD(DislocationDisplay::_lineWidth);
	INIT_PROPERTY_FIELD(DislocationDisplay::_shadingMode);
	INIT_PROPERTY_FIELD(DislocationDisplay::_burgersVectorWidth);
	INIT_PROPERTY_FIELD(DislocationDisplay::_burgersVectorScaling);
	INIT_PROPERTY_FIELD(DislocationDisplay::_burgersVectorColor);
	INIT_PROPERTY_FIELD(DislocationDisplay::_showBurgersVectors);
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 DislocationDisplay::boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState)
{
	SimulationCellObject* cellObject = flowState.findObject<SimulationCellObject>();
	if(!cellObject)
		return Box3();
	SimulationCell cell = cellObject->data();

	// Detect if the input data has changed since the last time we computed the bounding box.
	if(_boundingBoxCacheHelper.updateState(dataObject, cell,
			lineWidth(), showBurgersVectors(), burgersVectorScaling(),
			burgersVectorWidth()) || _cachedBoundingBox.isEmpty()) {
		// Recompute bounding box.
		Box3 bb = Box3(Point3(0,0,0), Point3(1,1,1)).transformed(cellObject->cellMatrix());
		FloatType padding = std::max(lineWidth(), 0.0f);
		if(showBurgersVectors()) {
			padding = std::max(padding, burgersVectorWidth() * 2.0f);
			if(OORef<DislocationNetworkObject> dislocationObj = dataObject->convertTo<DislocationNetworkObject>(time)) {
				for(DislocationSegment* segment : dislocationObj->segments()) {
					Point3 center = cell.wrapPoint(segment->getPointOnLine(0.5f));
					Vector3 dir = burgersVectorScaling() * segment->burgersVector.toSpatialVector();
					bb.addPoint(center + dir);
				}
			}
		}
		_cachedBoundingBox = bb.padBox(padding * 0.5f);
	}
	return _cachedBoundingBox;
}

/******************************************************************************
* Lets the display object render a data object.
******************************************************************************/
void DislocationDisplay::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode)
{
	// Get the simulation cell.
	SimulationCellObject* cellObject = flowState.findObject<SimulationCellObject>();
	if(!cellObject)
		return;

	// Do we have to re-create the geometry buffers from scratch?
	bool recreateBuffers = !_segmentBuffer || !_segmentBuffer->isValid(renderer)
						|| !_cornerBuffer || !_cornerBuffer->isValid(renderer)
						|| !_arrowBuffer || !_arrowBuffer->isValid(renderer);

	// Set up shading mode.
	ParticlePrimitive::ShadingMode cornerShadingMode = (shadingMode() == ArrowPrimitive::NormalShading)
			? ParticlePrimitive::NormalShading : ParticlePrimitive::FlatShading;
	if(!recreateBuffers) {
		recreateBuffers |= !_segmentBuffer->setShadingMode(shadingMode());
		recreateBuffers |= !_cornerBuffer->setShadingMode(cornerShadingMode);
		recreateBuffers |= !_arrowBuffer->setShadingMode(shadingMode());
	}

	// Get pattern catalog.
	PatternCatalog* patternCatalog = flowState.findObject<PatternCatalog>();

	// Do we have to update contents of the geometry buffers?
	bool updateContents = _geometryCacheHelper.updateState(
			dataObject, cellObject->data(), patternCatalog, lineWidth(),
			showBurgersVectors(), burgersVectorScaling(),
			burgersVectorWidth(), burgersVectorColor()) || recreateBuffers;

	// Re-create the geometry buffers if necessary.
	if(recreateBuffers) {
		_segmentBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, shadingMode(), ArrowPrimitive::HighQuality);
		_cornerBuffer = renderer->createParticlePrimitive(cornerShadingMode, ParticlePrimitive::HighQuality);
		_arrowBuffer = renderer->createArrowPrimitive(ArrowPrimitive::ArrowShape, shadingMode(), ArrowPrimitive::HighQuality);
	}

	// Update buffer contents.
	if(updateContents) {
		SimulationCell cellData = cellObject->data();
		if(OORef<DislocationNetworkObject> dislocationObj = dataObject->convertTo<DislocationNetworkObject>(time)) {
			int lineSegmentCount = 0, cornerCount = 0;
			for(DislocationSegment* segment : dislocationObj->segments()) {
				clipDislocationLine(segment->line, cellData, [&lineSegmentCount, &cornerCount](const Point3&, const Point3&, bool isInitialSegment) {
					lineSegmentCount++;
					if(!isInitialSegment) cornerCount++;
				});
			}
			_segmentBuffer->startSetElements(lineSegmentCount);
			std::vector<int> subobjToSegmentMap(lineSegmentCount + cornerCount);
			int lineSegmentIndex = 0;
			int dislocationIndex = 0;
			FloatType lineRadius = std::max(lineWidth() / 2, FloatType(0));
			QVector<Point3> cornerPoints;
			QVector<Color> cornerColors;
			cornerPoints.reserve(cornerCount);
			cornerColors.reserve(cornerCount);
			for(DislocationSegment* segment : dislocationObj->segments()) {
				BurgersVectorFamily* family = nullptr;
				if(patternCatalog) {
					Cluster* cluster = segment->burgersVector.cluster();
					OVITO_ASSERT(cluster != nullptr);
					StructurePattern* pattern = patternCatalog->structureById(cluster->structure);
					family = pattern->defaultBurgersVectorFamily();
					for(BurgersVectorFamily* f : pattern->burgersVectorFamilies()) {
						if(f->isMember(segment->burgersVector.localVec())) {
							family = f;
							break;
						}
					}
				}
				Color lineColor = family ? family->color() : Color(0.8,0.8,0.8);
				clipDislocationLine(segment->line, cellData, [this, &lineSegmentIndex, &cornerPoints, &cornerColors, lineColor, lineRadius, &subobjToSegmentMap, &dislocationIndex, lineSegmentCount](const Point3& v1, const Point3& v2, bool isInitialSegment) {
					subobjToSegmentMap[lineSegmentIndex] = dislocationIndex;
					_segmentBuffer->setElement(lineSegmentIndex++, v1, v2 - v1, ColorA(lineColor), lineRadius);
					if(!isInitialSegment) {
						subobjToSegmentMap[cornerPoints.size() + lineSegmentCount] = dislocationIndex;
						cornerPoints.push_back(v1);
						cornerColors.push_back(lineColor);
					}
				});
				dislocationIndex++;
			}
			OVITO_ASSERT(lineSegmentIndex == lineSegmentCount);
			OVITO_ASSERT(cornerPoints.size() == cornerCount);
			_segmentBuffer->endSetElements();
			_cornerBuffer->setSize(cornerPoints.size());
			_cornerBuffer->setParticlePositions(cornerPoints.empty() ? nullptr : cornerPoints.data());
			_cornerBuffer->setParticleColors(cornerColors.empty() ? nullptr : cornerColors.data());
			_cornerBuffer->setParticleRadius(lineRadius);

			if(showBurgersVectors()) {
				_arrowBuffer->startSetElements(dislocationObj->segments().size());
				subobjToSegmentMap.reserve(subobjToSegmentMap.size() + dislocationObj->segments().size());
				int arrowIndex = 0;
				ColorA arrowColor = burgersVectorColor();
				FloatType arrowRadius = std::max(burgersVectorWidth() / 2, FloatType(0));
				for(DislocationSegment* segment : dislocationObj->segments()) {
					subobjToSegmentMap.push_back(arrowIndex);
					Point3 center = cellData.wrapPoint(segment->getPointOnLine(0.5f));
					Vector3 dir = burgersVectorScaling() * segment->burgersVector.toSpatialVector();
					_arrowBuffer->setElement(arrowIndex++, center, dir, arrowColor, arrowRadius);
				}
			}
			else {
				_arrowBuffer->startSetElements(0);
			}
			_arrowBuffer->endSetElements();

			_pickInfo = new DislocationPickInfo(this, dislocationObj, patternCatalog, std::move(subobjToSegmentMap));

		}
		else {
			_cornerBuffer = nullptr;
			_segmentBuffer = nullptr;
			_arrowBuffer = nullptr;
			_pickInfo = nullptr;
		}
	}

	// Render segments.
	if(_cornerBuffer && _segmentBuffer) {
		renderer->beginPickObject(contextNode, _pickInfo);
		_segmentBuffer->render(renderer);
		_cornerBuffer->render(renderer);

		// Render Burgers vectors.
		if(_arrowBuffer && showBurgersVectors()) {
			_arrowBuffer->render(renderer);
		}

		renderer->endPickObject();
	}

}

/******************************************************************************
* Renders an overlay marker for a single dislocation segment.
******************************************************************************/
void DislocationDisplay::renderOverlayMarker(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, int segmentIndex, SceneRenderer* renderer, ObjectNode* contextNode)
{
	if(renderer->isPicking())
		return;

	// Get the simulation cell.
	SimulationCellObject* cellObject = flowState.findObject<SimulationCellObject>();
	if(!cellObject)
		return;
	SimulationCell cellData = cellObject->data();

	// Get the dislocations.
	OORef<DislocationNetworkObject> dislocationObj = dataObject->convertTo<DislocationNetworkObject>(time);
	if(!dislocationObj)
		return;

	if(segmentIndex < 0 || segmentIndex >= dislocationObj->segments().size())
		return;

	DislocationSegment* segment = dislocationObj->segments()[segmentIndex];

	// Generate the polyline segments to render.
	QVector<std::pair<Point3,Point3>> lineSegments;
	QVector<Point3> cornerVertices;
	clipDislocationLine(segment->line, cellData, [&lineSegments, &cornerVertices](const Point3& v1, const Point3& v2, bool isInitialSegment) {
		lineSegments.push_back({v1,v2});
		if(!isInitialSegment)
			cornerVertices.push_back(v1);
	});

	// Set up transformation.
	TimeInterval iv;
	const AffineTransformation& nodeTM = contextNode->getWorldTransform(time, iv);
	renderer->setWorldTransform(nodeTM);

	glDisable(GL_DEPTH_TEST);

	FloatType lineRadius = std::max(lineWidth() / 4, FloatType(0));
	std::shared_ptr<ArrowPrimitive> segmentBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::FlatShading, ArrowPrimitive::HighQuality);
	segmentBuffer->startSetElements(lineSegments.size());
	int index = 0;
	for(const auto& seg : lineSegments)
		segmentBuffer->setElement(index++, seg.first, seg.second - seg.first, ColorA(1,1,1), lineRadius);
	segmentBuffer->endSetElements();
	segmentBuffer->render(renderer);

	std::shared_ptr<ParticlePrimitive> cornerBuffer = renderer->createParticlePrimitive(ParticlePrimitive::FlatShading, ParticlePrimitive::HighQuality);
	cornerBuffer->setSize(cornerVertices.size());
	cornerBuffer->setParticlePositions(cornerVertices.constData());
	cornerBuffer->setParticleColor(Color(1,1,1));
	cornerBuffer->setParticleRadius(lineRadius);
	cornerBuffer->render(renderer);

	if(!segment->line.empty()) {
		Point3 wrappedHeadPos = cellData.wrapPoint(segment->line.front());
		std::shared_ptr<ParticlePrimitive> headBuffer = renderer->createParticlePrimitive(ParticlePrimitive::FlatShading, ParticlePrimitive::HighQuality);
		headBuffer->setSize(1);
		headBuffer->setParticlePositions(&wrappedHeadPos);
		headBuffer->setParticleColor(Color(1,1,1));
		headBuffer->setParticleRadius(lineRadius * 3);
		headBuffer->render(renderer);
	}

	glEnable(GL_DEPTH_TEST);
}

/******************************************************************************
* Clips a dislocation line at the periodic box boundaries.
******************************************************************************/
void DislocationDisplay::clipDislocationLine(const std::deque<Point3>& line, const SimulationCell& simulationCell, const std::function<void(const Point3&, const Point3&, bool)>& segmentCallback)
{
	auto v1 = line.cbegin();
	Point3 rp1 = simulationCell.absoluteToReduced(*v1);
	Vector3 shiftVector = Vector3::Zero();
	for(size_t dim = 0; dim < 3; dim++) {
		if(simulationCell.pbcFlags()[dim]) {
			while(rp1[dim] > 0) { rp1[dim] -= 1; shiftVector[dim] -= 1; }
			while(rp1[dim] < 0) { rp1[dim] += 1; shiftVector[dim] += 1; }
		}
	}
	bool isInitialSegment = true;
	for(auto v2 = v1 + 1; v2 != line.cend(); v1 = v2, ++v2) {
		Point3 rp2 = simulationCell.absoluteToReduced(*v2) + shiftVector;
		FloatType smallestT;
		do {
			size_t crossDim;
			FloatType crossDir;
			smallestT = FLOATTYPE_MAX;
			for(size_t dim = 0; dim < 3; dim++) {
				if(simulationCell.pbcFlags()[dim]) {
					int d = (int)floor(rp2[dim]) - (int)floor(rp1[dim]);
					if(d == 0) continue;
					FloatType t;
					if(d > 0)
						t = (ceil(rp1[dim]) - rp1[dim]) / (rp2[dim] - rp1[dim]);
					else
						t = (floor(rp1[dim]) - rp1[dim]) / (rp2[dim] - rp1[dim]);
					if(t > 0 && t < smallestT) {
						smallestT = t;
						crossDim = dim;
						crossDir = (d > 0) ? 1 : -1;
					}
				}
			}
			if(smallestT != FLOATTYPE_MAX) {
				Point3 intersection = rp1 + smallestT * (rp2 - rp1);
				intersection[crossDim] = floor(intersection[crossDim] + FloatType(0.5));
				segmentCallback(simulationCell.reducedToAbsolute(rp1), simulationCell.reducedToAbsolute(intersection), isInitialSegment);
				shiftVector[crossDim] -= crossDir;
				rp1 = intersection;
				rp1[crossDim] -= crossDir;
				rp2[crossDim] -= crossDir;
				isInitialSegment = true;
			}
		}
		while(smallestT != FLOATTYPE_MAX);

		segmentCallback(simulationCell.reducedToAbsolute(rp1), simulationCell.reducedToAbsolute(rp2), isInitialSegment);
		isInitialSegment = false;
		rp1 = rp2;
	}
}

/******************************************************************************
* Checks if the given floating point number is integer.
******************************************************************************/
static bool isInteger(FloatType v, int& intPart)
{
	static const FloatType epsilon = 1e-2f;
	FloatType ip;
	FloatType frac = std::modf(v, &ip);
	if(frac >= -epsilon && frac <= epsilon) intPart = (int)ip;
	else if(frac >= FloatType(1)-epsilon) intPart = (int)ip + 1;
	else if(frac <= FloatType(-1)+epsilon) intPart = (int)ip - 1;
	else return false;
	return true;
}

/******************************************************************************
* Generates a pretty string representation of the Burgers vector.
******************************************************************************/
QString DislocationDisplay::formatBurgersVector(const Vector3& b)
{
	FloatType smallestCompnt = FLOATTYPE_MAX;
	for(int i = 0; i < 3; i++) {
		FloatType c = std::abs(b[i]);
		if(c < smallestCompnt && c > 1e-3)
			smallestCompnt = c;
	}
	if(smallestCompnt != FLOATTYPE_MAX) {
		FloatType m = FloatType(1) / smallestCompnt;
		for(int f = 1; f <= 11; f++) {
			int multiplier;
			if(!isInteger(m*f, multiplier)) continue;
			if(multiplier < 80) {
				Vector3 bm = b * (FloatType)multiplier;
				Vector3I bmi;
				if(isInteger(bm.x(),bmi.x()) && isInteger(bm.y(),bmi.y()) && isInteger(bm.z(),bmi.z())) {
					return QString("1/%1[%2 %3 %4]")
							.arg(multiplier)
							.arg(bmi.x()).arg(bmi.y()).arg(bmi.z());
				}
			}
		}
	}

	return QString("%1 %2 %3")
			.arg(QLocale::c().toString(b.x(), 'f'), 7)
			.arg(QLocale::c().toString(b.y(), 'f'), 7)
			.arg(QLocale::c().toString(b.z(), 'f'), 7);
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString DislocationPickInfo::infoString(ObjectNode* objectNode, quint32 subobjectId)
{
	QString str;

	int segmentIndex = segmentIndexFromSubObjectID(subobjectId);
	if(segmentIndex >= 0 && segmentIndex < dislocationObj()->segments().size()) {
		DislocationSegment* segment = dislocationObj()->segments()[segmentIndex];
		str = tr("Dislocation | True Burgers vector: %1").arg(DislocationDisplay::formatBurgersVector(segment->burgersVector.localVec()));
		Vector3 transformedVector = segment->burgersVector.toSpatialVector();
		str += tr(" | Spatial Burgers vector: [%1 %2 %3]")
				.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
				.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
				.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
		str += tr(" | Cluster Id: %1").arg(segment->burgersVector.cluster()->id);
		str += tr(" | Segment Id: %1").arg(segment->id);
		if(patternCatalog() != nullptr) {
			int structureTypeId = segment->burgersVector.cluster()->structure;
			if(StructurePattern* s = patternCatalog()->structureById(structureTypeId))
				str += tr(" | Lattice structure: %1").arg(s->name());
		}
	}
	return str;
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DislocationDisplayEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Dislocation display"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* linesGroupBox = new QGroupBox(tr("Dislocation lines"));
	QGridLayout* sublayout = new QGridLayout(linesGroupBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(linesGroupBox);

	// Shading mode.
	VariantComboBoxParameterUI* shadingModeUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_shadingMode));
	shadingModeUI->comboBox()->addItem(tr("Normal"), qVariantFromValue(ArrowPrimitive::NormalShading));
	shadingModeUI->comboBox()->addItem(tr("Flat"), qVariantFromValue(ArrowPrimitive::FlatShading));
	sublayout->addWidget(new QLabel(tr("Shading mode:")), 0, 0);
	sublayout->addWidget(shadingModeUI->comboBox(), 0, 1);

	// Line width parameter.
	FloatParameterUI* lineWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_lineWidth));
	sublayout->addWidget(lineWidthUI->label(), 1, 0);
	sublayout->addLayout(lineWidthUI->createFieldLayout(), 1, 1);
	lineWidthUI->setMinValue(0);

	// Show Burgers vectors.
	BooleanGroupBoxParameterUI* showBurgersVectorsGroupUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_showBurgersVectors));
	showBurgersVectorsGroupUI->groupBox()->setTitle(tr("Burgers vectors"));
	sublayout = new QGridLayout(showBurgersVectorsGroupUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);
	layout->addWidget(showBurgersVectorsGroupUI->groupBox());

	// Arrow scaling.
	FloatParameterUI* burgersVectorScalingUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_burgersVectorScaling));
	sublayout->addWidget(new QLabel(tr("Scaling factor:")), 0, 0);
	sublayout->addLayout(burgersVectorScalingUI->createFieldLayout(), 0, 1);

	// Arrow width.
	FloatParameterUI* burgersVectorWidthUI = new FloatParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_burgersVectorWidth));
	sublayout->addWidget(new QLabel(tr("Width:")), 1, 0);
	sublayout->addLayout(burgersVectorWidthUI->createFieldLayout(), 1, 1);
	burgersVectorWidthUI->setMinValue(0);

	// Arrow color.
	ColorParameterUI* burgersVectorColorUI = new ColorParameterUI(this, PROPERTY_FIELD(DislocationDisplay::_burgersVectorColor));
	sublayout->addWidget(new QLabel(tr("Color:")), 2, 0);
	sublayout->addWidget(burgersVectorColorUI->colorPicker(), 2, 1);

}

}	// End of namespace
}	// End of namespace
}	// End of namespace
