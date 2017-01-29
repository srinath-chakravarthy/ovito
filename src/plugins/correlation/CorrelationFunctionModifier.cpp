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

#include <plugins/particles/Particles.h>

#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/app/Application.h>
#include <core/animation/AnimationSettings.h>
#include "CorrelationFunctionModifier.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CorrelationFunctionModifier, AsynchronousParticleModifier);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, sourceProperty1, "SourceProperty1");
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, sourceProperty2, "SourceProperty2");
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fftGridSpacing, "FftGridSpacing");
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, doComputeNeighCorrelation, "doComputeNeighCorrelation", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, neighCutoff, "NeighCutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, numberOfNeighBins, "NumberOfNeighBins", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, normalizeRealSpace, "NormalizeRealSpace", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, typeOfRealSpacePlot, "TypeOfRealSpacePlot");
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, normalizeReciprocalSpace, "NormalizeReciprocalSpace", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, typeOfReciprocalSpacePlot, "TypeOfReciprocalSpacePlot");
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fixRealSpaceXAxisRange, "FixRealSpaceXAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, realSpaceXAxisRangeStart, "RealSpaceXAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, realSpaceXAxisRangeEnd, "RealSpaceXAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fixRealSpaceYAxisRange, "FixRealSpaceYAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, realSpaceYAxisRangeStart, "RealSpaceYAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, realSpaceYAxisRangeEnd, "RealSpaceYAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fixReciprocalSpaceXAxisRange, "FixReciprocalSpaceXAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, reciprocalSpaceXAxisRangeStart, "ReciprocalSpaceXAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, reciprocalSpaceXAxisRangeEnd, "ReciprocalSpaceXAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(CorrelationFunctionModifier, fixReciprocalSpaceYAxisRange, "FixReciprocalSpaceYAxisRange");
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, reciprocalSpaceYAxisRangeStart, "ReciprocalSpaceYAxisRangeStart", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, reciprocalSpaceYAxisRangeEnd, "ReciprocalSpaceYAxisRangeEnd", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, sourceProperty1, "First property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, sourceProperty2, "Second property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fftGridSpacing, "FFT grid spacing");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, doComputeNeighCorrelation, "Direct summation");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, neighCutoff, "Neighbor cutoff radius");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, numberOfNeighBins, "Number of neighbor bins");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, normalizeRealSpace, "Normalize correlation function");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, normalizeReciprocalSpace, "Normalize correlation function");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CorrelationFunctionModifier, fftGridSpacing, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CorrelationFunctionModifier, neighCutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CorrelationFunctionModifier, numberOfNeighBins, IntegerParameterUnit, 4, 100000);
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fixRealSpaceXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, realSpaceXAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, realSpaceXAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fixRealSpaceYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, realSpaceYAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, realSpaceYAxisRangeEnd, "Y-range end");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fixReciprocalSpaceXAxisRange, "Fix x-range");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, reciprocalSpaceXAxisRangeStart, "X-range start");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, reciprocalSpaceXAxisRangeEnd, "X-range end");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, fixReciprocalSpaceYAxisRange, "Fix y-range");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, reciprocalSpaceYAxisRangeStart, "Y-range start");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, reciprocalSpaceYAxisRangeEnd, "Y-range end");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CorrelationFunctionModifier::CorrelationFunctionModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_fftGridSpacing(3.0), _doComputeNeighCorrelation(false), _neighCutoff(5.0), _numberOfNeighBins(50),
	_normalizeRealSpace(false), _typeOfRealSpacePlot(0), _normalizeReciprocalSpace(false), _typeOfReciprocalSpacePlot(0),
	_fixRealSpaceXAxisRange(false), _realSpaceXAxisRangeStart(0.0), _realSpaceXAxisRangeEnd(1.0),
	_fixRealSpaceYAxisRange(false), _realSpaceYAxisRangeStart(0.0), _realSpaceYAxisRangeEnd(1.0),
	_fixReciprocalSpaceXAxisRange(false), _reciprocalSpaceXAxisRangeStart(0.0), _reciprocalSpaceXAxisRangeEnd(1.0),
	_fixReciprocalSpaceYAxisRange(false), _reciprocalSpaceYAxisRangeStart(0.0), _reciprocalSpaceYAxisRangeEnd(1.0)
{
	INIT_PROPERTY_FIELD(sourceProperty1);
	INIT_PROPERTY_FIELD(sourceProperty2);
	INIT_PROPERTY_FIELD(fftGridSpacing);
	INIT_PROPERTY_FIELD(doComputeNeighCorrelation);
	INIT_PROPERTY_FIELD(neighCutoff);
	INIT_PROPERTY_FIELD(numberOfNeighBins);
	INIT_PROPERTY_FIELD(normalizeRealSpace);
	INIT_PROPERTY_FIELD(typeOfRealSpacePlot);
	INIT_PROPERTY_FIELD(normalizeReciprocalSpace);
	INIT_PROPERTY_FIELD(typeOfReciprocalSpacePlot);
	INIT_PROPERTY_FIELD(fixRealSpaceXAxisRange);
	INIT_PROPERTY_FIELD(realSpaceXAxisRangeStart);
	INIT_PROPERTY_FIELD(realSpaceXAxisRangeEnd);
	INIT_PROPERTY_FIELD(fixRealSpaceYAxisRange);
	INIT_PROPERTY_FIELD(realSpaceYAxisRangeStart);
	INIT_PROPERTY_FIELD(realSpaceYAxisRangeEnd);
	INIT_PROPERTY_FIELD(fixReciprocalSpaceXAxisRange);
	INIT_PROPERTY_FIELD(reciprocalSpaceXAxisRangeStart);
	INIT_PROPERTY_FIELD(reciprocalSpaceXAxisRangeEnd);
	INIT_PROPERTY_FIELD(fixReciprocalSpaceYAxisRange);
	INIT_PROPERTY_FIELD(reciprocalSpaceYAxisRangeStart);
	INIT_PROPERTY_FIELD(reciprocalSpaceYAxisRangeEnd);
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
													   std::max(0, sourceProperty1().vectorComponent()),
													   property2->storage(),
													   std::max(0, sourceProperty2().vectorComponent()),
													   inputCell->data(),
													   fftGridSpacing(),
													   doComputeNeighCorrelation(),
													   neighCutoff(),
													   numberOfNeighBins());
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

