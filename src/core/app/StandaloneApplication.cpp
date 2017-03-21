///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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

#include <core/Core.h>
#include <core/dataset/UndoStack.h>
#include <core/dataset/DataSetContainer.h>
#include <core/plugins/PluginManager.h>
#include <core/plugins/autostart/AutoStartObject.h>
#include "StandaloneApplication.h"

namespace Ovito {

/******************************************************************************
* This is called on program startup.
******************************************************************************/
bool StandaloneApplication::initialize(int& argc, char** argv)
{
	if(!Application::initialize())
		return false;

	// Set the application name.
	QCoreApplication::setApplicationName(tr("Ovito"));
	QCoreApplication::setOrganizationName(tr("Ovito"));
	QCoreApplication::setOrganizationDomain("ovito.org");
	QCoreApplication::setApplicationVersion(QStringLiteral(OVITO_VERSION_STRING));

	// Register command line arguments.
	_cmdLineParser.setApplicationDescription(tr("OVITO - Open Visualization Tool"));
	registerCommandLineParameters(_cmdLineParser);

	// Parse command line arguments.
	// Ignore unknown command line options for now.
	QStringList arguments;
	for(int i = 0; i < argc; i++)
		arguments << QString::fromLocal8Bit(argv[i]);
	
	// Because they may collide with our own options, we should ignore script arguments though. 
	QStringList filteredArguments;
	for(int i = 0; i < arguments.size(); i++) {
		if(arguments[i] == QStringLiteral("--scriptarg")) {
			i += 1;
			continue;
		}
		filteredArguments.push_back(arguments[i]);
	}
	_cmdLineParser.parse(filteredArguments);

	// Output program version if requested.
	if(cmdLineParser().isSet("version")) {
		std::cout << qPrintable(QCoreApplication::applicationName()) << " " << qPrintable(QCoreApplication::applicationVersion()) << std::endl;
		_consoleMode = true;
		return true;
	}

	try {
		// Interpret the command line arguments.
		if(!processCommandLineParameters()) {
			return true;
		}
	}
	catch(const Exception& ex) {
		ex.reportError(true);
		return false;
	}
	
	// Always use desktop OpenGL implementation (avoid ANGLE on Windows).
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

	// Create Qt application object.
	createQtApplication(argc, argv);

	// Reactivate default "C" locale, which, in the meantime, might have been changed by QCoreApplication.
	std::setlocale(LC_NUMERIC, "C");

	try {
		// Load plugins.
		PluginManager::initialize();
		PluginManager::instance().loadAllPlugins();

		// Load auto-start objects and let them register their custom command line options.
		for(const OvitoObjectType* clazz : PluginManager::instance().listClasses(AutoStartObject::OOType)) {
			OORef<AutoStartObject> obj = static_object_cast<AutoStartObject>(clazz->createInstance(nullptr));
			_autostartObjects.push_back(obj);
			obj->registerCommandLineOptions(_cmdLineParser);
		}

		// Parse the command line parameters again after the plugins have registered their options.
		if(!_cmdLineParser.parse(arguments)) {
	        std::cerr << "Error: " << qPrintable(_cmdLineParser.errorText()) << std::endl;
			_consoleMode = true;
			shutdown();
			return false;
		}

		// Help command line option implicitly activates console mode.
		if(_cmdLineParser.isSet("help"))
			_consoleMode = true;

		// Handle --help command line option. Print list of command line options and quit.
		if(_cmdLineParser.isSet("help")) {
			std::cout << qPrintable(_cmdLineParser.helpText()) << std::endl;
			return true;
		}

		// Prepares application to start running.
		if(!startupApplication())
			return true;

		// Invoke auto-start objects.
		for(const auto& obj : autostartObjects())
			obj->applicationStarted();
	}
	catch(const Exception& ex) {
		ex.reportError(true);
		shutdown();
		return false;
	}
	return true;
}

/******************************************************************************
* Defines the program's command line parameters.
******************************************************************************/
void StandaloneApplication::registerCommandLineParameters(QCommandLineParser& parser)
{
	parser.addOption(QCommandLineOption(QStringList{{"h", "help"}}, tr("Shows this list of program options and exits.")));
	parser.addOption(QCommandLineOption(QStringList{{"v", "version"}}, tr("Prints the program version and exits.")));
	parser.addOption(QCommandLineOption(QStringList{{"nthreads"}}, tr("Sets the number of parallel threads to use for computations."), QStringLiteral("N")));
}

/******************************************************************************
* Interprets the command line parameters provided to the application.
******************************************************************************/
bool StandaloneApplication::processCommandLineParameters()
{
	// Output program version if requested.
	if(cmdLineParser().isSet("version")) {
		std::cout << qPrintable(QCoreApplication::applicationName()) << " " << qPrintable(QCoreApplication::applicationVersion()) << std::endl;
		return false;
	}

	// User can overwrite the number of parallel threads to use.
	if(cmdLineParser().isSet("nthreads")) {
		bool ok;
		int nthreads = cmdLineParser().value("nthreads").toInt(&ok);
		if(!ok || nthreads <= 0)
			throw Exception(tr("Invalid thread count specified on command line."));
		setIdealThreadCount(nthreads);
	}

	return true;
}

/******************************************************************************
* Starts the main event loop.
******************************************************************************/
int StandaloneApplication::runApplication()
{
	if(guiMode()) {
		// Enter the main event loop.
		return QCoreApplication::exec();
	}
	else {
		// Deliver all events that have been posted during the initialization.
		QCoreApplication::processEvents();
		// Wait for all background tasks to finish before quitting.
		if(_datasetContainer)
			_datasetContainer->taskManager().waitForAll();
		return _exitCode;
	}
}

/******************************************************************************
* This is called on program shutdown.
******************************************************************************/
void StandaloneApplication::shutdown()
{
	// Release dataset and all contained objects.
	if(datasetContainer())
		datasetContainer()->setCurrentSet(nullptr);

	// Destroy auto-start objects.
	_autostartObjects.clear();

	// Unload plugins.
	PluginManager::shutdown();

	// Destroy Qt application object.
	delete QCoreApplication::instance();
}

}	// End of namespace
