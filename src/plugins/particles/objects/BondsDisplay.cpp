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

#include <plugins/particles/Particles.h>
#include <core/utilities/units/UnitsManager.h>
#include <core/rendering/SceneRenderer.h>
#include "BondsDisplay.h"
#include "ParticleDisplay.h"
#include "SimulationCellObject.h"
#include "ParticleTypeProperty.h"
#include "BondTypeProperty.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(BondsDisplay, DisplayObject);
IMPLEMENT_OVITO_OBJECT(BondPickInfo, ObjectPickInfo);
DEFINE_FLAGS_PROPERTY_FIELD(BondsDisplay, _bondWidth, "BondWidth", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BondsDisplay, _bondColor, "BondColor", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BondsDisplay, _useParticleColors, "UseParticleColors", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BondsDisplay, _shadingMode, "ShadingMode", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(BondsDisplay, _renderingQuality, "RenderingQuality");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, _bondWidth, "Bond width");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, _bondColor, "Bond color");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, _useParticleColors, "Use particle colors");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, _shadingMode, "Shading mode");
SET_PROPERTY_FIELD_LABEL(BondsDisplay, _renderingQuality, "RenderingQuality");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(BondsDisplay, _bondWidth, WorldParameterUnit, 0);

/******************************************************************************
* Constructor.
******************************************************************************/
BondsDisplay::BondsDisplay(DataSet* dataset) : DisplayObject(dataset),
	_bondWidth(0.4), _bondColor(0.6, 0.6, 0.6), _useParticleColors(true),
	_shadingMode(ArrowPrimitive::NormalShading),
	_renderingQuality(ArrowPrimitive::HighQuality)
{
	INIT_PROPERTY_FIELD(BondsDisplay::_bondWidth);
	INIT_PROPERTY_FIELD(BondsDisplay::_bondColor);
	INIT_PROPERTY_FIELD(BondsDisplay::_useParticleColors);
	INIT_PROPERTY_FIELD(BondsDisplay::_shadingMode);
	INIT_PROPERTY_FIELD(BondsDisplay::_renderingQuality);
}

/******************************************************************************
* Computes the bounding box of the object.
******************************************************************************/
Box3 BondsDisplay::boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState)
{
	BondsObject* bondsObj = dynamic_object_cast<BondsObject>(dataObject);
	ParticlePropertyObject* positionProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::PositionProperty);
	SimulationCellObject* simulationCell = flowState.findObject<SimulationCellObject>();

	// Detect if the input data has changed since the last time we computed the bounding box.
	if(_boundingBoxCacheHelper.updateState(
			bondsObj,
			positionProperty,
			simulationCell,
			bondWidth())) {

		// Recompute bounding box.
		_cachedBoundingBox.setEmpty();
		if(bondsObj && positionProperty) {

			unsigned int particleCount = (unsigned int)positionProperty->size();
			const Point3* positions = positionProperty->constDataPoint3();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			for(const Bond& bond : *bondsObj->storage()) {
				if(bond.index1 >= particleCount || bond.index2 >= particleCount)
					continue;

				_cachedBoundingBox.addPoint(positions[bond.index1]);
				if(bond.pbcShift != Vector_3<int8_t>::Zero()) {
					Vector3 vec = positions[bond.index2] - positions[bond.index1];
					for(size_t k = 0; k < 3; k++)
						if(bond.pbcShift[k] != 0) vec += cell.column(k) * (FloatType)bond.pbcShift[k];
					_cachedBoundingBox.addPoint(positions[bond.index1] + (vec * FloatType(0.5)));
				}
			}

			_cachedBoundingBox = _cachedBoundingBox.padBox(bondWidth() / 2);
		}
	}
	return _cachedBoundingBox;
}

