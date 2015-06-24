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

#ifndef __OVITO_CA_DISLOCATION_DISPLAY_H
#define __OVITO_CA_DISLOCATION_DISPLAY_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/scene/objects/DisplayObject.h>
#include <core/scene/objects/WeakVersionedObjectReference.h>
#include <core/rendering/ParticlePrimitive.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/SceneRenderer.h>
#include <core/gui/properties/PropertiesEditor.h>
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

	/// \brief Constructor.
	Q_INVOKABLE DislocationDisplay(DataSet* dataset);

	/// \brief Lets the display object render a data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// \brief Returns the line width used for dislocation rendering.
	FloatType lineWidth() const { return _lineWidth; }

	/// \brief Sets the line width used for dislocation rendering.
	void setLineWidth(FloatType width) { _lineWidth = width; }

	/// \brief Returns the selected shading mode for dislocation lines.
	ArrowPrimitive::ShadingMode shadingMode() const { return _shadingMode; }

	/// \brief Sets the shading mode for dislocation lines.
	void setShadingMode(ArrowPrimitive::ShadingMode mode) { _shadingMode = mode; }

	/// Returns the rendering width for Burgers vectors.
	FloatType burgersVectorWidth() const { return _burgersVectorWidth; }

	/// Sets the rendering width for Burgers vectors.
	void setBurgersVectorWidth(FloatType width) { _burgersVectorWidth = width; }

	/// Returns the scaling factor Burgers vectors.
	FloatType burgersVectorScaling() const { return _burgersVectorScaling; }

	/// Sets the scaling factor Burgers vectors.
	void setBurgersVectorScaling(FloatType factor) { _burgersVectorScaling = factor; }

	/// Returns the display color for Burgers vectors.
	const Color& burgersVectorColor() const { return _burgersVectorColor; }

	/// Sets the display color for Burgers vectors.
	void setBurgersVectorColor(const Color& color) { _burgersVectorColor = color; }

	/// Returns whether the display of Burgers vectors is enabled.
	bool showBurgersVectors() const { return _showBurgersVectors; }

	/// Controls the display of Burgers vectors.
	void setShowBurgersVectors(bool enabled) { _showBurgersVectors = enabled; }

	/// \brief Renders an overlay marker for a single dislocation segment.
	void renderOverlayMarker(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, int segmentIndex, SceneRenderer* renderer, ObjectNode* contextNode);

	/// \brief Generates a pretty string representation of a Burgers vector.
	static QString formatBurgersVector(const Vector3& b);

public:

	Q_PROPERTY(FloatType lineWidth READ lineWidth WRITE setLineWidth);
	Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);

protected:

	/// Clips a dislocation line at the periodic box boundaries.
	void clipDislocationLine(const std::deque<Point3>& line, const SimulationCell& simulationCell, const std::function<void(const Point3&, const Point3&, bool)>& segmentCallback);

protected:

	/// The geometry buffer used to render the dislocation segments.
	std::shared_ptr<ArrowPrimitive> _segmentBuffer;

	/// The geometry buffer used to render the segment corners.
	std::shared_ptr<ParticlePrimitive> _cornerBuffer;

	/// The buffered geometry used to render the Burgers vectors.
	std::shared_ptr<ArrowPrimitive> _arrowBuffer;

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
		Color								// Burgers vector color
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
	PropertyField<FloatType> _lineWidth;

	/// The shading mode for dislocation lines.
	PropertyField<ArrowPrimitive::ShadingMode, int> _shadingMode;

	/// The rendering width for Burgers vectors.
	PropertyField<FloatType> _burgersVectorWidth;

	/// The scaling factor Burgers vectors.
	PropertyField<FloatType> _burgersVectorScaling;

	/// Display color for Burgers vectors.
	PropertyField<Color, QColor> _burgersVectorColor;

	/// Controls the display of Burgers vectors.
	PropertyField<bool> _showBurgersVectors;

	/// The data record used for picking dislocations in the viewports.
	OORef<DislocationPickInfo> _pickInfo;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Dislocations");

	DECLARE_PROPERTY_FIELD(_lineWidth);
	DECLARE_PROPERTY_FIELD(_shadingMode);
	DECLARE_PROPERTY_FIELD(_burgersVectorWidth);
	DECLARE_PROPERTY_FIELD(_burgersVectorScaling);
	DECLARE_PROPERTY_FIELD(_burgersVectorColor);
	DECLARE_PROPERTY_FIELD(_showBurgersVectors);
};

/**
 * \brief A properties editor for the DislocationDisplay class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DislocationDisplayEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE DislocationDisplayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CA_DISLOCATION_DISPLAY_H
