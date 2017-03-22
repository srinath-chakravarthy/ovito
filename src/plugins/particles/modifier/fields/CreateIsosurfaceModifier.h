///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <plugins/particles/objects/FieldQuantityObject.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/objects/SurfaceMeshDisplay.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Fields)

/*
 * Constructs a surface mesh from a particle system.
 */
class OVITO_PARTICLES_EXPORT CreateIsosurfaceModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE CreateIsosurfaceModifier(DataSet* dataset);

	/// Asks the modifier whether it can be applied to the given input data.
	virtual bool isApplicableTo(const PipelineFlowState& input) override;

	/// Asks the modifier for its validity interval at the given time.
	virtual TimeInterval modifierValidity(TimePoint time) override;

	/// Returns the level at which to create the isosurface.
	FloatType isolevel() const { return isolevelController() ? isolevelController()->currentFloatValue() : 0; }

	/// Sets the level at which to create the isosurface.
	void setIsolevel(FloatType value) { if(isolevelController()) isolevelController()->setCurrentFloatValue(value); }

protected:

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Computation engine that builds the isosurface mesh.
	class ComputeIsosurfaceEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ComputeIsosurfaceEngine(const TimeInterval& validityInterval, FieldQuantity* quantity, int vectorComponent, const SimulationCell& simCell, FloatType isolevel) :
			ComputeEngine(validityInterval), _quantity(quantity), _vectorComponent(vectorComponent), _simCell(simCell), _isolevel(isolevel), _isCompletelySolid(false), _mesh(new HalfEdgeMesh<>()) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the generated mesh.
		HalfEdgeMesh<>* mesh() { return _mesh.data(); }

		/// Returns the input field quantity.
		FieldQuantity* quantity() const { return _quantity.data(); }

		/// Indicates whether the entire simulation cell is part of the solid region.
		bool isCompletelySolid() const { return _isCompletelySolid; }

		/// Returns the minimum field value that was encountered.
		FloatType minValue() const { return _minValue; }

		/// Returns the maximum field value that was encountered.
		FloatType maxValue() const { return _maxValue; }

	private:

		FloatType _isolevel;
		int _vectorComponent;
		QExplicitlySharedDataPointer<FieldQuantity> _quantity;
		QExplicitlySharedDataPointer<HalfEdgeMesh<>> _mesh;
		SimulationCell _simCell;
		bool _isCompletelySolid;
		FloatType _minValue, _maxValue;
	};

	/// The field quantity that serves input.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(FieldQuantityReference, sourceQuantity, setSourceQuantity);

	/// This controller stores the level at which to create the isosurface.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, isolevelController, setIsolevelController);

	/// The display object for rendering the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshDisplay, surfaceMeshDisplay, setSurfaceMeshDisplay);

	/// This stores the cached surface mesh produced by the modifier.
	QExplicitlySharedDataPointer<HalfEdgeMesh<>> _surfaceMesh;

	/// Indicates that the entire simulation cell is part of the solid region.
	bool _isCompletelySolid;

	/// The minimum field value that was encountered.
	FloatType _minValue;

	/// The maximum field value that was encountered.
	FloatType _maxValue;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Create isosurface");
	Q_CLASSINFO("ModifierCategory", "Fields");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
