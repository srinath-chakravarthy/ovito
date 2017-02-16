///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/rendering/SceneRenderer.h>
#include "ParticleDisplay.h"
#include "ParticleTypeProperty.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(ParticleDisplay, DisplayObject);
IMPLEMENT_OVITO_OBJECT(ParticlePickInfo, ObjectPickInfo);
DEFINE_FLAGS_PROPERTY_FIELD(ParticleDisplay, defaultParticleRadius, "DefaultParticleRadius", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(ParticleDisplay, renderingQuality, "RenderingQuality");
DEFINE_PROPERTY_FIELD(ParticleDisplay, particleShape, "ParticleShape");
SET_PROPERTY_FIELD_LABEL(ParticleDisplay, defaultParticleRadius, "Default particle radius");
SET_PROPERTY_FIELD_LABEL(ParticleDisplay, renderingQuality, "Rendering quality");
SET_PROPERTY_FIELD_LABEL(ParticleDisplay, particleShape, "Shape");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(ParticleDisplay, defaultParticleRadius, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
ParticleDisplay::ParticleDisplay(DataSet* dataset) : DisplayObject(dataset),
	_defaultParticleRadius(1.2),
	_renderingQuality(ParticlePrimitive::AutoQuality),
	_particleShape(Sphere)
{
	INIT_PROPERTY_FIELD(defaultParticleRadius);
	INIT_PROPERTY_FIELD(renderingQuality);
	INIT_PROPERTY_FIELD(particleShape);
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 ParticleDisplay::boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState)
{
	ParticlePropertyObject* positionProperty = dynamic_object_cast<ParticlePropertyObject>(dataObject);
	ParticlePropertyObject* radiusProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::RadiusProperty);
	ParticleTypeProperty* typeProperty = dynamic_object_cast<ParticleTypeProperty>(ParticlePropertyObject::findInState(flowState, ParticleProperty::ParticleTypeProperty));
	ParticlePropertyObject* shapeProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::AsphericalShapeProperty);

	// Detect if the input data has changed since the last time we computed the bounding box.
	if(_boundingBoxCacheHelper.updateState(
			positionProperty,
			radiusProperty,
			typeProperty,
			shapeProperty,
			defaultParticleRadius()) || _cachedBoundingBox.isEmpty()) {
		// Recompute bounding box.
		_cachedBoundingBox = particleBoundingBox(positionProperty, typeProperty, radiusProperty, shapeProperty);
	}
	return _cachedBoundingBox;
}

/******************************************************************************
* Computes the bounding box of the particles.
******************************************************************************/
Box3 ParticleDisplay::particleBoundingBox(ParticlePropertyObject* positionProperty, ParticleTypeProperty* typeProperty, ParticlePropertyObject* radiusProperty, ParticlePropertyObject* shapeProperty, bool includeParticleRadius)
{
	OVITO_ASSERT(positionProperty == nullptr || positionProperty->type() == ParticleProperty::PositionProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::ParticleTypeProperty);
	OVITO_ASSERT(radiusProperty == nullptr || radiusProperty->type() == ParticleProperty::RadiusProperty);
	OVITO_ASSERT(shapeProperty == nullptr || shapeProperty->type() == ParticleProperty::AsphericalShapeProperty);
	if(particleShape() != Sphere && particleShape() != Box && particleShape() != Cylinder && particleShape() != Spherocylinder)
		shapeProperty = nullptr;

	Box3 bbox;
	if(positionProperty) {
		bbox.addPoints(positionProperty->constDataPoint3(), positionProperty->size());
	}
	if(!includeParticleRadius)
		return bbox;

	// Extend box to account for radii/shape of particles.
	FloatType maxAtomRadius = defaultParticleRadius();
	if(typeProperty) {
		for(const auto& it : typeProperty->radiusMap()) {
			maxAtomRadius = std::max(maxAtomRadius, it.second);
		}
	}
	if(shapeProperty) {
		for(const Vector3& s : shapeProperty->constVector3Range())
			maxAtomRadius = std::max(maxAtomRadius, std::max(s.x(), std::max(s.y(), s.z())));
		if(particleShape() == Spherocylinder)
			maxAtomRadius *= 2;
	}
	if(radiusProperty && radiusProperty->size() > 0) {
		auto minmax = std::minmax_element(radiusProperty->constDataFloat(), radiusProperty->constDataFloat() + radiusProperty->size());
		if(*minmax.first <= 0)
			maxAtomRadius = std::max(maxAtomRadius, *minmax.second);
		else
			maxAtomRadius = *minmax.second;
	}

	// Extend the bounding box by the largest particle radius.
	return bbox.padBox(std::max(maxAtomRadius * (FloatType)sqrt(3), FloatType(0)));
}