// Use single precision FFTW if Ovito is compiled with single precision
// floating point type.
#ifdef FLOATTYPE_FLOAT
#define fftw_complex fftwf_complex
#define fftw_plan_dft_r2c_3d fftwf_plan_dft_r2c_3d
#define fftw_plan_dft_c2r_3d fftwf_plan_dft_c2r_3d
#define fftw_execute fftwf_execute
#define fftw_destroy_plan fftwf_destroy_plan
#endif

void CorrelationFunctionModifier::CorrelationAnalysisEngine::r2cFFT(int nX, int nY, int nZ,
																	QVector<FloatType> &rData,
																	QVector<std::complex<FloatType>> &cData)
{
	cData.resize(nX*nY*(nZ/2+1));
	auto plan = fftw_plan_dft_r2c_3d(
		nX, nY, nZ,
		rData.data(),
		reinterpret_cast<fftw_complex*>(cData.data()),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);
}

void CorrelationFunctionModifier::CorrelationAnalysisEngine::c2rFFT(int nX, int nY, int nZ,
																	QVector<std::complex<FloatType>> &cData,
																	QVector<FloatType> &rData)
{
	rData.resize(nX*nY*nZ);
	auto plan = fftw_plan_dft_c2r_3d(
		nX, nY, nZ,
		reinterpret_cast<fftw_complex*>(cData.data()),
		rData.data(),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);
}

