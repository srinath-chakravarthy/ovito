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

#include <gui/GUI.h>
#include <core/dataset/importexport/FileImporter.h>
#include <core/dataset/UndoStack.h>
#include <core/app/Application.h>
#include <core/animation/AnimationSettings.h>
#include <core/scene/SceneRoot.h>
#include <core/scene/SelectionSet.h>
#include <core/viewport/ViewportConfiguration.h>
#include <core/rendering/RenderSettings.h>
#include <core/utilities/io/ObjectSaveStream.h>
#include <core/utilities/io/ObjectLoadStream.h>
#include <core/utilities/io/FileManager.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/dataset/importexport/FileImporterEditor.h>
#include "GuiDataSetContainer.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

IMPLEMENT_OVITO_OBJECT(GuiDataSetContainer, DataSetContainer);

/******************************************************************************
* Initializes the dataset manager.
******************************************************************************/
GuiDataSetContainer::GuiDataSetContainer(MainWindow* mainWindow) : DataSetContainer(),
	_mainWindow(mainWindow)
{
	connect(&taskManager(), &TaskManager::localEventLoopEntered, this, &GuiDataSetContainer::localEventLoopEntered);
	connect(&taskManager(), &TaskManager::localEventLoopExited, this, &GuiDataSetContainer::localEventLoopExited);
}

/******************************************************************************
* Save the current dataset.
******************************************************************************/
bool GuiDataSetContainer::fileSave()
{
	if(currentSet() == nullptr)
		return false;

	// Ask the user for a filename if there is no one set.
	if(currentSet()->filePath().isEmpty())
		return fileSaveAs();

	// Save dataset to file.
	try {
		currentSet()->saveToFile(currentSet()->filePath());
		currentSet()->undoStack().setClean();
	}
	catch(const Exception& ex) {
		ex.reportError();
		return false;
	}

	return true;
}

/******************************************************************************
* This is the implementation of the "Save As" action.
* Returns true, if the scene has been saved.
******************************************************************************/
bool GuiDataSetContainer::fileSaveAs(const QString& filename)
{
	if(currentSet() == nullptr)
		return false;

	if(filename.isEmpty()) {

		if(!mainWindow())
			currentSet()->throwException(tr("Cannot save program state. No filename has been specified."));

		QFileDialog dialog(mainWindow(), tr("Save Program State As"));
		dialog.setNameFilter(tr("OVITO State Files (*.ovito);;All Files (*)"));
		dialog.setAcceptMode(QFileDialog::AcceptSave);
		dialog.setFileMode(QFileDialog::AnyFile);
		dialog.setConfirmOverwrite(true);
		dialog.setDefaultSuffix("ovito");

		QSettings settings;
		settings.beginGroup("file/scene");

		if(currentSet()->filePath().isEmpty()) {
			QString defaultPath = settings.value("last_directory").toString();
			if(!defaultPath.isEmpty())
				dialog.setDirectory(defaultPath);
		}
		else
			dialog.selectFile(currentSet()->filePath());

		if(!dialog.exec())
			return false;

		QStringList files = dialog.selectedFiles();
		if(files.isEmpty())
			return false;
		QString newFilename = files.front();

		// Remember directory for the next time...
		settings.setValue("last_directory", dialog.directory().absolutePath());

        currentSet()->setFilePath(newFilename);
	}
	else {
		currentSet()->setFilePath(filename);
	}
	return fileSave();
}

