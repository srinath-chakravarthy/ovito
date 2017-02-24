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

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames) override;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// \brief Writes the particles of one animation frame to the current output file.
	virtual bool exportObject(SceneNode* sceneNode, int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager) override;

private:

	/// Checks for NetCDF error and throws exception.
	void ncerr(int err, const char* file, int line);

	/// Checks for NetCDF error and throws exception (and attach additional information to exception string.
	void ncerr_with_info(int err, const char* file, int line, const QString& info);

	/// The NetCDF file handle.
	int _ncid = -1;

	// NetCDF file dimensions:
	int _frame_dim;
	int _spatial_dim;
	int _Voigt_dim;
	int _atom_dim = -1;
	int _cell_spatial_dim;
	int _cell_angular_dim;
	int _label_dim;

	// NetCDF file variables:
	int _spatial_var;
	int _cell_spatial_var;
	int _cell_angular_var;
	int _time_var;
	int _cell_origin_var;
	int _cell_lengths_var;
	int _cell_angles_var;
	int _coords_var;

	/// NetCDF file variables for global attributes.
	QMap<QString, int> _attributes_vars;

	struct NCOutputColumn {
		NCOutputColumn(const ParticlePropertyReference& p, int dt, size_t cc, int ncv) :
			property(p), dataType(dt), componentCount(cc), ncvar(ncv) {}

		ParticlePropertyReference property;
		int dataType;
		size_t componentCount;
		int ncvar;
	};

	std::vector<NCOutputColumn> _columns;

	/// The number of frames written.
	size_t _frameCounter;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


