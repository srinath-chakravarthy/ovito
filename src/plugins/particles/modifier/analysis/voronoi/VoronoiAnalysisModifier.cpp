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
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/gui/properties/BooleanGroupBoxParameterUI.h>
#include <core/gui/properties/IntegerParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <plugins/particles/util/OnTheFlyNeighborListBuilder.h>
#include <plugins/particles/util/TreeNeighborListBuilder.h>
#include "VoronoiAnalysisModifier.h"

#include <voro++.hh>

namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, VoronoiAnalysisModifier, AsynchronousParticleModifier);
IMPLEMENT_OVITO_OBJECT(Particles, VoronoiAnalysisModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(VoronoiAnalysisModifier, VoronoiAnalysisModifierEditor);
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, _useCutoff, "UseCutoff");
DEFINE_FLAGS_PROPERTY_FIELD(VoronoiAnalysisModifier, _cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, _onlySelected, "OnlySelected");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, _useRadii, "UseRadii");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, _computeIndices, "ComputeIndices");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, _edgeCount, "EdgeCount");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, _edgeThreshold, "EdgeThreshold");
DEFINE_PROPERTY_FIELD(VoronoiAnalysisModifier, _faceThreshold, "FaceThreshold");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, _useCutoff, "Use cutoff");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, _cutoff, "Cutoff distance");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, _onlySelected, "Use only selected particles");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, _useRadii, "Use particle radii");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, _computeIndices, "Compute Voronoi indices");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, _edgeCount, "Maximum edge count");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, _edgeThreshold, "Edge length threshold");
SET_PROPERTY_FIELD_LABEL(VoronoiAnalysisModifier, _faceThreshold, "Face area threshold");
SET_PROPERTY_FIELD_UNITS(VoronoiAnalysisModifier, _cutoff, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(VoronoiAnalysisModifier, _edgeThreshold, WorldParameterUnit);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
VoronoiAnalysisModifier::VoronoiAnalysisModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_useCutoff(false), _cutoff(6.0), _onlySelected(false), _computeIndices(false), _edgeCount(6),
	_useRadii(false), _edgeThreshold(0), _faceThreshold(0),
	_coordinationNumbers(new ParticleProperty(0, ParticleProperty::CoordinationProperty, 0, false)),
	_simulationBoxVolume(0), _voronoiVolumeSum(0)
{
	INIT_PROPERTY_FIELD(VoronoiAnalysisModifier::_useCutoff);
	INIT_PROPERTY_FIELD(VoronoiAnalysisModifier::_cutoff);
	INIT_PROPERTY_FIELD(VoronoiAnalysisModifier::_onlySelected);
	INIT_PROPERTY_FIELD(VoronoiAnalysisModifier::_useRadii);
	INIT_PROPERTY_FIELD(VoronoiAnalysisModifier::_computeIndices);
	INIT_PROPERTY_FIELD(VoronoiAnalysisModifier::_edgeCount);
	INIT_PROPERTY_FIELD(VoronoiAnalysisModifier::_edgeThreshold);
	INIT_PROPERTY_FIELD(VoronoiAnalysisModifier::_faceThreshold);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::Engine> VoronoiAnalysisModifier::createEngine(TimePoint time, TimeInterval& validityInterval)
{
	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCell* inputCell = expectSimulationCell();

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
			posProperty->storage(),
			selectionProperty ? selectionProperty->storage() : nullptr,
			std::move(radii),
			inputCell->data(),
			useCutoff() ? cutoff() : 0,
			qMax(1, edgeCount()),
			computeIndices(),
			edgeThreshold(),
			faceThreshold());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void VoronoiAnalysisModifier::VoronoiAnalysisEngine::compute(FutureInterfaceBase& futureInterface)
{
	futureInterface.setProgressText(tr("Computing Voronoi cells"));

	if(_positions->size() == 0)
		return;	// Nothing to do

	// Compute the total simulation cell volume.
	_simulationBoxVolume = _simCell.volume();
	_voronoiVolumeSum = 0;

	// Compute squared particle radii (input was just particle radii).
	FloatType maxRadius = 0;
	for(auto& r : _squaredRadii) {
		if(r > maxRadius) maxRadius = r;
		r = r*r;
	}

	OnTheFlyNeighborListBuilder onTheFlyNeighborListBuilder(_cutoff);
	TreeNeighborListBuilder treeNeighborListBuilder;

	double boxDiameter;
	if(_cutoff > 0) {
		// Prepare the cutoff-based neighbor list generator.
		if(!onTheFlyNeighborListBuilder.prepare(_positions.data(), _simCell) || futureInterface.isCanceled())
			return;
	}
	else {
		// Prepare the nearest neighbor list generator.
		if(!treeNeighborListBuilder.prepare(_positions.data(), _simCell) || futureInterface.isCanceled())
			return;
	}

	// This is the size we use to initialize Voronoi cells. Must be larger than the simulation box.
	boxDiameter = sqrt(
			  _simCell.matrix().column(0).squaredLength()
			+ _simCell.matrix().column(1).squaredLength()
			+ _simCell.matrix().column(2).squaredLength());

	// The squared edge length threshold.
	// Add additional factor of 4 because Voronoi cell vertex coordinates are all scaled by factor of 2.
	FloatType sqEdgeThreshold = _edgeThreshold * _edgeThreshold * 4;

	// The normal vectors of the three cell planes.
	Vector3 planeNormals[3] = {
			_simCell.cellNormalVector(0),
			_simCell.cellNormalVector(1),
			_simCell.cellNormalVector(2)
	};
	Point3 corner1 = Point3::Origin() + _simCell.matrix().column(3);
	Point3 corner2 = corner1 + _simCell.matrix().column(0) + _simCell.matrix().column(1) + _simCell.matrix().column(2);

	// Perform analysis, particle-wise parallel.
	std::mutex mutex;
	parallelFor(_positions->size(), futureInterface,
			[&onTheFlyNeighborListBuilder, &treeNeighborListBuilder, this, sqEdgeThreshold, &mutex, boxDiameter,
			 planeNormals, corner1, corner2](size_t index) {

		// Skip unselected particles (if requested).
		if(_selection && _selection->getInt(index) == 0)
			return;

		// Build Voronoi cell.
		voro::voronoicell v;

		// Initialize the Voronoi cell to be a cube larger than the simulation cell, centered at the origin.
		v.init(-boxDiameter, boxDiameter, -boxDiameter, boxDiameter, -boxDiameter, boxDiameter);

		// Cut Voronoi cell at simulation cell boundaries in non-periodic directions.
		bool skipParticle = false;
		for(size_t dim = 0; dim < 3; dim++) {
			if(!_simCell.pbcFlags()[dim]) {
				double r;
				r = 2 * planeNormals[dim].dot(corner2 - _positions->getPoint3(index));
				if(r <= 0) skipParticle = true;
				v.plane(planeNormals[dim].x() * r, planeNormals[dim].y() * r, planeNormals[dim].z() * r, r*r);
				r = 2 * planeNormals[dim].dot(_positions->getPoint3(index) - corner1);
				if(r <= 0) skipParticle = true;
				v.plane(-planeNormals[dim].x() * r, -planeNormals[dim].y() * r, -planeNormals[dim].z() * r, r*r);
			}
		}
		// Skip particles that are located outside of non-periodic box boundaries.
		if(skipParticle)
			return;

		if(_cutoff > 0) {
			for(OnTheFlyNeighborListBuilder::iterator niter(onTheFlyNeighborListBuilder, index); !niter.atEnd(); niter.next()) {
				// Skip unselected particles (if requested).
				if(_selection && _selection->getInt(niter.current()) == 0)
					continue;

				// Take into account particle radii.
				FloatType rs = niter.distanceSquared();
				if(!_squaredRadii.empty())
					 rs += _squaredRadii[index] - _squaredRadii[niter.current()];

				// Cut cell with bisecting plane.
				v.plane(niter.delta().x(), niter.delta().y(), niter.delta().z(), rs);
			}
		}
		else {
			// This function will be called for every neighbor particle.
			int nvisits = 0;
			auto visitFunc = [this, &v, &nvisits, index](const TreeNeighborListBuilder::Neighbor& n, FloatType& mrs) {
				// Skip unselected particles (if requested).
				if(!_selection || _selection->getInt(n.index())) {
					FloatType rs = n.distanceSq;
					if(!_squaredRadii.empty())
						 rs += _squaredRadii[index] - _squaredRadii[n.index()];
					v.plane(n.delta.x(), n.delta.y(), n.delta.z(), rs);
				}
				if(nvisits == 0) {
					mrs = v.max_radius_squared();
					nvisits = 100;
				}
				nvisits--;
			};

			// Visit all neighbors of the current particles.
			treeNeighborListBuilder.visitNeighbors(treeNeighborListBuilder.particlePos(index), visitFunc);
		}

		// Compute cell volume.
		double vol = v.volume();
		_atomicVolumes->setFloat(index, (FloatType)vol);

		// Compute total volume of Voronoi cells.
		{
			std::lock_guard<std::mutex> lock(mutex);
			_voronoiVolumeSum += vol;
		}

		// Iterate over the Voronoi faces and their edges.
		int coordNumber = 0;
		for(int i = 1; i < v.p; i++) {
			for(int j = 0; j < v.nu[i]; j++) {
				int k = v.ed[i][j];
				if(k >= 0) {
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
						faceOrder--;
						if(_voronoiIndices && faceOrder < _voronoiIndices->componentCount())
							_voronoiIndices->setIntComponent(index, faceOrder, _voronoiIndices->getIntComponent(index, faceOrder) + 1);
					}
				}
			}
		}

		// Store computed result.
		_coordinationNumbers->setInt(index, coordNumber);
	});
}

/******************************************************************************
* Unpacks the computation results stored in the given engine object.
******************************************************************************/
void VoronoiAnalysisModifier::retrieveModifierResults(Engine* engine)
{
	VoronoiAnalysisEngine* eng = static_cast<VoronoiAnalysisEngine*>(engine);
	_coordinationNumbers = eng->coordinationNumbers();
	_atomicVolumes = eng->atomicVolumes();
	_voronoiIndices = eng->voronoiIndices();
	_simulationBoxVolume = eng->simulationBoxVolume();
	_voronoiVolumeSum = eng->voronoiVolumeSum();
}

/******************************************************************************
* Inserts the computed and cached modifier results into the modification pipeline.
******************************************************************************/
PipelineStatus VoronoiAnalysisModifier::applyModifierResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!coordinationNumbers() || inputParticleCount() != coordinationNumbers()->size())
		throw Exception(tr("The number of input particles has changed. The stored results have become invalid."));

	outputStandardProperty(coordinationNumbers());
	outputCustomProperty(atomicVolumes());
	if(voronoiIndices())
		outputCustomProperty(voronoiIndices());

	// Check computed Voronoi cell volume sum.
	if(std::abs(_voronoiVolumeSum - _simulationBoxVolume) > 1e-10 * inputParticleCount() * _simulationBoxVolume) {
		if(useCutoff()) {
			return PipelineStatus(PipelineStatus::Warning,
					tr("The volume sum of all Voronoi cells does not match the simulation box volume. "
							"This may be a result of some Voronoi cells being larger than the selected cutoff distance. "
							"Increase the cutoff parameter or disable the cutoff completely to avoid this warning. "
							"See user manual for more information.\n"
							"Simulation box volume: %1\n"
							"Voronoi cell volume sum: %2").arg(_simulationBoxVolume).arg(_voronoiVolumeSum));
		}
		else {
			return PipelineStatus(PipelineStatus::Warning,
					tr("The volume sum of all Voronoi cells does not match the simulation box volume. "
							"This may be a result of particles positioned outside the simulation box boundaries. "
							"See user manual for more information.\n"
							"Simulation box volume: %1\n"
							"Voronoi cell volume sum: %2").arg(_simulationBoxVolume).arg(_voronoiVolumeSum));
		}
	}

	return PipelineStatus::Success;
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void VoronoiAnalysisModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	// Recompute modifier results when the parameters have been changed.
	if(autoUpdateEnabled()) {
		invalidateCachedResults();
	}
	AsynchronousParticleModifier::propertyChanged(field);
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VoronoiAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Voronoi analysis"), rolloutParams, "particles.modifiers.voronoi_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setSpacing(4);
	gridlayout->setColumnStretch(1, 1);

	// Face threshold.
	FloatParameterUI* faceThresholdPUI = new FloatParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_faceThreshold));
	gridlayout->addWidget(faceThresholdPUI->label(), 0, 0);
	gridlayout->addLayout(faceThresholdPUI->createFieldLayout(), 0, 1);
	faceThresholdPUI->setMinValue(0);

	// Use cutoff.
	BooleanGroupBoxParameterUI* useCutoffPUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_useCutoff));
	gridlayout->addWidget(useCutoffPUI->groupBox(), 1, 0, 1, 2);
	QGridLayout* sublayout = new QGridLayout(useCutoffPUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_cutoff));
	sublayout->addWidget(cutoffRadiusPUI->label(), 0, 0);
	sublayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);
	cutoffRadiusPUI->setMinValue(0);

	// Compute indices.
	BooleanGroupBoxParameterUI* computeIndicesPUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_computeIndices));
	gridlayout->addWidget(computeIndicesPUI->groupBox(), 2, 0, 1, 2);
	sublayout = new QGridLayout(computeIndicesPUI->childContainer());
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	// Edge count parameter.
	IntegerParameterUI* edgeCountPUI = new IntegerParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_edgeCount));
	sublayout->addWidget(edgeCountPUI->label(), 0, 0);
	sublayout->addLayout(edgeCountPUI->createFieldLayout(), 0, 1);
	edgeCountPUI->setMinValue(3);
	edgeCountPUI->setMaxValue(18);

	// Edge threshold.
	FloatParameterUI* edgeThresholdPUI = new FloatParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_edgeThreshold));
	sublayout->addWidget(edgeThresholdPUI->label(), 1, 0);
	sublayout->addLayout(edgeThresholdPUI->createFieldLayout(), 1, 1);
	edgeThresholdPUI->setMinValue(0);

	// Atomic radii.
	BooleanParameterUI* useRadiiPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_useRadii));
	gridlayout->addWidget(useRadiiPUI->checkBox(), 3, 0, 1, 2);

	// Only selected particles.
	BooleanParameterUI* onlySelectedPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoronoiAnalysisModifier::_onlySelected));
	gridlayout->addWidget(onlySelectedPUI->checkBox(), 4, 0, 1, 2);

	layout->addLayout(gridlayout);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

};	// End of namespace
