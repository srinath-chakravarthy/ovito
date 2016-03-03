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
#include <core/utilities/concurrent/ParallelFor.h>
#include "PartitionMesh.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, PartitionMesh, DataObject);
DEFINE_PROPERTY_FIELD(PartitionMesh, _spaceFillingRegion, "SpaceFillingRegion");

/******************************************************************************
* Constructs an empty surface mesh object.
******************************************************************************/
PartitionMesh::PartitionMesh(DataSet* dataset, PartitionMeshData* mesh) : DataObjectWithSharedStorage(dataset, mesh ? mesh : new PartitionMeshData()),
		_spaceFillingRegion(0)
{
	INIT_PROPERTY_FIELD(PartitionMesh::_spaceFillingRegion);
}

/******************************************************************************
* Creates a copy of this object.
******************************************************************************/
OORef<RefTarget> PartitionMesh::clone(bool deepCopy, CloneHelper& cloneHelper)
{
	// Let the base class create an instance of this class.
	OORef<PartitionMesh> clone = static_object_cast<PartitionMesh>(DataObjectWithSharedStorage<PartitionMeshData>::clone(deepCopy, cloneHelper));

	// Copy internal data.
	clone->_cuttingPlanes = this->_cuttingPlanes;

	return clone;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