/******************************************************************************
* If the scene has been changed this will ask the user if he wants
* to save the changes.
* Returns false if the operation has been canceled by the user.
******************************************************************************/
bool GuiDataSetContainer::askForSaveChanges()
{
	if(!currentSet() || currentSet()->undoStack().isClean() || currentSet()->filePath().isEmpty() || !mainWindow())
		return true;

	QString message;
	if(currentSet()->filePath().isEmpty() == false) {
		message = tr("The current scene has been modified. Do you want to save the changes?");
		message += QString("\n\nFile: %1").arg(currentSet()->filePath());
	}
	else {
		message = tr("The current scene has not been saved. Do you want to save it?");
	}

	QMessageBox::StandardButton result = QMessageBox::question(mainWindow(), tr("Save changes"),
		message,
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);
	if(result == QMessageBox::Cancel)
		return false; // Operation canceled by user
	else if(result == QMessageBox::No)
		return true; // Continue without saving scene first.
	else {
		// Save scene first.
        return fileSave();
	}
}

/******************************************************************************
* Creates an empty dataset and makes it the current dataset.
******************************************************************************/
bool GuiDataSetContainer::fileNew()
{
	OORef<DataSet> newSet = new DataSet();
	newSet->loadUserDefaults();
	setCurrentSet(newSet);
	return true;
}

/******************************************************************************
* Loads the given state file.
******************************************************************************/
bool GuiDataSetContainer::fileLoad(const QString& filename)
{
	// Load dataset from file.
	OORef<DataSet> dataSet;
	try {
		QFile fileStream(filename);
		if(!fileStream.open(QIODevice::ReadOnly))
			throw Exception(tr("Failed to open state file '%1' for reading.").arg(filename), this);

		QDataStream dataStream(&fileStream);
		ObjectLoadStream stream(dataStream);

		// Issue a warning when the floating-point precision of the input file does not match
		// the precision used in this build.
		if(stream.floatingPointPrecision() > sizeof(FloatType)) {
			if(mainWindow()) {
				QString msg = tr("The state file has been written with a version of this program that uses %1-bit floating-point precision. "
					   "The version of this program that you are currently using only supports %2-bit precision numbers. "
					   "The precision of all numbers stored in the input file will be truncated during loading.").arg(stream.floatingPointPrecision()*8).arg(sizeof(FloatType)*8);
				QMessageBox::warning(mainWindow(), tr("Floating-point precision mismatch"), msg);
			}
		}

		dataSet = stream.loadObject<DataSet>();
		stream.close();
	}
	catch(Exception& ex) {
		// Provide a local context for the error.
		ex.setContext(this);
		throw ex;
	}
	OVITO_CHECK_OBJECT_POINTER(dataSet);
	dataSet->setFilePath(filename);
	setCurrentSet(dataSet);
	return true;
}