/******************************************************************************
* Determines the display particle colors.
******************************************************************************/
void ParticleDisplay::particleColors(std::vector<Color>& output, ParticlePropertyObject* colorProperty, ParticleTypeProperty* typeProperty, ParticlePropertyObject* selectionProperty)
{
	OVITO_ASSERT(colorProperty == nullptr || colorProperty->type() == ParticleProperty::ColorProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::ParticleTypeProperty);
	OVITO_ASSERT(selectionProperty == nullptr || selectionProperty->type() == ParticleProperty::SelectionProperty);

	Color defaultColor = defaultParticleColor();
	if(colorProperty && colorProperty->size() == output.size()) {
		// Take particle colors directly from the color property.
		std::copy(colorProperty->constDataColor(), colorProperty->constDataColor() + output.size(), output.begin());
	}
	else if(typeProperty && typeProperty->size() == output.size()) {
		// Assign colors based on particle types.
		// Generate a lookup map for particle type colors.
		const std::map<int,Color> colorMap = typeProperty->colorMap();
		std::array<Color,16> colorArray;
		// Check if all type IDs are within a small, non-negative range.
		// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
		if(std::all_of(colorMap.begin(), colorMap.end(),
				[&colorArray](const std::map<int,Color>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
			colorArray.fill(defaultColor);
			for(const auto& entry : colorMap)
				colorArray[entry.first] = entry.second;
			// Fill color array.
			const int* t = typeProperty->constDataInt();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				if(*t >= 0 && *t < (int)colorArray.size())
					*c = colorArray[*t];
				else
					*c = defaultColor;
			}
		}
		else {
			// Fill color array.
			const int* t = typeProperty->constDataInt();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				auto it = colorMap.find(*t);
				if(it != colorMap.end())
					*c = it->second;
				else
					*c = defaultColor;
			}
		}
	}
	else {
		// Assign a uniform color to all particles.
		std::fill(output.begin(), output.end(), defaultColor);
	}

	// Highlight selected particles.
	if(selectionProperty && selectionProperty->size() == output.size()) {
		const Color selColor = selectionParticleColor();
		const int* t = selectionProperty->constDataInt();
		for(auto c = output.begin(); c != output.end(); ++c, ++t) {
			if(*t)
				*c = selColor;
		}
	}
}

/******************************************************************************
* Determines the display particle radii.
******************************************************************************/
void ParticleDisplay::particleRadii(std::vector<FloatType>& output, ParticlePropertyObject* radiusProperty, ParticleTypeProperty* typeProperty)
{
	OVITO_ASSERT(radiusProperty == nullptr || radiusProperty->type() == ParticleProperty::RadiusProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::ParticleTypeProperty);

	FloatType defaultRadius = defaultParticleRadius();
	if(radiusProperty && radiusProperty->size() == output.size()) {
		// Take particle radii directly from the radius property.
		std::transform(radiusProperty->constDataFloat(), radiusProperty->constDataFloat() + output.size(), 
			output.begin(), [defaultRadius](FloatType r) { return r > 0 ? r : defaultRadius; } );
	}
	else if(typeProperty && typeProperty->size() == output.size()) {
		// Assign radii based on particle types.
		// Build a lookup map for particle type radii.
		const std::map<int,FloatType> radiusMap = typeProperty->radiusMap();
		// Skip the following loop if all per-type radii are zero. In this case, simply use the default radius for all particles.
		if(std::any_of(radiusMap.cbegin(), radiusMap.cend(), [](const std::pair<int,FloatType>& it) { return it.second != 0; })) {
			// Fill radius array.
			const int* t = typeProperty->constDataInt();
			for(auto c = output.begin(); c != output.end(); ++c, ++t) {
				auto it = radiusMap.find(*t);
				// Set particle radius only if the type's radius is non-zero.
				if(it != radiusMap.end() && it->second != 0)
					*c = it->second;
			}
		}
		else {
			// Assign a uniform radius to all particles.
			std::fill(output.begin(), output.end(), defaultRadius);
		}
	}
	else {
		// Assign a uniform radius to all particles.
		std::fill(output.begin(), output.end(), defaultRadius);
	}
}

/******************************************************************************
* Determines the display radius of a single particle.
******************************************************************************/
FloatType ParticleDisplay::particleRadius(size_t particleIndex, ParticlePropertyObject* radiusProperty, ParticleTypeProperty* typeProperty)
{
	OVITO_ASSERT(radiusProperty == nullptr || radiusProperty->type() == ParticleProperty::RadiusProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::ParticleTypeProperty);

	if(radiusProperty && radiusProperty->size() > particleIndex) {
		// Take particle radius directly from the radius property.
		FloatType r = radiusProperty->getFloat(particleIndex);
		if(r > 0) return r;
	}
	else if(typeProperty && typeProperty->size() > particleIndex) {
		// Assign radius based on particle types.
		ParticleType* ptype = typeProperty->particleType(typeProperty->getInt(particleIndex));
		if(ptype && ptype->radius() > 0)
			return ptype->radius();
	}

	return defaultParticleRadius();
}

