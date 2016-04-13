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
#include <plugins/particles/import/xyz/XYZImporter.h>
#include <plugins/particles/gui/import/InputColumnMappingDialog.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/mainwin/MainWindow.h>
#include <core/dataset/importexport/FileSource.h>
#include "XYZImporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, XYZImporterEditor, FileImporterEditor);
SET_OVITO_OBJECT_EDITOR(XYZImporter, XYZImporterEditor);

/******************************************************************************
* This method is called by the FileSource each time a new source
* file has been selected by the user.
******************************************************************************/
bool XYZImporterEditor::inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	XYZImporter* xyzImporter = static_object_cast<XYZImporter>(importer);
	InputColumnMapping mapping = xyzImporter->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(mapping.empty()) return false;

	// If column names were given in the XYZ file, use them rather than popping up a dialog.
	if(mapping.hasFileColumnNames()) {
		return true;
	}

	// If this is a newly created file importer, load old mapping from application settings store.
	if(xyzImporter->columnMapping().empty()) {
		QSettings settings;
		settings.beginGroup("viz/importer/xyz/");
		if(settings.contains("columnmapping")) {
			try {
				InputColumnMapping storedMapping;
				storedMapping.fromByteArray(settings.value("columnmapping").toByteArray());
				std::copy_n(storedMapping.begin(), std::min(storedMapping.size(), mapping.size()), mapping.begin());
			}
			catch(Exception& ex) {
				ex.prependGeneralMessage(tr("Failed to load last used column-to-property mapping from application settings store."));
				ex.logError();
			}
			for(auto& column : mapping)
				column.columnName.clear();
		}
	}

	InputColumnMappingDialog dialog(mapping, parent);
	if(dialog.exec() == QDialog::Accepted) {
		xyzImporter->setColumnMapping(dialog.mapping());

		// Remember the user-defined mapping for the next time.
		QSettings settings;
		settings.beginGroup("viz/importer/xyz/");
		settings.setValue("columnmapping", dialog.mapping().toByteArray());
		settings.endGroup();

		return true;
	}

	return false;
}

/******************************************************************************
 * Displays a dialog box that allows the user to edit the custom file column to particle
 * property mapping.
 *****************************************************************************/
bool XYZImporterEditor::showEditColumnMappingDialog(XYZImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	InputColumnMapping mapping = importer->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(mapping.empty()) return false;

	if(!importer->columnMapping().empty()) {
		InputColumnMapping customMapping = importer->columnMapping();
		customMapping.resize(mapping.size());
		for(size_t i = 0; i < customMapping.size(); i++)
			customMapping[i].columnName = mapping[i].columnName;
		mapping = customMapping;
	}

	InputColumnMappingDialog dialog(mapping, parent);
	if(dialog.exec() == QDialog::Accepted) {
		importer->setColumnMapping(dialog.mapping());
		// Remember the user-defined mapping for the next time.
		QSettings settings;
		settings.beginGroup("viz/importer/xyz/");
		settings.setValue("columnmapping", dialog.mapping().toByteArray());
		settings.endGroup();
		return true;
	}
	else {
		return false;
	}
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void XYZImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("XYZ"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* animFramesBox = new QGroupBox(tr("Timesteps"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(animFramesBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(animFramesBox);

	// Multi-timestep file
	BooleanParameterUI* multitimestepUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::_isMultiTimestepFile));
	sublayout->addWidget(multitimestepUI->checkBox());

	QGroupBox* columnMappingBox = new QGroupBox(tr("File columns"), rollout);
	sublayout = new QVBoxLayout(columnMappingBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(columnMappingBox);

	QPushButton* editMappingButton = new QPushButton(tr("Edit column mapping..."));
	sublayout->addWidget(editMappingButton);
	connect(editMappingButton, &QPushButton::clicked, this, &XYZImporterEditor::onEditColumnMapping);
}

/******************************************************************************
* Is called when the user pressed the "Edit column mapping" button.
******************************************************************************/
void XYZImporterEditor::onEditColumnMapping()
{
	if(XYZImporter* importer = static_object_cast<XYZImporter>(editObject())) {

		// Determine URL of current input file.
		FileSource* fileSource = nullptr;
		for(RefMaker* refmaker : importer->dependents()) {
			fileSource = dynamic_object_cast<FileSource>(refmaker);
			if(fileSource) break;
		}
		if(!fileSource || fileSource->frames().empty()) return;

		QUrl sourceUrl;
		if(fileSource->loadedFrameIndex() >= 0)
			sourceUrl = fileSource->frames()[fileSource->loadedFrameIndex()].sourceFile;
		else
			sourceUrl = fileSource->frames().front().sourceFile;

		UndoableTransaction::handleExceptions(importer->dataset()->undoStack(), tr("Change file column mapping"), [this, &sourceUrl, importer]() {
			if(showEditColumnMappingDialog(importer, sourceUrl, mainWindow())) {
				importer->requestReload();
			}
		});
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
