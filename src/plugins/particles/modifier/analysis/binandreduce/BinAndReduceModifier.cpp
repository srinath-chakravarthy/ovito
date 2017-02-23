///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
//  Copyright (2014) Lars Pastewka
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
#include <core/scene/pipeline/PipelineObject.h>
#include <core/animation/AnimationSettings.h>
#include "BinAndReduceModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(BinAndReduceModifier, ParticleModifier);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, reductionOperation, "ReductionOperation", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, firstDerivative, "firstDerivative", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, binDirection, "BinDirection", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, numberOfBinsX, "NumberOfBinsX", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, numberOfBinsY, "NumberOfBinsY", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(BinAndReduceModifier, fixPropertyAxisRange, "FixPropertyAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, propertyAxisRangeStart, "PropertyAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(BinAndReduceModifier, propertyAxisRangeEnd, "PropertyAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(BinAndReduceModifier, sourceProperty, "SourceProperty");
DEFINE_PROPERTY_FIELD(BinAndReduceModifier, onlySelected, "OnlySelected");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, reductionOperation, "Reduction operation");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, firstDerivative, "Compute first derivative");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, binDirection, "Bin direction");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, numberOfBinsX, "Number of spatial bins");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, numberOfBinsY, "Number of spatial bins");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, fixPropertyAxisRange, "Fix property axis range");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, propertyAxisRangeStart, "Property axis range start");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, propertyAxisRangeEnd, "Property axis range end");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, sourceProperty, "Source property");
SET_PROPERTY_FIELD_LABEL(BinAndReduceModifier, onlySelected, "Use only selected particles");
SET_PROPERTY_FIELD_UNITS_AND_RANGE(BinAndReduceModifier, numberOfBinsX, IntegerParameterUnit, 1, 100000);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(BinAndReduceModifier, numberOfBinsY, IntegerParameterUnit, 1, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
BinAndReduceModifier::BinAndReduceModifier(DataSet* dataset) : 
    ParticleModifier(dataset), _reductionOperation(RED_MEAN), _firstDerivative(false),
    _binDirection(CELL_VECTOR_3), _numberOfBinsX(200), _numberOfBinsY(200),
    _fixPropertyAxisRange(false), _propertyAxisRangeStart(0), _propertyAxisRangeEnd(0),
	_xAxisRangeStart(0), _xAxisRangeEnd(0),
	_yAxisRangeStart(0), _yAxisRangeEnd(0),
	_onlySelected(false)
{
	INIT_PROPERTY_FIELD(reductionOperation);
	INIT_PROPERTY_FIELD(firstDerivative);
	INIT_PROPERTY_FIELD(binDirection);
	INIT_PROPERTY_FIELD(numberOfBinsX);
	INIT_PROPERTY_FIELD(numberOfBinsY);
	INIT_PROPERTY_FIELD(fixPropertyAxisRange);
	INIT_PROPERTY_FIELD(propertyAxisRangeStart);
	INIT_PROPERTY_FIELD(propertyAxisRangeEnd);
	INIT_PROPERTY_FIELD(sourceProperty);
	INIT_PROPERTY_FIELD(onlySelected);
}

/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void BinAndReduceModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty().isNull()) {
		PipelineFlowState input = getModifierInput(modApp);
		ParticlePropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull()) {
			setSourceProperty(bestProperty);
		}
	}
}