/******************************************************************************
* Determines the display color of a single particle.
******************************************************************************/
ColorA ParticleDisplay::particleColor(size_t particleIndex, ParticlePropertyObject* colorProperty, ParticleTypeProperty* typeProperty, ParticlePropertyObject* selectionProperty, ParticlePropertyObject* transparencyProperty)
{
	OVITO_ASSERT(colorProperty == nullptr || colorProperty->type() == ParticleProperty::ColorProperty);
	OVITO_ASSERT(typeProperty == nullptr || typeProperty->type() == ParticleProperty::ParticleTypeProperty);
	OVITO_ASSERT(selectionProperty == nullptr || selectionProperty->type() == ParticleProperty::SelectionProperty);
	OVITO_ASSERT(transparencyProperty == nullptr || transparencyProperty->type() == ParticleProperty::TransparencyProperty);

	// Check if particle is selected.
	if(selectionProperty && selectionProperty->size() > particleIndex) {
		if(selectionProperty->getInt(particleIndex))
			return selectionParticleColor();
	}

	ColorA c = defaultParticleColor();
	if(colorProperty && colorProperty->size() > particleIndex) {
		// Take particle color directly from the color property.
		c = colorProperty->getColor(particleIndex);
	}
	else if(typeProperty && typeProperty->size() > particleIndex) {
		// Return color based on particle types.
		ParticleType* ptype = typeProperty->particleType(typeProperty->getInt(particleIndex));
		if(ptype)
			c = ptype->color();
	}

	// Apply alpha component.
	if(transparencyProperty && transparencyProperty->size() > particleIndex) {
		c.a() = FloatType(1) - transparencyProperty->getFloat(particleIndex);
	}

	return c;
}

/******************************************************************************
* Returns the actual rendering quality used to render the particles.
******************************************************************************/
ParticlePrimitive::RenderingQuality ParticleDisplay::effectiveRenderingQuality(SceneRenderer* renderer, ParticlePropertyObject* positionProperty) const
{
	ParticlePrimitive::RenderingQuality renderQuality = renderingQuality();
	if(renderQuality == ParticlePrimitive::AutoQuality) {
		if(!positionProperty) return ParticlePrimitive::HighQuality;
		size_t particleCount = positionProperty->size();
		if(particleCount < 4000 || renderer->isInteractive() == false)
			renderQuality = ParticlePrimitive::HighQuality;
		else if(particleCount < 400000)
			renderQuality = ParticlePrimitive::MediumQuality;
		else
			renderQuality = ParticlePrimitive::LowQuality;
	}
	return renderQuality;
}

/******************************************************************************
* Returns the actual particle shape used to render the particles.
******************************************************************************/
ParticlePrimitive::ParticleShape ParticleDisplay::effectiveParticleShape(ParticlePropertyObject* shapeProperty, ParticlePropertyObject* orientationProperty) const
{
	if(particleShape() == Sphere) {
		if(shapeProperty != nullptr) return ParticlePrimitive::EllipsoidShape;
		else return ParticlePrimitive::SphericalShape;
	}
	else if(particleShape() == Box) {
		if(shapeProperty != nullptr || orientationProperty != nullptr) return ParticlePrimitive::BoxShape;
		else return ParticlePrimitive::SquareShape;
	}
	else if(particleShape() == Circle) {
		return ParticlePrimitive::SphericalShape;
	}
	else if(particleShape() == Square) {
		return ParticlePrimitive::SquareShape;
	}
	else {
		OVITO_ASSERT(false);
		return ParticlePrimitive::SphericalShape;
	}
}

