///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/objects/DataObject.h>
#include <core/reference/CloneHelper.h>
#include <core/reference/RefTargetListener.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include <plugins/particles/objects/SimulationCellObject.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers)

/**
 * \brief Abstract base class for modifiers that operate on a system of particles.
 */
class OVITO_PARTICLES_EXPORT ParticleModifier : public Modifier
{
protected:

	/// Constructor.
	ParticleModifier(DataSet* dataset) : Modifier(dataset) {}

public:

	/// This modifies the input object.
	virtual PipelineStatus modifyObject(TimePoint time, ModifierApplication* modApp, PipelineFlowState& state) override;

	/// \brief Returns a structure that describes the current status of the modifier.
	virtual PipelineStatus status() const override { return _modifierStatus; }

	/// \brief Asks the modifier whether it can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override;

	/// Returns a standard particle property from the input state.
	/// The returned property may be NULL if it does not exist.
	ParticlePropertyObject* inputStandardProperty(ParticleProperty::Type which) const;

	/// Returns a standard bond property from the input state.
	/// The returned property may be NULL if it does not exist.
	BondPropertyObject* inputStandardBondProperty(BondProperty::Type which) const;

	/// Returns the given standard particle property from the input object.
	/// The returned property may not be modified. If the input object does
	/// not contain the standard property then an exception is thrown.
	ParticlePropertyObject* expectStandardProperty(ParticleProperty::Type which) const;

	/// Returns the given standard bond property from the input object.
	/// The returned property may not be modified. If the input object does
	/// not contain the standard property then an exception is thrown.
	BondPropertyObject* expectStandardBondProperty(BondProperty::Type which) const;

	/// Returns the property with the given name from the input particles.
	/// The returned property may not be modified. If the input object does
	/// not contain a property with the given name and data type, then an exception is thrown.
	ParticlePropertyObject* expectCustomProperty(const QString& propertyName, int dataType, size_t componentCount = 1) const;

	/// Returns the input simulation cell.
	/// The returned object may not be modified. If the input does
	/// not contain a simulation cell, an exception is thrown.
	SimulationCellObject* expectSimulationCell() const;

	/// Returns the input bonds.
	/// The returned object may not be modified. If the input does
	/// not contain any bonds, an exception is thrown.
	BondsObject* expectBonds() const;

	/// Creates a standard particle property in the modifier's output.
	/// If the particle property already exists in the input, its contents are copied to the
	/// output property by this method.
	ParticlePropertyObject* outputStandardProperty(ParticleProperty::Type which, bool initializeMemory = false);

	/// Creates a standard particle property in the modifier's output and sets its content.
	ParticlePropertyObject* outputStandardProperty(ParticleProperty* storage);

	/// Creates a custom particle property in the modifier's output.
	/// If the particle property already exists in the input, its contents are copied to the
	/// output property by this method.
	ParticlePropertyObject* outputCustomProperty(const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory = false);

	/// Creates a custom particle property in the modifier's output and sets its content.
	ParticlePropertyObject* outputCustomProperty(ParticleProperty* storage);

	/// Creates a standard bond property in the modifier's output.
	/// If the bond property already exists in the input, its contents are copied to the
	/// output property by this method.
	BondPropertyObject* outputStandardBondProperty(BondProperty::Type which, bool initializeMemory = false);

	/// Creates a standard bond property in the modifier's output and sets its content.
	BondPropertyObject* outputStandardBondProperty(BondProperty* storage);

	/// Creates a custom bond property in the modifier's output.
	/// If the bond property already exists in the input, its contents are copied to the
	/// output property by this method.
	BondPropertyObject* outputCustomBondProperty(const QString& name, int dataType, size_t componentCount, size_t stride, bool initializeMemory = false);

	/// Creates a custom bond property in the modifier's output and sets its content.
	BondPropertyObject* outputCustomBondProperty(BondProperty* storage);

	/// Returns the modifier's output simulation cell.
	SimulationCellObject* outputSimulationCell();

	/// Removes the given particle property from the modifier's output.
	void removeOutputProperty(ParticlePropertyObject* property);

	/// Returns the number of particles in the input.
	size_t inputParticleCount() const { return _inputParticleCount; }

	/// Returns the number of particles in the output.
	size_t outputParticleCount() const { return _outputParticleCount; }

	/// Returns the number of bonds in the input.
	size_t inputBondCount() const { return _inputBondCount; }

	/// Returns the number of bonds in the output.
	size_t outputBondCount() const { return _outputBondCount; }

	/// Returns a vector with the input particles colors.
	std::vector<Color> inputParticleColors(TimePoint time, TimeInterval& validityInterval);

	/// Returns a vector with the input particles radii.
	std::vector<FloatType> inputParticleRadii(TimePoint time, TimeInterval& validityInterval);

	/// Returns a vector with the input bond colors.
	std::vector<Color> inputBondColors(TimePoint time, TimeInterval& validityInterval);

	/// Deletes the particles given by the bit-mask.
	/// Returns the number of remaining particles.
	size_t deleteParticles(const boost::dynamic_bitset<>& mask, size_t deleteCount);

	/// Adds a set of new bonds to the system.
	BondsObject* addBonds(BondsStorage* newBonds, BondsDisplay* bondsDisplay, const std::vector<BondProperty*>& bondProperties = std::vector<BondProperty*>());

	/// Returns a reference to the input state.
	PipelineFlowState& input() { return _input; }

	/// Returns a reference to the output state.
	PipelineFlowState& output() { return _output; }

	/// Returns a clone helper object that should be used to create shallow and deep copies.
	CloneHelper* cloneHelper() {
		if(!_cloneHelper) _cloneHelper.reset(new CloneHelper());
		return _cloneHelper.data();
	}

protected:

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Modifies the particle object. This function must be implemented by sub-classes
	/// do the modifier specific work. The time interval passed
	/// to the function should be reduced to the interval where the returned object is valid/constant.
	virtual PipelineStatus modifyParticles(TimePoint time, TimeInterval& validityInterval) = 0;

	/// Sets the status returned by the modifier and generates a ReferenceEvent::ObjectStatusChanged event.
	void setStatus(const PipelineStatus& status);

	/// Returns the current ModifierApplication object. It is only valid to call this
	/// method from an implementation of modifyParticles().
	ModifierApplication* modifierApplication() const { OVITO_CHECK_POINTER(_modApp); return _modApp; }

protected:

	/// The clone helper object that is used to create shallow and deep copies
	/// of the atoms object and its channels.
	QScopedPointer<CloneHelper> _cloneHelper;

	/// The current ModifierApplication object.
	ModifierApplication* _modApp;

	/// The input state.
	PipelineFlowState _input;

	/// The output state.
	PipelineFlowState _output;

	/// The number of particles in the input.
	size_t _inputParticleCount;

	/// The number of particles in the output.
	size_t _outputParticleCount;

	/// The number of bonds in the input.
	size_t _inputBondCount;

	/// The number of bonds in the output.
	size_t _outputBondCount;

	/// The status returned by the modifier.
	PipelineStatus _modifierStatus;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