/******************************************************************************
* This modifies the input object.
******************************************************************************/
PipelineStatus BinAndReduceModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	int binDataSizeX = std::max(1, numberOfBinsX());
	int binDataSizeY = std::max(1, numberOfBinsY());
    if(is1D()) binDataSizeY = 1;
    size_t binDataSize = binDataSizeX*binDataSizeY;
	_binData.resize(binDataSize);
	std::fill(_binData.begin(), _binData.end(), 0.0);

    // Return coordinate indices (0, 1 or 2).
    int binDirX = binDirectionX(binDirection());
    int binDirY = binDirectionY(binDirection());

    // Number of particles for averaging.
    std::vector<int> numberOfParticlesPerBin(binDataSize, 0);

	// Get the source property.
	if(sourceProperty().isNull())
		throwException(tr("Select a particle property first."));
	ParticlePropertyObject* property = sourceProperty().findInState(input());
	if(!property)
		throwException(tr("The selected particle property with the name '%1' does not exist.").arg(sourceProperty().name()));
	if(sourceProperty().vectorComponent() >= (int)property->componentCount())
		throwException(tr("The selected vector component is out of range. The particle property '%1' contains only %2 values per particle.").arg(sourceProperty().name()).arg(property->componentCount()));

	size_t vecComponent = std::max(0, sourceProperty().vectorComponent());
	size_t vecComponentCount = property->componentCount();

	// Get input selection.
	ParticleProperty* inputSelectionProperty = nullptr;
	if(onlySelected()) {
		inputSelectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty)->storage();
		OVITO_ASSERT(inputSelectionProperty->size() == property->size());
	}

    // Get bottom-left and top-right corner of the simulation cell.
	SimulationCell cell = expectSimulationCell()->data();
    AffineTransformation reciprocalCell = cell.inverseMatrix();

    // Get periodic boundary flag.
	std::array<bool, 3> pbc = cell.pbcFlags();

    // Compute the surface normal vector.
    Vector3 normalX, normalY(1, 1, 1);
    if(binDirection() == CELL_VECTOR_1) {
        normalX = expectSimulationCell()->cellVector2().cross(expectSimulationCell()->cellVector3());
    }
    else if(binDirection() == CELL_VECTOR_2) {
        normalX = expectSimulationCell()->cellVector3().cross(expectSimulationCell()->cellVector1());
    }
    else if(binDirection() == CELL_VECTOR_3) {
        normalX = expectSimulationCell()->cellVector1().cross(expectSimulationCell()->cellVector2());
    }
    else if(binDirection() == CELL_VECTORS_1_2) {
        normalX = expectSimulationCell()->cellVector2().cross(expectSimulationCell()->cellVector3());
        normalY = expectSimulationCell()->cellVector3().cross(expectSimulationCell()->cellVector1());
    }
    else if(_binDirection == CELL_VECTORS_2_3) {
        normalX = expectSimulationCell()->cellVector3().cross(expectSimulationCell()->cellVector1());
        normalY = expectSimulationCell()->cellVector1().cross(expectSimulationCell()->cellVector2());
    }
    else if(binDirection() == CELL_VECTORS_1_3) {
        normalX = expectSimulationCell()->cellVector2().cross(expectSimulationCell()->cellVector3());
        normalY = expectSimulationCell()->cellVector1().cross(expectSimulationCell()->cellVector2());
    }
	if(normalX == Vector3::Zero() || normalY == Vector3::Zero())
		throwException(tr("Simulation cell is degenerate."));

    // Compute the distance of the two cell faces (normal.length() is area of face).
    FloatType cellVolume = cell.volume3D();
    _xAxisRangeStart = (expectSimulationCell()->cellOrigin() - Point3::Origin()).dot(normalX.normalized());
    _xAxisRangeEnd = _xAxisRangeStart + cellVolume / normalX.length();
    if(!is1D()) {
        _yAxisRangeStart = (expectSimulationCell()->cellOrigin() - Point3::Origin()).dot(normalY.normalized());
		_yAxisRangeEnd = _yAxisRangeStart + cellVolume / normalY.length();
    }
    else {
        _yAxisRangeStart = 0;
        _yAxisRangeEnd = 0;
    }

	// Get the particle positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	OVITO_ASSERT(posProperty->size() == property->size());

	if(property->size() > 0) {
        const Point3* pos = posProperty->constDataPoint3();
        const Point3* pos_end = pos + posProperty->size();

		if(property->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* v = property->constDataFloat() + vecComponent;
			const FloatType* v_end = v + (property->size() * vecComponentCount);
			const int* sel = inputSelectionProperty ? inputSelectionProperty->constDataInt() : nullptr;
            for(; v != v_end; v += vecComponentCount, ++pos) {
				if(sel && !*sel++) continue;
                if(!std::isnan(*v)) {
                    FloatType fractionalPosX = reciprocalCell.prodrow(*pos, binDirX);
                    FloatType fractionalPosY = reciprocalCell.prodrow(*pos, binDirY);
                    int binIndexX = int( fractionalPosX * binDataSizeX );
                    int binIndexY = int( fractionalPosY * binDataSizeY );
                    if(pbc[binDirX]) binIndexX = SimulationCell::modulo(binIndexX, binDataSizeX);
                    if(pbc[binDirY]) binIndexY = SimulationCell::modulo(binIndexY, binDataSizeY);
                    if(binIndexX >= 0 && binIndexX < binDataSizeX && binIndexY >= 0 && binIndexY < binDataSizeY) {
                        size_t binIndex = binIndexY*binDataSizeX+binIndexX;
                        if(_reductionOperation == RED_MEAN || _reductionOperation == RED_SUM || _reductionOperation == RED_SUM_VOL) {
                            _binData[binIndex] += *v;
                        } 
                        else {
                            if(numberOfParticlesPerBin[binIndex] == 0) {
                                _binData[binIndex] = *v;  
                            }
                            else {
                                if(_reductionOperation == RED_MAX) {
                                    _binData[binIndex] = std::max(_binData[binIndex], (double)*v);
                                }
                                else if(_reductionOperation == RED_MIN) {
                                    _binData[binIndex] = std::min(_binData[binIndex], (double)*v);
                                }
                            }
                        }
                        numberOfParticlesPerBin[binIndex]++;
                    }
                }
            }
		}
		else if(property->dataType() == qMetaTypeId<int>()) {
			const int* v = property->constDataInt() + vecComponent;
			const int* v_end = v + (property->size() * vecComponentCount);
			const int* sel = inputSelectionProperty ? inputSelectionProperty->constDataInt() : nullptr;
            for(; v != v_end; v += vecComponentCount, ++pos) {
				if(sel && !*sel++) continue;
                FloatType fractionalPosX = reciprocalCell.prodrow(*pos, binDirX);
                FloatType fractionalPosY = reciprocalCell.prodrow(*pos, binDirY);
                int binIndexX = int( fractionalPosX * binDataSizeX );
                int binIndexY = int( fractionalPosY * binDataSizeY );
                if(pbc[binDirX])  binIndexX = SimulationCell::modulo(binIndexX, binDataSizeX);
                if(pbc[binDirY])  binIndexY = SimulationCell::modulo(binIndexY, binDataSizeY);
                if(binIndexX >= 0 && binIndexX < binDataSizeX && binIndexY >= 0 && binIndexY < binDataSizeY) {
                    size_t binIndex = binIndexY*binDataSizeX+binIndexX;
                    if(_reductionOperation == RED_MEAN || _reductionOperation == RED_SUM || _reductionOperation == RED_SUM) {
                        _binData[binIndex] += *v;
                    }
                    else {
                        if(numberOfParticlesPerBin[binIndex] == 0) {
                            _binData[binIndex] = *v;  
                        }
                        else {
                            if(_reductionOperation == RED_MAX) {
                                _binData[binIndex] = std::max(_binData[binIndex], (double)*v);
                            }
                            else if(_reductionOperation == RED_MIN) {
                                _binData[binIndex] = std::min(_binData[binIndex], (double)*v);
                            }
                        }
                    }
                    numberOfParticlesPerBin[binIndex]++;
                }
            }
		}

        if(reductionOperation() == RED_MEAN) {
            // Normalize.
            auto a = _binData.begin();
            for(auto n : numberOfParticlesPerBin) {
                if (n > 0) *a /= n;
                ++a;
            }
        }
        else if(reductionOperation() == RED_SUM_VOL) {
            // Divide by bin volume.
            double binVolume = cellVolume / (binDataSizeX*binDataSizeY);
            std::for_each(_binData.begin(), _binData.end(), [binVolume](double &x) { x /= binVolume; });
        }
	}

	// Compute first derivative using finite differences.
    if(firstDerivative()) {
        FloatType binSpacingX = (_xAxisRangeEnd - _xAxisRangeStart) / binDataSizeX;
        if(binDataSizeX > 1 && _xAxisRangeEnd > _xAxisRangeStart) {
        	QVector<double> derivativeData(binDataSize);
			for(int j = 0; j < binDataSizeY; j++) {
				for(int i = 0; i < binDataSizeX; i++) {
					int ndx = 2;
					int i_plus_1 = i+1;
					int i_minus_1 = i-1;
					if(i_plus_1 == binDataSizeX) {
						if(pbc[binDirX]) i_plus_1 = 0;
						else { i_plus_1 = binDataSizeX-1; ndx = 1; }
					}
					if(i_minus_1 == -1) {
						if(pbc[binDirX]) i_minus_1 = binDataSizeX-1;
						else { i_minus_1 = 0; ndx = 1; }
					}
					OVITO_ASSERT(j*binDataSizeX + i_plus_1 < binDataSize);
					OVITO_ASSERT(j*binDataSizeX + i_minus_1 < binDataSize);
					derivativeData[j*binDataSizeX + i] = (_binData[j*binDataSizeX + i_plus_1] - _binData[j*binDataSizeX + i_minus_1]) / (ndx*binSpacingX);
				}
			}
			_binData = std::move(derivativeData);
        }
        else std::fill(_binData.begin(), _binData.end(), 0.0);
    }

	if(!fixPropertyAxisRange()) {
		auto minmax = std::minmax_element(_binData.begin(), _binData.end());
        setPropertyAxisRangeStart(*minmax.first);
		setPropertyAxisRangeEnd(*minmax.second);
	}

	// Inform the editor component that the stored data has changed
	// and it should update the display.
	notifyDependents(ReferenceEvent::ObjectStatusChanged);

	return PipelineStatus(PipelineStatus::Success);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
