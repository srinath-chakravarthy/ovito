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

#ifndef __OVITO_VORONOI_ANALYSIS_MODIFIER_H
#define __OVITO_VORONOI_ANALYSIS_MODIFIER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>

namespace Ovito { namespace Plugins { namespace Particles { namespace Modifiers { namespace Analysis {

/*
 * This modifier computes the Voronoi indices of particles.
 */
class OVITO_PARTICLES_EXPORT VoronoiAnalysisModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE VoronoiAnalysisModifier(DataSet* dataset);

	/// Returns whether the modifier uses a distance cutoff to speed up the Voronoi cell calculation.
	bool useCutoff() const { return _useCutoff; }

	/// Sets whether the modifier uses a distance cutoff to speed up the Voronoi cell calculation.
	void setUseCutoff(bool useCutoff) { _useCutoff = useCutoff; }

	/// Returns the cutoff radius used to build the neighbor lists for the analysis.
	FloatType cutoff() const { return _cutoff; }

	/// \brief Sets the cutoff radius used to build the neighbor lists for the analysis.
	void setCutoff(FloatType newCutoff) { _cutoff = newCutoff; }

	/// Returns whether the modifier takes into account only selected particles.
	bool onlySelected() const { return _onlySelected; }

	/// Sets whether the modifier takes into account only selected particles.
	void setOnlySelected(bool onlySelected) { _onlySelected = onlySelected; }

	/// Returns whether the modifier takes into account particle radii.
	bool useRadii() const { return _useRadii; }

	/// Sets whether the modifier takes into account particle radii.
	void setUseRadii(bool useRadii) { _useRadii = useRadii; }

	/// Returns whether the modifier computes Voronoi indices.
	bool computeIndices() const { return _computeIndices; }

	/// Sets whether the modifier computes Voronoi indices.
	void setComputeIndices(bool computeIndices) { _computeIndices = computeIndices; }

	/// Returns up to which edge count Voronoi indices are being computed.
	int edgeCount() const { return _edgeCount; }

	/// Sets up to which edge count Voronoi indices are being computed.
	void setEdgeCount(int edgeCount) { _edgeCount = edgeCount; }

	/// Returns the minimum length for an edge to be counted.
	FloatType edgeThreshold() const { return _edgeThreshold; }

	/// Sets the minimum length for an edge to be counted.
	void setEdgeThreshold(FloatType threshold) { _edgeThreshold = threshold; }

	/// Returns the minimum area for a face to be counted.
	FloatType faceThreshold() const { return _faceThreshold; }

	/// Sets the minimum area for a face to be counted.
	void setFaceThreshold(FloatType threshold) { _faceThreshold = threshold; }

	/// Returns the computed coordination numbers.
	ParticleProperty* coordinationNumbers() const { return _coordinationNumbers.data(); }

	/// Returns the computed atomic volumes.
	ParticleProperty* atomicVolumes() const { return _atomicVolumes.data(); }

	/// Returns the computed Voronoi indices.
	ParticleProperty* voronoiIndices() const { return _voronoiIndices.data(); }

	/// Returns the total volume of the simulation cell computed by the modifier.
	double simulationBoxVolume() const { return _simulationBoxVolume; }

	/// Returns the volume sum of all Voronoi cells computed by the modifier.
	double voronoiVolumeSum() const { return _voronoiVolumeSum; }

private:

	/// Computes the modifier's results.
	class VoronoiAnalysisEngine : public AsynchronousParticleModifier::Engine
	{
	public:

		/// Constructor.
		VoronoiAnalysisEngine(ParticleProperty* positions, ParticleProperty* selection, std::vector<FloatType>&& radii,
				const SimulationCellData& simCell, FloatType cutoff,
				int edgeCount, bool computeIndices, FloatType edgeThreshold, FloatType faceThreshold) :
			_positions(positions),
			_selection(selection),
			_squaredRadii(radii),
			_simCell(simCell),
			_cutoff(cutoff),
			_edgeThreshold(edgeThreshold),
			_faceThreshold(faceThreshold),
			_coordinationNumbers(new ParticleProperty(positions->size(), ParticleProperty::CoordinationProperty, 0, true)),
			_atomicVolumes(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), sizeof(FloatType), 1, sizeof(FloatType), QStringLiteral("Atomic Volume"), true)),
			_voronoiIndices(computeIndices ? new ParticleProperty(positions->size(), qMetaTypeId<int>(), sizeof(int), edgeCount, sizeof(int) * edgeCount, QStringLiteral("Voronoi Index"), true) : nullptr) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void compute(FutureInterfaceBase& futureInterface) override;

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

	private:

		FloatType _cutoff;
		FloatType _edgeThreshold;
		FloatType _faceThreshold;
		double _simulationBoxVolume;
		double _voronoiVolumeSum;
		SimulationCellData _simCell;
		std::vector<FloatType> _squaredRadii;
		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _selection;
		QExplicitlySharedDataPointer<ParticleProperty> _coordinationNumbers;
		QExplicitlySharedDataPointer<ParticleProperty> _atomicVolumes;
		QExplicitlySharedDataPointer<ParticleProperty> _voronoiIndices;
	};

protected:

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates and initializes a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<Engine> createEngine(TimePoint time, TimeInterval& validityInterval) override;

	/// Unpacks the computation results stored in the given engine object.
	virtual void retrieveModifierResults(Engine* engine) override;

	/// Inserts the computed and cached modifier results into the modification pipeline.
	virtual PipelineStatus applyModifierResults(TimePoint time, TimeInterval& validityInterval) override;

	/// This stores the cached coordination numbers computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _coordinationNumbers;

	/// This stores the cached atomic volumes computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _atomicVolumes;

	/// This stores the cached Voronoi indices computed by the modifier.
	QExplicitlySharedDataPointer<ParticleProperty> _voronoiIndices;

	/// Controls whether the modifier uses a distance cutoff to speed up the Voronoi cell calculation.
	PropertyField<bool> _useCutoff;

	/// Controls the cutoff radius for Voronoi cell generation.
	PropertyField<FloatType> _cutoff;

	/// Controls whether the modifier takes into account only selected particles.
	PropertyField<bool> _onlySelected;

	/// Controls whether the modifier takes into account particle radii.
	PropertyField<bool> _useRadii;

	/// Controls whether the modifier computes Voronoi indices.
	PropertyField<bool> _computeIndices;

	/// Controls up to which edge count Voronoi indices are being computed.
	PropertyField<int> _edgeCount;

	/// The minimum length for an edge to be counted.
	PropertyField<FloatType> _edgeThreshold;

	/// The minimum area for a face to be counted.
	PropertyField<FloatType> _faceThreshold;

	/// The total volume of the simulation cell computed by the modifier.
	double _simulationBoxVolume;

	/// The volume sum of all Voronoi cells.
	double _voronoiVolumeSum;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Voronoi analysis");
	Q_CLASSINFO("ModifierCategory", "Analysis");

	DECLARE_PROPERTY_FIELD(_useCutoff);
	DECLARE_PROPERTY_FIELD(_cutoff);
	DECLARE_PROPERTY_FIELD(_onlySelected);
	DECLARE_PROPERTY_FIELD(_useRadii);
	DECLARE_PROPERTY_FIELD(_computeIndices);
	DECLARE_PROPERTY_FIELD(_edgeCount);
	DECLARE_PROPERTY_FIELD(_edgeThreshold);
	DECLARE_PROPERTY_FIELD(_faceThreshold);
};

namespace Internal {

/**
 * A properties editor for the VoronoiAnalysisModifier class.
 */
class VoronoiAnalysisModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE VoronoiAnalysisModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace

}}}}}	// End of namespace

#endif // __OVITO_VORONOI_ANALYSIS_MODIFIER_H