/******************************************************************************
* Imports a given file into the scene.
******************************************************************************/
bool GuiDataSetContainer::importFile(const QUrl& url, const OvitoObjectType* importerType)
{
	OVITO_ASSERT(currentSet() != nullptr);

	if(!url.isValid())
		throw Exception(tr("Failed to import file. URL is not valid: %1").arg(url.toString()), currentSet());

	OORef<FileImporter> importer;
	if(!importerType) {

		// Download file so we can determine its format.
		Future<QString> fetchFileFuture = Application::instance()->fileManager()->fetchUrl(*this, url);
		if(!taskManager().waitForTask(fetchFileFuture))
			return false;

		// Detect file format.
		importer = FileImporter::autodetectFileFormat(currentSet(), fetchFileFuture.result(), url.path());
		if(!importer)
			currentSet()->throwException(tr("Could not detect the format of the file to be imported. The format might not be supported."));
	}
	else {
		importer = static_object_cast<FileImporter>(importerType->createInstance(currentSet()));
		if(!importer)
			currentSet()->throwException(tr("Failed to import file. Could not initialize import service."));
	}

	// Load user-defined default settings for the importer.
	importer->loadUserDefaults();

	// Show the optional user interface (which is provided by the corresponding FileImporterEditor class) for the new importer.
	for(const OvitoObjectType* clazz = &importer->getOOType(); clazz != nullptr; clazz = clazz->superClass()) {
		const OvitoObjectType* editorClass = PropertiesEditor::registry().getEditorClass(clazz);
		if(editorClass && editorClass->isDerivedFrom(FileImporterEditor::OOType)) {
			OORef<FileImporterEditor> editor = dynamic_object_cast<FileImporterEditor>(editorClass->createInstance(nullptr));
			if(editor) {
				if(!editor->inspectNewFile(importer, url, mainWindow()))
					return false;
			}
		}
	}

	// Determine how the file's data should be inserted into the current scene.
	FileImporter::ImportMode importMode = FileImporter::ResetScene;

	if(mainWindow()) {
		if(importer->isReplaceExistingPossible(url)) {
			// Ask user if the current import node including any applied modifiers should be kept.
			QMessageBox msgBox(QMessageBox::Question, tr("Import file"),
					tr("When importing the selected file, do you want to keep the existing objects?"),
					QMessageBox::NoButton, mainWindow());

			QPushButton* cancelButton = msgBox.addButton(QMessageBox::Cancel);
			QPushButton* resetSceneButton = msgBox.addButton(tr("No"), QMessageBox::NoRole);
			QPushButton* addToSceneButton = msgBox.addButton(tr("Add to scene"), QMessageBox::YesRole);
			QPushButton* replaceSourceButton = msgBox.addButton(tr("Replace selected"), QMessageBox::AcceptRole);
			msgBox.setDefaultButton(resetSceneButton);
			msgBox.setEscapeButton(cancelButton);
			msgBox.exec();

			if(msgBox.clickedButton() == cancelButton) {
				return false; // Operation canceled by user.
			}
			else if(msgBox.clickedButton() == resetSceneButton) {
				importMode = FileImporter::ResetScene;
				// Ask user if current scene should be saved before it is replaced by the imported data.
				if(!askForSaveChanges())
					return false;
			}
			else if(msgBox.clickedButton() == addToSceneButton) {
				importMode = FileImporter::AddToScene;
			}
			else {
				importMode = FileImporter::ReplaceSelected;
			}
		}
		else if(currentSet()->sceneRoot()->children().empty() == false) {
			// Ask user if the current scene should be completely replaced by the imported data.
			QMessageBox::StandardButton result = QMessageBox::question(mainWindow(), tr("Import file"),
				tr("Do you want to keep the existing objects in the current scene?"),
				QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Cancel);

			if(result == QMessageBox::Cancel) {
				return false; // Operation canceled by user.
			}
			else if(result == QMessageBox::No) {
				importMode = FileImporter::ResetScene;

				// Ask user if current scene should be saved before it is replaced by the imported data.
				if(!askForSaveChanges())
					return false;
			}
			else {
				importMode = FileImporter::AddToScene;
			}
		}
	}

	return importer->importFile(url, importMode, true);
}

/******************************************************************************
* Is called whenever a local event loop is entered to wait for a task to finish.
******************************************************************************/
void GuiDataSetContainer::localEventLoopEntered()
{
	if(!currentSet() || !Application::instance()->guiMode()) return;

	// Suspend viewport updates while waiting.
	currentSet()->viewportConfig()->suspendViewportUpdates();

	// Disable viewport repaints while processing events to
	// avoid recursive calls to repaint().
	if(currentSet()->viewportConfig()->isRendering()) {
		if(!_viewportRepaintsDisabled)
			mainWindow()->viewportsPanel()->setUpdatesEnabled(false);
		_viewportRepaintsDisabled++;
	}
}

/******************************************************************************
* Is called whenever a local event loop was exited after waiting for a task to finish.
******************************************************************************/
void GuiDataSetContainer::localEventLoopExited()
{
	if(!currentSet() || !Application::instance()->guiMode()) return;

	// Handle viewport updates again.
	currentSet()->viewportConfig()->resumeViewportUpdates();

	// Re-enable viewport repaints.
	if(currentSet()->viewportConfig()->isRendering()) {
		if(--_viewportRepaintsDisabled == 0)
			mainWindow()->viewportsPanel()->setUpdatesEnabled(true);
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
