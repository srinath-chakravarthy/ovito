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
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/objects/SurfaceMeshDisplay.h>
#include <plugins/particles/data/BondsStorage.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify)

/**
 * \brief A modifier that creates coordination polyhedra around atoms.
 */
class OVITO_PARTICLES_EXPORT CoordinationPolyhedraModifier : public AsynchronousParticleModifier
{
public:

	/// Constructor.
	Q_INVOKABLE CoordinationPolyhedraModifier(DataSet* dataset);

protected:

	/// Resets the modifier's result cache.
	virtual void invalidateCachedResults() override;

	/// Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// Handles reference events sent by reference targets of this object.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// This virtual method is called by the system when the modifier has been inserted into a PipelineObject.
	virtual void initializeModifier(PipelineObject* pipelineObject, ModifierApplication* modApp) override;

	/// Creates a computation engine that will compute the modifier's results.
	virtual std::shared_ptr<ComputeEngine> createEngine(TimePoint time, TimeInterval validityInterval) override;

	/// Unpacks the results of the computation engine and stores them in the modifier.
	virtual void transferComputationResults(ComputeEngine* engine) override;

	/// Lets the modifier insert the cached computation results into the modification pipeline.
	virtual PipelineStatus applyComputationResults(TimePoint time, TimeInterval& validityInterval) override;

private:

	/// Computation engine that builds the polyhedra.
	class ComputePolyhedraEngine : public ComputeEngine
	{
	public:

		/// Constructor.
		ComputePolyhedraEngine(const TimeInterval& validityInterval, ParticleProperty* positions, ParticleProperty* selection, ParticleProperty* particleTypes, 
			BondsStorage* bonds, const SimulationCell& simCell) :
			ComputeEngine(validityInterval), _positions(positions), _selection(selection), _particleTypes(particleTypes), 
			_bonds(bonds), _simCell(simCell), _mesh(new HalfEdgeMesh<>()) {}

		/// Computes the modifier's results and stores them in this object for later retrieval.
		virtual void perform() override;

		/// Returns the generated mesh.
		HalfEdgeMesh<>* mesh() { return _mesh.data(); }

	private:

		/// Constructs the convex hull from a set of points and adds the resulting polyhedron to the mesh.
		void constructConvexHull(std::vector<Point3>& vecs);

	private:

		QExplicitlySharedDataPointer<ParticleProperty> _positions;
		QExplicitlySharedDataPointer<ParticleProperty> _selection;
		QExplicitlySharedDataPointer<ParticleProperty> _particleTypes;
		QExplicitlySharedDataPointer<BondsStorage> _bonds;
		QExplicitlySharedDataPointer<HalfEdgeMesh<>> _mesh;
		SimulationCell _simCell;
	};

private:

	/// The display object for rendering the computed mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(SurfaceMeshDisplay, surfaceMeshDisplay, setSurfaceMeshDisplay);

	/// This stores the cached mesh produced by the modifier.
	QExplicitlySharedDataPointer<HalfEdgeMesh<>> _polyhedraMesh;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Coordination polyhedra");
	Q_CLASSINFO("ModifierCategory", "Modification");
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


