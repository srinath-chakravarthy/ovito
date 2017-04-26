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
#include <plugins/particles/objects/SimulationCellObject.h>
#include <core/rendering/SceneRenderer.h>
#include "DislocationDisplay.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(DislocationDisplay, DisplayObject);
IMPLEMENT_OVITO_OBJECT(DislocationPickInfo, ObjectPickInfo);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, lineWidth, "LineWidth", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, shadingMode, "ShadingMode", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, burgersVectorWidth, "BurgersVectorWidth", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, burgersVectorScaling, "BurgersVectorScaling", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationDisplay, burgersVectorColor, "BurgersVectorColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(DislocationDisplay, showBurgersVectors, "ShowBurgersVectors");
DEFINE_PROPERTY_FIELD(DislocationDisplay, showLineDirections, "ShowLineDirections");
DEFINE_PROPERTY_FIELD(DislocationDisplay, lineColoringMode, "LineColoringMode");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, lineWidth, "Dislocation line width");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, burgersVectorWidth, "Burgers vector width");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, burgersVectorScaling, "Burgers vector scaling");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, burgersVectorColor, "Burgers vector color");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, showBurgersVectors, "Show Burgers vectors");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, showLineDirections, "Indicate line directions");
SET_PROPERTY_FIELD_LABEL(DislocationDisplay, lineColoringMode, "Line coloring");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationDisplay, lineWidth, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(DislocationDisplay, burgersVectorWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationDisplay::DislocationDisplay(DataSet* dataset) : DisplayObject(dataset),
	_lineWidth(1.0f), _shadingMode(ArrowPrimitive::NormalShading),
	_burgersVectorWidth(0.6f), _burgersVectorScaling(3.0f),
	_burgersVectorColor(0.7, 0.7, 0.7),
	_showBurgersVectors(false), _showLineDirections(false), _lineColoringMode(ColorByDislocationType)
{
	INIT_PROPERTY_FIELD(lineWidth);
	INIT_PROPERTY_FIELD(shadingMode);
	INIT_PROPERTY_FIELD(burgersVectorWidth);
	INIT_PROPERTY_FIELD(burgersVectorScaling);
	INIT_PROPERTY_FIELD(burgersVectorColor);
	INIT_PROPERTY_FIELD(showBurgersVectors);
	INIT_PROPERTY_FIELD(showLineDirections);
	INIT_PROPERTY_FIELD(lineColoringMode);
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
		FloatType padding = std::max(lineWidth(), FloatType(0));
		if(showBurgersVectors()) {
			padding = std::max(padding, burgersVectorWidth() * FloatType(2));
			if(OORef<DislocationNetworkObject> dislocationObj = dataObject->convertTo<DislocationNetworkObject>(time)) {
				for(DislocationSegment* segment : dislocationObj->segments()) {
					Point3 center = cell.wrapPoint(segment->getPointOnLine(FloatType(0.5)));
					Vector3 dir = burgersVectorScaling() * segment->burgersVector.toSpatialVector();
					bb.addPoint(center + dir);
				}
			}
		}
		_cachedBoundingBox = bb.padBox(padding * FloatType(0.5));
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
						|| !_burgersArrowBuffer || !_burgersArrowBuffer->isValid(renderer);

	ArrowPrimitive::Shape segmentShape = (showLineDirections() ? ArrowPrimitive::ArrowShape : ArrowPrimitive::CylinderShape);

	// Set up shading mode.
	ParticlePrimitive::ShadingMode cornerShadingMode = (shadingMode() == ArrowPrimitive::NormalShading)
			? ParticlePrimitive::NormalShading : ParticlePrimitive::FlatShading;
	if(!recreateBuffers) {
		recreateBuffers |= !_segmentBuffer->setShadingMode(shadingMode());
		recreateBuffers |= !_cornerBuffer->setShadingMode(cornerShadingMode);
		recreateBuffers |= !_burgersArrowBuffer->setShadingMode(shadingMode());
		if(_segmentBuffer->shape() != segmentShape)
			recreateBuffers = true;
	}

	// Get the pattern catalog.
	PatternCatalog* patternCatalog = flowState.findObject<PatternCatalog>();

	// Do we have to update contents of the geometry buffers?
	bool updateContents = _geometryCacheHelper.updateState(
			dataObject, cellObject->data(), patternCatalog, lineWidth(),
			showBurgersVectors(), burgersVectorScaling(),
			burgersVectorWidth(), burgersVectorColor(), lineColoringMode()) || recreateBuffers;

	// Re-create the geometry buffers if necessary.
	if(recreateBuffers) {
		_segmentBuffer = renderer->createArrowPrimitive(segmentShape, shadingMode(), ArrowPrimitive::HighQuality);
		_cornerBuffer = renderer->createParticlePrimitive(cornerShadingMode, ParticlePrimitive::HighQuality);
		_burgersArrowBuffer = renderer->createArrowPrimitive(ArrowPrimitive::ArrowShape, shadingMode(), ArrowPrimitive::HighQuality);
	}

	// Update buffer contents.
	if(updateContents) {
		SimulationCell cellData = cellObject->data();
		if(OORef<DislocationNetworkObject> dislocationObj = dataObject->convertTo<DislocationNetworkObject>(time)) {
			// First perform a dry run to count number of render vertices/segments that are going to be generated.
			int lineSegmentCount = 0, cornerCount = 0;
			for(DislocationSegment* segment : dislocationObj->segments()) {
				clipDislocationLine(segment->line, cellData, dislocationObj->cuttingPlanes(), [&lineSegmentCount, &cornerCount](const Point3&, const Point3&, bool isInitialSegment) {
					lineSegmentCount++;
					if(!isInitialSegment) cornerCount++;
				});
			}
			// Allocate render buffer.
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
				Color lineColor(0.8f,0.8f,0.8f);
				if(patternCatalog) {
					Cluster* cluster = segment->burgersVector.cluster();
					OVITO_ASSERT(cluster != nullptr);
					StructurePattern* pattern = patternCatalog->structureById(cluster->structure);
					if(lineColoringMode() == ColorByDislocationType) {
						BurgersVectorFamily* family = pattern->defaultBurgersVectorFamily();
						for(BurgersVectorFamily* f : pattern->burgersVectorFamilies()) {
							if(f->isMember(segment->burgersVector.localVec(), pattern)) {
								family = f;
								break;
							}
						}
						if(family)
							lineColor = family->color();
					}
					else if(lineColoringMode() == ColorByBurgersVector) {
						lineColor = StructurePattern::getBurgersVectorColor(pattern->shortName(), segment->burgersVector.localVec());
					}
				}
				Vector3 normalizedBurgersVector = segment->burgersVector.toSpatialVector();
				normalizedBurgersVector.normalizeSafely();
				clipDislocationLine(segment->line, cellData, dislocationObj->cuttingPlanes(), [this, &lineSegmentIndex, &cornerPoints, &cornerColors, lineColor, lineRadius, &subobjToSegmentMap, &dislocationIndex, lineSegmentCount, normalizedBurgersVector](const Point3& v1, const Point3& v2, bool isInitialSegment) mutable {
					subobjToSegmentMap[lineSegmentIndex] = dislocationIndex;
					if(lineColoringMode() == ColorByCharacter) {
						Vector3 delta = v2 - v1;
						FloatType dot = std::abs(delta.dot(normalizedBurgersVector));
						if(dot != 0) dot /= delta.length();
						if(dot > 1) dot = 1;
						FloatType angle = std::acos(dot) / (FLOATTYPE_PI/2);
						if(angle <= FloatType(0.5))
							lineColor = Color(1, angle * 2, angle * 2);
						else
							lineColor = Color((FloatType(1)-angle) * 2, (FloatType(1)-angle) * 2, 1);
					}
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
				_burgersArrowBuffer->startSetElements(dislocationObj->segments().size());
				subobjToSegmentMap.reserve(subobjToSegmentMap.size() + dislocationObj->segments().size());
				int arrowIndex = 0;
				ColorA arrowColor = burgersVectorColor();
				FloatType arrowRadius = std::max(burgersVectorWidth() / 2, FloatType(0));
				for(DislocationSegment* segment : dislocationObj->segments()) {
					subobjToSegmentMap.push_back(arrowIndex);
					Point3 center = cellData.wrapPoint(segment->getPointOnLine(0.5f));
					Vector3 dir = burgersVectorScaling() * segment->burgersVector.toSpatialVector();
					// Check if arrow is clipped away by cutting planes.
					for(const Plane3& plane : dislocationObj->cuttingPlanes()) {
						if(plane.classifyPoint(center) > 0) {
							dir.setZero(); // Hide arrow by setting length to zero.
							break;
						}
					}
					_burgersArrowBuffer->setElement(arrowIndex++, center, dir, arrowColor, arrowRadius);
				}
			}
			else {
				_burgersArrowBuffer->startSetElements(0);
			}
			_burgersArrowBuffer->endSetElements();

			_pickInfo = new DislocationPickInfo(this, dislocationObj, patternCatalog, std::move(subobjToSegmentMap));

		}
		else {
			_cornerBuffer = nullptr;
			_segmentBuffer = nullptr;
			_burgersArrowBuffer = nullptr;
			_pickInfo = nullptr;
		}
	}

	// Render segments.
	if(_cornerBuffer && _segmentBuffer) {
		renderer->beginPickObject(contextNode, _pickInfo);
		_segmentBuffer->render(renderer);
		_cornerBuffer->render(renderer);

		// Render Burgers vectors.
		if(_burgersArrowBuffer && showBurgersVectors()) {
			_burgersArrowBuffer->render(renderer);
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
	clipDislocationLine(segment->line, cellData, dislocationObj->cuttingPlanes(), [&lineSegments, &cornerVertices](const Point3& v1, const Point3& v2, bool isInitialSegment) {
		lineSegments.push_back({v1,v2});
		if(!isInitialSegment)
			cornerVertices.push_back(v1);
	});

	// Set up transformation.
	TimeInterval iv;
	const AffineTransformation& nodeTM = contextNode->getWorldTransform(time, iv);
	renderer->setWorldTransform(nodeTM);

	// Draw the marker on top of everything.
	renderer->setDepthTestEnabled(false);

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

	// Restore old state.
	renderer->setDepthTestEnabled(true);
}

/******************************************************************************
* Clips a dislocation line at the periodic box boundaries.
******************************************************************************/
void DislocationDisplay::clipDislocationLine(const std::deque<Point3>& line, const SimulationCell& simulationCell, const QVector<Plane3>& clippingPlanes, const std::function<void(const Point3&, const Point3&, bool)>& segmentCallback)
{
	bool isInitialSegment = true;
	auto clippingFunction = [&clippingPlanes, &segmentCallback, &isInitialSegment](Point3 p1, Point3 p2) {
		bool isClipped = false;
		for(const Plane3& plane : clippingPlanes) {
			FloatType c1 = plane.pointDistance(p1);
			FloatType c2 = plane.pointDistance(p2);
			if(c1 >= 0 && c2 >= 0.0) {
				isClipped = true;
				break;
			}
			else if(c1 > FLOATTYPE_EPSILON && c2 < -FLOATTYPE_EPSILON) {
				p1 += (p2 - p1) * (c1 / (c1 - c2));
			}
			else if(c1 < -FLOATTYPE_EPSILON && c2 > FLOATTYPE_EPSILON) {
				p2 += (p1 - p2) * (c2 / (c2 - c1));
			}
		}
		if(!isClipped) {
			segmentCallback(p1, p2, isInitialSegment);
			isInitialSegment = false;
		}
	};

	auto v1 = line.cbegin();
	Point3 rp1 = simulationCell.absoluteToReduced(*v1);
	Vector3 shiftVector = Vector3::Zero();
	for(size_t dim = 0; dim < 3; dim++) {
		if(simulationCell.pbcFlags()[dim]) {
			while(rp1[dim] > 0) { rp1[dim] -= 1; shiftVector[dim] -= 1; }
			while(rp1[dim] < 0) { rp1[dim] += 1; shiftVector[dim] += 1; }
		}
	}
	for(auto v2 = v1 + 1; v2 != line.cend(); v1 = v2, ++v2) {
		Point3 rp2 = simulationCell.absoluteToReduced(*v2) + shiftVector;
		FloatType smallestT;
		bool clippedDimensions[3] = { false, false, false };
		do {
			size_t crossDim;
			FloatType crossDir;
			smallestT = FLOATTYPE_MAX;
			for(size_t dim = 0; dim < 3; dim++) {
				if(simulationCell.pbcFlags()[dim] && !clippedDimensions[dim]) {
					int d = (int)floor(rp2[dim]) - (int)floor(rp1[dim]);
					if(d == 0) continue;
					FloatType t;
					if(d > 0)
						t = (ceil(rp1[dim]) - rp1[dim]) / (rp2[dim] - rp1[dim]);
					else
						t = (floor(rp1[dim]) - rp1[dim]) / (rp2[dim] - rp1[dim]);
					if(t >= 0 && t < smallestT) {
						smallestT = t;
						crossDim = dim;
						crossDir = (d > 0) ? 1 : -1;
					}
				}
			}
			if(smallestT != FLOATTYPE_MAX) {
				clippedDimensions[crossDim] = true;
				Point3 intersection = rp1 + smallestT * (rp2 - rp1);
				intersection[crossDim] = floor(intersection[crossDim] + FloatType(0.5));
				Point3 rp1abs = simulationCell.reducedToAbsolute(rp1);
				Point3 intabs = simulationCell.reducedToAbsolute(intersection);
				if(!intabs.equals(rp1abs)) {
					clippingFunction(rp1abs, intabs);
				}
				shiftVector[crossDim] -= crossDir;
				rp1 = intersection;
				rp1[crossDim] -= crossDir;
				rp2[crossDim] -= crossDir;
				isInitialSegment = true;
			}
		}
		while(smallestT != FLOATTYPE_MAX);

		clippingFunction(simulationCell.reducedToAbsolute(rp1), simulationCell.reducedToAbsolute(rp2));
		rp1 = rp2;
	}
}

/******************************************************************************
* Checks if the given floating point number is integer.
******************************************************************************/
static bool isInteger(FloatType v, int& intPart)
{
	static const FloatType epsilon = FloatType(1e-2);
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
QString DislocationDisplay::formatBurgersVector(const Vector3& b, StructurePattern* structure)
{
	if(structure) {
		if(structure->symmetryType() == StructurePattern::CubicSymmetry) {
			FloatType smallestCompnt = FLOATTYPE_MAX;
			for(int i = 0; i < 3; i++) {
				FloatType c = std::abs(b[i]);
				if(c < smallestCompnt && c > FloatType(1e-3))
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
							if(multiplier != 1)
								return QString("1/%1[%2 %3 %4]")
										.arg(multiplier)
										.arg(bmi.x()).arg(bmi.y()).arg(bmi.z());
							else
								return QString("[%1 %2 %3]")
										.arg(bmi.x()).arg(bmi.y()).arg(bmi.z());
						}
					}
				}
			}
		}
		else if(structure->symmetryType() == StructurePattern::HexagonalSymmetry) {
			// Determine vector components U, V, and W, with b = U*a1 + V*a2 + W*c.
			FloatType U = sqrt(2.0f)*b.x() - sqrt(2.0f/3.0f)*b.y();
			FloatType V = sqrt(2.0f)*b.x() + sqrt(2.0f/3.0f)*b.y();
			FloatType W = sqrt(3.0f/4.0f)*b.z();
			Vector4 uvwt((2*U-V)/3, (2*V-U)/3, -(U+V)/3, W);
			FloatType smallestCompnt = FLOATTYPE_MAX;
			for(int i = 0; i < 4; i++) {
				FloatType c = std::abs(uvwt[i]);
				if(c < smallestCompnt && c > FloatType(1e-3))
					smallestCompnt = c;
			}
			if(smallestCompnt != FLOATTYPE_MAX) {
				FloatType m = FloatType(1) / smallestCompnt;
				for(int f = 1; f <= 11; f++) {
					int multiplier;
					if(!isInteger(m*f, multiplier)) continue;
					if(multiplier < 80) {
						Vector4 bm = uvwt * (FloatType)multiplier;
						int bmi[4];
						if(isInteger(bm.x(),bmi[0]) && isInteger(bm.y(),bmi[1]) && isInteger(bm.z(),bmi[2]) && isInteger(bm.w(),bmi[3])) {
							if(multiplier != 1)
								return QString("1/%1[%2 %3 %4 %5]")
										.arg(multiplier)
										.arg(bmi[0]).arg(bmi[1]).arg(bmi[2]).arg(bmi[3]);
							else
								return QString("[%1 %2 %3 %4]")
										.arg(bmi[0]).arg(bmi[1]).arg(bmi[2]).arg(bmi[3]);
						}
					}
				}
			}
			return QString("[%1 %2 %3 %4]")
					.arg(QLocale::c().toString(uvwt.x(), 'f'), 7)
					.arg(QLocale::c().toString(uvwt.y(), 'f'), 7)
					.arg(QLocale::c().toString(uvwt.z(), 'f'), 7)
					.arg(QLocale::c().toString(uvwt.w(), 'f'), 7);
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
		StructurePattern* structure = nullptr;
		if(patternCatalog() != nullptr) {
			structure = patternCatalog()->structureById(segment->burgersVector.cluster()->structure);
		}
		QString formattedBurgersVector = DislocationDisplay::formatBurgersVector(segment->burgersVector.localVec(), structure);
		str = tr("True Burgers vector: %1").arg(formattedBurgersVector);
		Vector3 transformedVector = segment->burgersVector.toSpatialVector();
		str += tr(" | Spatial Burgers vector: [%1 %2 %3]")
				.arg(QLocale::c().toString(transformedVector.x(), 'f', 4), 7)
				.arg(QLocale::c().toString(transformedVector.y(), 'f', 4), 7)
				.arg(QLocale::c().toString(transformedVector.z(), 'f', 4), 7);
		str += tr(" | Cluster Id: %1").arg(segment->burgersVector.cluster()->id);
		str += tr(" | Segment Id: %1").arg(segment->id);
		if(structure) {
			str += tr(" | Crystal structure: %1").arg(structure->name());
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
