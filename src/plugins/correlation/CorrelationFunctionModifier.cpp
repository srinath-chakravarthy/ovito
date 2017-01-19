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
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, _cutoff, "Cutoff", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(CorrelationFunctionModifier, _numberOfBinsForShortRangedCalculation, "NumberOfBinsForShortRangedCalculation", PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _sourceProperty1, "First property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _sourceProperty2, "Second property");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _cutoff, "Cutoff radius");
SET_PROPERTY_FIELD_LABEL(CorrelationFunctionModifier, _numberOfBinsForShortRangedCalculation, "Number of bins for correlation at short ranges");
SET_PROPERTY_FIELD_UNITS_AND_MINIMUM(CorrelationFunctionModifier, _cutoff, WorldParameterUnit, 0);
SET_PROPERTY_FIELD_UNITS_AND_RANGE(CorrelationFunctionModifier, _numberOfBinsForShortRangedCalculation, IntegerParameterUnit, 4, 100000);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
CorrelationFunctionModifier::CorrelationFunctionModifier(DataSet* dataset) : AsynchronousParticleModifier(dataset),
	_cutoff(3.2), _numberOfBinsForShortRangedCalculation(50)
{
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty1);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty2);
	INIT_PROPERTY_FIELD(CorrelationFunctionModifier::_cutoff);
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
	qDebug() << "CorrelationFunctionModifier::createEngine";

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

	qDebug() << posProperty << property1 << property2;

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<CorrelationAnalysisEngine>(validityInterval,
													   posProperty->storage(),
													   property1->storage(),
													   property2->storage(),
													   inputCell->data(),
													   cutoff());
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

	qDebug() << numberOfGridPoints;

	// Resize real space grid.
	gridData.fill(0.0, numberOfGridPoints);

	qDebug() << "2";

	// Get periodic boundary flag.
	std::array<bool, 3> pbc = cell().pbcFlags();

	qDebug() << "3";

	if(property->size() > 0) {
		const Point3* pos = positions()->constDataPoint3();
		const Point3* pos_end = pos + positions()->size();

		qDebug() << "4";

		if(property->dataType() == qMetaTypeId<FloatType>()) {
			const FloatType* v = property->constDataFloat() + vecComponent;
			const FloatType* v_end = v + (property->size() * vecComponentCount);
			qDebug() << "5";
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
			qDebug() << "6";
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
	qDebug() << "CorrelationFunctionModifier::CorrelationAnalysisEngine::perform";

	setProgressText(tr("Computing correlation function"));

	// Get reciprocal cell.
	AffineTransformation cellMatrix = cell().matrix(); 
	AffineTransformation reciprocalCellMatrix = cell().inverseMatrix();

	int nX = cellMatrix.column(0).length()/cutoff();
	int nY = cellMatrix.column(1).length()/cutoff();
	int nZ = cellMatrix.column(2).length()/cutoff();

	qDebug() << nX << " " << nY << " " << nZ;

	// Map all quantities onto a spatial grid.
	QVector<FloatType> gridProperty1, gridProperty2;
	qDebug() << "A";
	mapToSpatialGrid(sourceProperty1(),
					 0, // FIXME! Selected vector component should be passed to engine.
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridProperty1);
	qDebug() << "B";
	mapToSpatialGrid(sourceProperty2(),
					 0, // FIXME! Selected vector component should be passed to engine.
					 reciprocalCellMatrix,
					 nX, nY, nZ,
					 gridProperty2);

	// FIXME. Apply windowing function in nonperiodic directions here.

	// Use single precision FFTW if Ovito is compiled with single precision
	// floating point type.
#ifdef FLOATTYPE_FLOAT
#define fftw_complex fftwf_complex
#define fftw_plan_dft_r2c_3d fftwf_plan_dft_r2c_3d
#define fftw_plan_dft_c2r_3d fftwf_plan_dft_c2r_3d
#define fftw_execute fftwf_execute
#define fftw_destroy_plan fftwf_destroy_plan
#endif

	// Compute Fourier transform of spatial grid.
	qDebug() << "C";
	QVector<std::complex<FloatType>> ftProperty1(nX*nY*(nZ/2+1));
	auto plan = fftw_plan_dft_r2c_3d(
		nX, nY, nZ,
		gridProperty1.data(),
		reinterpret_cast<fftw_complex*>(ftProperty1.data()),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);

	qDebug() << "D";
		QVector<std::complex<FloatType>> ftProperty2(nX*nY*(nZ/2+1));
		plan = fftw_plan_dft_r2c_3d(
		nX, nY, nZ,
		gridProperty2.data(),
		reinterpret_cast<fftw_complex*>(ftProperty2.data()),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);

	// Compute distance of cell faces.
	FloatType cellFaceDistance1 = 1/reciprocalCellMatrix.row(0).length();
	FloatType cellFaceDistance2 = 1/reciprocalCellMatrix.row(1).length();
	FloatType cellFaceDistance3 = 1/reciprocalCellMatrix.row(2).length();
	qDebug() << "cell face distances " << cellFaceDistance1 << " " << cellFaceDistance2 << " " << cellFaceDistance3;

	FloatType minCellFaceDistance = std::min({cellFaceDistance1, cellFaceDistance2, cellFaceDistance3});

	// Minimum reciprocal space vector is given by the minimum distance of cell faces.
	FloatType minReciprocalSpaceVector = 1/minCellFaceDistance;
	int numberOfWavevectorBins = 1/(2*minReciprocalSpaceVector*cutoff());
	qDebug() << "numberOfWavevectorBins = " << numberOfWavevectorBins;

	// Radially average reciprocal space correlation function.
	_reciprocalSpaceCorrelationFunction.fill(0.0, numberOfWavevectorBins);
	_reciprocalSpaceCorrelationFunctionX.resize(numberOfWavevectorBins);
	QVector<int> numberOfValues(numberOfWavevectorBins, 0);

	// Populate array with reciprocal space vectors.
	for (int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		_reciprocalSpaceCorrelationFunctionX[wavevectorBinIndex] = 2*M_PI*(wavevectorBinIndex+0.5)*minReciprocalSpaceVector;
	}

	qDebug() << "E";

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

	qDebug() << "F";

	// Compute averages and normalize reciprocal-space correlation function.
	FloatType normalizationFactor = cell().volume3D()/(sourceProperty1()->size()*sourceProperty2()->size());
	for (int wavevectorBinIndex = 0; wavevectorBinIndex < numberOfWavevectorBins; wavevectorBinIndex++) {
		if (numberOfValues[wavevectorBinIndex] > 0) {
			_reciprocalSpaceCorrelationFunction[wavevectorBinIndex] *= normalizationFactor/numberOfValues[wavevectorBinIndex];
		}
	}

	qDebug() << "G";

	// Computer inverse Fourier transform of correlation function.
	plan = fftw_plan_dft_c2r_3d(
		nX, nY, nZ,
		reinterpret_cast<fftw_complex*>(ftProperty1.data()),
		gridProperty1.data(),
		FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);

	qDebug() << "H";

	// Determine number of grid points for real-space correlation function.
	int numberOfDistanceBins = minCellFaceDistance/(2*cutoff());
	FloatType gridSpacing = minCellFaceDistance/(2*numberOfDistanceBins);
	qDebug() << "numberOfDistanceBins = " << numberOfDistanceBins;
	qDebug() << "gridSpacing = " << gridSpacing;

	// Radially average reciprocal space correlation function.
	_realSpaceCorrelationFunction.fill(0.0, numberOfDistanceBins);
	_realSpaceCorrelationFunctionX.resize(numberOfDistanceBins);
	numberOfValues.fill(0, numberOfDistanceBins);

	// Populate array with reciprocal space vectors.
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
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void CorrelationFunctionModifier::transferComputationResults(ComputeEngine* engine)
{
	qDebug() << "CorrelationFunctionModifier::transferComputationResults";

	CorrelationAnalysisEngine* eng = static_cast<CorrelationAnalysisEngine*>(engine);
	qDebug() << "rr" << eng->realSpaceCorrelationFunction();
	_realSpaceCorrelationFunction = eng->realSpaceCorrelationFunction();
	_realSpaceCorrelationFunctionX = eng->realSpaceCorrelationFunctionX();
	_reciprocalSpaceCorrelationFunction = eng->reciprocalSpaceCorrelationFunction();
	_reciprocalSpaceCorrelationFunctionX = eng->reciprocalSpaceCorrelationFunctionX();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus CorrelationFunctionModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	qDebug() << "CorrelationFunctionModifier::applyComputationResults";
	return PipelineStatus::Success;
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void CorrelationFunctionModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	qDebug() << "CorrelationFunctionModifier::propertyChanged";

	AsynchronousParticleModifier::propertyChanged(field);

	// Recompute modifier results when the parameters have been changed.
	if (field == PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty1) ||
		field == PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty2) ||
		field == PROPERTY_FIELD(CorrelationFunctionModifier::_cutoff) ||
	    field == PROPERTY_FIELD(CorrelationFunctionModifier::_numberOfBinsForShortRangedCalculation))
		invalidateCachedResults();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
