///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
//  Copyright (2017) Lars Pastewka
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

#include <fftw3.h>

#include <complex>

#include <plugins/particles/Particles.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/app/Application.h>
#include <core/animation/AnimationSettings.h>
#include "CorrelationFunctionModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CorrelationFunctionModifierPlugin, CorrelationFunctionModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, _sourceProperty1, "SourceProperty1");
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, _sourceProperty2, "SourceProperty2");
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, _fftGridSpacing, "FftGridSpacing", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, _shortRangedCutoff, "ShortRangedCutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, _numberOfBinsForShortRangedCalculation, "NumberOfBinsForShortRangedCalculation", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _sourceProperty1, "First property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _sourceProperty2, "Second property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _fftGridSpacing, "FFT grid spacing");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _shortRangedCutoff, "Neighbor cutoff radius");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _numberOfBinsForShortRangedCalculation, "Number of neighbor bins");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CorrelationFunctionModifier, _fftGridSpacing, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CorrelationFunctionModifier, _shortRangedCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CorrelationFunctionModifier, _numberOfBinsForShortRangedCalculation, IntegerParameterUnit, 4, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CorrelationFunctionModifier::CorrelationFunctionModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_fftGridSpacing(1.0), _shortRangedCutoff(5.0), _numberOfBinsForShortRangedCalculation(50)
{
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty1);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty2);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_fftGridSpacing);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_shortRangedCutoff);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_numberOfBinsForShortRangedCalculation);
}


/******************************************************************************
* This method is called by the system when the modifier has been inserted
* into a pipeline.
******************************************************************************/
void CorrelationFunctionModifier::initializeModifier(PipelineObject* pipeline, ModifierApplication* modApp)
{
	ParticleModifier::initializeModifier(pipeline, modApp);

	// Use the first available particle property from the input state as data source when the modifier is newly created.
	if(sourceProperty1().isNull() || sourceProperty2().isNull()) {
		PipelineFlowState input = pipeline->evaluatePipeline(dataset()->animationSettings()->time(), modApp, false);
		ParticlePropertyReference bestProperty;
		for(DataObject* o : input.objects()) {
			ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
			if(property && (property->dataType() == qMetaTypeId<int>() || property->dataType() == qMetaTypeId<FloatType>())) {
				bestProperty = ParticlePropertyReference(property, (property->componentCount() > 1) ? 0 : -1);
			}
		}
		if(!bestProperty.isNull()) {
			if (sourceProperty1().isNull())
				setSourceProperty1(bestProperty);
			if (sourceProperty2().isNull())
				setSourceProperty2(bestProperty);
		}
	}
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> CorrelationFunctionModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get the source
	if(sourceProperty1().isNull())
		throwException(tr("Select a first particle property first."));
	if(sourceProperty2().isNull())
		throwException(tr("Select a second particle property first."));

	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get the current selected properties.
	ParticlePropertyObject* property1 = sourceProperty1().findInState(input());
	ParticlePropertyObject* property2 = sourceProperty2().findInState(input());

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();


	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CorrelationAnalysisEngine>(validityInterval,
													   posProperty->storage(),
													   property1->storage(),
													   property2->storage(),
													   inputCell->data(),
													   fftGridSpacing(),
													   shortRangedCutoff(),
													   numberOfBinsForShortRangedCalculation());
}

