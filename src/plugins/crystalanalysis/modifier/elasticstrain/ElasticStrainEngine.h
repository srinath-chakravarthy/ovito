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

#pragma once


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/*
 * Computation engine of the ElasticStrainModifier, which performs the actual strain tensor calculation.
 */
class ElasticStrainEngine : public StructureIdentificationModifier::StructureIdentificationEngine
{
public:

	/// Constructor.
	ElasticStrainEngine(const TimeInterval& validityInterval,
			ParticleProperty* positions, const SimulationCell& simCell,
			int inputCrystalStructure, std::vector<Matrix3>&& preferredCrystalOrientations,
			bool calculateDeformationGradients, bool calculateStrainTensors,
			FloatType latticeConstant, FloatType caRatio, bool pushStrainTensorsForward);

	/// Computes the modifier's results and stores them in this object for later retrieval.
	virtual void perform() override;

	/// Returns the array of atom cluster IDs.
	ParticleProperty* atomClusters() const { return _structureAnalysis.atomClusters(); }

	/// Returns the created cluster graph.
	ClusterGraph* clusterGraph() { return &_structureAnalysis.clusterGraph(); }

	/// Returns the property storage that contains the computed per-particle volumetric strain values.
	ParticleProperty* volumetricStrains() const { return _volumetricStrains.data(); }

	/// Returns the property storage that contains the computed per-particle strain tensors.
	ParticleProperty* strainTensors() const { return _strainTensors.data(); }

	/// Returns the property storage that contains the computed per-particle deformation gradient tensors.
	ParticleProperty* deformationGradients() const { return _deformationGradients.data(); }

private:

	int _inputCrystalStructure;
	FloatType _latticeConstant;
	FloatType _axialScaling;
	bool _pushStrainTensorsForward;
	StructureAnalysis _structureAnalysis;
	QExplicitlySharedDataPointer<ParticleProperty> _volumetricStrains;
	QExplicitlySharedDataPointer<ParticleProperty> _strainTensors;
	QExplicitlySharedDataPointer<ParticleProperty> _deformationGradients;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


