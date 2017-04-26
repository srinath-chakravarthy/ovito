///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <core/dataset/importexport/FileSourceImporter.h>
#include <core/utilities/io/CompressedTextReader.h>
#include "ParticleFrameLoader.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/**
 * \brief Base class for file parsers that read particle-position data.
 */
class OVITO_PARTICLES_EXPORT ParticleImporter : public FileSourceImporter
{
public:

	/// \brief Constructs a new instance of this class.
	ParticleImporter(DataSet* dataset) : FileSourceImporter(dataset), _isMultiTimestepFile(false) {
		INIT_PROPERTY_FIELD(isMultiTimestepFile);
	}

	/// Scans the given external path (which may be a directory and a wild-card pattern,
	/// or a single file containing multiple frames) to find all available animation frames.
	///
	/// \param sourceUrl The source file or wild-card pattern to scan for animation frames.
	/// \return A Future that will yield the list of discovered animation frames.
	virtual Future<QVector<FileSourceImporter::Frame>> discoverFrames(const QUrl& sourceUrl) override;

	/// This method indicates whether a wildcard pattern should be automatically generated
	/// when the user picks a new input filename.
	virtual bool autoGenerateWildcardPattern() override { return !isMultiTimestepFile(); }

protected:

	/// \brief Is called when the value of a property of this object has changed.
	virtual void propertyChanged(const PropertyFieldDescriptor& field) override;

	/// \brief Determines whether the input file should be scanned to discover all contained frames.
	virtual bool shouldScanFileForTimesteps(const QUrl& sourceUrl) {
		return isMultiTimestepFile();
	}

	/// \brief Scans the given input file to find all contained simulation frames.
	virtual void scanFileForTimesteps(PromiseBase& promise, QVector<FileSourceImporter::Frame>& frames, const QUrl& sourceUrl, CompressedTextReader& stream);

private:

	/// Retrieves the given file in the background and scans it for simulation timesteps.
	QVector<FileSourceImporter::Frame> discoverFramesInFile(const QUrl sourceUrl, PromiseBase& promise);

	/// Indicates that the input file contains multiple timesteps.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, isMultiTimestepFile, setMultiTimestepFile);

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


