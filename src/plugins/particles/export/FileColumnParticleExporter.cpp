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
#include "FileColumnParticleExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FileColumnParticleExporter, ParticleExporter);

/******************************************************************************
 * Loads the user-defined default values of this object's parameter fields from the
* application's settings store.
 *****************************************************************************/
void FileColumnParticleExporter::loadUserDefaults()
{
	ParticleExporter::loadUserDefaults();

	// Restore last output column mapping.
	QSettings settings;
	settings.beginGroup("exporter/particles/");
	if(settings.contains("columnmapping")) {
		try {
			_columnMapping.fromByteArray(settings.value("columnmapping").toByteArray());
		}
		catch(Exception& ex) {
			ex.setContext(dataset());
			ex.prependGeneralMessage(tr("Failed to load previous output column mapping from application settings store."));
			ex.logError();
		}
	}
	settings.endGroup();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