/******************************************************************************
* Lets the display object render the data object.
******************************************************************************/
void BondsDisplay::render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode)
{
	BondsObject* bondsObj = dynamic_object_cast<BondsObject>(dataObject);
	ParticlePropertyObject* positionProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::PositionProperty);
	SimulationCellObject* simulationCell = flowState.findObject<SimulationCellObject>();
	ParticlePropertyObject* particleColorProperty = ParticlePropertyObject::findInState(flowState, ParticleProperty::ColorProperty);
	ParticleTypeProperty* particleTypeProperty = dynamic_object_cast<ParticleTypeProperty>(ParticlePropertyObject::findInState(flowState, ParticleProperty::ParticleTypeProperty));
	BondTypeProperty* bondTypeProperty = dynamic_object_cast<BondTypeProperty>(BondPropertyObject::findInState(flowState, BondProperty::BondTypeProperty));
	BondPropertyObject* bondColorProperty = BondPropertyObject::findInState(flowState, BondProperty::ColorProperty);
	BondPropertyObject* bondSelectionProperty = BondPropertyObject::findInState(flowState, BondProperty::SelectionProperty);
	if(!useParticleColors()) {
		particleColorProperty = nullptr;
		particleTypeProperty = nullptr;
	}

	if(_geometryCacheHelper.updateState(
			bondsObj,
			positionProperty,
			particleColorProperty,
			particleTypeProperty,
			bondColorProperty,
			bondTypeProperty,
			bondSelectionProperty,
			simulationCell,
			bondWidth(), bondColor(), useParticleColors())
			|| !_buffer	|| !_buffer->isValid(renderer)
			|| !_buffer->setShadingMode(shadingMode())
			|| !_buffer->setRenderingQuality(renderingQuality())) {

		FloatType bondRadius = bondWidth() / 2;
		if(bondsObj && positionProperty && bondRadius > 0) {

			// Create bond geometry buffer.
			_buffer = renderer->createArrowPrimitive(ArrowPrimitive::CylinderShape, shadingMode(), renderingQuality());
			_buffer->startSetElements((int)bondsObj->storage()->size());

			// Obtain particle display object.
			ParticleDisplay* particleDisplay = nullptr;
			if(useParticleColors()) {
				for(DisplayObject* displayObj : positionProperty->displayObjects()) {
					particleDisplay = dynamic_object_cast<ParticleDisplay>(displayObj);
					if(particleDisplay) break;
				}
			}

			// Determine bond colors.
			std::vector<Color> colors(bondsObj->storage()->size());
			bondColors(colors, positionProperty->size(), bondsObj,
					bondColorProperty, bondTypeProperty, bondSelectionProperty,
					particleDisplay, particleColorProperty, particleTypeProperty);

			// Cache some variables.
			unsigned int particleCount = (unsigned int)positionProperty->size();
			const Point3* positions = positionProperty->constDataPoint3();
			const AffineTransformation cell = simulationCell ? simulationCell->cellMatrix() : AffineTransformation::Zero();

			int elementIndex = 0;
			auto color = colors.cbegin();
			for(const Bond& bond : *bondsObj->storage()) {
				if(bond.index1 < particleCount && bond.index2 < particleCount) {
					Vector3 vec = positions[bond.index2] - positions[bond.index1];
					for(size_t k = 0; k < 3; k++)
						if(bond.pbcShift[k] != 0) vec += cell.column(k) * (FloatType)bond.pbcShift[k];
					_buffer->setElement(elementIndex, positions[bond.index1], vec * FloatType(0.5), (ColorA)*color++, bondRadius);
				}
				else _buffer->setElement(elementIndex, Point3::Origin(), Vector3::Zero(), (ColorA)*color++, 0);
				elementIndex++;
			}

			_buffer->endSetElements();
		}
		else _buffer.reset();
	}

	if(!_buffer)
		return;

	if(renderer->isPicking()) {
		OORef<BondPickInfo> pickInfo(new BondPickInfo(bondsObj, flowState));
		renderer->beginPickObject(contextNode, pickInfo);
	}

	_buffer->render(renderer);

	if(renderer->isPicking()) {
		renderer->endPickObject();
	}
}