/******************************************************************************
* Lets the display object render the data object.
******************************************************************************/
void ParticleDisplay::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode)
{
	// Get input data.
	ParticlePropertyObject* positionProperty = dynamic_object_cast<ParticlePropertyObject>(dataObject);
	ParticlePropertyObject* radiusProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::RadiusProperty);
	ParticlePropertyObject* colorProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::ColorProperty);
	ParticleTypeProperty* typeProperty = dynamic_object_cast<ParticleTypeProperty>(ParticlePropertyObject::findInState(flowState, ParticleProperty::ParticleTypeProperty));
	ParticlePropertyObject* selectionProperty = renderer->isInteractive() ? ParticlePropertyObject::findInState(flowState, ParticleProperty::SelectionProperty) : nullptr;
	ParticlePropertyObject* transparencyProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::TransparencyProperty);
	ParticlePropertyObject* shapeProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::AsphericalShapeProperty);
	ParticlePropertyObject* orientationProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::OrientationProperty);
	if(particleShape() != Sphere && particleShape() != Box && particleShape() != Cylinder && particleShape() != Spherocylinder) {
		shapeProperty = nullptr;
		orientationProperty = nullptr;
	}
	if(particleShape() == Sphere && shapeProperty == nullptr)
		orientationProperty = nullptr;

	// Get number of particles.
	int particleCount = positionProperty ? (int)positionProperty->size() : 0;

	if(particleShape() != Cylinder && particleShape() != Spherocylinder) {

		// Not rendering any cylinder primitives.
		_cylinderBuffer.reset();
		_spherocylinderBuffer.reset();

		// Do we have to re-create the geometry buffer from scratch?
		bool recreateBuffer = !_particleBuffer || !_particleBuffer->isValid(renderer);

		// If rendering quality is set to automatic, pick quality level based on number of particles.
		ParticlePrimitive::RenderingQuality renderQuality = effectiveRenderingQuality(renderer, positionProperty);

		// Determine primitive particle shape and shading mode.
		ParticlePrimitive::ParticleShape primitiveParticleShape = effectiveParticleShape(shapeProperty, orientationProperty);
		ParticlePrimitive::ShadingMode primitiveShadingMode = ParticlePrimitive::NormalShading;
		if(particleShape() == Circle || particleShape() == Square)
			primitiveShadingMode = ParticlePrimitive::FlatShading;

		// Set primitive shading mode, shape, and rendering quality.
		if(!recreateBuffer) {
			recreateBuffer |= !(_particleBuffer->setShadingMode(primitiveShadingMode));
			recreateBuffer |= !(_particleBuffer->setRenderingQuality(renderQuality));
			recreateBuffer |= !(_particleBuffer->setParticleShape(primitiveParticleShape));
			recreateBuffer |= ((transparencyProperty != nullptr) != _particleBuffer->translucentParticles());
		}

		// Do we have to resize the render buffer?
		bool resizeBuffer = recreateBuffer || (_particleBuffer->particleCount() != particleCount);

		// Do we have to update the particle positions in the render buffer?
		bool updatePositions = _positionsCacheHelper.updateState(positionProperty)
				|| resizeBuffer;

		// Do we have to update the particle radii in the geometry buffer?
		bool updateRadii = _radiiCacheHelper.updateState(
				radiusProperty,
				typeProperty,
				defaultParticleRadius())
				|| resizeBuffer;

		// Do we have to update the particle colors in the geometry buffer?
		bool updateColors = _colorsCacheHelper.updateState(
				colorProperty,
				typeProperty,
				selectionProperty,
				transparencyProperty,
				positionProperty) || resizeBuffer;

		// Do we have to update the particle shapes in the geometry buffer?
		bool updateShapes = _shapesCacheHelper.updateState(
				shapeProperty, orientationProperty) || resizeBuffer;

		// Re-create the geometry buffer if necessary.
		if(recreateBuffer)
			_particleBuffer = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape, transparencyProperty != nullptr);

		// Re-size the geometry buffer if necessary.
		if(resizeBuffer)
			_particleBuffer->setSize(particleCount);

		// Update position buffer.
		if(updatePositions && positionProperty) {
			OVITO_ASSERT(positionProperty->size() == particleCount);
			_particleBuffer->setParticlePositions(positionProperty->constDataPoint3());
		}

		// Update radius buffer.
		if(updateRadii && particleCount) {
			if(radiusProperty && radiusProperty->size() == particleCount) {
				// Allocate memory buffer.
				std::vector<FloatType> particleRadii(particleCount);
				FloatType defaultRadius = defaultParticleRadius();
				std::transform(radiusProperty->constDataFloat(), radiusProperty->constDataFloat() + particleCount, 
					particleRadii.begin(), [defaultRadius](FloatType r) { return r > 0 ? r : defaultRadius; } );
				_particleBuffer->setParticleRadii(particleRadii.data());
			}
			else if(typeProperty && typeProperty->size() == particleCount) {
				// Assign radii based on particle types.
				// Build a lookup map for particle type radii.
				const std::map<int,FloatType> radiusMap = typeProperty->radiusMap();
				// Skip the following loop if all per-type radii are zero. In this case, simply use the default radius for all particles.
				if(std::any_of(radiusMap.cbegin(), radiusMap.cend(), [](const std::pair<int,FloatType>& it) { return it.second != 0; })) {
					// Allocate memory buffer.
					std::vector<FloatType> particleRadii(particleCount, defaultParticleRadius());
					// Fill radius array.
					const int* t = typeProperty->constDataInt();
					for(auto c = particleRadii.begin(); c != particleRadii.end(); ++c, ++t) {
						auto it = radiusMap.find(*t);
						// Set particle radius only if the type's radius is non-zero.
						if(it != radiusMap.end() && it->second != 0)
							*c = it->second;
					}
					_particleBuffer->setParticleRadii(particleRadii.data());
				}
				else {
					// Assign a constant radius to all particles.
					_particleBuffer->setParticleRadius(defaultParticleRadius());
				}
			}
			else {
				// Assign a constant radius to all particles.
				_particleBuffer->setParticleRadius(defaultParticleRadius());
			}
		}

		// Update color buffer.
		if(updateColors && particleCount) {
			if(colorProperty && !selectionProperty && !transparencyProperty && colorProperty->size() == particleCount) {
				// Direct particle colors.
				_particleBuffer->setParticleColors(colorProperty->constDataColor());
			}
			else {
				std::vector<Color> colors(particleCount);
				particleColors(colors, colorProperty, typeProperty, selectionProperty);
				if(!transparencyProperty || transparencyProperty->size() != particleCount) {
					_particleBuffer->setParticleColors(colors.data());
				}
				else {
					// Add alpha channel based on transparency particle property.
					std::vector<ColorA> colorsWithAlpha(particleCount);
					const FloatType* t = transparencyProperty->constDataFloat();
					auto c_in = colors.cbegin();
					for(auto c_out = colorsWithAlpha.begin(); c_out != colorsWithAlpha.end(); ++c_out, ++c_in, ++t) {
						c_out->r() = c_in->r();
						c_out->g() = c_in->g();
						c_out->b() = c_in->b();
						c_out->a() = FloatType(1) - (*t);
					}
					_particleBuffer->setParticleColors(colorsWithAlpha.data());
				}
			}
		}

		// Update shapes and orientation buffer.
		if(updateShapes && particleCount) {
			if(shapeProperty && shapeProperty->size() == particleCount)
				_particleBuffer->setParticleShapes(shapeProperty->constDataVector3());
			else
				_particleBuffer->clearParticleShapes();

			if(orientationProperty && orientationProperty->size() == particleCount)
				_particleBuffer->setParticleOrientations(orientationProperty->constDataQuaternion());
			else
				_particleBuffer->clearParticleOrientations();
		}

		if(renderer->isPicking()) {
			OORef<ParticlePickInfo> pickInfo(new ParticlePickInfo(this, flowState, particleCount));
			renderer->beginPickObject(contextNode, pickInfo);
		}

		_particleBuffer->render(renderer);

		if(renderer->isPicking()) {
			renderer->endPickObject();
		}
	}
	else {
		// Rendering cylindrical and spherocylindrical particles.

		// Not rendering point-like particles.
		_particleBuffer.reset();

		// Do we have to re-create the cylinder geometry buffer?
		bool recreateCylinderBuffer = !_cylinderBuffer || !_cylinderBuffer->isValid(renderer);
		if(!recreateCylinderBuffer) {
			recreateCylinderBuffer |= !(_cylinderBuffer->setShadingMode(ArrowPrimitive::NormalShading));
			recreateCylinderBuffer |= !(_cylinderBuffer->setRenderingQuality(ArrowPrimitive::HighQuality));
			recreateCylinderBuffer |= (_cylinderBuffer->shape() != ArrowPrimitive::CylinderShape);
			recreateCylinderBuffer |= (_cylinderBuffer->elementCount() != particleCount);
		}
		if(recreateCylinderBuffer) {
			_cylinderBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);
		}

		if(particleShape() == Spherocylinder) {
			// Do we have to re-create the particle geometry buffer?
			bool recreateParticleBuffer = !_spherocylinderBuffer ||
											!_spherocylinderBuffer->isValid(renderer) ||
											(_spherocylinderBuffer->particleCount() != particleCount * 2);
			if(recreateParticleBuffer) {
				_spherocylinderBuffer = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape, false);
				_spherocylinderBuffer->setSize(particleCount * 2);
				recreateCylinderBuffer = true;
			}
		}
		else {
			_spherocylinderBuffer.reset();
		}

		if(_cylinderCacheHelper.updateState(positionProperty, typeProperty, selectionProperty,
				colorProperty, shapeProperty, orientationProperty,
				defaultParticleRadius()) || recreateCylinderBuffer) {

			// Determine cylinder colors.
			std::vector<Color> colors(particleCount);
			particleColors(colors, colorProperty, typeProperty, selectionProperty);

			std::vector<Point3> sphereCapPositions;
			std::vector<FloatType> sphereRadii;
			std::vector<Color> sphereColors;
			if(_spherocylinderBuffer) {
				sphereCapPositions.resize(particleCount * 2);
				sphereRadii.resize(particleCount * 2);
				sphereColors.resize(particleCount * 2);
			}

			// Fill cylinder buffer.
			_cylinderBuffer->startSetElements(particleCount);
			for(int index = 0; index < particleCount; index++) {
				const Point3& center = positionProperty->getPoint3(index);
				FloatType radius, length;
				if(shapeProperty) {
					radius = std::abs(shapeProperty->getVector3(index).x());
					length = shapeProperty->getVector3(index).z();
				}
				else {
					radius = defaultParticleRadius();
					length = radius * 2;
				}
				Vector3 dir = Vector3(0, 0, length);
				if(orientationProperty) {
					const Quaternion& q = orientationProperty->getQuaternion(index);
					dir = q * dir;
				}
				Point3 p = center - (dir * FloatType(0.5));
				if(_spherocylinderBuffer) {
					sphereCapPositions[index*2] = p;
					sphereCapPositions[index*2+1] = p + dir;
					sphereRadii[index*2] = sphereRadii[index*2+1] = radius;
					sphereColors[index*2] = sphereColors[index*2+1] = colors[index];
				}
				_cylinderBuffer->setElement(index, p, dir, (ColorA)colors[index], radius);
			}
			_cylinderBuffer->endSetElements();

			// Fill geometry buffer for spherical caps of spherocylinders.
			if(_spherocylinderBuffer) {
				_spherocylinderBuffer->setSize(particleCount * 2);
				_spherocylinderBuffer->setParticlePositions(sphereCapPositions.data());
				_spherocylinderBuffer->setParticleRadii(sphereRadii.data());
				_spherocylinderBuffer->setParticleColors(sphereColors.data());
			}
		}

		if(renderer->isPicking()) {
			OORef<ParticlePickInfo> pickInfo(new ParticlePickInfo(this, flowState, particleCount));
			renderer->beginPickObject(contextNode, pickInfo);
		}
		_cylinderBuffer->render(renderer);
		if(_spherocylinderBuffer)
			_spherocylinderBuffer->render(renderer);
		if(renderer->isPicking()) {
			renderer->endPickObject();
		}
	}
}

