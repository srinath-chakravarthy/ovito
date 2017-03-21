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
#include <core/scene/objects/DisplayObject.h>
#include <core/scene/objects/WeakVersionedObjectReference.h>
#include <core/rendering/ParticlePrimitive.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/SceneRenderer.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

class DislocationDisplay;	// defined below

/**
 * \brief This information record is attached to the dislocation segments by the DislocationDisplay when rendering
 * them in the viewports. It facilitates the picking of dislocations with the mouse.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationPickInfo : public ObjectPickInfo
{
public:

	/// Constructor.
	DislocationPickInfo(DislocationDisplay* displayObj, DislocationNetworkObject* dislocationObj, PatternCatalog* patternCatalog, std::vector<int>&& subobjToSegmentMap) :
		_displayObject(displayObj), _dislocationObj(dislocationObj), _patternCatalog(patternCatalog), _subobjToSegmentMap(std::move(subobjToSegmentMap)) {}

	/// The data object containing the dislocations.
	DislocationNetworkObject* dislocationObj() const { return _dislocationObj; }

	/// Returns the display object that rendered the dislocations.
	DislocationDisplay* displayObject() const { return _displayObject; }

	/// Returns the associated pattern catalog.
	PatternCatalog* patternCatalog() const { return _patternCatalog; }

	/// \brief Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding dislocation segment.
	int segmentIndexFromSubObjectID(quint32 subobjID) const {
		if(subobjID < _subobjToSegmentMap.size())
			return _subobjToSegmentMap[subobjID];
		else
			return -1;
	}

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(ObjectNode* objectNode, quint32 subobjectId) override;

private:

	/// The data object containing the dislocations.
	OORef<DislocationNetworkObject> _dislocationObj;

	/// The display object that rendered the dislocations.
	OORef<DislocationDisplay> _displayObject;

	/// The data object containing the lattice structure.
	OORef<PatternCatalog> _patternCatalog;

	/// This array is used to map sub-object picking IDs back to dislocation segments.
	std::vector<int> _subobjToSegmentMap;

	Q_OBJECT
	OVITO_OBJECT
};

/**
 * \brief A display object for the dislocation lines.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationDisplay : public DisplayObject
{
public:

	enum LineColoringMode {
		ColorByDislocationType,
		ColorByBurgersVector,
		ColorByCharacter
	};
	Q_ENUMS(LineColoringMode);

public:

	/// \brief Constructor.
	Q_INVOKABLE DislocationDisplay(DataSet* dataset);

	/// \brief Lets the display object render a data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// \brief Renders an overlay marker for a single dislocation segment.
	void renderOverlayMarker(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, int segmentIndex, SceneRenderer* renderer, ObjectNode* contextNode);

	/// \brief Generates a pretty string representation of a Burgers vector.
	static QString formatBurgersVector(const Vector3& b, StructurePattern* structure);

public:

	Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);

protected:

	/// Clips a dislocation line at the periodic box boundaries.
	void clipDislocationLine(const std::deque<Point3>& line, const SimulationCell& simulationCell, const QVector<Plane3>& clippingPlanes, const std::function<void(const Point3&, const Point3&, bool)>& segmentCallback);

protected:

	/// The geometry buffer used to render the dislocation segments.
	std::shared_ptr<ArrowPrimitive> _segmentBuffer;

	/// The geometry buffer used to render the segment corners.
	std::shared_ptr<ParticlePrimitive> _cornerBuffer;

	/// The buffered geometry used to render the Burgers vectors.
	std::shared_ptr<ArrowPrimitive> _burgersArrowBuffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffers.
	SceneObjectCacheHelper<
		WeakVersionedOORef<DataObject>,		// Source object + revision number
		SimulationCell,						// Simulation cell geometry
		WeakVersionedOORef<PatternCatalog>,	// The pattern catalog
		FloatType,							// Line width
		bool,								// Burgers vector display
		FloatType,							// Burgers vectors scaling
		FloatType,							// Burgers vector width
		Color,								// Burgers vector color
		LineColoringMode					// Way to color lines
		> _geometryCacheHelper;

	/// The cached bounding box.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input
	/// that require recalculating the bounding box.
	SceneObjectCacheHelper<
		WeakVersionedOORef<DataObject>,		// Source object + revision number
		SimulationCell,						// Simulation cell geometry
		FloatType,							// Line width
		bool,								// Burgers vector display
		FloatType,							// Burgers vectors scaling
		FloatType							// Burgers vector width
		> _boundingBoxCacheHelper;

	/// The rendering width for dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, lineWidth, setLineWidth);

	/// The shading mode for dislocation lines.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ArrowPrimitive::ShadingMode, shadingMode, setShadingMode);

	/// The rendering width for Burgers vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, burgersVectorWidth, setBurgersVectorWidth);

	/// The scaling factor Burgers vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, burgersVectorScaling, setBurgersVectorScaling);

	/// Display color for Burgers vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, burgersVectorColor, setBurgersVectorColor);

	/// Controls the display of Burgers vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showBurgersVectors, setShowBurgersVectors);

	/// Controls the display of the line directions.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showLineDirections, setShowLineDirections);

	/// Controls how the display color of dislocation lines is chosen.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(LineColoringMode, lineColoringMode, setLineColoringMode);

	/// The data record used for picking dislocations in the viewports.
	OORef<DislocationPickInfo> _pickInfo;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Dislocations");
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


