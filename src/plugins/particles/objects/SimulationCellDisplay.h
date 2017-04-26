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

#pragma once


#include <plugins/particles/Particles.h>
#include <core/scene/objects/DisplayObject.h>
#include <core/scene/objects/WeakVersionedObjectReference.h>
#include <core/rendering/LinePrimitive.h>
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/ParticlePrimitive.h>
#include "SimulationCellObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief A display object for SimulationCellObject.
 */
class OVITO_PARTICLES_EXPORT SimulationCellDisplay : public DisplayObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE SimulationCellDisplay(DataSet* dataset);

	/// \brief Lets the display object render the data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// \brief Indicates whether this object should be surrounded by a selection marker in the viewports when it is selected.
	virtual bool showSelectionMarker() override { return false; }

protected:

	/// Renders the given simulation using wireframe mode.
	void renderWireframe(SimulationCellObject* cell, SceneRenderer* renderer, ObjectNode* contextNode);

	/// Renders the given simulation using solid shading mode.
	void renderSolid(SimulationCellObject* cell, SceneRenderer* renderer, ObjectNode* contextNode);

protected:

	/// Controls the line width used to render the simulation cell.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, cellLineWidth, setCellLineWidth);

	/// Controls whether the simulation cell is visible.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, renderCellEnabled, setRenderCellEnabled);

	/// Controls the rendering color of the simulation cell.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, cellColor, setCellColor);

	/// The geometry buffer used to render the simulation cell in wireframe mode.
	std::shared_ptr<LinePrimitive> _wireframeGeometry;

	/// The geometry buffer used to render the wireframe simulation cell in object picking mode.
	std::shared_ptr<LinePrimitive> _wireframePickingGeometry;

	/// This helper structure is used to detect any changes in the input simulation cell
	/// that require updating the display geometry buffer for wireframe rendering.
	SceneObjectCacheHelper<
		WeakVersionedOORef<SimulationCellObject>,
		ColorA
		> _wireframeGeometryCacheHelper;

	/// The geometry buffer used to render the edges of the cell.
	std::shared_ptr<ArrowPrimitive> _edgeGeometry;

	/// The geometry buffer used to render the corners of the cell.
	std::shared_ptr<ParticlePrimitive> _cornerGeometry;

	/// This helper structure is used to detect any changes in the input simulation cell
	/// that require updating the display geometry buffer for solid rendering mode.
	SceneObjectCacheHelper<
		WeakVersionedOORef<SimulationCellObject>,			// The simulation cell + revision number
		FloatType, Color							// Line width + color
		> _solidGeometryCacheHelper;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Simulation cell");
};

}	// End of namespace
}	// End of namespace


