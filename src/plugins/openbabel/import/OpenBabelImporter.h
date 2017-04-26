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

/**
 * \brief Base class for file parser that make use of the OpenBabel library.
 */
class OpenBabelImporter : public ParticleImporter
{
public:

	/// \brief Constructs a new instance of this class.
	OpenBabelImporter(DataSet *dataset) : ParticleImporter(dataset) {}

	/// Returns the OpenBabel format string used by this class.
	virtual const char* openBabelFormat() const = 0;

	/// Creates an asynchronous loader object that loads the data for the given frame from the external file.
	virtual std::shared_ptr<FrameLoader> createFrameLoader(const Frame& frame, bool isNewlySelectedFile) override {
		return std::make_shared<OpenBabelImportTask>(dataset()->container(), frame, isNewlySelectedFile, openBabelFormat());
	}

private:

	/// The format-specific task object that is responsible for reading an input file in the background.
	class OpenBabelImportTask : public ParticleFrameLoader
	{
	public:

		/// Constructor.
		OpenBabelImportTask(DataSetContainer* container, const FileSourceImporter::Frame& frame, bool isNewFile, const char* obFormat)
			: ParticleFrameLoader(container, frame, isNewFile), _obFormat(obFormat) {}

	protected:

		/// Parses the given input file and stores the data in this container object.
		virtual void parseFile(CompressedTextReader& stream) override;

	private:

		/// The OpenBabel format.
		const char* _obFormat;
	};

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


