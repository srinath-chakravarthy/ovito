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

#include <plugins/vorotop/VoroTopPlugin.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/gui/properties/BooleanGroupBoxParameterUI.h>
#include <core/gui/properties/IntegerParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include "VoroTopModifier.h"

#include <voro++.hh>

namespace Ovito { namespace Plugins { namespace VoroTop {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(VoroTop, VoroTopModifier, StructureIdentificationModifier);
IMPLEMENT_OVITO_OBJECT(VoroTop, VoroTopModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(VoroTopModifier, VoroTopModifierEditor);
DEFINE_PROPERTY_FIELD(VoroTopModifier, _useRadii, "UseRadii");
SET_PROPERTY_FIELD_LABEL(VoroTopModifier, _useRadii, "Use particle radii");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
VoroTopModifier::VoroTopModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
	_useRadii(false)
{
	INIT_PROPERTY_FIELD(VoroTopModifier::_useRadii);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> VoroTopModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	// Get selection particle property.
	ParticlePropertyObject* selectionProperty = nullptr;
	if(onlySelectedParticles())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty);

	// Get particle radii.
	std::vector<FloatType> radii;
	if(useRadii())
		radii = std::move(inputParticleRadii(time, validityInterval));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<VoroTopAnalysisEngine>(
			validityInterval,
			posProperty->storage(),
			selectionProperty ? selectionProperty->storage() : nullptr,
			std::move(radii),
			inputCell->data());
}

/******************************************************************************
* Processes a single Voronoi cell.
******************************************************************************/
void VoroTopModifier::VoroTopAnalysisEngine::processCell(voro::voronoicell_neighbor& vcell, size_t particleIndex, QMutex* mutex)
{
	// Assign a structure type to the current particle.
	// As an example, we use the particle index as structure type, i.e., as if every
	// particle had a unique local structure.
	int structureType = particleIndex;
	structures()->setInt(particleIndex, structureType);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void VoroTopModifier::VoroTopAnalysisEngine::perform()
{
	setProgressText(tr("Performing VoroTop analysis"));

	if(positions()->size() == 0)
		return;	// Nothing to do

	// Decide whether to use Voro++ container class or our own implementation.
	if(cell().isAxisAligned()) {
		// Use Voro++ container.
		double ax = cell().matrix()(0,3);
		double ay = cell().matrix()(1,3);
		double az = cell().matrix()(2,3);
		double bx = ax + cell().matrix()(0,0);
		double by = ay + cell().matrix()(1,1);
		double bz = az + cell().matrix()(2,2);
		if(ax > bx) std::swap(ax,bx);
		if(ay > by) std::swap(ay,by);
		if(az > bz) std::swap(az,bz);
		double volumePerCell = (bx - ax) * (by - ay) * (bz - az) * voro::optimal_particles / positions()->size();
		double cellSize = pow(volumePerCell, 1.0/3.0);
		int nx = (int)std::ceil((bx - ax) / cellSize);
		int ny = (int)std::ceil((by - ay) / cellSize);
		int nz = (int)std::ceil((bz - az) / cellSize);

		if(_radii.empty()) {
			voro::container voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
					cell().pbcFlags()[0], cell().pbcFlags()[1], cell().pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

			// Insert particles into Voro++ container.
			size_t count = 0;
			for(size_t index = 0; index < positions()->size(); index++) {
				// Skip unselected particles (if requested).
				if(selection() && selection()->getInt(index) == 0) {
					structures()->setInt(index, 0);
					continue;
				}
				const Point3& p = positions()->getPoint3(index);
				voroContainer.put(index, p.x(), p.y(), p.z());
				count++;
			}
			if(!count) return;

			setProgressRange(count);
			setProgressValue(0);
			voro::c_loop_all cl(voroContainer);
			voro::voronoicell_neighbor v;
			if(cl.start()) {
				do {
					if(!incrementProgressValue())
						return;
					if(!voroContainer.compute_cell(v,cl))
						continue;
					processCell(v, cl.pid(), nullptr);
					count--;
				}
				while(cl.inc());
			}
			if(count)
				throw Exception(tr("Could not compute Voronoi cell for some particles."));
		}
		else {
			voro::container_poly voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
					cell().pbcFlags()[0], cell().pbcFlags()[1], cell().pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

			// Insert particles into Voro++ container.
			size_t count = 0;
			for(size_t index = 0; index < positions()->size(); index++) {
				structures()->setInt(index, 0);
				// Skip unselected particles (if requested).
				if(selection() && selection()->getInt(index) == 0) {
					continue;
				}
				const Point3& p = positions()->getPoint3(index);
				voroContainer.put(index, p.x(), p.y(), p.z(), _radii[index]);
				count++;
			}

			if(!count) return;
			setProgressRange(count);
			setProgressValue(0);
			voro::c_loop_all cl(voroContainer);
			voro::voronoicell_neighbor v;
			if(cl.start()) {
				do {
					if(!incrementProgressValue())
						return;
					if(!voroContainer.compute_cell(v,cl))
						continue;
					processCell(v, cl.pid(), nullptr);
					count--;
				}
				while(cl.inc());
			}
			if(count)
				throw Exception(tr("Could not compute Voronoi cell for some particles."));
		}
	}
	else {
		// Prepare the nearest neighbor list generator.
		NearestNeighborFinder nearestNeighborFinder;
		if(!nearestNeighborFinder.prepare(positions(), cell(), selection(), this))
			return;

		// Squared particle radii (input was just radii).
		for(auto& r : _radii)
			r = r*r;

		// This is the size we use to initialize Voronoi cells. Must be larger than the simulation box.
		double boxDiameter = sqrt(
				  cell().matrix().column(0).squaredLength()
				+ cell().matrix().column(1).squaredLength()
				+ cell().matrix().column(2).squaredLength());

		// The normal vectors of the three cell planes.
		std::array<Vector3,3> planeNormals;
		planeNormals[0] = cell().cellNormalVector(0);
		planeNormals[1] = cell().cellNormalVector(1);
		planeNormals[2] = cell().cellNormalVector(2);

		Point3 corner1 = Point3::Origin() + cell().matrix().column(3);
		Point3 corner2 = corner1 + cell().matrix().column(0) + cell().matrix().column(1) + cell().matrix().column(2);

		QMutex mutex;

		// Perform analysis, particle-wise parallel.
		parallelFor(positions()->size(), *this,
				[&nearestNeighborFinder, this, boxDiameter,
				 planeNormals, corner1, corner2, &mutex](size_t index) {

			// Reset structure type.
			structures()->setInt(index, 0);

			// Skip unselected particles (if requested).
			if(selection() && selection()->getInt(index) == 0)
				return;

			// Build Voronoi cell.
			voro::voronoicell_neighbor v;

			// Initialize the Voronoi cell to be a cube larger than the simulation cell, centered at the origin.
			v.init(-boxDiameter, boxDiameter, -boxDiameter, boxDiameter, -boxDiameter, boxDiameter);

			// Cut Voronoi cell at simulation cell boundaries in non-periodic directions.
			bool skipParticle = false;
			for(size_t dim = 0; dim < 3; dim++) {
				if(!cell().pbcFlags()[dim]) {
					double r;
					r = 2 * planeNormals[dim].dot(corner2 - positions()->getPoint3(index));
					if(r <= 0) skipParticle = true;
					v.nplane(planeNormals[dim].x() * r, planeNormals[dim].y() * r, planeNormals[dim].z() * r, r*r, -1);
					r = 2 * planeNormals[dim].dot(positions()->getPoint3(index) - corner1);
					if(r <= 0) skipParticle = true;
					v.nplane(-planeNormals[dim].x() * r, -planeNormals[dim].y() * r, -planeNormals[dim].z() * r, r*r, -1);
				}
			}
			// Skip particles that are located outside of non-periodic box boundaries.
			if(skipParticle)
				return;

			// This function will be called for every neighbor particle.
			int nvisits = 0;
			auto visitFunc = [this, &v, &nvisits, index](const NearestNeighborFinder::Neighbor& n, FloatType& mrs) {
				// Skip unselected particles (if requested).
				OVITO_ASSERT(!selection() || selection()->getInt(n.index));
				FloatType rs = n.distanceSq;
				if(!_radii.empty())
					 rs += _radii[index] - _radii[n.index];
				v.nplane(n.delta.x(), n.delta.y(), n.delta.z(), rs, n.index);
				if(nvisits == 0) {
					mrs = v.max_radius_squared();
					nvisits = 100;
				}
				nvisits--;
			};

			// Visit all neighbors of the current particles.
			nearestNeighborFinder.visitNeighbors(nearestNeighborFinder.particlePos(index), visitFunc);

			processCell(v, index, &mutex);
		});
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void VoroTopModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute modifier results when the parameters change.
	if(field == PROPERTY_FIELD(VoroTopModifier::_useRadii))
		invalidateCachedResults();
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VoroTopModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("VoroTop analysis"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	QGridLayout* sublayout;
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setSpacing(4);
	gridlayout->setColumnStretch(1, 1);
	int row = 0;

	// Atomic radii.
	BooleanParameterUI* useRadiiPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoroTopModifier::_useRadii));
	gridlayout->addWidget(useRadiiPUI->checkBox(), row++, 0, 1, 2);

	// Only selected particles.
	BooleanParameterUI* onlySelectedPUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::_onlySelectedParticles));
	gridlayout->addWidget(onlySelectedPUI->checkBox(), row++, 0, 1, 2);

	layout->addLayout(gridlayout);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
