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


#include <core/Core.h>
#include <core/dataset/importexport/FileExporter.h>
#include <core/utilities/io/CompressedTextWriter.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

/**
 * \brief File exporter class that writes out scalar attributes computed by the data pipeline to a text file.
 */
class OVITO_CORE_EXPORT AttributeFileExporter : public FileExporter
{
public:

	/// \brief Constructs a new instance of this class.
	Q_INVOKABLE AttributeFileExporter(DataSet* dataset);

	/// \brief Returns the file filter that specifies the files that can be exported by this service.
	virtual QString fileFilter() override { return QStringLiteral("*"); }

	/// \brief Returns the filter description that is displayed in the drop-down box of the file dialog.
	virtual QString fileFilterDescription() override { return tr("Calculation Results Text File"); }

	/// \brief Loads the user-defined default values of this object's parameter fields from the
	///        application's settings store.
	virtual void loadUserDefaults() override;

 	/// \brief Selects the natural scene nodes to be exported by this exporter under normal circumstances.
	virtual void selectStandardOutputData() override; 	

	/// \brief Evaluates the pipeline of an ObjectNode and returns the computed attributes.
	bool getAttributes(SceneNode* sceneNode, TimePoint time, QVariantMap& attributes, TaskManager& taskManager);

protected:

	/// \brief This is called once for every output file to be written and before exportData() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames) override;

	/// \brief This is called once for every output file written after exportData() has been called.
	virtual void closeOutputFile(bool exportCompleted) override;

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager) override;

	/// Returns the current file this exporter is writing to.
	QFile& outputFile() { return _outputFile; }

	/// Returns the text stream used to write into the current output file.
	CompressedTextWriter& textStream() { return *_outputStream; }

private:

	/// The output file stream.
	QFile _outputFile;

	/// The stream object used to write into the output file.
	std::unique_ptr<CompressedTextWriter> _outputStream;

	/// The list of global attributes to export.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QStringList, attributesToExport, setAttributesToExport);

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


