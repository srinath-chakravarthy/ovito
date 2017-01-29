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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/import/lammps/LAMMPSBinaryDumpImporter.h>
#include <plugins/particles/gui/import/InputColumnMappingDialog.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/mainwin/MainWindow.h>
#include <core/dataset/DataSetContainer.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "LAMMPSBinaryDumpImporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(LAMMPSBinaryDumpImporterEditor, FileImporterEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSBinaryDumpImporter, LAMMPSBinaryDumpImporterEditor);

/******************************************************************************
* This is called by the system when the user has selected a new file to import.
******************************************************************************/
bool LAMMPSBinaryDumpImporterEditor::inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	// Retrieve column information of input file.
	LAMMPSBinaryDumpImporter* lammpsImporter = static_object_cast<LAMMPSBinaryDumpImporter>(importer);
	InputColumnMapping mapping = lammpsImporter->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(mapping.empty()) return false;

	if(lammpsImporter->columnMapping().size() != mapping.size()) {
		// If this is a newly created file importer, load old mapping from application settings store.
		if(lammpsImporter->columnMapping().empty()) {
			QSettings settings;
			settings.beginGroup("viz/importer/lammps_binary_dump/");
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
			}
		}

		InputColumnMappingDialog dialog(mapping, parent);
		if(dialog.exec() == QDialog::Accepted) {
			lammpsImporter->setColumnMapping(dialog.mapping());
			return true;
		}

		return false;
	}
	else {
		// If number of columns did not change since last time, only update the stored file excerpt.
		InputColumnMapping newMapping = lammpsImporter->columnMapping();
		newMapping.setFileExcerpt(mapping.fileExcerpt());
		lammpsImporter->setColumnMapping(newMapping);
		return true;
	}
}

/******************************************************************************
 * Displays a dialog box that allows the user to edit the custom file column to particle
 * property mapping.
 *****************************************************************************/
bool LAMMPSBinaryDumpImporterEditor::showEditColumnMappingDialog(LAMMPSBinaryDumpImporter* importer, QWidget* parent)
{
	InputColumnMappingDialog dialog(importer->columnMapping(), parent);
	if(dialog.exec() == QDialog::Accepted) {
		importer->setColumnMapping(dialog.mapping());
		// Remember the user-defined mapping for the next time.
		QSettings settings;
		settings.beginGroup("viz/importer/lammps_binary_dump/");
		settings.setValue("columnmapping", dialog.mapping().toByteArray());
		settings.endGroup();
		return true;
	}
	return false;
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSBinaryDumpImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("LAMMPS binary dump"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGroupBox* animFramesBox = new QGroupBox(tr("Timesteps"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(animFramesBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(animFramesBox);

	// Multi-timestep file
	BooleanParameterUI* multitimestepUI = new BooleanParameterUI(this, PROPERTY_FIELD(ParticleImporter::isMultiTimestepFile));
	sublayout->addWidget(multitimestepUI->checkBox());

	QGroupBox* columnMappingBox = new QGroupBox(tr("File columns"), rollout);
	sublayout = new QVBoxLayout(columnMappingBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(columnMappingBox);

	QPushButton* editMappingButton = new QPushButton(tr("Edit column mapping..."));
	sublayout->addWidget(editMappingButton);
	connect(editMappingButton, &QPushButton::clicked, this, &LAMMPSBinaryDumpImporterEditor::onEditColumnMapping);
}

/******************************************************************************
* Is called when the user pressed the "Edit column mapping" button.
******************************************************************************/
void LAMMPSBinaryDumpImporterEditor::onEditColumnMapping()
{
	if(LAMMPSBinaryDumpImporter* importer = static_object_cast<LAMMPSBinaryDumpImporter>(editObject())) {
		UndoableTransaction::handleExceptions(importer->dataset()->undoStack(), tr("Change file column mapping"), [this, importer]() {
			if(showEditColumnMappingDialog(importer, mainWindow())) {
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
