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
#include "ParticlePropertyObject.h"

namespace Ovito { namespace Particles {

/**
 * \brief A scene display object for per-particle vectors.
 */
class OVITO_PARTICLES_EXPORT VectorDisplay : public DisplayObject
{
public:

	/// The position mode for the arrows.
	enum ArrowPosition {
		Base,
		Center,
		Head
	};
	Q_ENUMS(ArrowPosition);

public:

	/// \brief Constructor.
	Q_INVOKABLE VectorDisplay(DataSet* dataset);

	/// \brief Lets the display object render the data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

public:

    Q_PROPERTY(Ovito::ArrowPrimitive::ShadingMode shadingMode READ shadingMode WRITE setShadingMode);
    Q_PROPERTY(Ovito::ArrowPrimitive::RenderingQuality renderingQuality READ renderingQuality WRITE setRenderingQuality);

protected:

	/// Computes the bounding box of the arrows.
	Box3 arrowBoundingBox(ParticlePropertyObject* vectorProperty, ParticlePropertyObject* positionProperty);

    /// Loads the data of this class from an input stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Parses the serialized contents of a property field in a custom way.
	virtual bool loadPropertyFieldFromStream(ObjectLoadStream& stream, const ObjectLoadStream::SerializedPropertyField& serializedField) override;

protected:

	/// Reverses of the arrow pointing direction.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, reverseArrowDirection, setReverseArrowDirection);

	/// Controls how the arrows are positioned relative to the particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ArrowPosition, arrowPosition, setArrowPosition);

	/// Controls the color of the arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, arrowColor, setArrowColor);

	/// Controls the width of the arrows in world units.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, arrowWidth, setArrowWidth);

	/// Controls the scaling factor applied to the vectors.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, scalingFactor, setScalingFactor);

	/// Controls the shading mode for arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ArrowPrimitive::ShadingMode, shadingMode, setShadingMode);

	/// Controls the rendering quality mode for arrows.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(ArrowPrimitive::RenderingQuality, renderingQuality, setRenderingQuality);

	/// The buffered geometry used to render the arrows.
	std::shared_ptr<ArrowPrimitive> _buffer;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	SceneObjectCacheHelper<
		WeakVersionedOORef<ParticlePropertyObject>,			// Vector property + revision number
		WeakVersionedOORef<ParticlePropertyObject>,			// Particle position property + revision number
		FloatType,											// Scaling factor
		FloatType,											// Arrow width
		Color,												// Arrow color
		bool,												// Reverse arrow direction
		ArrowPosition,										// Arrow position
		WeakVersionedOORef<ParticlePropertyObject>			// Vector color property + revision number
		> _geometryCacheHelper;

	/// The bounding box that includes all arrows.
	Box3 _cachedBoundingBox;

	/// This helper structure is used to detect changes in the input
	/// that require recalculating the bounding box.
	SceneObjectCacheHelper<
		WeakVersionedOORef<ParticlePropertyObject>,		// Vector property + revision number
		WeakVersionedOORef<ParticlePropertyObject>,		// Particle position property + revision number
		FloatType,										// Scaling factor
		FloatType										// Arrow width
		> _boundingBoxCacheHelper;

	/// This is for backward compatibility with OVITO 2.6.0.
	bool _flipVectors = false;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Vectors");
};

/**
 * \brief This information record is attached to the arrows by the VectorDisplay when rendering
 * them in the viewports. It facilitates the picking of arrows with the mouse.
 */
class OVITO_PARTICLES_EXPORT VectorPickInfo : public ObjectPickInfo
{
public:

	/// Constructor.
	VectorPickInfo(VectorDisplay* displayObj, const PipelineFlowState& pipelineState, ParticlePropertyObject* vectorProperty) :
		_displayObject(displayObj), _pipelineState(pipelineState), _vectorProperty(vectorProperty) {}

	/// The pipeline flow state containing the particle properties.
	const PipelineFlowState& pipelineState() const { return _pipelineState; }

	/// Returns a human-readable string describing the picked object, which will be displayed in the status bar by OVITO.
	virtual QString infoString(ObjectNode* objectNode, quint32 subobjectId) override;

	/// Given an sub-object ID returned by the Viewport::pick() method, looks up the
	/// corresponding particle index.
	int particleIndexFromSubObjectID(quint32 subobjID) const;

private:

	/// The pipeline flow state containing the particle properties.
	PipelineFlowState _pipelineState;

	/// The display object that rendered the arrows.
	OORef<VectorDisplay> _displayObject;

	/// The vector property.
	OORef<ParticlePropertyObject> _vectorProperty;

	Q_OBJECT
	OVITO_OBJECT
};


}	// End of namespace
}	// End of namespace

Q_DECLARE_METATYPE(Ovito::Particles::VectorDisplay::ArrowPosition);
Q_DECLARE_TYPEINFO(Ovito::Particles::VectorDisplay::ArrowPosition, Q_PRIMITIVE_TYPE);