/******************************************************************************
* Render a marker around a particle to highlight it in the viewports.
******************************************************************************/
void ParticleDisplay::highlightParticle(int particleIndex, const PipelineFlowState& flowState, SceneRenderer* renderer)
{
	// Fetch properties of selected particle which are needed to render the overlay.
	ParticlePropertyObject* posProperty = nullptr;
	ParticlePropertyObject* radiusProperty = nullptr;
	ParticlePropertyObject* colorProperty = nullptr;
	ParticlePropertyObject* selectionProperty = nullptr;
	ParticlePropertyObject* transparencyProperty = nullptr;
	ParticlePropertyObject* shapeProperty = nullptr;
	ParticlePropertyObject* orientationProperty = nullptr;
	ParticleTypeProperty* typeProperty = nullptr;
	for(DataObject* dataObj : flowState.objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(dataObj);
		if(!property) continue;
		if(property->type() == ParticleProperty::PositionProperty && property->size() >= particleIndex)
			posProperty = property;
		else if(property->type() == ParticleProperty::RadiusProperty && property->size() >= particleIndex)
			radiusProperty = property;
		else if(property->type() == ParticleProperty::ParticleTypeProperty && property->size() >= particleIndex)
			typeProperty = dynamic_object_cast<ParticleTypeProperty>(property);
		else if(property->type() == ParticleProperty::ColorProperty && property->size() >= particleIndex)
			colorProperty = property;
		else if(property->type() == ParticleProperty::SelectionProperty && property->size() >= particleIndex)
			selectionProperty = property;
		else if(property->type() == ParticleProperty::TransparencyProperty && property->size() >= particleIndex)
			transparencyProperty = property;
		else if(property->type() == ParticleProperty::AsphericalShapeProperty && property->size() >= particleIndex)
			shapeProperty = property;
		else if(property->type() == ParticleProperty::OrientationProperty && property->size() >= particleIndex)
			orientationProperty = property;
	}
	if(!posProperty || particleIndex >= posProperty->size())
		return;

	// Determine position of selected particle.
	Point3 pos = posProperty->getPoint3(particleIndex);

	// Determine radius of selected particle.
	FloatType radius = particleRadius(particleIndex, radiusProperty, typeProperty);

	// Determine the display color of selected particle.
	ColorA color = particleColor(particleIndex, colorProperty, typeProperty, selectionProperty, transparencyProperty);
	ColorA highlightColor = selectionParticleColor();
	color = color * FloatType(0.5) + highlightColor * FloatType(0.5);

	// Determine rendering quality used to render the particles.
	ParticlePrimitive::RenderingQuality renderQuality = effectiveRenderingQuality(renderer, posProperty);

	std::shared_ptr<ParticlePrimitive> particleBuffer;
	std::shared_ptr<ParticlePrimitive> highlightParticleBuffer;
	std::shared_ptr<ArrowPrimitive> cylinderBuffer;
	std::shared_ptr<ArrowPrimitive> highlightCylinderBuffer;
	if(particleShape() != Cylinder && particleShape() != Spherocylinder) {
		// Determine effective particle shape and shading mode.
		ParticlePrimitive::ParticleShape primitiveParticleShape = effectiveParticleShape(shapeProperty, orientationProperty);
		ParticlePrimitive::ShadingMode primitiveShadingMode = ParticlePrimitive::NormalShading;
		if(particleShape() == ParticleDisplay::Circle || particleShape() == ParticleDisplay::Square)
			primitiveShadingMode = ParticlePrimitive::FlatShading;

		particleBuffer = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape, false);
		particleBuffer->setSize(1);
		particleBuffer->setParticleColor(color);
		particleBuffer->setParticlePositions(&pos);
		particleBuffer->setParticleRadius(radius);
		if(shapeProperty)
			particleBuffer->setParticleShapes(shapeProperty->constDataVector3() + particleIndex);
		if(orientationProperty)
			particleBuffer->setParticleOrientations(orientationProperty->constDataQuaternion() + particleIndex);

		// Prepare marker geometry buffer.
		highlightParticleBuffer = renderer->createParticlePrimitive(primitiveShadingMode, renderQuality, primitiveParticleShape, false);
		highlightParticleBuffer->setSize(1);
		highlightParticleBuffer->setParticleColor(highlightColor);
		highlightParticleBuffer->setParticlePositions(&pos);
		highlightParticleBuffer->setParticleRadius(radius + renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1));
		if(shapeProperty) {
			Vector3 shape = shapeProperty->getVector3(particleIndex);
			shape += Vector3(renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1));
			highlightParticleBuffer->setParticleShapes(&shape);
		}
		if(orientationProperty)
			highlightParticleBuffer->setParticleOrientations(orientationProperty->constDataQuaternion() + particleIndex);
	}
	else {
		FloatType radius, length;
		if(shapeProperty) {
			radius = std::abs(shapeProperty->getVector3(particleIndex).x());
			length = shapeProperty->getVector3(particleIndex).z();
		}
		else {
			radius = defaultParticleRadius();
			length = radius * 2;
		}
		Vector3 dir = Vector3(0, 0, length);
		if(orientationProperty) {
			const Quaternion& q = orientationProperty->getQuaternion(particleIndex);
			dir = q * dir;
		}
		Point3 p = pos - (dir * FloatType(0.5));
		cylinderBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);
		highlightCylinderBuffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, ArrowPrimitive::NormalShading, ArrowPrimitive::HighQuality);
		cylinderBuffer->startSetElements(1);
		cylinderBuffer->setElement(0, p, dir, (ColorA)color, radius);
		cylinderBuffer->endSetElements();
		FloatType padding = renderer->viewport()->nonScalingSize(renderer->worldTransform() * pos) * FloatType(1e-1);
		highlightCylinderBuffer->startSetElements(1);
		highlightCylinderBuffer->setElement(0, p, dir, highlightColor, radius + padding);
		highlightCylinderBuffer->endSetElements();
		if(particleShape() == Spherocylinder) {
			particleBuffer = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape, false);
			particleBuffer->setSize(2);
			highlightParticleBuffer = renderer->createParticlePrimitive(ParticlePrimitive::NormalShading, ParticlePrimitive::HighQuality, ParticlePrimitive::SphericalShape, false);
			highlightParticleBuffer->setSize(2);
			Point3 sphereCapPositions[2] = {p, p + dir};
			FloatType sphereRadii[2] = {radius, radius};
			FloatType sphereHighlightRadii[2] = {radius + padding, radius + padding};
			Color sphereColors[2] = {(Color)color, (Color)color};
			particleBuffer->setParticlePositions(sphereCapPositions);
			particleBuffer->setParticleRadii(sphereRadii);
			particleBuffer->setParticleColors(sphereColors);
			highlightParticleBuffer->setParticlePositions(sphereCapPositions);
			highlightParticleBuffer->setParticleRadii(sphereHighlightRadii);
			highlightParticleBuffer->setParticleColor(highlightColor);
		}
	}

	renderer->setHighlightMode(1);
	if(particleBuffer)
		particleBuffer->render(renderer);
	if(cylinderBuffer)
		cylinderBuffer->render(renderer);
	renderer->setHighlightMode(2);
	if(highlightParticleBuffer)
		highlightParticleBuffer->render(renderer);
	if(highlightCylinderBuffer)
		highlightCylinderBuffer->render(renderer);
	renderer->setHighlightMode(0);
}

