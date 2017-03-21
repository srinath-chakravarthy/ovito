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

#include <plugins/particles/Particles.h>
#include <core/utilities/io/FileManager.h>
#include <core/utilities/concurrent/Future.h>
#include <core/dataset/importexport/FileSource.h>
#include "FHIAimsLogFileImporter.h"

#include <boost/algorithm/string.hpp>
#include <QRegularExpression>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FHIAimsLogFileImporter, ParticleImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool FHIAimsLogFileImporter::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation)
{
	// Open input file.
	CompressedTextReader stream(input, sourceLocation.path());

	// Look for 'Invoking FHI-aims' message.
	// It must appear within the first 20 lines of the file.
	for(int i = 0; i < 20 && !stream.eof(); i++) {
		const char* line = stream.readLineTrimLeft(128);
		if(boost::algorithm::starts_with(line, "Invoking FHI-aims")) {
			return true;
		}
	}
	return false;
}

/******************************************************************************
* Scans the given input file to find all contained simulation frames.
******************************************************************************/
void FHIAimsLogFileImporter::scanFileForTimesteps(PromiseBase& promise, QVector<FileSourceImporter::Frame>& frames, const QUrl& sourceUrl, CompressedTextReader& stream)
{
	promise.setProgressText(tr("Scanning FHI-aims log file %1").arg(stream.filename()));
	promise.setProgressMaximum(stream.underlyingSize() / 1000);

	// Regular expression for whitespace characters.
	QRegularExpression ws_re(QStringLiteral("\\s+"));

	QFileInfo fileInfo(stream.device().fileName());
	QString filename = fileInfo.fileName();
	QDateTime lastModified = fileInfo.lastModified();
	int frameNumber = 0;

	while(!stream.eof() && !promise.isCanceled()) {
		const char* line = stream.readLineTrimLeft();
		if(boost::algorithm::starts_with(line, "Updated atomic structure:")) {
			stream.readLine();
			Frame frame;
			frame.sourceFile = sourceUrl;
			frame.byteOffset = stream.byteOffset();
			frame.lineNumber = stream.lineNumber();
			frame.lastModificationTime = lastModified;
			frame.label = QString("%1 (Frame %2)").arg(filename).arg(frameNumber++);
			frames.push_back(frame);
		}

		promise.setProgressValueIntermittent(stream.underlyingByteOffset() / 1000);
		if(promise.isCanceled())
			return;
	}
}

/******************************************************************************
* Parses the given input file and stores the data in the given container object.
******************************************************************************/
void FHIAimsLogFileImporter::FHIAimsImportTask::parseFile(CompressedTextReader& stream)
{
	setProgressText(tr("Reading FHI-aims log file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
	qint64 fileOffset = stream.byteOffset();

	// First pass: determine the cell geometry and number of atoms.
	AffineTransformation cell = AffineTransformation::Identity();
	int lattVecCount = 0;
	int totalAtomCount = 0;
	while(!stream.eof()) {
		const char* line = stream.readLineTrimLeft();
		if(boost::algorithm::starts_with(line, "lattice_vector")) {
			if(lattVecCount >= 3)
				throw Exception(tr("FHI-aims file contains more than three lattice vectors (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
			if(sscanf(line, "lattice_vector " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING, &cell(0,lattVecCount), &cell(1,lattVecCount), &cell(2,lattVecCount)) != 3 || cell.column(lattVecCount) == Vector3::Zero())
				throw Exception(tr("Invalid cell vector in FHI-aims (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
			lattVecCount++;
		}
		else if(boost::algorithm::starts_with(line, "atom")) {
			totalAtomCount++;
		}
		else if(line[0] > ' ') {
			break;
		}
	}
	if(totalAtomCount == 0)
		throw Exception(tr("Invalid FHI-aims log file: No atoms found."));

	// Create the particle properties.
	ParticleProperty* posProperty = new ParticleProperty(totalAtomCount, ParticleProperty::PositionProperty, 0, false);
	addParticleProperty(posProperty);
	ParticleProperty* typeProperty = new ParticleProperty(totalAtomCount, ParticleProperty::ParticleTypeProperty, 0, false);
	ParticleFrameLoader::ParticleTypeList* typeList = new ParticleFrameLoader::ParticleTypeList();
	addParticleProperty(typeProperty, typeList);

	// Return to beginning of frame.
	stream.seek(fileOffset);

	// Second pass: read atom coordinates and types.
	for(int i = 0; i < totalAtomCount; i++) {
		while(true) {
			const char* line = stream.readLineTrimLeft();
			if(boost::algorithm::starts_with(line, "atom")) {
				bool isFractional = boost::algorithm::starts_with(line, "atom_frac");
				Point3& pos = posProperty->dataPoint3()[i];
				char atomTypeName[16];
				if(sscanf(line + (isFractional ? 9 : 4), FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " " FLOATTYPE_SCANF_STRING " %15s", &pos.x(), &pos.y(), &pos.z(), atomTypeName) != 4)
					throw Exception(tr("Invalid atom specification (line %1): %2").arg(stream.lineNumber()).arg(stream.lineString()));
				if(isFractional) {
					if(lattVecCount != 3)
						throw Exception(tr("Invalid fractional atom coordinates (in line %1). Cell vectors have not been specified: %2").arg(stream.lineNumber()).arg(stream.lineString()));
					pos = cell * pos;
				}
				typeProperty->setInt(i, typeList->addParticleTypeName(atomTypeName));
				break;
			}
		}
	}

	// Since we created particle types on the go while reading the particles, the assigned particle type IDs
	// depend on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
	// why we sort them now.
	typeList->sortParticleTypesByName(typeProperty);

	// Set simulation cell.
	if(lattVecCount == 3) {
		simulationCell().setMatrix(cell);
		simulationCell().setPbcFlags(true, true, true);
	}
	else {
		// If the input file does not contain simulation cell info,
		// Use bounding box of particles as simulation cell.

		Box3 boundingBox;
		boundingBox.addPoints(posProperty->constDataPoint3(), posProperty->size());
		simulationCell().setMatrix(AffineTransformation(
				Vector3(boundingBox.sizeX(), 0, 0),
				Vector3(0, boundingBox.sizeY(), 0),
				Vector3(0, 0, boundingBox.sizeZ()),
				boundingBox.minc - Point3::Origin()));
		simulationCell().setPbcFlags(false, false, false);
	}

	setStatus(tr("%1 atoms").arg(totalAtomCount));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
