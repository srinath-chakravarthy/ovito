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
#include "GSDImporter.h"
#include "GSDFile.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(GSDImporter, ParticleImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool GSDImporter::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation)
{
	QString filename = QDir::toNativeSeparators(input.fileName());

	gsd_handle handle;
	if(::gsd_open(&handle, filename.toLocal8Bit().constData(), GSD_OPEN_READONLY) == 0) {
		::gsd_close(&handle);
		return true;
	}

	return false;
}

/******************************************************************************
* Scans the input file for simulation timesteps.
******************************************************************************/
void GSDImporter::scanFileForTimesteps(PromiseBase& promise, QVector<FileSourceImporter::Frame>& frames, const QUrl& sourceUrl, CompressedTextReader& stream)
{
	// First close text stream, we don't need it here.
	QFileDevice& file = stream.device();
	file.close();
	QString filename = QDir::toNativeSeparators(file.fileName());

	// Open GSD file for reading.
	GSDFile gsd(filename.toLocal8Bit().constData());
	uint64_t nFrames = gsd.numerOfFrames();

	QFileInfo fileInfo(filename);
	QDateTime lastModified = fileInfo.lastModified();
	for(uint64_t i = 0; i < nFrames; i++) {
		Frame frame;
		frame.sourceFile = sourceUrl;
		frame.byteOffset = 0;
		frame.lineNumber = i;
		frame.lastModificationTime = lastModified;
		frame.label = tr("Frame %1").arg(i);
		frames.push_back(frame);
	}
}