/******************************************************************************
* Compute the (local) bounding box of the marker around a particle used to highlight it in the viewports.
******************************************************************************/
Box3 ParticleDisplay::highlightParticleBoundingBox(int particleIndex, const PipelineFlowState& flowState, const AffineTransformation& tm, Viewport* viewport)
{
	// Fetch properties of selected particle needed to compute the bounding box.
	ParticlePropertyObject* posProperty = nullptr;
	ParticlePropertyObject* radiusProperty = nullptr;
	ParticlePropertyObject* shapeProperty = nullptr;
	ParticleTypeProperty* typeProperty = nullptr;
	for(DataObject* dataObj : flowState.objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(dataObj);
		if(!property) continue;
		if(property->type() == ParticleProperty::PositionProperty && property->size() >= particleIndex)
			posProperty = property;
		else if(property->type() == ParticleProperty::RadiusProperty && property->size() >= particleIndex)
			radiusProperty = property;
		else if(property->type() == ParticleProperty::AsphericalShapeProperty && property->size() >= particleIndex)
			shapeProperty = property;
		else if(property->type() == ParticleProperty::ParticleTypeProperty && property->size() >= particleIndex)
			typeProperty = dynamic_object_cast<ParticleTypeProperty>(property);
	}
	if(!posProperty)
		return Box3();

	// Determine position of selected particle.
	Point3 pos = posProperty->getPoint3(particleIndex);

	// Determine radius of selected particle.
	FloatType radius = particleRadius(particleIndex, radiusProperty, typeProperty);
	if(shapeProperty) {
		radius = std::max(radius, shapeProperty->getVector3(particleIndex).x());
		radius = std::max(radius, shapeProperty->getVector3(particleIndex).y());
		radius = std::max(radius, shapeProperty->getVector3(particleIndex).z());
		radius *= 2;
	}

	if(radius <= 0)
		return Box3();

	return Box3(pos, radius + viewport->nonScalingSize(tm * pos) * 1e-1f);
}