/******************************************************************************
* Map property onto grid.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::mapToSpatialGrid(ParticleProperty *property,
																			  size_t propertyVectorComponent,
																			  const AffineTransformation &reciprocalCellMatrix,
																			  int nX, int nY, int nZ,
																			  QVector<FloatType> &gridData)
{
	size_t vecComponent = std::max(size_t(0), propertyVectorComponent);
	size_t vecComponentCount = property->componentCount();

	int numberOfGridPoints = nX*nY*nZ;


	// Resize real space grid.
	gridData.fill(0.0, numberOfGridPoints);


	// Get periodic boundary flag.
	std::array<bool, 3> pbc = cell().pbcFlags();


	if(property->size() > 0) {
		const Point3* pos = positions()->constDataPoint3();
		const Point3* pos_end = pos + positions()->size();


		if(property->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* v = property->constDataFloat() + vecComponent;
			const FloatType* v_end = v + (property->size() * vecComponentCount);
			for(; v != v_end; v += vecComponentCount, ++pos) {
				if(!std::isnan(*v)) {
					Point3 fractionalPos = reciprocalCellMatrix*(*pos);
					int binIndexX = int( fractionalPos.x() * nX );
					int binIndexY = int( fractionalPos.y() * nY );
					int binIndexZ = int( fractionalPos.z() * nZ );
					if(pbc[0]) binIndexX = SimulationCell::modulo(binIndexX, nX);
					if(pbc[1]) binIndexY = SimulationCell::modulo(binIndexY, nY);
					if(pbc[2]) binIndexZ = SimulationCell::modulo(binIndexZ, nZ);
					if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
						// Store in row-major format.
						size_t binIndex = binIndexZ+nZ*(binIndexY+nY*binIndexX);
						gridData[binIndex] += *v;
					}
				}
			}
		}
		else if(property->dataType() == qMetaTypeId<int>()) {
			const int* v = property->constDataInt() + vecComponent;
			const int* v_end = v + (property->size() * vecComponentCount);
			for(; v != v_end; v += vecComponentCount, ++pos) {
				if(!std::isnan(*v)) {
					Point3 fractionalPos = reciprocalCellMatrix*(*pos);
					int binIndexX = int( fractionalPos.x() * nX );
					int binIndexY = int( fractionalPos.y() * nY );
					int binIndexZ = int( fractionalPos.z() * nZ );
					if(pbc[0]) binIndexX = SimulationCell::modulo(binIndexX, nX);
					if(pbc[1]) binIndexY = SimulationCell::modulo(binIndexY, nY);
					if(pbc[2]) binIndexZ = SimulationCell::modulo(binIndexZ, nZ);
					if(binIndexX >= 0 && binIndexX < nX && binIndexY >= 0 && binIndexY < nY && binIndexZ >= 0 && binIndexZ < nZ) {
						// Store in row-major format.
						size_t binIndex = binIndexZ+nZ*(binIndexY+nY*binIndexX);
						gridData[binIndex] += *v;
					}
				}
			}
		}
	}
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::perform()
{
	setProgressText(tr("Computing correlation function"));

	// Get reciprocal cell.
	AffineTransformation cellMatrix = cell().matrix(); 
	AffineTransformation reciprocalCellMatrix = cell().inverseMatrix();

	int nX = cellMatrix.column(0).length()/fftGridSpacing();
	int nY = cellMatrix.column(1).length()/fftGridSpacing();
	int nZ = cellMatrix.column(2).length()/fftGridSpacing();


	// Map all quantities onto a spatial grid.
	QVector<FloatType> gridProperty1, gridProperty2;
	mapToSpatialGrid(sourceProperty1(),
					 0, // FIXME! Selected vector component should be passed to engine.
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridProperty1);
	mapToSpatialGrid(sourceProperty2(),
					 0, // FIXME! Selected vector component should be passed to engine.
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridProperty2);

	if (isCanceled())
		return;

	// FIXME. Apply windowing function in nonperiodic directions here.
	// FIXME. Imaginary part of cross-correlation function is ignored.

	// Use single precision FFTW if Ovito is compiled with single precision
	// floating point type.
#ifdef FLOATTYPE_FLOAT
#define fftw_complex fftwf_complex
#define fftw_plan_dft_r2c_3d fftwf_plan_dft_r2c_3d
#define fftw_plan_dft_c2r_3d fftwf_plan_dft_c2r_3d
#define fftw_execute fftwf_execute
#define fftw_destroy_plan fftwf_destroy_plan
#endif

	// Compute reciprocal-space correlation function from a product in Fourier space.

	// Compute Fourier transform of spatial grid.
	QVector<std::complex<FloatType>> ftProperty1(nX*nY*(nZ/2+1));
	auto plan = fftw_plan_dft_r2c_3d(
		nX, nY, nZ,
		gridProperty1.data(),
		reinterpret_cast<fftw_complex*>(ftProperty1.data()),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);

	if (isCanceled())
		return;

		QVector<std::complex<FloatType>> ftProperty2(nX*nY*(nZ/2+1));
		plan = fftw_plan_dft_r2c_3d(
		nX, nY, nZ,
		gridProperty2.data(),
		reinterpret_cast<fftw_complex*>(ftProperty2.data()),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);

	if (isCanceled())
		return;

	// Compute distance of cell faces.
	FloatType cellFaceDistance1 = 1/reciprocalCellMatrix.row(0).length();
	FloatType cellFaceDistance2 = 1/reciprocalCellMatrix.row(1).length();
	FloatType cellFaceDistance3 = 1/reciprocalCellMatrix.row(2).length();

	FloatType minCellFaceDistance = std::min({cellFaceDistance1, cellFaceDistance2, cellFaceDistance3});

	// Minimum reciprocal space vector is given by the minimum distance of cell faces.
	FloatType minReciprocalSpaceVector = 1/minCellFaceDistance;
	int numberOfWavevectorBins = 1/(2*minReciprocalSpaceVector*fftGridSpacing());

	// Radially averaged reciprocal space correlation function.
	_reciprocalSpaceCorrelationFunction.fill(0.0, numberOfWavevectorBins);
	_reciprocalSpaceCorrelationFunctionX.resize(numberOfWavevectorBins);
	QVector<int> numberOfValues(numberOfWavevectorBins, 0);

	// Populate array with reciprocal space vectors.
	for (int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		_reciprocalSpaceCorrelationFunctionX[wavevectorBinIndex] = 2*FLOATTYPE_PI*(wavevectorBinIndex+0.5)*minReciprocalSpaceVector;
	}


	// Compute Fourier-transformed correlation function and put it on a radial
	// grid.
	int binIndex = 0;
	for (int binIndexX = 0; binIndexX < nX; binIndexX++) {
		for (int binIndexY = 0; binIndexY < nY; binIndexY++) {
			for (int binIndexZ = 0; binIndexZ < nZ/2+1; binIndexZ++, binIndex++) {
				// Compute correlation function.
				std::complex<FloatType> corr = ftProperty1[binIndex]*std::conj(ftProperty2[binIndex]);

				// Store correlation function to property1 for back transform.
				ftProperty1[binIndex] = corr;

				// Compute wavevector. (FIXME! Check that this is actually correct for even and odd numbers of grid points.)
				int iX = SimulationCell::modulo(binIndexX+nX/2, nX)-nX/2;
				int iY = SimulationCell::modulo(binIndexY+nY/2, nY)-nY/2;
				int iZ = SimulationCell::modulo(binIndexZ+nZ/2, nZ)-nZ/2;
				// This is the reciprocal space vector (without a factor of 2*pi).
				Vector_4<FloatType> wavevector = FloatType(iX)*reciprocalCellMatrix.row(0) +
						 		   	 			 FloatType(iY)*reciprocalCellMatrix.row(1) +
						 			 			 FloatType(iZ)*reciprocalCellMatrix.row(2);
				wavevector.w() = 0.0;

				// Length of reciprocal space vector.
				int wavevectorBinIndex = int(std::floor(wavevector.length()/minReciprocalSpaceVector));
				if (wavevectorBinIndex >= 0 && wavevectorBinIndex < numberOfWavevectorBins) {
					_reciprocalSpaceCorrelationFunction[wavevectorBinIndex] += std::real(corr);
					numberOfValues[wavevectorBinIndex]++;
				}
			}
		}
	}


	// Compute averages and normalize reciprocal-space correlation function.
	FloatType normalizationFactor = cell().volume3D()/(sourceProperty1()->size()*sourceProperty2()->size());
	for (int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		if (numberOfValues[wavevectorBinIndex] > 0) {
			_reciprocalSpaceCorrelationFunction[wavevectorBinIndex] *= normalizationFactor/numberOfValues[wavevectorBinIndex];
		}
	}


	if (isCanceled())
		return;

	// Compute long-ranged part of the real-space correlation function from the FFT convolution.

	// Computer inverse Fourier transform of correlation function.
	plan = fftw_plan_dft_c2r_3d(
		nX, nY, nZ,
		reinterpret_cast<fftw_complex*>(ftProperty1.data()),
		gridProperty1.data(),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);


	if (isCanceled())
		return;

	// Determine number of grid points for real-space correlation function.
	int numberOfDistanceBins = minCellFaceDistance/(2*fftGridSpacing());
	FloatType gridSpacing = minCellFaceDistance/(2*numberOfDistanceBins);

	// Radially averaged real space correlation function.
	_realSpaceCorrelationFunction.fill(0.0, numberOfDistanceBins);
	_realSpaceCorrelationFunctionX.resize(numberOfDistanceBins);
	numberOfValues.fill(0, numberOfDistanceBins);

	// Populate array with real space vectors.
	for (int distanceBinIndex = 0; distanceBinIndex < numberOfDistanceBins; distanceBinIndex++) {
		_realSpaceCorrelationFunctionX[distanceBinIndex] = (distanceBinIndex+0.5)*gridSpacing;
	}

	// Put real-space correlation function on a radial grid.
	binIndex = 0;
	for (int binIndexX = 0; binIndexX < nX; binIndexX++) {
		for (int binIndexY = 0; binIndexY < nY; binIndexY++) {
			for (int binIndexZ = 0; binIndexZ < nZ; binIndexZ++, binIndex++) {
				// Compute distance. (FIXME! Check that this is actually correct for even and odd numbers of grid points.)
				FloatType fracX = FloatType(SimulationCell::modulo(binIndexX+nX/2, nX)-nX/2)/nX;
				FloatType fracY = FloatType(SimulationCell::modulo(binIndexY+nY/2, nY)-nY/2)/nY;
				FloatType fracZ = FloatType(SimulationCell::modulo(binIndexZ+nZ/2, nZ)-nZ/2)/nZ;
				// This is the real space vector.
				Vector_3<FloatType> distance = fracX*cellMatrix.column(0) +
						 		   	 	       fracY*cellMatrix.column(1) +
						 			 		   fracZ*cellMatrix.column(2);

				// Length of real space vector.
				int distanceBinIndex = int(std::floor(distance.length()/gridSpacing));
				if (distanceBinIndex >= 0 && distanceBinIndex < numberOfDistanceBins) {
					_realSpaceCorrelationFunction[distanceBinIndex] += gridProperty1[binIndex];
					numberOfValues[distanceBinIndex]++;
				}
			}
		}
	}

	// Compute averages and normalize real-space correlation function. Note FFTW computes an unnormalized transform.
	normalizationFactor = 1.0/(sourceProperty1()->size()*sourceProperty2()->size());
	for (int distanceBinIndex = 0; distanceBinIndex < numberOfDistanceBins; distanceBinIndex++) {
		if (numberOfValues[distanceBinIndex] > 0) {
			_realSpaceCorrelationFunction[distanceBinIndex] *= normalizationFactor/numberOfValues[distanceBinIndex];
		}
	}


	if (isCanceled())
		return;

	// Compute short-ranged part of the real-space correlation function from a direct loop over particle neighbors.

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if (!neighborListBuilder.prepare(_shortRangedCutoff, positions(), cell(), nullptr, this))
		return;

	size_t particleCount = positions()->size();
	setProgressValue(0);
	setProgressRange(particleCount / 1000);

	// Get pointers to data.
	const FloatType *floatData1, *floatData2;
	const int *intData1, *intData2; 
	floatData1 = floatData2 = nullptr;
	intData1 = intData2 = nullptr;
	if(sourceProperty1()->dataType() == qMetaTypeId<FloatType>()) {
		floatData1 = sourceProperty1()->constDataFloat();
	}
	else if (sourceProperty1()->dataType() == qMetaTypeId<int>()) {
		intData1 = sourceProperty1()->constDataInt();
	}
	if(sourceProperty2()->dataType() == qMetaTypeId<FloatType>()) {
		floatData2 = sourceProperty2()->constDataFloat();
	}
	else if (sourceProperty2()->dataType() == qMetaTypeId<int>()) {
		intData2 = sourceProperty2()->constDataInt();
	}

	// Perform analysis on each particle in parallel.
	std::vector<std::thread> workers;
	size_t num_threads = Application::instance().idealThreadCount();
	size_t chunkSize = particleCount / num_threads;
	size_t startIndex = 0;
	size_t endIndex = chunkSize;
	std::mutex mutex;
	for (size_t t = 0; t < num_threads; t++) {
		if (t == num_threads - 1) {
			endIndex += particleCount % num_threads;
		}
		workers.push_back(std::thread([&neighborListBuilder, startIndex, endIndex, floatData1, intData1, floatData2, intData2, &mutex, this]() {
			FloatType gridSpacing = (_shortRangedCutoff + FLOATTYPE_EPSILON) / _shortRangedRealSpaceCorrelationFunction.size();
			std::vector<double> threadLocalCorrelation(_shortRangedRealSpaceCorrelationFunction.size(), 0);
			for (size_t i = startIndex; i < endIndex;) {

				for (CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
					size_t distanceBinIndex = (size_t)(sqrt(neighQuery.distanceSquared()) / gridSpacing);
					distanceBinIndex = std::min(distanceBinIndex, threadLocalCorrelation.size() - 1);
					FloatType data1 = 0.0, data2 = 0.0;
					if (floatData1)
						data1 = floatData1[i];
					else if (intData1)
						data1 = intData1[i];
					if (floatData2)
						data2 = floatData2[i];
					else if (intData2)
						data2 = intData2[i];
					threadLocalCorrelation[distanceBinIndex] += data1*data2;
				}

				i++;

				// Update progress indicator.
				if ((i % 1000) == 0)
					incrementProgressValue();
				// Abort loop when operation was canceled by the user.
				if (isCanceled())
					return;
			}
			std::lock_guard<std::mutex> lock(mutex);
			auto iter_out = _shortRangedRealSpaceCorrelationFunction.begin();
			for (auto iter = threadLocalCorrelation.cbegin(); iter != threadLocalCorrelation.cend(); ++iter, ++iter_out)
				*iter_out += *iter;
		}));
		startIndex = endIndex;
		endIndex += chunkSize;
	}

	for (auto& t: workers)
		t.join();

	// Normalize short-ranged real-space correlation function and populate x-array.
	gridSpacing = (_shortRangedCutoff + FLOATTYPE_EPSILON) / _shortRangedRealSpaceCorrelationFunction.size();
	normalizationFactor = 3.0*cell().volume3D()/(4.0*FLOATTYPE_PI*sourceProperty1()->size()*sourceProperty2()->size());
	for (int distanceBinIndex = 0; distanceBinIndex < _shortRangedRealSpaceCorrelationFunction.size(); distanceBinIndex++) {
		FloatType distance = distanceBinIndex*gridSpacing;
		FloatType distance2 = (distanceBinIndex+1)*gridSpacing;
		_shortRangedRealSpaceCorrelationFunctionX[distanceBinIndex] = (distance+distance2)/2;
		_shortRangedRealSpaceCorrelationFunction[distanceBinIndex] *= normalizationFactor/(distance2*distance2*distance2-distance*distance*distance);
	}

}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void CorrelationFunctionModifier::transferComputationResults(ComputeEngine* engine)
{
	CorrelationAnalysisEngine* eng = static_cast<CorrelationAnalysisEngine*>(engine);
	_realSpaceCorrelationFunction = eng->realSpaceCorrelationFunction();
	_realSpaceCorrelationFunctionX = eng->realSpaceCorrelationFunctionX();
	_shortRangedRealSpaceCorrelationFunction = eng->shortRangedRealSpaceCorrelationFunction();
	_shortRangedRealSpaceCorrelationFunctionX = eng->shortRangedRealSpaceCorrelationFunctionX();
	_reciprocalSpaceCorrelationFunction = eng->reciprocalSpaceCorrelationFunction();
	_reciprocalSpaceCorrelationFunctionX = eng->reciprocalSpaceCorrelationFunctionX();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus CorrelationFunctionModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	return PipelineStatus::Success;
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CorrelationFunctionModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	if (field == PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty1) ||
		field == PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty2) ||
		field == PROPERTY_FIELD(CorrelationFunctionModifier::_fftGridSpacing) ||
		field == PROPERTY_FIELD(CorrelationFunctionModifier::_shortRangedCutoff) ||
	    field == PROPERTY_FIELD(CorrelationFunctionModifier::_numberOfBinsForShortRangedCalculation))
		invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
