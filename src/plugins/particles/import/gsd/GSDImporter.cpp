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
#include "gsd.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, GSDImporter, ParticleImporter);

/******************************************************************************
* Checks if the given file has format that can be read by this importer.
******************************************************************************/
bool GSDImporter::checkFileFormat(QFileDevice& input, const QUrl& sourceLocation)
{
	QString filename = QDir::toNativeSeparators(input.fileName());

	gsd_handle handle;
	if(gsd_open(&handle, filename.toLocal8Bit().constData(), GSD_OPEN_READONLY) == 0) {
		gsd_close(&handle);
		return true;
	}

	return false;
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
	gsd_handle handle;
	int err = gsd_open(&handle, filename.toLocal8Bit().constData(), GSD_OPEN_READONLY);
	switch(err) {
	case 0: break; // Success
	case -1: throw Exception(tr("Failed to open GSD file for reading. I/O error."));
	case -2: throw Exception(tr("Failed to open GSD file for reading. Not a GSD file."));
	case -3: throw Exception(tr("Failed to open GSD file for reading. Invalid GSD file version."));
	case -4: throw Exception(tr("Failed to open GSD file for reading. Corrupt file."));
	case -5: throw Exception(tr("Failed to open GSD file for reading. Unable to allocate memory."));
	default: throw Exception(tr("Failed to open GSD file for reading. Unknown error."));
	}

	try {
		gsd_close(&handle);
	}
	catch(...) {
		gsd_close(&handle);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
