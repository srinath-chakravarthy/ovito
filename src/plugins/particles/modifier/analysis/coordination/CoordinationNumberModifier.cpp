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
#include <core/scene/objects/DataObject.h>
#include <core/app/Application.h>
#include "CoordinationNumberModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CoordinationNumberModifier, AsynchronousParticleModifier);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinationNumberModifier, cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CoordinationNumberModifier, numberOfBins, "NumberOfBins", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CoordinationNumberModifier, cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CoordinationNumberModifier, numberOfBins, "Number of histogram bins");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CoordinationNumberModifier, cutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CoordinationNumberModifier, numberOfBins, IntegerParameterUnit, 4, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CoordinationNumberModifier::CoordinationNumberModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_cutoff(3.2), _numberOfBins(200)
{
	INIT_PROPERTY_FIELD(cutoff);
	INIT_PROPERTY_FIELD(numberOfBins);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> CoordinationNumberModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	// The number of sampling intervals for the radial distribution function.
	int rdfSampleCount = std::max(numberOfBins(), 4);
	if(rdfSampleCount > 100000)
		throwException(tr("Number of histogram bins is too large."));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CoordinationAnalysisEngine>(validityInterval, posProperty->storage(), inputCell->data(), cutoff(), rdfSampleCount);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CoordinationNumberModifier::CoordinationAnalysisEngine::perform()
{
	setProgressText(tr("Computing coordination numbers"));

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if(!neighborListBuilder.prepare(_cutoff, positions(), cell(), nullptr, *this))
		return;

	size_t particleCount = positions()->size();
	setProgressValue(0);
	setProgressMaximum(particleCount / 1000);

	// Perform analysis on each particle in parallel.
	std::vector<std::thread> workers;
	size_t num_threads = Application::instance()->idealThreadCount();
	size_t chunkSize = particleCount / num_threads;
	size_t startIndex = 0;
	size_t endIndex = chunkSize;
	std::mutex mutex;
	for(size_t t = 0; t < num_threads; t++) {
		if(t == num_threads - 1) {
			endIndex += particleCount % num_threads;
		}
		workers.push_back(std::thread([&neighborListBuilder, startIndex, endIndex, &mutex, this]() {
			FloatType rdfBinSize = (_cutoff + FLOATTYPE_EPSILON) / _rdfHistogram.size();
			std::vector<double> threadLocalRDF(_rdfHistogram.size(), 0);
			int* coordOutput = _coordinationNumbers->dataInt() + startIndex;
			for(size_t i = startIndex; i < endIndex; ++coordOutput) {

				OVITO_ASSERT(*coordOutput == 0);
				for(CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
					(*coordOutput)++;
					size_t rdfInterval = (size_t)(sqrt(neighQuery.distanceSquared()) / rdfBinSize);
					rdfInterval = std::min(rdfInterval, threadLocalRDF.size() - 1);
					threadLocalRDF[rdfInterval]++;
				}

				i++;

				// Update progress indicator.
				if((i % 1000) == 0)
					incrementProgressValue();
				// Abort loop when operation was canceled by the user.
				if(isCanceled())
					return;
			}
			std::lock_guard<std::mutex> lock(mutex);
			auto iter_out = _rdfHistogram.begin();
			for(auto iter = threadLocalRDF.cbegin(); iter != threadLocalRDF.cend(); ++iter, ++iter_out)
				*iter_out += *iter;
		}));
		startIndex = endIndex;
		endIndex += chunkSize;
	}

	for(auto& t : workers)
		t.join();
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void CoordinationNumberModifier::transferComputationResults(ComputeEngine* engine)
{
	CoordinationAnalysisEngine* eng = static_cast<CoordinationAnalysisEngine*>(engine);
	_coordinationNumbers = eng->coordinationNumbers();
	_rdfY.resize(eng->rdfHistogram().size());
	_rdfX.resize(eng->rdfHistogram().size());
	if(!eng->cell().is2D()) {
		double rho = eng->positions()->size() / eng->cell().volume3D();
		double constant = 4.0/3.0 * FLOATTYPE_PI * rho * eng->positions()->size();
		double stepSize = eng->cutoff() / _rdfX.size();
		for(int i = 0; i < _rdfX.size(); i++) {
			double r = stepSize * i;
			double r2 = r + stepSize;
			_rdfX[i] = r + 0.5 * stepSize;
			_rdfY[i] = eng->rdfHistogram()[i] / (constant * (r2*r2*r2 - r*r*r));
		}
	}
	else {
		double rho = eng->positions()->size() / eng->cell().volume2D();
		double constant = FLOATTYPE_PI * rho * eng->positions()->size();
		double stepSize = eng->cutoff() / _rdfX.size();
		for(int i = 0; i < _rdfX.size(); i++) {
			double r = stepSize * i;
			double r2 = r + stepSize;
			_rdfX[i] = r + 0.5 * stepSize;
			_rdfY[i] = eng->rdfHistogram()[i] / (constant * (r2*r2 - r*r));
		}
	}
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus CoordinationNumberModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	if(!_coordinationNumbers)
		throwException(tr("No computation results available."));

	if(inputParticleCount() != _coordinationNumbers->size())
		throwException(tr("The number of input particles has changed. The stored results have become invalid."));

	outputStandardProperty(_coordinationNumbers.data());
	return PipelineStatus::Success;
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CoordinationNumberModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	if(field == PROPERTY_FIELD(cutoff) ||
			field == PROPERTY_FIELD(numberOfBins))
		invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
