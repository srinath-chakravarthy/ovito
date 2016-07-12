///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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
#include <core/utilities/concurrent/ParallelFor.h>
#include "PolyhedralTemplateMatchingModifier.h"

#include <ptm/index_ptm.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, PolyhedralTemplateMatchingModifier, StructureIdentificationModifier);
DEFINE_FLAGS_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, _rmsdCutoff, "RMSDCutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, _outputRmsd, "OutputRmsd");
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, _outputScaleFactor, "OutputScaleFactor");
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, _outputOrientation, "OutputOrientation");
DEFINE_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier, _outputDeformationGradient, "OutputDeformationGradient");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, _rmsdCutoff, "RMSD cutoff");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, _outputRmsd, "Output RMSD values");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, _outputScaleFactor, "Output scale factors");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, _outputOrientation, "Output orientations");
SET_PROPERTY_FIELD_LABEL(PolyhedralTemplateMatchingModifier, _outputDeformationGradient, "Output deformation gradients");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(PolyhedralTemplateMatchingModifier, _rmsdCutoff, FloatParameterUnit, 0);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
PolyhedralTemplateMatchingModifier::PolyhedralTemplateMatchingModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_rmsdCutoff(0), _rmsdHistogramBinSize(0), _outputRmsd(false), _outputScaleFactor(false),
		_outputOrientation(false), _outputDeformationGradient(false)
{
	INIT_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_rmsdCutoff);
	INIT_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputRmsd);
	INIT_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputScaleFactor);
	INIT_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputOrientation);
	INIT_PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputDeformationGradient);

	// Define the structure types.
	createStructureType(OTHER, ParticleTypeProperty::PredefinedStructureType::OTHER);
	createStructureType(FCC, ParticleTypeProperty::PredefinedStructureType::FCC);
	createStructureType(HCP, ParticleTypeProperty::PredefinedStructureType::HCP);
	createStructureType(BCC, ParticleTypeProperty::PredefinedStructureType::BCC);
	createStructureType(ICO, ParticleTypeProperty::PredefinedStructureType::ICO);
	createStructureType(SC, ParticleTypeProperty::PredefinedStructureType::SC);
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void PolyhedralTemplateMatchingModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Re-perform analysis when settings change.
	if(field == PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputRmsd) ||
		field == PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputScaleFactor) ||
		field == PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputOrientation) ||
		field == PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputDeformationGradient))
		invalidateCachedResults();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> PolyhedralTemplateMatchingModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	if(structureTypes().size() != NUM_STRUCTURE_TYPES)
		throwException(tr("The number of structure types has changed. Please remove this modifier from the modification pipeline and insert it again."));

	// Get modifier input.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Get particle selection.
	ParticleProperty* selectionProperty = nullptr;
	if(onlySelectedParticles())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty)->storage();

	// Initialize PTM library.
	ptm_initialize_global();

	return std::make_shared<PTMEngine>(validityInterval, posProperty->storage(), simCell->data(),
			getTypesToIdentify(NUM_STRUCTURE_TYPES), selectionProperty,
			outputScaleFactor(), outputOrientation(), outputDeformationGradient());
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void PolyhedralTemplateMatchingModifier::PTMEngine::perform()
{
	setProgressText(tr("Performing polyhedral template matching"));

	// Prepare the neighbor list.
	NearestNeighborFinder neighFinder(MAX_NEIGHBORS);
	if(!neighFinder.prepare(positions(), cell(), selection(), this))
		return;

	// Create output storage.
	ParticleProperty* output = structures();

	setProgressRange(positions()->size());
	setProgressValue(0);

	// Perform analysis on each particle.
	parallelForChunks(positions()->size(), *this, [this, &neighFinder, output](size_t startIndex, size_t count, FutureInterfaceBase& progress) {

		// Initialize thread-local storage for PTM routine.
		ptm_local_handle_t ptm_local_handle = ptm_initialize_local();

		size_t endIndex = startIndex + count;
		for(size_t index = startIndex; index < endIndex; index++) {

			// Update progress indicator.
			if((index % 256) == 0)
				progress.incrementProgressValue(256);

			// Break out of loop when operation was canceled.
			if(progress.isCanceled())
				break;

			// Skip particles that are not included in the analysis.
			if(selection() && !selection()->getInt(index)) {
				output->setInt(index, OTHER);
				_rmsd->setFloat(index, 0);
				continue;
			}

			// Find nearest neighbors.
			NearestNeighborFinder::Query<MAX_NEIGHBORS> neighQuery(neighFinder);
			neighQuery.findNeighbors(neighFinder.particlePos(index));
			int numNeighbors = neighQuery.results().size();
			OVITO_ASSERT(numNeighbors <= MAX_NEIGHBORS);

			// Bring neighbor coordinates into a form suitable for the PTM library.
			double points[(MAX_NEIGHBORS+1) * 3];
			points[0] = points[1] = points[2] = 0;
			for(int i = 0; i < numNeighbors; i++) {
				points[i*3 + 3] = neighQuery.results()[i].delta.x();
				points[i*3 + 4] = neighQuery.results()[i].delta.y();
				points[i*3 + 5] = neighQuery.results()[i].delta.z();
			}

			// Determine which structures to look for. This depends on how
			// much neighbors are present.
			int32_t flags = 0;
			if(numNeighbors >= 6 && typesToIdentify()[SC]) flags |= PTM_CHECK_SC;
			if(numNeighbors >= 12) {
				if(typesToIdentify()[FCC]) flags |= PTM_CHECK_FCC;
				if(typesToIdentify()[HCP]) flags |= PTM_CHECK_HCP;
				if(typesToIdentify()[ICO]) flags |= PTM_CHECK_ICO;
			}
			if(numNeighbors >= 14 && typesToIdentify()[BCC]) flags |= PTM_CHECK_BCC;

			// Call PTM library to identify local structure.
			int32_t type, alloy_type;
			double scale;
			double rmsd;
			double q[4];
			double F[9], F_res[3];
			ptm_index(ptm_local_handle, numNeighbors + 1, points, nullptr, flags, true,
					&type, &alloy_type, &scale, &rmsd, q,
					_deformationGradients ? F : nullptr,
					_deformationGradients ? F_res : nullptr,
					nullptr, nullptr, nullptr, nullptr);

			// Convert PTM classification to our own scheme and store computed quantities.
			if(type == PTM_MATCH_NONE) {
				output->setInt(index, OTHER);
				_rmsd->setFloat(index, 0);
			}
			else {
				if(type == PTM_MATCH_SC) output->setInt(index, SC);
				else if(type == PTM_MATCH_FCC) output->setInt(index, FCC);
				else if(type == PTM_MATCH_HCP) output->setInt(index, HCP);
				else if(type == PTM_MATCH_ICO) output->setInt(index, ICO);
				else if(type == PTM_MATCH_BCC) output->setInt(index, BCC);
				else OVITO_ASSERT(false);
				_rmsd->setFloat(index, rmsd);
				if(_scaleFactors) _scaleFactors->setFloat(index, scale);
				if(_orientations) _orientations->setQuaternion(index, Quaternion((FloatType)q[1], (FloatType)q[2], (FloatType)q[3], (FloatType)q[0]));
				if(_deformationGradients) {
					for(size_t j = 0; j < 9; j++)
						_deformationGradients->setFloatComponent(index, j, (FloatType)F[j]);
				}
			}
		}

		// Release thread-local storage of PTM routine.
		ptm_uninitialize_local(ptm_local_handle);
	});
	if(isCanceled() || output->size() == 0)
		return;

	// Determine histogram bin size based on maximum RMSD value.
	_rmsdHistogramData.resize(100);
	_rmsdHistogramBinSize = *std::max_element(_rmsd->constDataFloat(), _rmsd->constDataFloat() + output->size()) * 1.01f;
	_rmsdHistogramBinSize /= _rmsdHistogramData.size();
	if(_rmsdHistogramBinSize <= 0) _rmsdHistogramBinSize = 1;

	// Build RMSD histogram.
	for(size_t index = 0; index < output->size(); index++) {
		if(output->getInt(index) != OTHER) {
			OVITO_ASSERT(_rmsd->getFloat(index) >= 0);
			int binIndex = _rmsd->getFloat(index) / _rmsdHistogramBinSize;
			if(binIndex < _rmsdHistogramData.size())
				_rmsdHistogramData[binIndex]++;
		}
	}
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void PolyhedralTemplateMatchingModifier::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);
	PTMEngine* ptmEngine = static_cast<PTMEngine*>(engine);

	// Copy RMDS histogram data.
	_rmsdHistogramData = ptmEngine->_rmsdHistogramData;
	_rmsdHistogramBinSize = ptmEngine->_rmsdHistogramBinSize;

	// Copy assigned structure types and RMSD values.
	_originalStructureTypes = ptmEngine->structures();
	_rmsd = ptmEngine->_rmsd;

	// Transfer per-particle data.
	_scaleFactors = ptmEngine->_scaleFactors;
	_orientations = ptmEngine->_orientations;
	_deformationGradients = ptmEngine->_deformationGradients;
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the modification pipeline.
******************************************************************************/
PipelineStatus PolyhedralTemplateMatchingModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	// Enforce RMSD cutoff.
	if(rmsdCutoff() > 0 && _rmsd && _originalStructureTypes) {

		// Start off with the original particle classifications and make a copy.
		QExplicitlySharedDataPointer<ParticleProperty> finalStructureTypes = _originalStructureTypes;
		finalStructureTypes.detach();

		// Mark those particles whose RMSD exceeds the cutoff as 'OTHER'.
		for(size_t i = 0; i < _rmsd->size(); i++) {
			if(_rmsd->getFloat(i) > rmsdCutoff())
				finalStructureTypes->setInt(i, OTHER);
		}

		// Replace old classifications with updated ones.
		setStructureData(finalStructureTypes.data());
	}
	else {
		// When no RMSD cutoff is defined, use the original classifications.
		setStructureData(_originalStructureTypes.data());
	}

	// Output per-particle properties.
	if(_rmsd && outputRmsd()) {
		if(outputParticleCount() != _rmsd->size())
			throwException(tr("The number of input particles has changed. The stored results have become invalid."));
		outputCustomProperty(_rmsd.data());
	}
	if(_scaleFactors && outputScaleFactor()) {
		if(outputParticleCount() != _scaleFactors->size())
			throwException(tr("The number of input particles has changed. The stored results have become invalid."));
		outputCustomProperty(_scaleFactors.data());
	}
	if(_orientations && outputOrientation()) {
		if(outputParticleCount() != _orientations->size())
			throwException(tr("The number of input particles has changed. The stored results have become invalid."));
		outputStandardProperty(_orientations.data());
	}
	if(_deformationGradients && outputDeformationGradient()) {
		if(outputParticleCount() != _deformationGradients->size())
			throwException(tr("The number of input particles has changed. The stored results have become invalid."));
		outputStandardProperty(_deformationGradients.data());
	}

	// Let the base class output the structure type property to the pipeline.
	PipelineStatus status = StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	// Also output structure type counts, which have been computed by the base class.
	if(status.type() == PipelineStatus::Success) {
		output().attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.OTHER"), QVariant::fromValue(structureCounts()[OTHER]));
		output().attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.FCC"), QVariant::fromValue(structureCounts()[FCC]));
		output().attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.HCP"), QVariant::fromValue(structureCounts()[HCP]));
		output().attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.BCC"), QVariant::fromValue(structureCounts()[BCC]));
		output().attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.ICO"), QVariant::fromValue(structureCounts()[ICO]));
		output().attributes().insert(QStringLiteral("PolyhedralTemplateMatching.counts.SC"), QVariant::fromValue(structureCounts()[SC]));
	}

	return status;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
