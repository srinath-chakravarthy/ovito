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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/data/BondsStorage.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief This modifier computes the atomic volume and the Voronoi indices of particles.
 */
class OVITO_PARTICLES_EXPORT VoronoiAnalysisModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE VoronoiAnalysisModifier(DataSet* dataset);

	/// Returns the total volume of the simulation cell computed by the modifier.
	double simulationBoxVolume() const { return _simulationBoxVolume; }

	/// Returns the volume sum of all Voronoi cells computed by the modifier.
	double voronoiVolumeSum() const { return _voronoiVolumeSum; }

	/// Returns the maximum number of edges of any Voronoi face.
	int maxFaceOrder() const { return _maxFaceOrder; }

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Computes the modifier's results.
	class VoronoiAnalysisEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		VoronoiAnalysisEngine(const TimeInterval& validityInterval, ParticleProperty* positions, ParticleProperty* selection, std::vector<FloatType>&& radii,
							const SimulationCell& simCell,
							int edgeCount, bool computeIndices, bool computeBonds, FloatType edgeThreshold, FloatType faceThreshold) :
			ComputeEngine(validityInterval),
			_positions(positions),
			_selection(selection),
			_radii(std::move(radii)),
			_simCell(simCell),
			_maxFaceOrder(0),
			_edgeThreshold(edgeThreshold),
			_faceThreshold(faceThreshold),
			_voronoiVolumeSum(0),
			_simulationBoxVolume(0),
			_coordinationNumbers(new ParticleProperty(positions->size(), ParticleProperty::CoordinationProperty, 0, true)),
			_atomicVolumes(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 1, 0, QStringLiteral("Atomic Volume"), true)),
			_voronoiIndices(computeIndices ? new ParticleProperty(positions->size(), qMetaTypeId<int>(), edgeCount, 0, QStringLiteral("Voronoi Index"), true) : nullptr),
			_bonds(computeBonds ? new BondsStorage() : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the property storage that contains the computed coordination numbers.
		ParticleProperty* coordinationNumbers() const { return _coordinationNumbers.data(); }

		/// Returns the property storage that contains the computed atomic volumes.
		ParticleProperty* atomicVolumes() const { return _atomicVolumes.data(); }

		/// Returns the property storage that contains the computed Voronoi indices.
		ParticleProperty* voronoiIndices() const { return _voronoiIndices.data(); }

		/// Returns the total volume of the simulation cell computed by the modifier.
		double simulationBoxVolume() const { return _simulationBoxVolume; }

		/// Returns the volume sum of all Voronoi cells.
		double voronoiVolumeSum() const { return _voronoiVolumeSum; }

		/// Returns the maximum number of edges of a Voronoi face.
		int maxFaceOrder() const { return _maxFaceOrder; }

		/// Returns the computed nearest neighbor bonds.
		BondsStorage* bonds() const { return _bonds.data(); }

	private:

		FloatType _edgeThreshold;
		FloatType _faceThreshold;
		double _simulationBoxVolume;
		std::atomic<double> _voronoiVolumeSum;
		std::atomic<int> _maxFaceOrder;
		SimulationCell _simCell;
		std::vector<FloatType> _radii;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _selection;
		QExplicitlySharedDataPointer<ParticleProperty> _coordinationNumbers;
		QExplicitlySharedDataPointer<ParticleProperty> _atomicVolumes;
		QExplicitlySharedDataPointer<ParticleProperty> _voronoiIndices;
		QExplicitlySharedDataPointer<BondsStorage> _bonds;
	};

	/// This stores the cached coordination numbers computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _coordinationNumbers;

	/// This stores the cached atomic volumes computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _atomicVolumes;

	/// This stores the cached Voronoi indices computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _voronoiIndices;

	/// Controls whether the modifier takes into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelected, setOnlySelected);

	/// Controls whether the modifier takes into account particle radii.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useRadii, setUseRadii);

	/// Controls whether the modifier computes Voronoi indices.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, computeIndices, setComputeIndices);

	/// Controls up to which edge count Voronoi indices are being computed.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, edgeCount, setEdgeCount);

	/// The minimum length for an edge to be counted.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, edgeThreshold, setEdgeThreshold);

	/// The minimum area for a face to be counted.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FloatType, faceThreshold, setFaceThreshold);

	/// Controls whether the modifier output nearest neighbor bonds.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, computeBonds, setComputeBonds);

	/// The total volume of the simulation cell computed by the modifier.
	double _simulationBoxVolume;

	/// The volume sum of all Voronoi cells.
	double _voronoiVolumeSum;

	/// The maximum number of edges of a Voronoi face.
	int _maxFaceOrder;

	/// The display object for rendering the bonds generated by the modifier.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(BondsDisplay, bondsDisplay, setBondsDisplay);

	/// This stores the cached results of the modifier, i.e. the bonds information.
	QExplicitlySharedDataPointer<BondsStorage> _bonds;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Voronoi analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


