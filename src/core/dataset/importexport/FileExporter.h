///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2014) Alexander Stukowski
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
#include <core/reference/RefTarget.h>
#include <core/dataset/DataSet.h>
#include <core/scene/SceneNode.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

/**
 * \brief Abstract base class for file exporters that export data from OVITO to an external file.
 *
 * To add an exporter for a new file format to OVITO you should derive a new class from FileExporter or
 * one of its specializations and implement the abstract methods fileFilter(), fileFilterDescription(),
 * and exportToFile().
 *
 * A list of all available exporters can be obtained via FileExporter::availableExporters().
 */
class OVITO_CORE_EXPORT FileExporter : public RefTarget
{
public:

	/// Returns the list of all available exporter types installed in the system.
	static QVector<OvitoObjectType*> availableExporters();

public:

	/// \brief Returns the filename filter that specifies the file extension that can be exported by this service.
	/// \return A wild-card pattern for the file types that can be produced by this export class (e.g. \c "*.xyz" or \c "*").
	virtual QString fileFilter() = 0;

	/// \brief Returns the file type description that is displayed in the drop-down box of the export file dialog.
	/// \return A human-readable string describing the file format written by this FileExporter.
	virtual QString fileFilterDescription() = 0;

	/// \brief Selects the natural scene nodes to be exported by this exporter under normal circumstances.
	virtual void selectStandardOutputData() = 0; 

	/// \brief Sets the scene objects to be exported.
	void setOutputData(const QVector<SceneNode*>& nodes);

	/// \brief Returns the list of the scene objects to be exported.
	const QVector<OORef<SceneNode>>& outputData() const { return _nodesToExport; }

	/// \brief Sets the name of the output file that should be written by this exporter.
	virtual void setOutputFilename(const QString& filename);
	
	/// \brief Exports the scene objects to the output file(s).
	/// \return \c true if the output file has been successfully written;
	///         \c false if the export operation has been canceled by the user.
	/// \throws Util::Exception if the export operation has failed due to an error.
	virtual bool exportNodes(TaskManager& taskManager);

protected:

	/// Initializes the object.
	FileExporter(DataSet* dataset);

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames) = 0;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) = 0;

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager);

private:

	/// The output file path.
	DECLARE_PROPERTY_FIELD(QString, outputFilename);

	/// Controls whether only the current animation frame or an entire animation interval should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, exportAnimation, setExportAnimation);

	/// Indicates that the exporter should produce a separate file for each timestep.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, useWildcardFilename, setUseWildcardFilename);

	/// The wildcard name that is used to generate the output filenames.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(QString, wildcardFilename, setWildcardFilename);

	/// The first animation frame that should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, startFrame, setStartFrame);

	/// The last animation frame that should be exported.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, endFrame, setEndFrame);

	/// Controls the interval between exported frames.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(int, everyNthFrame, setEveryNthFrame);

	/// Holds the scene objects to be exported.
	QVector<OORef<SceneNode>> _nodesToExport;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


