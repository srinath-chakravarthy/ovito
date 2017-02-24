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

#pragma once


#include <plugins/particles/Particles.h>
#include <plugins/particles/import/ParticleImporter.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

class GSDFile;	// Defined in GSDFile.h

/**
 * \brief File parser for GSD (General Simulation Data) files.
 */
class OVITO_PARTICLES_EXPORT GSDImporter : public ParticleImporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE GSDImporter(DataSet* dataset) : ParticleImporter(dataset) {
		setMultiTimestepFile(true);
	}

	/// \brief Returns the file filter that specifies the files that can be imported by this service.
	/// \return A wild-card pattern that specifies the file types that can be handled by this import class.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	/// \return A string that describes the file format.
	virtual QString fileFilterDescription() override { return tr("GSD/HOOMD Files"); }

	/// \brief Checks if the given file has format that can be read by this importer.
	virtual bool checkFileFormat(QFileDevice& input, const QUrl& sourceLocation) override;

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("GSD"); }

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FrameLoader> createFrameLoader(const Frame& frame, bool isNewlySelectedFile) override {
		return std::make_shared<GSDImportTask>(dataset()->container(), frame, isNewlySelectedFile);
	}

protected:

	/// \brief Scans the given input file to find all contained simulation frames.
	virtual void scanFileForTimesteps(PromiseBase& promise, QVector<FileSourceImporter::Frame>& frames, const QUrl& sourceUrl, CompressedTextReader& stream) override;

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class GSDImportTask : public ParticleFrameLoader
	{
	public:

		/// Normal constructor.
		GSDImportTask(DataSetContainer* container, const FileSourceImporter::Frame& frame, bool isNewFile)
			: ParticleFrameLoader(container, frame, isNewFile) {}

	protected:

		/// Parses the given input file and stores the data in this container object.
		virtual void parseFile(CompressedTextReader& stream) override;

		/// Reads the values of a particle property from the GSD file.
		ParticleProperty* readOptionalParticleProperty(GSDFile& gsd, const char* chunkName, uint64_t frameNumber, uint32_t numParticles, ParticleProperty::Type propertyType);
	};

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


