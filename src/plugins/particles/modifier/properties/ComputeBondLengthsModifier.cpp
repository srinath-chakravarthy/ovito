///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "ComputeBondLengthsModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Properties)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, ComputeBondLengthsModifier, ParticleModifier);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ComputeBondLengthsModifier::ComputeBondLengthsModifier(DataSet* dataset) : ParticleModifier(dataset)
{
}

/******************************************************************************
* Modifies the particle object.
******************************************************************************/
PipelineStatus ComputeBondLengthsModifier::modifyParticles(TimePoint time, TimeInterval& validityInterval)
{
	// Inputs:
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	BondsObject* bondsObj = expectBonds();
	SimulationCellObject* simCell = input().findObject<SimulationCellObject>();
	AffineTransformation cellMatrix = simCell ? simCell->cellMatrix() : AffineTransformation::Identity();

	// Outputs:
	BondPropertyObject* lengthProperty = outputStandardBondProperty(BondProperty::LengthProperty, false);

	// Perform bond length calculation.
	parallelFor(bondsObj->size(), [posProperty, bondsObj, simCell, &cellMatrix, lengthProperty](size_t bondIndex) {
		const Bond& bond = (*bondsObj->storage())[bondIndex];
		if(posProperty->size() > bond.index1 && posProperty->size() > bond.index2) {
			const Point3& p1 = posProperty->getPoint3(bond.index1);
			const Point3& p2 = posProperty->getPoint3(bond.index2);
			Vector3 delta = p2 - p1;
			if(simCell) {
				if(bond.pbcShift.x()) delta += cellMatrix.column(0) * (FloatType)bond.pbcShift.x();
				if(bond.pbcShift.y()) delta += cellMatrix.column(1) * (FloatType)bond.pbcShift.y();
				if(bond.pbcShift.z()) delta += cellMatrix.column(2) * (FloatType)bond.pbcShift.z();
			}
			lengthProperty->setFloat(bondIndex, delta.length());
		}
		else lengthProperty->setFloat(bondIndex, 0);
	});
	lengthProperty->changed();

	return PipelineStatus::Success;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
