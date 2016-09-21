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
#include <gui/mainwin/MainWindow.h>
#include <gui/dataset/GuiDataSetContainer.h>
#include <gui/rendering/ViewportSceneRenderer.h>
#include <gui/utilities/io/GuiFileManager.h>
#include <core/utilities/io/FileManager.h>
#include "GuiApplication.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) 

/******************************************************************************
* Defines the program's command line parameters.
******************************************************************************/
void GuiApplication::registerCommandLineParameters(QCommandLineParser& parser)
{
	Application::registerCommandLineParameters(parser);

	parser.addOption(QCommandLineOption(QStringList{{"nogui"}}, tr("Run in console mode without showing the graphical user interface.")));
	parser.addOption(QCommandLineOption(QStringList{{"glversion"}}, tr("Selects a specific version of the OpenGL standard."), tr("VERSION")));
	parser.addOption(QCommandLineOption(QStringList{{"glcompatprofile"}}, tr("Request the OpenGL compatibility profile instead of the core profile.")));
}

/******************************************************************************
* Interprets the command line parameters provided to the application.
******************************************************************************/
bool GuiApplication::processCommandLineParameters()
{
	if(!Application::processCommandLineParameters())
		return false;

	// Check if program was started in console mode.
	if(!_cmdLineParser.isSet("nogui")) {
		// Enable GUI mode by default.
		_consoleMode = false;
		_headlessMode = false;
	}
	else {
		// Activate console mode.
		_consoleMode = true;
#if defined(Q_OS_LINUX)
		// On Unix/Linux, console mode means headless mode if no X server is available.
		if(!qEnvironmentVariableIsEmpty("DISPLAY")) {
			_headlessMode = false;
		}
#elif defined(Q_OS_OSX)
		// Don't let Qt move the app to the foreground when running in console mode.
		::setenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1", 1);
		_headlessMode = false;
#elif defined(Q_OS_WIN)
		// On Windows, there is always an OpenGL implementation available for background rendering.
		_headlessMode = false;
#endif
	}

	return true;
}

/******************************************************************************
* Create the global instance of the right QCoreApplication derived class.
******************************************************************************/
void GuiApplication::createQtApplication(int& argc, char** argv)
{
	if(headlessMode()) {
		Application::createQtApplication(argc, argv);
	}
	else {
		_app.reset(new QApplication(argc, argv));
	}

	// Install GUI exception handler.
	if(guiMode())
		Exception::setExceptionHandler(guiExceptionHandler);

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
	// Set the global default OpenGL surface format.
	// This will let Qt use core profile contexts.
	QSurfaceFormat::setDefaultFormat(ViewportSceneRenderer::getDefaultSurfaceFormat());
#endif

}

/******************************************************************************
* Creates the global FileManager class instance.
******************************************************************************/
FileManager* GuiApplication::createFileManager()
{
	return new GuiFileManager();
}

/******************************************************************************
* Prepares application to start running.
******************************************************************************/
bool GuiApplication::startupApplication()
{
	GuiDataSetContainer* container;
	if(guiMode()) {
		// Set up graphical user interface.

		// Set the application icon.
		QIcon mainWindowIcon;
		mainWindowIcon.addFile(":/gui/mainwin/window_icon_256.png");
		mainWindowIcon.addFile(":/gui/mainwin/window_icon_128.png");
		mainWindowIcon.addFile(":/gui/mainwin/window_icon_48.png");
		mainWindowIcon.addFile(":/gui/mainwin/window_icon_32.png");
		mainWindowIcon.addFile(":/gui/mainwin/window_icon_16.png");
		QApplication::setWindowIcon(mainWindowIcon);

		// Create the main window.
		MainWindow* mainWin = new MainWindow();
		container = &mainWin->datasetContainer();
		_datasetContainer = container;

		// Make the application shutdown as soon as the last main window has been closed.
		QGuiApplication::setQuitOnLastWindowClosed(true);

		// Show the main window.
#ifndef OVITO_DEBUG
		mainWin->showMaximized();
#else
		mainWin->show();
#endif
		mainWin->restoreLayout();
	}
	else {
		// Create a dataset container.
		container = new GuiDataSetContainer();
		container->setParent(this);
		_datasetContainer = container;
	}

	// Load state file specified on the command line.
	if(cmdLineParser().positionalArguments().empty() == false) {
		QString startupFilename = cmdLineParser().positionalArguments().front();
		if(startupFilename.endsWith(".ovito", Qt::CaseInsensitive))
			container->fileLoad(startupFilename);
	}

	// Create an empty dataset if nothing has been loaded.
	if(container->currentSet() == nullptr)
		container->fileNew();

	// Import data file specified on the command line.
	if(cmdLineParser().positionalArguments().empty() == false) {
		QString importFilename = cmdLineParser().positionalArguments().front();
		if(!importFilename.endsWith(".ovito", Qt::CaseInsensitive)) {
			QUrl importURL = FileManager::instance().urlFromUserInput(importFilename);
			container->importFile(importURL);
			container->currentSet()->undoStack().setClean();
		}
	}

	return true;
}

/******************************************************************************
* Handler function for exceptions used in GUI mode.
******************************************************************************/
void GuiApplication::guiExceptionHandler(const Exception& exception)
{
	// Always display errors in the terminal window.
	exception.logError();

	// Prepare a message box dialog.
	QMessageBox msgbox;
	msgbox.setWindowTitle(tr("Error - %1").arg(QCoreApplication::applicationName()));
	msgbox.setStandardButtons(QMessageBox::Ok);
	msgbox.setText(exception.message());
	msgbox.setIcon(QMessageBox::Critical);

	// If the exception has been thrown within the context of a DataSet or a DataSetContainer,
	// show the message box under the corresponding main window.
	if(DataSet* dataset = qobject_cast<DataSet*>(exception.context())) {
		if(MainWindow* window = MainWindow::fromDataset(dataset)) {
			msgbox.setParent(window);
			msgbox.setWindowModality(Qt::WindowModal);
		}
	}
	if(GuiDataSetContainer* datasetContainer = qobject_cast<GuiDataSetContainer*>(exception.context())) {
		if(MainWindow* window = datasetContainer->mainWindow()) {
			msgbox.setParent(window);
			msgbox.setWindowModality(Qt::WindowModal);
		}
	}

	// If the exception is associated with additional message strings,
	// show them in the Details section of the message box dialog.
	if(exception.messages().size() > 1) {
		QString detailText;
		for(int i = 1; i < exception.messages().size(); i++)
			detailText += exception.messages()[i] + "\n";
		msgbox.setDetailedText(detailText);
	}

	// Show message box.
	msgbox.exec();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