/******************************************************************************
* Compute real and reciprocal space correlation function via FFT.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeFftCorrelation()
{
	// Get reciprocal cell.
	AffineTransformation cellMatrix = cell().matrix(); 
	AffineTransformation reciprocalCellMatrix = cell().inverseMatrix();

	// Note: Cell vectors are in columns. Those are 3-vectors.
	int nX = cellMatrix.column(0).length()/fftGridSpacing();
	int nY = cellMatrix.column(1).length()/fftGridSpacing();
	int nZ = cellMatrix.column(2).length()/fftGridSpacing();

	// Map all quantities onto a spatial grid.
	QVector<FloatType> gridProperty1, gridProperty2;
	mapToSpatialGrid(sourceProperty1(),
					 _vecComponent1,
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridProperty1);

	incrementProgressValue();
	if (isCanceled())
		return;

	mapToSpatialGrid(sourceProperty2(),
					 _vecComponent2,
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridProperty2);

	incrementProgressValue();
	if (isCanceled())
		return;

	// FIXME. Apply windowing function in nonperiodic directions here.

	// Compute reciprocal-space correlation function from a product in Fourier space.

	// Compute Fourier transform of spatial grid.
	QVector<std::complex<FloatType>> ftProperty1;
	r2cFFT(nX, nY, nZ, gridProperty1, ftProperty1);

	incrementProgressValue();
	if (isCanceled())
		return;

	QVector<std::complex<FloatType>> ftProperty2(nX*nY*(nZ/2+1));
	r2cFFT(nX, nY, nZ, gridProperty2, ftProperty2);

	incrementProgressValue();
	if (isCanceled())
		return;

	// Note: Reciprocal cell vectors are in rows. Those are 4-vectors.
	Vector_4<FloatType> recCell1 = reciprocalCellMatrix.row(0);
	Vector_4<FloatType> recCell2 = reciprocalCellMatrix.row(1);
	Vector_4<FloatType> recCell3 = reciprocalCellMatrix.row(2);

	// Compute distance of cell faces.
	FloatType cellFaceDistance1 = 1/sqrt(recCell1.x()*recCell1.x() + recCell1.y()*recCell1.y() + recCell1.z()*recCell1.z());
	FloatType cellFaceDistance2 = 1/sqrt(recCell2.x()*recCell2.x() + recCell2.y()*recCell2.y() + recCell2.z()*recCell2.z());
	FloatType cellFaceDistance3 = 1/sqrt(recCell3.x()*recCell3.x() + recCell3.y()*recCell3.y() + recCell3.z()*recCell3.z());

	FloatType minCellFaceDistance = std::min({cellFaceDistance1, cellFaceDistance2, cellFaceDistance3});

	// Minimum reciprocal space vector is given by the minimum distance of cell faces.
	FloatType minReciprocalSpaceVector = 1/minCellFaceDistance;
	int numberOfWavevectorBins = 1/(2*minReciprocalSpaceVector*fftGridSpacing());

	// Radially averaged reciprocal space correlation function.
	_reciprocalSpaceCorrelation.fill(0.0, numberOfWavevectorBins);
	_reciprocalSpaceCorrelationX.resize(numberOfWavevectorBins);
	QVector<int> numberOfValues(numberOfWavevectorBins, 0);

	// Populate array with reciprocal space vectors.
	for (int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		_reciprocalSpaceCorrelationX[wavevectorBinIndex] = 2*FLOATTYPE_PI*(wavevectorBinIndex+0.5)*minReciprocalSpaceVector;
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

				// Ignore Gamma-point for radial average.
				if (binIndex == 0 && binIndexY == 0 && binIndexZ == 0)
					continue;

				// Compute wavevector.
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
					_reciprocalSpaceCorrelation[wavevectorBinIndex] += std::real(corr);
					numberOfValues[wavevectorBinIndex]++;
				}
			}
		}
	}

	// Compute averages and normalize reciprocal-space correlation function.
	FloatType normalizationFactor = cell().volume3D()/(sourceProperty1()->size()*sourceProperty2()->size());
	for (int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		if (numberOfValues[wavevectorBinIndex] > 0) {
			_reciprocalSpaceCorrelation[wavevectorBinIndex] *= normalizationFactor/numberOfValues[wavevectorBinIndex];
		}
	}

	incrementProgressValue();
	if (isCanceled())
		return;

	// Compute long-ranged part of the real-space correlation function from the FFT convolution.

	// Computer inverse Fourier transform of correlation function.
	c2rFFT(nX, nY, nZ, ftProperty1, gridProperty1);

	incrementProgressValue();
	if (isCanceled())
		return;

	// Determine number of grid points for reciprocal-spacespace correlation function.
	int numberOfDistanceBins = minCellFaceDistance/(2*fftGridSpacing());
	FloatType gridSpacing = minCellFaceDistance/(2*numberOfDistanceBins);

	// Radially averaged real space correlation function.
	_realSpaceCorrelation.fill(0.0, numberOfDistanceBins);
	_realSpaceCorrelationX.resize(numberOfDistanceBins);
	numberOfValues.fill(0, numberOfDistanceBins);

	// Populate array with real space vectors.
	for (int distanceBinIndex = 0; distanceBinIndex < numberOfDistanceBins; distanceBinIndex++) {
		_realSpaceCorrelationX[distanceBinIndex] = (distanceBinIndex+0.5)*gridSpacing;
	}

	// Put real-space correlation function on a radial grid.
	binIndex = 0;
	for (int binIndexX = 0; binIndexX < nX; binIndexX++) {
		for (int binIndexY = 0; binIndexY < nY; binIndexY++) {
			for (int binIndexZ = 0; binIndexZ < nZ; binIndexZ++, binIndex++) {
				// Ignore origin for radial average (which is just the covariance of the quantities).
				if (binIndex == 0 && binIndexY == 0 && binIndexZ == 0)
					continue;

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
					_realSpaceCorrelation[distanceBinIndex] += gridProperty1[binIndex];
					numberOfValues[distanceBinIndex]++;
				}
			}
		}
	}

	// Compute averages and normalize real-space correlation function. Note FFTW computes an unnormalized transform.
	normalizationFactor = 1.0/(sourceProperty1()->size()*sourceProperty2()->size());
	for (int distanceBinIndex = 0; distanceBinIndex < numberOfDistanceBins; distanceBinIndex++) {
		if (numberOfValues[distanceBinIndex] > 0) {
			_realSpaceCorrelation[distanceBinIndex] *= normalizationFactor/numberOfValues[distanceBinIndex];
		}
	}

	incrementProgressValue();
}

/******************************************************************************
* Compute real space correlation function via direction summation over neighbors.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeNeighCorrelation()
{
	// Get number of particles.
	size_t particleCount = positions()->size();

	// Get pointers to data.
	const FloatType *floatData1 = nullptr, *floatData2 = nullptr;
	const int *intData1 = nullptr, *intData2 = nullptr; 
	size_t componentCount1 = sourceProperty1()->componentCount();
	size_t componentCount2 = sourceProperty2()->componentCount();
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

	// Prepare the neighbor list.
	CutoffNeighborFinder neighborListBuilder;
	if (!neighborListBuilder.prepare(_neighCutoff, positions(), cell(), nullptr, this))
		return;

	// Perform analysis on each particle in parallel.
	std::vector<std::thread> workers;
	size_t num_threads = Application::instance().idealThreadCount();
	size_t chunkSize = particleCount / num_threads;
	size_t startIndex = 0;
	size_t endIndex = chunkSize;
	size_t vecComponent1 = _vecComponent1;
	size_t vecComponent2 = _vecComponent2;
	std::mutex mutex;
	for (size_t t = 0; t < num_threads; t++) {
		if (t == num_threads - 1) {
			endIndex += particleCount % num_threads;
		}
		workers.push_back(std::thread([&neighborListBuilder, startIndex, endIndex,
								   	   floatData1, intData1, componentCount1, vecComponent1,
								   	   floatData2, intData2, componentCount2, vecComponent2,
								   	   &mutex, this]() {
			FloatType gridSpacing = (_neighCutoff + FLOATTYPE_EPSILON) / _neighCorrelation.size();
			std::vector<double> threadLocalCorrelation(_neighCorrelation.size(), 0);
			for (size_t i = startIndex; i < endIndex;) {
					for (CutoffNeighborFinder::Query neighQuery(neighborListBuilder, i); !neighQuery.atEnd(); neighQuery.next()) {
					size_t distanceBinIndex = (size_t)(sqrt(neighQuery.distanceSquared()) / gridSpacing);
					distanceBinIndex = std::min(distanceBinIndex, threadLocalCorrelation.size() - 1);
					FloatType data1 = 0.0, data2 = 0.0;
					if (floatData1)
						data1 = floatData1[i*componentCount1 + vecComponent1];
					else if (intData1)
						data1 = intData1[i*componentCount1 + vecComponent1];
					if (floatData2)
						data2 = floatData2[neighQuery.current() * componentCount2 + vecComponent2];
					else if (intData2)
						data2 = intData2[neighQuery.current() * componentCount2 + vecComponent2];
					threadLocalCorrelation[distanceBinIndex] += data1*data2;
				}
					i++;
					// Abort loop when operation was canceled by the user.
				if (isCanceled())
					return;
			}
			std::lock_guard<std::mutex> lock(mutex);
			auto iter_out = _neighCorrelation.begin();
			for (auto iter = threadLocalCorrelation.cbegin(); iter != threadLocalCorrelation.cend(); ++iter, ++iter_out)
				*iter_out += *iter;
		}));
		startIndex = endIndex;
		endIndex += chunkSize;
	}

	for (auto& t: workers)
		t.join();
	incrementProgressValue();

	// Normalize short-ranged real-space correlation function and populate x-array.
	FloatType gridSpacing = (_neighCutoff + FLOATTYPE_EPSILON) / _neighCorrelation.size();
	FloatType normalizationFactor = 3.0*cell().volume3D()/(4.0*FLOATTYPE_PI*sourceProperty1()->size()*sourceProperty2()->size());
	for (int distanceBinIndex = 0; distanceBinIndex < _neighCorrelation.size(); distanceBinIndex++) {
		FloatType distance = distanceBinIndex*gridSpacing;
		FloatType distance2 = (distanceBinIndex+1)*gridSpacing;
		_neighCorrelationX[distanceBinIndex] = (distance+distance2)/2;
		_neighCorrelation[distanceBinIndex] *= normalizationFactor/(distance2*distance2*distance2-distance*distance*distance);
	}

	incrementProgressValue();
}

/******************************************************************************
* Compute means and covariance.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::computeLimits()
{
	// Get pointers to data.
	const FloatType *floatData1 = nullptr, *floatData2 = nullptr;
	const int *intData1 = nullptr, *intData2 = nullptr; 
	size_t componentCount1 = sourceProperty1()->componentCount();
	size_t componentCount2 = sourceProperty2()->componentCount();
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

	// Compute mean and covariance values.
	_mean1 = _mean2 = _covariance = 0.0;
	if (sourceProperty1()->size() != sourceProperty2()->size())
		return;
	for (int particleIndex = 0; particleIndex < sourceProperty1()->size(); particleIndex++) {
		FloatType data1, data2;
		if (floatData1)
			data1 = floatData1[particleIndex];
		else if (intData1)
			data1 = intData1[particleIndex];
		if (floatData2)
			data2 = floatData2[particleIndex];
		else if (intData2)
			data2 = intData2[particleIndex];
		_mean1 += data1;
		_mean2 += data2;
		_covariance += data1*data2;
	}
	_mean1 /= sourceProperty1()->size();
	_mean2 /= sourceProperty2()->size();
	_covariance /= sourceProperty1()->size();

	incrementProgressValue();
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CorrelationFunctionModifier::CorrelationAnalysisEngine::perform()
{
	setProgressText(tr("Computing correlation function"));
	setProgressValue(0);
	if (_neighCorrelation.empty())
		setProgressRange(7);
	else
		setProgressRange(9);

	// Compute reciprocal space correlation function and long-ranged part of
	// the real-space correlation function from an FFT.
	computeFftCorrelation();
	if (isCanceled())
		return;

	// Compute short-ranged part of the real-space correlation function from a direct loop over particle neighbors.
	if (!_neighCorrelation.empty()) {
		if (isCanceled())
			return;
		computeNeighCorrelation();
	}

	computeLimits();
}

/******************************************************************************
* Update plot ranges.
******************************************************************************/
void CorrelationFunctionModifier::updateRanges(FloatType offset, FloatType fac, FloatType reciprocalFac)
{
	// Compute data ranges
	if (!_fixRealSpaceXAxisRange) {
		if (!_realSpaceCorrelationX.empty() && !_neighCorrelationX.empty() && _doComputeNeighCorrelation) {
			_realSpaceXAxisRangeStart = std::min(_realSpaceCorrelationX.first(), _neighCorrelationX.first());
			_realSpaceXAxisRangeEnd = std::max(_realSpaceCorrelationX.last(), _neighCorrelationX.last());
		}
		else if (!_realSpaceCorrelationX.empty()) {
			_realSpaceXAxisRangeStart = _realSpaceCorrelationX.first();
			_realSpaceXAxisRangeEnd = _realSpaceCorrelationX.last();
		}
		else if (!_neighCorrelationX.empty() && _doComputeNeighCorrelation) {
			_realSpaceXAxisRangeStart = _neighCorrelationX.first();
			_realSpaceXAxisRangeEnd = _neighCorrelationX.last();
		}
	}
	if (!_fixRealSpaceYAxisRange) {
		if (!_realSpaceCorrelation.empty() && !_neighCorrelation.empty() && _doComputeNeighCorrelation) {
			auto realSpace = std::minmax_element(_realSpaceCorrelation.begin(), _realSpaceCorrelation.end());
			auto neigh = std::minmax_element(_neighCorrelation.begin(), _neighCorrelation.end());
			_realSpaceYAxisRangeStart = fac*(std::min(*realSpace.first, *neigh.first)-offset);
			_realSpaceYAxisRangeEnd = fac*(std::max(*realSpace.second, *neigh.second)-offset);
		}
		else if (!_realSpaceCorrelation.empty()) {
			auto realSpace = std::minmax_element(_realSpaceCorrelation.begin(), _realSpaceCorrelation.end());
			_realSpaceYAxisRangeStart = fac*(*realSpace.first-offset);
			_realSpaceYAxisRangeEnd = fac*(*realSpace.second-offset);
		}
		else if (!_neighCorrelation.empty() && _doComputeNeighCorrelation) {
			auto neigh = std::minmax_element(_neighCorrelation.begin(), _neighCorrelation.end());
			_realSpaceYAxisRangeStart = fac*(*neigh.first-offset);
			_realSpaceYAxisRangeEnd = fac*(*neigh.second-offset);
		}	
	}
	if (!_fixReciprocalSpaceXAxisRange && !_reciprocalSpaceCorrelationX.empty()) {
		_reciprocalSpaceXAxisRangeStart = _reciprocalSpaceCorrelationX.first();
		_reciprocalSpaceXAxisRangeEnd = _reciprocalSpaceCorrelationX.last();
	}
	if (!_fixReciprocalSpaceYAxisRange && !_reciprocalSpaceCorrelation.empty()) {
		auto reciprocalSpace = std::minmax_element(_reciprocalSpaceCorrelation.begin(), _reciprocalSpaceCorrelation.end());
		_reciprocalSpaceYAxisRangeStart = reciprocalFac*(*reciprocalSpace.first);
		_reciprocalSpaceYAxisRangeEnd = reciprocalFac*(*reciprocalSpace.second);
	}
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void CorrelationFunctionModifier::transferComputationResults(ComputeEngine* engine)
{
	CorrelationAnalysisEngine* eng = static_cast<CorrelationAnalysisEngine*>(engine);
	_realSpaceCorrelation = eng->realSpaceCorrelation();
	_realSpaceCorrelationX = eng->realSpaceCorrelationX();
	_neighCorrelation = eng->neighCorrelation();
	_neighCorrelationX = eng->neighCorrelationX();
	_reciprocalSpaceCorrelation = eng->reciprocalSpaceCorrelation();
	_reciprocalSpaceCorrelationX = eng->reciprocalSpaceCorrelationX();
	_mean1 = eng->mean1();
	_mean2 = eng->mean2();
	_covariance = eng->covariance();
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
	if (field == PROPERTY_FIELD(sourceProperty1) ||
		field == PROPERTY_FIELD(sourceProperty2) ||
		field == PROPERTY_FIELD(fftGridSpacing) ||
		field == PROPERTY_FIELD(doComputeNeighCorrelation) ||
		field == PROPERTY_FIELD(neighCutoff) ||
	    field == PROPERTY_FIELD(numberOfNeighBins))
		invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
