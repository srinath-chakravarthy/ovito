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

#include <plugins/particles/Particles.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/particles/objects/BondsObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "VoronoiAnalysisModifier.h"

#include <voro++.hh>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(VoronoiAnalysisModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, onlySelected, "OnlySelected");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, useRadii, "UseRadii");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, computeIndices, "ComputeIndices");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, computeBonds, "ComputeBonds");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, edgeCount, "EdgeCount");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, edgeThreshold, "EdgeThreshold");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, faceThreshold, "FaceThreshold");
DEFINE_FLAGS_REFERENCE_FIELD(VoronoiAnalysisModifier, bondsDisplay, "BondsDisplay", BondsDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, onlySelected, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, useRadii, "Use particle radii");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, computeIndices, "Compute Voronoi indices");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, computeBonds, "Generate neighbor bonds");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, edgeCount, "Maximum edge count");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, edgeThreshold, "Edge length threshold");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, faceThreshold, "Face area threshold");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, bondsDisplay, "Bonds display");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VoronoiAnalysisModifier, edgeThreshold, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(VoronoiAnalysisModifier, faceThreshold, FloatParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(VoronoiAnalysisModifier, edgeCount, IntegerParameterUnit, 3, 18);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
VoronoiAnalysisModifier::VoronoiAnalysisModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_onlySelected(false), _computeIndices(false), _edgeCount(6),
	_useRadii(false), _edgeThreshold(0), _faceThreshold(0),
	_simulationBoxVolume(0), _voronoiVolumeSum(0), _maxFaceOrder(0), _computeBonds(false)
{
	INIT_PROPERTY_FIELD(onlySelected);
	INIT_PROPERTY_FIELD(useRadii);
	INIT_PROPERTY_FIELD(computeIndices);
	INIT_PROPERTY_FIELD(computeBonds);
	INIT_PROPERTY_FIELD(edgeCount);
	INIT_PROPERTY_FIELD(edgeThreshold);
	INIT_PROPERTY_FIELD(faceThreshold);
	INIT_PROPERTY_FIELD(bondsDisplay);

	// Create the display object for bonds rendering.
	setBondsDisplay(new BondsDisplay(dataset));
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool VoronoiAnalysisModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display object.
	if(source == bondsDisplay())
		return false;

	return AsynchronousParticleModifier::referenceEvent(source, event);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> VoronoiAnalysisModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	// Get selection particle property.
	ParticlePropertyObject* selectionProperty = nullptr;
	if(onlySelected())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty);

	// Get particle radii.
	std::vector<FloatType> radii;
	if(useRadii())
		radii = std::move(inputParticleRadii(time, validityInterval));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<VoronoiAnalysisEngine>(
			validityInterval,
			posProperty->storage(),
			selectionProperty ? selectionProperty->storage() : nullptr,
			std::move(radii),
			inputCell->data(),
			qMax(1, edgeCount()),
			computeIndices(),
			computeBonds(),
			edgeThreshold(),
			faceThreshold());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void VoronoiAnalysisModifier::VoronoiAnalysisEngine::perform()
{
	setProgressText(tr("Computing Voronoi cells"));

	// Compute the total simulation cell volume.
	_simulationBoxVolume = _simCell.volume3D();

	if(_positions->size() == 0 || _simulationBoxVolume == 0)
		return;	// Nothing to do

	// The squared edge length threshold.
	// Add additional factor of 4 because Voronoi cell vertex coordinates are all scaled by factor of 2.
	FloatType sqEdgeThreshold = _edgeThreshold * _edgeThreshold * 4;

	auto processCell = [this, sqEdgeThreshold](voro::voronoicell_neighbor& v, size_t index, QMutex* mutex) {
		// Compute cell volume.
		double vol = v.volume();
		_atomicVolumes->setFloat(index, (FloatType)vol);

		// Compute total volume of Voronoi cells.
		// Loop is for lock-free write access to shared max counter.
		double prevVolumeSum = _voronoiVolumeSum;
		while(!_voronoiVolumeSum.compare_exchange_weak(prevVolumeSum, prevVolumeSum + vol));

		int localMaxFaceOrder = 0;
		// Iterate over the Voronoi faces and their edges.
		int coordNumber = 0;
		for(int i = 1; i < v.p; i++) {
			for(int j = 0; j < v.nu[i]; j++) {
				int k = v.ed[i][j];
				if(k >= 0) {
					int neighbor_id = v.ne[i][j];
					int faceOrder = 0;
					FloatType area = 0;
					// Compute length of first face edge.
					Vector3 d(v.pts[3*k] - v.pts[3*i], v.pts[3*k+1] - v.pts[3*i+1], v.pts[3*k+2] - v.pts[3*i+2]);
					if(d.squaredLength() > sqEdgeThreshold)
						faceOrder++;
					v.ed[i][j] = -1 - k;
					int l = v.cycle_up(v.ed[i][v.nu[i]+j], k);
					do {
						int m = v.ed[k][l];
						// Compute length of current edge.
						if(sqEdgeThreshold != 0) {
							Vector3 u(v.pts[3*m] - v.pts[3*k], v.pts[3*m+1] - v.pts[3*k+1], v.pts[3*m+2] - v.pts[3*k+2]);
							if(u.squaredLength() > sqEdgeThreshold)
								faceOrder++;
						}
						else faceOrder++;
						if(_faceThreshold != 0) {
							Vector3 w(v.pts[3*m] - v.pts[3*i], v.pts[3*m+1] - v.pts[3*i+1], v.pts[3*m+2] - v.pts[3*i+2]);
							area += d.cross(w).length() / 8;
							d = w;
						}
						v.ed[k][l] = -1 - m;
						l = v.cycle_up(v.ed[k][v.nu[k]+l], m);
						k = m;
					}
					while(k != i);
					if((_faceThreshold == 0 || area > _faceThreshold) && faceOrder >= 3) {
						coordNumber++;
						if(faceOrder > localMaxFaceOrder)
							localMaxFaceOrder = faceOrder;
						if(_bonds && neighbor_id >= 0) {
							OVITO_ASSERT(neighbor_id < _positions->size());
							Vector3 delta = _positions->getPoint3(index) - _positions->getPoint3(neighbor_id);
							Vector_3<int8_t> pbcShift = Vector_3<int8_t>::Zero();
							for(size_t dim = 0; dim < 3; dim++) {
								if(_simCell.pbcFlags()[dim])
									pbcShift[dim] = (int8_t)floor(_simCell.inverseMatrix().prodrow(delta, dim) + FloatType(0.5));
							}
							QMutexLocker locker(mutex);
							_bonds->push_back(Bond{ pbcShift, (unsigned int)index, (unsigned int)neighbor_id });
						}
						faceOrder--;
						if(_voronoiIndices && faceOrder < (int)_voronoiIndices->componentCount())
							_voronoiIndices->setIntComponent(index, faceOrder, _voronoiIndices->getIntComponent(index, faceOrder) + 1);
					}
				}
			}
		}

		// Store computed result.
		_coordinationNumbers->setInt(index, coordNumber);

		// Keep track of the maximum number of edges per face.
		// Loop is for lock-free write access to shared max counter.
		int prevMaxFaceOrder = _maxFaceOrder;
		while(localMaxFaceOrder > prevMaxFaceOrder && !_maxFaceOrder.compare_exchange_weak(prevMaxFaceOrder, localMaxFaceOrder));
	};

	// Decide whether to use Voro++ container class or our own implementation.
	if(_simCell.isAxisAligned()) {
		// Use Voro++ container.
		double ax = _simCell.matrix()(0,3);
		double ay = _simCell.matrix()(1,3);
		double az = _simCell.matrix()(2,3);
		double bx = ax + _simCell.matrix()(0,0);
		double by = ay + _simCell.matrix()(1,1);
		double bz = az + _simCell.matrix()(2,2);
		if(ax > bx) std::swap(ax,bx);
		if(ay > by) std::swap(ay,by);
		if(az > bz) std::swap(az,bz);
		double volumePerCell = (bx - ax) * (by - ay) * (bz - az) * voro::optimal_particles / _positions->size();
		double cellSize = pow(volumePerCell, 1.0/3.0);
		int nx = (int)std::ceil((bx - ax) / cellSize);
		int ny = (int)std::ceil((by - ay) / cellSize);
		int nz = (int)std::ceil((bz - az) / cellSize);

		if(_radii.empty()) {
			voro::container voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
					_simCell.pbcFlags()[0], _simCell.pbcFlags()[1], _simCell.pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

			// Insert particles into Voro++ container.
			size_t count = 0;
			for(size_t index = 0; index < _positions->size(); index++) {
				// Skip unselected particles (if requested).
				if(_selection && _selection->getInt(index) == 0)
					continue;
				const Point3& p = _positions->getPoint3(index);
				voroContainer.put(index, p.x(), p.y(), p.z());
				count++;
			}
			if(!count) return;

			setProgressValue(0);
			setProgressMaximum(count);
#if 1
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
#else
			// Perform analysis on each particle.
			QMutex mutex;
			parallelForChunks(_positions->size(), *this, [this, &processCell, &voroContainer, &mutex, nx, ny, nz](size_t startIndex, size_t count, FutureInterfaceBase& progress) {
				size_t endIndex = startIndex + count;
				voro::voronoicell_neighbor v;
				voro::c_loop_all cl(voroContainer);
				voro::voro_compute<voro::container> vc(voroContainer,
						_simCell.pbcFlags()[0] ? 2*nx+1 : nx,
						_simCell.pbcFlags()[1] ? 2*ny+1 : ny,
						_simCell.pbcFlags()[2] ? 2*nz+1 : nz);
				if(cl.start()) {
					do {
						if(cl.pid() < startIndex || cl.pid() >= endIndex)
							continue;
						// Update progress indicator.
						if((cl.pid() % 256) == 0)
							progress.incrementProgressValue(256);
						// Break out of loop when operation was canceled.
						if(progress.isCanceled())
							break;
						if(!vc.compute_cell(v,cl.ijk,cl.q,cl.i,cl.j,cl.k))
							continue;
						processCell(v, cl.pid(), &mutex);
					}
					while(cl.inc());
				}
			});
#endif
		}
		else {
#if 1
			voro::container_poly voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
					_simCell.pbcFlags()[0], _simCell.pbcFlags()[1], _simCell.pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

			// Insert particles into Voro++ container.
			size_t count = 0;
			for(size_t index = 0; index < _positions->size(); index++) {
				// Skip unselected particles (if requested).
				if(_selection && _selection->getInt(index) == 0)
					continue;
				const Point3& p = _positions->getPoint3(index);
				voroContainer.put(index, p.x(), p.y(), p.z(), _radii[index]);
				count++;
			}

			if(!count) return;
			setProgressValue(0);
			setProgressMaximum(count);

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
#else
			setProgressRange(_positions->size());
			setProgressValue(0);

			// Perform analysis on each particle.
			QMutex mutex;
			parallelForChunks(_positions->size(), *this, [this, &processCell, &mutex, ax, bx, ay, by, az, bz, nx, ny, nz](size_t startIndex, size_t count, FutureInterfaceBase& progress) {

				voro::container_poly voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
						_simCell.pbcFlags()[0], _simCell.pbcFlags()[1], _simCell.pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

				// Insert particles into Voro++ container.
				for(size_t index = 0; index < _positions->size(); index++) {
					// Skip unselected particles (if requested).
					if(_selection && _selection->getInt(index) == 0)
						continue;
					const Point3& p = _positions->getPoint3(index);
					voroContainer.put(index, p.x(), p.y(), p.z(), _radii[index]);
				}

				size_t endIndex = startIndex + count;
				voro::voronoicell_neighbor v;
				voro::c_loop_all cl(voroContainer);
				if(cl.start()) {
					do {
						if(cl.pid() < startIndex || cl.pid() >= endIndex)
							continue;
						// Update progress indicator.
						if((cl.pid() % 256) == 0)
							progress.incrementProgressValue(256);
						// Break out of loop when operation was canceled.
						if(progress.isCanceled())
							break;
						if(!voroContainer.compute_cell(v,cl))
							continue;
						processCell(v, cl.pid(), &mutex);
					}
					while(cl.inc());
				}
			});
#endif
		}
	}
	else {
		// Prepare the nearest neighbor list generator.
		NearestNeighborFinder nearestNeighborFinder;
		if(!nearestNeighborFinder.prepare(_positions.data(), _simCell, _selection.data(), *this))
			return;

		// Squared particle radii (input was just radii).
		for(auto& r : _radii)
			r = r*r;

		// This is the size we use to initialize Voronoi cells. Must be larger than the simulation box.
		double boxDiameter = sqrt(
				  _simCell.matrix().column(0).squaredLength()
				+ _simCell.matrix().column(1).squaredLength()
				+ _simCell.matrix().column(2).squaredLength());

		// The normal vectors of the three cell planes.
		std::array<Vector3,3> planeNormals;
		planeNormals[0] = _simCell.cellNormalVector(0);
		planeNormals[1] = _simCell.cellNormalVector(1);
		planeNormals[2] = _simCell.cellNormalVector(2);

		Point3 corner1 = Point3::Origin() + _simCell.matrix().column(3);
		Point3 corner2 = corner1 + _simCell.matrix().column(0) + _simCell.matrix().column(1) + _simCell.matrix().column(2);

		QMutex mutex;

		// Perform analysis, particle-wise parallel.
		parallelFor(_positions->size(), *this,
				[&nearestNeighborFinder, this, sqEdgeThreshold, boxDiameter,
				 planeNormals, corner1, corner2, &processCell, &mutex](size_t index) {

			// Skip unselected particles (if requested).
			if(_selection && _selection->getInt(index) == 0)
				return;

			// Build Voronoi cell.
			voro::voronoicell_neighbor v;

			// Initialize the Voronoi cell to be a cube larger than the simulation cell, centered at the origin.
			v.init(-boxDiameter, boxDiameter, -boxDiameter, boxDiameter, -boxDiameter, boxDiameter);

			// Cut Voronoi cell at simulation cell boundaries in non-periodic directions.
			bool skipParticle = false;
			for(size_t dim = 0; dim < 3; dim++) {
				if(!_simCell.pbcFlags()[dim]) {
					double r;
					r = 2 * planeNormals[dim].dot(corner2 - _positions->getPoint3(index));
					if(r <= 0) skipParticle = true;
					v.nplane(planeNormals[dim].x() * r, planeNormals[dim].y() * r, planeNormals[dim].z() * r, r*r, -1);
					r = 2 * planeNormals[dim].dot(_positions->getPoint3(index) - corner1);
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
				OVITO_ASSERT(!_selection || _selection->getInt(n.index));
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
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void VoronoiAnalysisModifier::transferComputationResults(ComputeEngine* engine)
{
	VoronoiAnalysisEngine* eng = static_cast<VoronoiAnalysisEngine*>(engine);
	_coordinationNumbers = eng->coordinationNumbers();
	_atomicVolumes = eng->atomicVolumes();
	_voronoiIndices = eng->voronoiIndices();
	_simulationBoxVolume = eng->simulationBoxVolume();
	_voronoiVolumeSum = eng->voronoiVolumeSum();
	_maxFaceOrder = eng->maxFaceOrder();
	_bonds = eng->bonds();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus VoronoiAnalysisModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_coordinationNumbers)
		throwException(tr("No computation results available."));

	if(inputParticleCount() != _coordinationNumbers->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	outputStandardProperty(_coordinationNumbers.data());
	outputCustomProperty(_atomicVolumes.data());
	if(_voronoiIndices)
		outputCustomProperty(_voronoiIndices.data());

	// Check computed Voronoi cell volume sum.
	if(std::abs(_voronoiVolumeSum - _simulationBoxVolume) > 1e-8 * inputParticleCount() * _simulationBoxVolume) {
		//qDebug() << _voronoiVolumeSum;
		//qDebug() << _simulationBoxVolume;
		//qDebug() << std::abs(_voronoiVolumeSum - _simulationBoxVolume);
		//qDebug() << (1e-8 * inputParticleCount() * _simulationBoxVolume);
		return PipelineStatus(PipelineStatus::Warning,
				tr("The volume sum of all Voronoi cells does not match the simulation box volume. "
						"This may be a result of particles being located outside of the simulation box boundaries. "
						"See user manual for more information.\n"
						"Simulation box volume: %1\n"
						"Voronoi cell volume sum: %2").arg(_simulationBoxVolume).arg(_voronoiVolumeSum));
	}

	if(_bonds) {
		// Insert output object into the pipeline.
		addBonds(_bonds.data(), bondsDisplay());
	}

	output().attributes().insert(QStringLiteral("Voronoi.max_face_order"), QVariant::fromValue(_maxFaceOrder));

	if(_voronoiIndices && _maxFaceOrder > _voronoiIndices->componentCount()) {
		return PipelineStatus(PipelineStatus::Warning,
				tr("The Voronoi tessellation contains faces with up to %1 edges "
						"(ignoring edges below the length threshold). "
						"This number exceeds the current maximum edge count, "
						"and the computed Voronoi index vectors are therefore truncated. "
						"You should consider increasing the maximum edge count parameter to %1 edges "
						"to not truncate the Voronoi index vectors and avoid this message."
						).arg(_maxFaceOrder));
	}
	else {
		return PipelineStatus::Success;
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void VoronoiAnalysisModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
