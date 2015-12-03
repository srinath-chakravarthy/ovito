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
#include <core/utilities/concurrent/ParallelFor.h>
#include "GrainSegmentationEngine.h"
#include "GrainSegmentationModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Constructor.
******************************************************************************/
GrainSegmentationEngine::GrainSegmentationEngine(const TimeInterval& validityInterval,
		ParticleProperty* positions, const SimulationCell& simCell,
		int inputCrystalStructure) :
	StructureIdentificationModifier::StructureIdentificationEngine(validityInterval, positions, simCell),
	_structureAnalysis(positions, simCell, (StructureAnalysis::LatticeStructureType)inputCrystalStructure, selection(), structures()),
	_inputCrystalStructure(inputCrystalStructure),
	_deformationGradients(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 9, 0, QStringLiteral("Elastic Deformation Gradient"), false))
{
	// Set component names of tensor property.
	deformationGradients()->setComponentNames(QStringList() << "XX" << "YX" << "ZX" << "XY" << "YY" << "ZY" << "XZ" << "YZ" << "ZZ");
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void GrainSegmentationEngine::perform()
{
	setProgressText(GrainSegmentationModifier::tr("Performing grain segmentation"));

	beginProgressSubSteps({ 35, 6, 1, 1, 20 });
	if(!_structureAnalysis.identifyStructures(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.buildClusters(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.connectClusters(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.formSuperClusters(*this))
		return;

	nextProgressSubStep();
	parallelFor(positions()->size(), *this, [this](size_t particleIndex) {

		Cluster* localCluster = _structureAnalysis.atomCluster(particleIndex);
		if(localCluster->id != 0) {

			Matrix3 idealUnitCellTM = Matrix3::Identity();

			// If the cluster is a defect (stacking fault), find the parent crystal cluster.
			Cluster* parentCluster = nullptr;
			if(localCluster->parentTransition != nullptr) {
				parentCluster = localCluster->parentTransition->cluster2;
				idealUnitCellTM = localCluster->parentTransition->tm;
			}
			else if(localCluster->structure == _inputCrystalStructure) {
				parentCluster = localCluster;
			}

			if(parentCluster != nullptr) {
				OVITO_ASSERT(parentCluster->structure == _inputCrystalStructure);

				// For calculating the cluster orientation.
				Matrix_3<double> orientationV = Matrix_3<double>::Zero();
				Matrix_3<double> orientationW = Matrix_3<double>::Zero();

				int numneigh = _structureAnalysis.numNeighbors(particleIndex);
				for(int n = 0; n < numneigh; n++) {
					int neighborAtomIndex = _structureAnalysis.getNeighbor(particleIndex, n);
					// Add vector pair to matrices for computing the elastic deformation gradient.
					Vector3 latticeVector = idealUnitCellTM * _structureAnalysis.neighborLatticeVector(particleIndex, n);
					const Vector3& spatialVector = cell().wrapVector(positions()->getPoint3(neighborAtomIndex) - positions()->getPoint3(particleIndex));
					for(size_t i = 0; i < 3; i++) {
						for(size_t j = 0; j < 3; j++) {
							orientationV(i,j) += (double)(latticeVector[j] * latticeVector[i]);
							orientationW(i,j) += (double)(latticeVector[j] * spatialVector[i]);
						}
					}
				}

				// Calculate deformation gradient tensor.
				Matrix_3<double> elasticF = orientationW * orientationV.inverse();
				for(size_t col = 0; col < 3; col++) {
					for(size_t row = 0; row < 3; row++) {
						deformationGradients()->setFloatComponent(particleIndex, col*3+row, (FloatType)elasticF(row,col));
					}
				}

				return;
			}
		}

		// Mark atom as invalid by setting all components of the tensor to zero.
		for(size_t component = 0; component < 9; component++)
			deformationGradients()->setFloatComponent(particleIndex, component, 0);
	});

	endProgressSubSteps();
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

