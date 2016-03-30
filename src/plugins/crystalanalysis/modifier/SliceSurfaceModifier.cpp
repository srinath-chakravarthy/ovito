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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>
#include <plugins/crystalanalysis/objects/slip_surface/SlipSurface.h>
#include "SliceSurfaceModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, SliceSurfaceFunction, SliceModifierFunction);
IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, SliceDislocationsFunction, SliceModifierFunction);

/******************************************************************************
* Performs the actual rejection of particles.
******************************************************************************/
PipelineStatus SliceSurfaceFunction::apply(SliceModifier* modifier, TimePoint time, const Plane3& plane, FloatType sliceWidth)
{
	for(DataObject* obj : modifier->input().objects()) {
		if(SurfaceMesh* inputMesh = dynamic_object_cast<SurfaceMesh>(obj)) {
			OORef<SurfaceMesh> outputMesh = modifier->cloneHelper()->cloneObject(inputMesh, false);
			QVector<Plane3> planes = inputMesh->cuttingPlanes();
			if(sliceWidth <= 0) {
				planes.push_back(plane);
			}
			else {
				planes.push_back(Plane3(plane.normal, plane.dist + sliceWidth/2));
				planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth/2));
			}
			outputMesh->setCuttingPlanes(planes);
			modifier->output().replaceObject(inputMesh, outputMesh);
		}
		else if(PartitionMesh* inputMesh = dynamic_object_cast<PartitionMesh>(obj)) {
			OORef<PartitionMesh> outputMesh = modifier->cloneHelper()->cloneObject(inputMesh, false);
			QVector<Plane3> planes = inputMesh->cuttingPlanes();
			if(sliceWidth <= 0) {
				planes.push_back(plane);
			}
			else {
				planes.push_back(Plane3(plane.normal, plane.dist + sliceWidth/2));
				planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth/2));
			}
			outputMesh->setCuttingPlanes(planes);
			modifier->output().replaceObject(inputMesh, outputMesh);
		}
		else if(SlipSurface* inputMesh = dynamic_object_cast<SlipSurface>(obj)) {
			OORef<SlipSurface> outputMesh = modifier->cloneHelper()->cloneObject(inputMesh, false);
			QVector<Plane3> planes = inputMesh->cuttingPlanes();
			if(sliceWidth <= 0) {
				planes.push_back(plane);
			}
			else {
				planes.push_back(Plane3(plane.normal, plane.dist + sliceWidth/2));
				planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth/2));
			}
			outputMesh->setCuttingPlanes(planes);
			modifier->output().replaceObject(inputMesh, outputMesh);
		}
	}

	return PipelineStatus::Success;
}

/******************************************************************************
* Performs the actual rejection of particles.
******************************************************************************/
PipelineStatus SliceDislocationsFunction::apply(SliceModifier* modifier, TimePoint time, const Plane3& plane, FloatType sliceWidth)
{
	for(DataObject* obj : modifier->input().objects()) {
		if(DislocationNetworkObject* inputDislocations = dynamic_object_cast<DislocationNetworkObject>(obj)) {
			OORef<DislocationNetworkObject> outputDislocations = modifier->cloneHelper()->cloneObject(inputDislocations, false);
			QVector<Plane3> planes = inputDislocations->cuttingPlanes();
			if(sliceWidth <= 0) {
				planes.push_back(plane);
			}
			else {
				planes.push_back(Plane3(plane.normal, plane.dist + sliceWidth/2));
				planes.push_back(Plane3(-plane.normal, -plane.dist + sliceWidth/2));
			}
			outputDislocations->setCuttingPlanes(planes);
			modifier->output().replaceObject(inputDislocations, outputDislocations);
		}
	}

	return PipelineStatus::Success;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