/******************************************************************************
* Determines the display colors of bonds.
******************************************************************************/
void BondsDisplay::bondColors(std::vector<Color>& output, size_t particleCount, BondsObject* bondsObject,
		BondPropertyObject* bondColorProperty, BondTypeProperty* bondTypeProperty, BondPropertyObject* bondSelectionProperty,
		ParticleDisplay* particleDisplay, ParticlePropertyObject* particleColorProperty, ParticleTypeProperty* particleTypeProperty)
{
	OVITO_ASSERT(bondColorProperty == nullptr || bondColorProperty->type() == BondProperty::ColorProperty);
	OVITO_ASSERT(bondTypeProperty == nullptr || bondTypeProperty->type() == BondProperty::BondTypeProperty);
	OVITO_ASSERT(bondSelectionProperty == nullptr || bondSelectionProperty->type() == BondProperty::SelectionProperty);

	if(useParticleColors() && particleDisplay != nullptr && output.size() == bondsObject->storage()->size()) {
		// Derive bond colors from particle colors.
		std::vector<Color> particleColors(particleCount);
		particleDisplay->particleColors(particleColors, particleColorProperty, particleTypeProperty, nullptr);
		auto bc = output.begin();
		for(const Bond& bond : *bondsObject->storage()) {
			if(bond.index1 < particleCount && bond.index2 < particleCount)
				*bc++ = particleColors[bond.index1];
			else
				*bc++ = bondColor();
		}
	}
	else {
		Color defaultColor = bondColor();
		if(bondColorProperty && bondColorProperty->size() == output.size()) {
			// Take bond colors directly from the color property.
			std::copy(bondColorProperty->constDataColor(), bondColorProperty->constDataColor() + output.size(), output.begin());
		}
		else if(bondTypeProperty && bondTypeProperty->size() == output.size()) {
			// Assign colors based on bond types.
			// Generate a lookup map for bond type colors.
			const std::map<int,Color> colorMap = bondTypeProperty->colorMap();
			std::array<Color,16> colorArray;
			// Check if all type IDs are within a small, non-negative range.
			// If yes, we can use an array lookup strategy. Otherwise we have to use a dictionary lookup strategy, which is slower.
			if(std::all_of(colorMap.begin(), colorMap.end(),
					[&colorArray](const std::map<int,Color>::value_type& i) { return i.first >= 0 && i.first < (int)colorArray.size(); })) {
				colorArray.fill(defaultColor);
				for(const auto& entry : colorMap)
					colorArray[entry.first] = entry.second;
				// Fill color array.
				const int* t = bondTypeProperty->constDataInt();
				for(auto c = output.begin(); c != output.end(); ++c, ++t) {
					if(*t >= 0 && *t < (int)colorArray.size())
						*c = colorArray[*t];
					else
						*c = defaultColor;
				}
			}
			else {
				// Fill color array.
				const int* t = bondTypeProperty->constDataInt();
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
			// Assign a uniform color to all bonds.
			std::fill(output.begin(), output.end(), defaultColor);
		}
	}

	// Highlight selected bonds.
	if(bondSelectionProperty && bondSelectionProperty->size() == output.size()) {
		const Color selColor = selectionBondColor();
		const int* t = bondSelectionProperty->constDataInt();
		for(auto c = output.begin(); c != output.end(); ++c, ++t) {
			if(*t)
				*c = selColor;
		}
	}
}

/******************************************************************************
* Returns a human-readable string describing the picked object,
* which will be displayed in the status bar by OVITO.
******************************************************************************/
QString BondPickInfo::infoString(ObjectNode* objectNode, quint32 subobjectId)
{
	QString str;
	int bondIndex = subobjectId;
	if(_bondsObj && _bondsObj->storage()->size() > bondIndex) {
		str = tr("Bond");
		const Bond& bond = (*_bondsObj->storage())[bondIndex];

		// Bond length
		ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(pipelineState(), ParticleProperty::PositionProperty);
		if(posProperty && posProperty->size() > bond.index1 && posProperty->size() > bond.index2) {
			const Point3& p1 = posProperty->getPoint3(bond.index1);
			const Point3& p2 = posProperty->getPoint3(bond.index2);
			Vector3 delta = p2 - p1;
			if(SimulationCellObject* simCell = pipelineState().findObject<SimulationCellObject>()) {
				delta += simCell->cellMatrix() * Vector3(bond.pbcShift);
			}
			str += QString(" | Length: %1 | Delta: (%2 %3 %4)").arg(delta.length()).arg(delta.x()).arg(delta.y()).arg(delta.z());
		}

		// Bond properties
		for(DataObject* dataObj : pipelineState().objects()) {
			BondPropertyObject* property = dynamic_object_cast<BondPropertyObject>(dataObj);
			if(!property || property->size() <= bondIndex) continue;
			if(property->type() == BondProperty::SelectionProperty) continue;
			if(property->type() == BondProperty::ColorProperty) continue;
			if(property->dataType() != qMetaTypeId<int>() && property->dataType() != qMetaTypeId<FloatType>()) continue;
			if(!str.isEmpty()) str += QStringLiteral(" | ");
			str += property->name();
			str += QStringLiteral(" ");
			for(size_t component = 0; component < property->componentCount(); component++) {
				if(component != 0) str += QStringLiteral(", ");
				QString valueString;
				if(property->dataType() == qMetaTypeId<int>()) {
					str += QString::number(property->getIntComponent(bondIndex, component));
					BondTypeProperty* typeProperty = dynamic_object_cast<BondTypeProperty>(property);
					if(typeProperty && typeProperty->bondTypes().empty() == false) {
						if(BondType* btype = typeProperty->bondType(property->getIntComponent(bondIndex, component)))
							str += QString(" (%1)").arg(btype->name());
					}
				}
				else if(property->dataType() == qMetaTypeId<FloatType>())
					str += QString::number(property->getFloatComponent(bondIndex, component));
			}
		}

		// Pair type info.
		ParticleTypeProperty* typeProperty = dynamic_object_cast<ParticleTypeProperty>(ParticlePropertyObject::findInState(pipelineState(), ParticleProperty::ParticleTypeProperty));
		if(typeProperty && typeProperty->size() > bond.index1 && typeProperty->size() > bond.index2) {
			ParticleType* type1 = typeProperty->particleType(typeProperty->getInt(bond.index1));
			ParticleType* type2 = typeProperty->particleType(typeProperty->getInt(bond.index2));
			if(type1 && type2) {
				str += QString(" | Particles: %1 - %2").arg(type1->name(), type2->name());
			}
		}
	}
	return str;
}

}	// End of namespace
}	// End of namespace
