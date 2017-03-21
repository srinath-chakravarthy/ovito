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

#pragma once


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/dataset/importexport/FileExporter.h>
#include <core/utilities/io/CompressedTextWriter.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief Exporter that exports dislocation lines to a Crystal Analysis Tool (CA) file.
 */
class OVITO_CRYSTALANALYSIS_EXPORT CAExporter : public FileExporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE CAExporter(DataSet* dataset) : FileExporter(dataset), _meshExportEnabled(true) {}

	/// \brief Returns the file filter that specifies the files that can be exported by this service.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	virtual QString fileFilterDescription() override { return tr("Crystal Analysis File"); }

	/// \brief Selects the natural scene nodes to be exported by this exporter under normal circumstances.
	virtual void selectStandardOutputData() override; 

	/// Returns whether the DXA defect mesh is exported (in addition to the dislocation lines).
	bool meshExportEnabled() const { return _meshExportEnabled; }

	/// Sets whether the DXA defect mesh is exported (in addition to the dislocation lines).
	void setMeshExportEnabled(bool enable) { _meshExportEnabled = enable; }

protected:

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager) override;

	/// \brief This is called once for every output file to be written and before exportData() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames) override;

	/// \brief This is called once for every output file written after exportData() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// Returns the current file this exporter is writing to.
	QFile& outputFile() { return _outputFile; }

	/// Returns the text stream used to write into the current output file.
	CompressedTextWriter& textStream() { return *_outputStream; }

private:

	/// The output file stream.
	QFile _outputFile;

	/// The stream object used to write into the output file.
	std::unique_ptr<CompressedTextWriter> _outputStream;

	/// Controls whether the DXA defect mesh is exported (in addition to the dislocation lines).
	bool _meshExportEnabled;

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


