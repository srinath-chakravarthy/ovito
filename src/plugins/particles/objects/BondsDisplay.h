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
#include <core/rendering/ArrowPrimitive.h>
#include <core/rendering/SceneRenderer.h>
#include "BondsObject.h"
#include "ParticlePropertyObject.h"
#include "BondPropertyObject.h"
#include "BondTypeProperty.h"
#include "SimulationCellObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief A scene display object for bonds.
 */
class OVITO_PARTICLES_EXPORT BondsDisplay : public DisplayObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE BondsDisplay(DataSet* dataset);

	/// \brief Renders the associated data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the display bounding box of the data object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// Returns the display color used for selected bonds.
	Color selectionBondColor() const { return Color(1,0,0); }

	/// Determines the display colors of bonds.
	void bondColors(std::vector<Color>& output, size_t particleCount, BondsObject* bondsObject,
			BondPropertyObject* bondColorProperty, BondTypeProperty* bondTypeProperty, BondPropertyObject* bondSelectionProperty,
			ParticleDisplay* particleDisplay, ParticlePropertyObject* particleColorProperty, ParticleTypeProperty* particleTypeProperty);

public:

    Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);
    Q_PROPERTY(Ovito::ArrowPrimitive::RenderingQuality renderingQuality READ renderingQuality WRITE setRenderingQuality);

protected:

	/// Controls the display width of bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, bondWidth, setBondWidth);

	/// Controls the color of the bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, bondColor, setBondColor);

	/// Controls whether bonds colors are derived from particle colors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useParticleColors, setUseParticleColors);

	/// Controls the shading mode for bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ArrowPrimitive::ShadingMode, shadingMode, setShadingMode);

	/// Controls the rendering quality mode for bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ArrowPrimitive::RenderingQuality, renderingQuality, setRenderingQuality);

	/// The buffered geometry used to render the bonds.
	std::shared_ptr<ArrowPrimitive> _buffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	SceneObjectCacheHelper<
		WeakVersionedOORef<BondsObject>,				// The bonds data object + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle position property + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle color property + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle type property + revision number
		WeakVersionedOORef<BondPropertyObject>,			// Bond color property + revision number
		WeakVersionedOORef<BondPropertyObject>,			// Bond type property + revision number
		WeakVersionedOORef<BondPropertyObject>,			// Bond selection property + revision number
		WeakVersionedOORef<SimulationCellObject>,		// Simulation cell + revision number
		FloatType,										// Bond width
		Color,											// Bond color
		bool											// Use particle colors
	> _geometryCacheHelper;

	/// The bounding box that includes all bonds.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input data
	/// that require recomputing the bounding box.
	SceneObjectCacheHelper<
		WeakVersionedOORef<BondsObject>,				// The bonds data object + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle position property + revision number
		WeakVersionedOORef<SimulationCellObject>,		// Simulation cell + revision number
		FloatType										// Bond width
	> _boundingBoxCacheHelper;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Bonds");
};

/**
 * \brief This information record is attached to the bonds by the BondsDisplay when rendering
 * them in the viewports. It facilitates the picking of bonds with the mouse.
 */
class OVITO_PARTICLES_EXPORT BondPickInfo : public ObjectPickInfo
{
public:

	/// Constructor.
	BondPickInfo(BondsObject* bondsObj, const PipelineFlowState& pipelineState) : _bondsObj(bondsObj), _pipelineState(pipelineState) {}

	/// The pipeline flow state containing the bonds.
	const PipelineFlowState& pipelineState() const { return _pipelineState; }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(ObjectNode* objectNode, quint32 subobjectId) override;

private:

	/// The pipeline flow state containing the bonds.
	PipelineFlowState _pipelineState;

	/// The bonds data object.
	OORef<BondsObject> _bondsObj;

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace


