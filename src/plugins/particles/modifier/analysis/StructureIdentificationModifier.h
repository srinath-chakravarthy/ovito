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
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/ParticleType.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

/**
 * \brief Base class for modifiers that assign a structure type to each particle.
 */
class OVITO_PARTICLES_EXPORT StructureIdentificationModifier : public AsynchronousParticleModifier
{
public:

	/// Computes the modifier's results.
	class StructureIdentificationEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		StructureIdentificationEngine(const TimeInterval& validityInterval, ParticleProperty* positions, const SimulationCell& simCell, const QVector<bool>& typesToIdentify, ParticleProperty* selection = nullptr) :
			ComputeEngine(validityInterval),
			_positions(positions), _simCell(simCell),
			_typesToIdentify(typesToIdentify),
			_selection(selection),
			_structures(new ParticleProperty(positions->size(), ParticleProperty::StructureTypeProperty, 0, false)) {}

		/// Returns the property storage that contains the input particle positions.
		ParticleProperty* positions() const { return _positions.data(); }

		/// Returns the property storage that contains the computed per-particle structure types.
		ParticleProperty* structures() const { return _structures.data(); }

		/// Returns the property storage that contains the particle selection (optional).
		ParticleProperty* selection() const { return _selection.data(); }

		/// Returns the simulation cell data.
		const SimulationCell& cell() const { return _simCell; }

		/// Returns the list of structure types to search for.
		const QVector<bool>& typesToIdentify() const { return _typesToIdentify; }

	private:

		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _structures;
		QExplicitlySharedDataPointer<ParticleProperty> _selection;
		SimulationCell _simCell;
		QVector<bool> _typesToIdentify;
	};

public:

	/// Constructor.
	StructureIdentificationModifier(DataSet* dataset);

	/// Returns an array that contains the number of matching particles for each structure type.
	const QList<int>& structureCounts() const { return _structureCounts; }

	/// Returns the cached results of the modifier, i.e. the structures assigned to the particles.
	ParticleProperty* structureData() const { return _structureData.data(); }

	/// Replaces the cached results of the modifier, i.e. the structures assigned to the particles.
	void setStructureData(ParticleProperty* structureData) { _structureData = structureData; }

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Is called when a RefTarget referenced by this object has generated an event.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Inserts a structure type into the list.
	void addStructureType(ParticleType* type) { _structureTypes.push_back(type); }

	/// Create an instance of the ParticleType class to represent a structure type.
	void createStructureType(int id, ParticleTypeProperty::PredefinedStructureType predefType);

	/// Returns a bit flag array which indicates what structure types to search for.
	QVector<bool> getTypesToIdentify(int numTypes) const;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// This stores the cached results of the modifier, i.e. the structures assigned to the particles.
	QExplicitlySharedDataPointer<ParticleProperty> _structureData;

	/// Contains the list of structure types recognized by this analysis modifier.
	DECLARE_MODIFIABLE_VECTOR_REFERENCE_FIELD(ParticleType, structureTypes, setStructureTypes);

	/// Controls whether analysis should take into account only selected particles.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, onlySelectedParticles, setOnlySelectedParticles);

	/// The number of matching particles for each structure type.
	QList<int> _structureCounts;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