/******************************************************************************
* Parses the given input file and stores the data in the given container object.
******************************************************************************/
void GSDImporter::GSDImportTask::parseFile(CompressedTextReader& stream)
{
	setProgressText(tr("Reading GSD file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// First close text stream, we don't need it here.
	QFileDevice& file = stream.device();
	file.close();

	// Open GSD file for reading.
	QString filename = QDir::toNativeSeparators(file.fileName());
	GSDFile gsd(filename.toLocal8Bit().constData());

	// Check schema name.
	if(qstrcmp(gsd.schemaName(), "hoomd") != 0)
		throw Exception(tr("Failed to open GSD file for reading. File schema must be 'hoomd', but found '%1'.").arg(gsd.schemaName()));

	// Parse number of frames in file.
	uint64_t nFrames = gsd.numerOfFrames();

	// The animation frame to read from the GSD file.
	uint64_t frameNumber = frame().lineNumber;

	// Parse simulation step.
	uint64_t simulationStep = gsd.readOptionalScalar<uint64_t>("configuration/step", frameNumber, 0);
	attributes().insert(QStringLiteral("Timestep"), QVariant::fromValue(simulationStep));

	// Parse number of dimensions.
	uint8_t ndimensions = gsd.readOptionalScalar<uint8_t>("configuration/dimensions", frameNumber, 3);

	// Parse simulation box.
	std::array<float,6> boxValues = {1,1,1,0,0,0};
	gsd.readOptional1DArray("configuration/box", frameNumber, boxValues);
	AffineTransformation simCell = AffineTransformation::Identity();
	simCell(0,0) = boxValues[0];
	simCell(1,1) = boxValues[1];
	simCell(2,2) = boxValues[2];
	simCell(0,1) = boxValues[3] * boxValues[1];
	simCell(0,2) = boxValues[4] * boxValues[2];
	simCell(1,2) = boxValues[5] * boxValues[2];
	simCell.column(3) = simCell * Vector3(FloatType(-0.5));
	simulationCell().setMatrix(simCell);
	simulationCell().setPbcFlags(true, true, true);
	simulationCell().set2D(ndimensions == 2);

	// Parse number of particles.
	uint32_t numParticles = gsd.readOptionalScalar<uint32_t>("particles/N", frameNumber, 0);

	// Parse list of particle type names.
	QStringList particleTypeNames = gsd.readStringTable("particles/types", frameNumber);
	if(particleTypeNames.empty())
		particleTypeNames.push_back(QStringLiteral("A"));

	// Read particle positions.
	ParticleProperty* posProperty = new ParticleProperty(numParticles, ParticleProperty::PositionProperty, 0, false);
	addParticleProperty(posProperty);
	gsd.readFloatArray("particles/position", frameNumber, posProperty->dataPoint3(), numParticles, posProperty->componentCount());

	// Create particle types.
	ParticleProperty* typeProperty = new ParticleProperty(numParticles, ParticleProperty::ParticleTypeProperty, 0, false);
	ParticleFrameLoader::ParticleTypeList* typeList = new ParticleFrameLoader::ParticleTypeList();
	addParticleProperty(typeProperty, typeList);
	for(int i = 0; i < particleTypeNames.size(); i++)
		typeList->addParticleTypeId(i, particleTypeNames[i]);

	// Read particle types.
	if(gsd.hasChunk("particles/typeid", frameNumber))
		gsd.readIntArray("particles/typeid", frameNumber, typeProperty->dataInt(), numParticles);
	else
		std::fill(typeProperty->dataInt(), typeProperty->dataInt() + typeProperty->size(), 0);

	ParticleProperty* massProperty = readOptionalParticleProperty(gsd, "particles/mass", frameNumber, numParticles, ParticleProperty::MassProperty);
	readOptionalParticleProperty(gsd, "particles/charge", frameNumber, numParticles, ParticleProperty::ChargeProperty);
	ParticleProperty* velocityProperty = readOptionalParticleProperty(gsd, "particles/velocity", frameNumber, numParticles, ParticleProperty::VelocityProperty);
	ParticleProperty* radiusProperty = readOptionalParticleProperty(gsd, "particles/diameter", frameNumber, numParticles, ParticleProperty::RadiusProperty);
	if(radiusProperty) {
		// Convert particle diameter to radius by dividing by 2.
		std::for_each(radiusProperty->dataFloat(), radiusProperty->dataFloat() + radiusProperty->size(), [](FloatType& r) { r /= 2; });
	}
	ParticleProperty* orientationProperty = readOptionalParticleProperty(gsd, "particles/orientation", frameNumber, numParticles, ParticleProperty::OrientationProperty);
	if(orientationProperty) {
		// Convert quaternion representation from GSD format to internal format.
		// Left-shift all quaternion components by one: (W,X,Y,Z) -> (X,Y,Z,W).
		std::for_each(orientationProperty->dataQuaternion(), orientationProperty->dataQuaternion() + orientationProperty->size(), [](Quaternion& q) { std::rotate(q.begin(), q.begin() + 1, q.end()); });
	}

	// Parse number of bonds.
	uint32_t numBonds = gsd.readOptionalScalar<uint32_t>("bonds/N", frameNumber, 0);
	if(numBonds != 0) {
		// Read bond list.
		std::vector<int> bondList(numBonds * 2);
		gsd.readIntArray("bonds/group", frameNumber, bondList.data(), numBonds, 2);

		// Convert to OVITO format.
		setBonds(new BondsStorage());
		bonds()->reserve(numBonds * 2);
		for(auto b = bondList.cbegin(); b != bondList.cend(); ) {
			unsigned int atomIndex1 = *b++;
			unsigned int atomIndex2 = *b++;
			if(atomIndex1 >= numParticles || atomIndex2 >= numParticles)
				throw Exception(tr("Nonexistent atom tag in bond list in GSD file."));

			// Use minimum image convention to determine PBC shift vector of the bond.
			Vector3 delta = simulationCell().absoluteToReduced(posProperty->getPoint3(atomIndex2) - posProperty->getPoint3(atomIndex1));
			Vector_3<int8_t> shift = Vector_3<int8_t>::Zero();
			for(size_t dim = 0; dim < 3; dim++) {
				if(simulationCell().pbcFlags()[dim])
					shift[dim] -= (int8_t)floor(delta[dim] + FloatType(0.5));
			}

			// Create two half-bonds.
			bonds()->push_back({  shift, atomIndex1, atomIndex2 });
			bonds()->push_back({ -shift, atomIndex2, atomIndex1 });
		}

		// Read bond types.
		if(gsd.hasChunk("bonds/types", frameNumber)) {

			// Parse list of bond type names.
			QStringList bondTypeNames = gsd.readStringTable("bonds/types", frameNumber);
			if(bondTypeNames.empty())
				bondTypeNames.push_back(QStringLiteral("A"));

			// Create bond types.
			BondProperty* bondTypeProperty = new BondProperty(numBonds * 2, BondProperty::BondTypeProperty, 0, false);
			ParticleFrameLoader::BondTypeList* bondTypeList = new ParticleFrameLoader::BondTypeList();
			addBondProperty(bondTypeProperty, bondTypeList);
			for(int i = 0; i < bondTypeNames.size(); i++)
				bondTypeList->addBondTypeId(i, bondTypeNames[i]);

			// Read bond types.
			if(gsd.hasChunk("bonds/typeid", frameNumber)) {
				gsd.readIntArray("bonds/typeid", frameNumber, bondTypeProperty->dataInt(), numBonds);
				// Duplicate data for half-bonds.
				for(size_t i = numBonds; i-- != 0; ) {
					bondTypeProperty->setInt(i*2+1, bondTypeProperty->getInt(i));
					bondTypeProperty->setInt(i*2+0, bondTypeProperty->getInt(i));
				}
			}
			else {
				std::fill(bondTypeProperty->dataInt(), bondTypeProperty->dataInt() + bondTypeProperty->size(), 0);
			}
		}
	}

	QString statusString = tr("Number of particles: %1").arg(numParticles);
	if(numBonds != 0)
		statusString += tr("\nNumber of bonds: %1").arg(numBonds);
	setStatus(statusString);
}

/******************************************************************************
* Reads the values of a particle property from the GSD file.
******************************************************************************/
ParticleProperty* GSDImporter::GSDImportTask::readOptionalParticleProperty(GSDFile& gsd, const char* chunkName, uint64_t frameNumber, uint32_t numParticles, ParticleProperty::Type propertyType)
{
	if(gsd.hasChunk(chunkName, frameNumber)) {
		ParticleProperty* prop = new ParticleProperty(numParticles, propertyType, 0, false);
		addParticleProperty(prop);
		gsd.readFloatArray(chunkName, frameNumber, prop->dataFloat(), numParticles, prop->componentCount());
		return prop;
	}
	else return nullptr;
}


OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
