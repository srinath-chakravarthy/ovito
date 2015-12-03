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

#ifndef __OVITO_GRAIN_SEGMENTATION_ENGINE_H
#define __OVITO_GRAIN_SEGMENTATION_ENGINE_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Computation engine of the GrainSegmentationModifier, which performs decomposes a crystalline structure into individual grains.
 */
class GrainSegmentationEngine : public StructureIdentificationModifier::StructureIdentificationEngine
{
public:

	/// Constructor.
	GrainSegmentationEngine(const TimeInterval& validityInterval,
			ParticleProperty* positions, const SimulationCell& simCell,
			int inputCrystalStructure);

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Returns the array of atom cluster IDs.
	ParticleProperty* atomClusters() const { return _structureAnalysis.atomClusters(); }

	/// Returns the created cluster graph.
	ClusterGraph* clusterGraph() { return &_structureAnalysis.clusterGraph(); }

	/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
	ParticleProperty* deformationGradients() const { return _deformationGradients.data(); }

private:

	int _inputCrystalStructure;
	StructureAnalysis _structureAnalysis;
	QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_GRAIN_SEGMENTATION_ENGINE_H