/******************************************************************************
* Given an sub-object ID returned by the Viewport::pick() method, looks up the
* corresponding particle index.
******************************************************************************/
int ParticlePickInfo::particleIndexFromSubObjectID(quint32 subobjID) const
{
	if(_displayObject->particleShape() != ParticleDisplay::Cylinder
			&& _displayObject->particleShape() != ParticleDisplay::Spherocylinder) {
		return subobjID;
	}
	else {
		if(subobjID < (quint32)_particleCount)
			return subobjID;
		else
			return (subobjID - _particleCount) / 2;
	}
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString ParticlePickInfo::infoString(ObjectNode* objectNode, quint32 subobjectId)
{
	int particleIndex = particleIndexFromSubObjectID(subobjectId);
	if(particleIndex < 0) return QString();
	return particleInfoString(pipelineState(), particleIndex);
}

/******************************************************************************
* Builds the info string for a particle to be displayed in the status bar.
******************************************************************************/
QString ParticlePickInfo::particleInfoString(const PipelineFlowState& pipelineState, size_t particleIndex)
{
	QString str;
	for(DataObject* dataObj : pipelineState.objects()) {
		ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(dataObj);
		if(!property || property->size() <= particleIndex) continue;
		if(property->type() == ParticleProperty::SelectionProperty) continue;
		if(property->type() == ParticleProperty::ColorProperty) continue;
		if(property->dataType() != qMetaTypeId<int>() && property->dataType() != qMetaTypeId<FloatType>()) continue;
		if(!str.isEmpty()) str += QStringLiteral(" | ");
		str += property->name();
		str += QStringLiteral(" ");
		for(size_t component = 0; component < property->componentCount(); component++) {
			if(component != 0) str += QStringLiteral(", ");
			QString valueString;
			if(property->dataType() == qMetaTypeId<int>()) {
				str += QString::number(property->getIntComponent(particleIndex, component));
				ParticleTypeProperty* typeProperty = dynamic_object_cast<ParticleTypeProperty>(property);
				if(typeProperty && typeProperty->particleTypes().empty() == false) {
					if(ParticleType* ptype = typeProperty->particleType(property->getIntComponent(particleIndex, component)))
						str += QString(" (%1)").arg(ptype->name());
				}
			}
			else if(property->dataType() == qMetaTypeId<FloatType>())
				str += QString::number(property->getFloatComponent(particleIndex, component));
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
