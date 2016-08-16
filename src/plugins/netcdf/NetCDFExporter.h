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

#ifndef __OVITO_NETCDF_FILE_EXPORTER_H
#define __OVITO_NETCDF_FILE_EXPORTER_H

#include <plugins/particles/Particles.h>
#include <plugins/particles/export/FileColumnParticleExporter.h>

#ifdef NetCDFPlugin_EXPORTS		// This is defined by CMake when building the plugin library.
#  define OVITO_NETCDF_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_NETCDF_EXPORT Q_DECL_IMPORT
#endif

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

/**
 * \brief Exporter that writes the particles to an extended AMBER NetCDF file.
 */
class OVITO_NETCDF_EXPORT NetCDFExporter : public FileColumnParticleExporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE NetCDFExporter(DataSet* dataset) : FileColumnParticleExporter(dataset) {}

	/// \brief Returns the file filter that specifies the files that can be exported by this service.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	virtual QString fileFilterDescription() override { return tr("NetCDF File"); }

protected:

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportObject(SceneNode* sceneNode, int frameNumber, TimePoint time, const QString& filePath, AbstractProgressDisplay* progressDisplay) override;

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace

#endif // __OVITO_NETCDF_FILE_EXPORTER_H
