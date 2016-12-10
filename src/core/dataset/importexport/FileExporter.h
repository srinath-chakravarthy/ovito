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

#ifndef __OVITO_FILE_EXPORTER_H
#define __OVITO_FILE_EXPORTER_H

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

	/// \brief Returns the path of the output file written by this exporter.
	const QString& outputFilename() const { return _outputFilename; }

	/// Returns whether only the current animation frame or an entire animation interval should be exported.
	bool exportAnimation() const { return _exportAnimation; }

	/// Sets whether only the current animation frame or an entire animation interval should be exported.
	void setExportAnimation(bool exportAnim) { _exportAnimation = exportAnim; }

	/// \brief Controls whether the exporter should produce separate files for each exported frame.
	void setUseWildcardFilename(bool enable) { _useWildcardFilename = enable; }

	/// \brief Returns whether the exporter produces separate files for each exported frame.
	bool useWildcardFilename() const { return _useWildcardFilename; }

	/// \brief Sets the wildcard pattern used to generate filenames when writing
	///        a separate file for each exported frame.
	///
	/// The wildcard filename must contain the character '*', which will be replaced by the
	/// frame number.
	void setWildcardFilename(const QString& filename) { _wildcardFilename = filename; }

	/// \brief Returns the wild-card pattern used to generate filenames when writing
	///        a separate file for each exported frame.
	const QString& wildcardFilename() const { return _wildcardFilename; }

	/// \brief Sets the start of the animation interval that should be exported to the output file.
	void setStartFrame(int frame) { _startFrame = frame; }

	/// \brief Returns the first frame of the animation interval that will be exported to the output file.
	TimePoint startFrame() const { return _startFrame; }

	/// \brief Sets the end of the animation interval that should be exported to the output file.
	void setEndFrame(int frame) { _endFrame = frame; }

	/// \brief Returns the last frame of the animation interval that will be exported to the output file.
	TimePoint endFrame() const { return _endFrame; }

	/// \brief Returns the interval between exported frames.
	int everyNthFrame() const { return _everyNthFrame; }

	/// \brief Sets the interval between exported frames.
	void setEveryNthFrame(int n) { _everyNthFrame = n; }

	/// \brief Exports the scene objects to the output file(s).
	/// \param progressDisplay Optional callback object which is used by the function to report progress.
	/// \return \c true if the output file has been successfully written;
	///         \c false if the export operation has been canceled by the user.
	/// \throws Util::Exception if the export operation has failed due to an error.
	virtual bool exportNodes(AbstractProgressDisplay* progressDisplay);

protected:

	/// Initializes the object.
	FileExporter(DataSet* dataset);

	/// \brief This is called once for every output file to be written and before exportFrame() is called.
	virtual bool openOutputFile(const QString& filePath, int numberOfFrames) = 0;

	/// \brief This is called once for every output file written after exportFrame() has been called.
	virtual void closeOutputFile(bool exportCompleted) = 0;

	/// \brief Exports a single animation frame to the current output file.
	virtual bool exportFrame(int frameNumber, TimePoint time, const QString& filePath, AbstractProgressDisplay* progressDisplay);

private:

	/// The output file path.
	PropertyField<QString> _outputFilename;

	/// Controls whether only the current animation frame or an entire animation interval should be exported.
	PropertyField<bool> _exportAnimation;

	/// Indicates that the exporter should produce a separate file for each timestep.
	PropertyField<bool> _useWildcardFilename;

	/// The wildcard name that is used to generate the output filenames.
	PropertyField<QString> _wildcardFilename;

	/// The first animation frame that should be exported.
	PropertyField<int> _startFrame;

	/// The last animation frame that should be exported.
	PropertyField<int> _endFrame;

	/// Controls the interval between exported frames.
	PropertyField<int> _everyNthFrame;

	/// Holds the scene objects to be exported.
	QVector<OORef<SceneNode>> _nodesToExport;

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_PROPERTY_FIELD(_outputFilename);
	DECLARE_PROPERTY_FIELD(_exportAnimation);
	DECLARE_PROPERTY_FIELD(_useWildcardFilename);
	DECLARE_PROPERTY_FIELD(_wildcardFilename);
	DECLARE_PROPERTY_FIELD(_startFrame);
	DECLARE_PROPERTY_FIELD(_endFrame);
	DECLARE_PROPERTY_FIELD(_everyNthFrame);
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace

#endif // __OVITO_FILE_EXPORTER_H
