///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include "DislocationAnalysisEngine.h"
#include "DislocationAnalysisModifier.h"

#include <fstream>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationAnalysisEngine::DislocationAnalysisEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell) :
	AsynchronousParticleModifier::ComputeEngine(validityInterval),
	_structureAnalysis(positions, simCell),
	_isDefectRegionEverywhere(false),
	_defectMesh(new HalfEdgeMesh<>()),
	_elasticMapping(_structureAnalysis, _tessellation),
	_interfaceMesh(_elasticMapping),
	_dislocationTracer(_interfaceMesh, _structureAnalysis.clusterGraph())
{
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void DislocationAnalysisEngine::perform()
{
	setProgressText(DislocationAnalysisModifier::tr("Dislocation analysis (DXA)"));

	beginProgressSubSteps({ 35, 6, 1, 220, 60, 1, 53, 104, 90, 146 });
	if(!_structureAnalysis.identifyStructures(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.buildClusters(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.connectClusters(*this))
		return;

	nextProgressSubStep();
	FloatType ghostLayerSize = 3.0f * _structureAnalysis.maximumNeighborDistance();
	qDebug() << "Delaunay ghost layer size:" << ghostLayerSize;
	if(!_tessellation.generateTessellation(_structureAnalysis.cell(), _structureAnalysis.positions()->constDataPoint3(), _structureAnalysis.atomCount(), ghostLayerSize, this))
		return;

	// Build list of edges in the tessellation.
	nextProgressSubStep();
	if(!_elasticMapping.generateTessellationEdges(*this))
		return;

	// Assign each vertex to a cluster.
	nextProgressSubStep();
	if(!_elasticMapping.assignVerticesToClusters(*this))
		return;

	// Determine the ideal vector corresponding to each edge of the tessellation.
	nextProgressSubStep();
	if(!_elasticMapping.assignIdealVectorsToEdges(2, *this))
		return;

	// Assign tetrahedra to good or bad crystal region.
	nextProgressSubStep();
	if(!_interfaceMesh.classifyTetrahedra(*this))
		return;

	// Create the mesh facets.
	nextProgressSubStep();
	if(!_interfaceMesh.createMesh(*this))
		return;

	_defectMesh->copyFrom(_interfaceMesh);

	// Trace dislocation lines.
	nextProgressSubStep();
	if(!_dislocationTracer.traceDislocationSegments(*this))
		return;

	endProgressSubSteps();

#if 0
	_tessellation.dumpToVTKFile("tessellation.vtk");

	Point3 corners[8];
	corners[0] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,0,0));
	corners[1] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,0,0));
	corners[2] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,1,0));
	corners[3] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,1,0));
	corners[4] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,0,1));
	corners[5] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,0,1));
	corners[6] = _structureAnalysis.cell().reducedToAbsolute(Point3(1,1,1));
	corners[7] = _structureAnalysis.cell().reducedToAbsolute(Point3(0,1,1));

	std::ofstream stream("cell.vtk");
	stream << "# vtk DataFile Version 3.0" << std::endl;
	stream << "# Simulation cell" << std::endl;
	stream << "ASCII" << std::endl;
	stream << "DATASET UNSTRUCTURED_GRID" << std::endl;
	stream << "POINTS 8 double" << std::endl;
	for(int i = 0; i < 8; i++)
		stream << corners[i].x() << " " << corners[i].y() << " " << corners[i].z() << std::endl;

	stream << std::endl << "CELLS 1 9" << std::endl;
	stream << "8 0 1 2 3 4 5 6 7" << std::endl;

	stream << std::endl << "CELL_TYPES 1" << std::endl;
	stream << "12" << std::endl;  // Hexahedron
#endif
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

