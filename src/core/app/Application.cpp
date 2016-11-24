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

#include <core/Core.h>
#include <core/dataset/UndoStack.h>
#include <core/dataset/DataSetContainer.h>
#include <core/animation/controller/Controller.h>
#include <core/plugins/PluginManager.h>
#include <core/plugins/autostart/AutoStartObject.h>
#include <core/utilities/io/FileManager.h>
#include "Application.h"

namespace Ovito {

/// The one and only instance of this class.
Application* Application::_instance = nullptr;

/// Stores a pointer to the original Qt message handler function, which has been replaced with our own handler.
QtMessageHandler Application::defaultQtMessageHandler = nullptr;

/******************************************************************************
* Handler method for Qt error messages.
* This can be used to set a debugger breakpoint for the OVITO_ASSERT macros.
******************************************************************************/
void Application::qtMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	// Forward message to default handler.
	if(defaultQtMessageHandler) defaultQtMessageHandler(type, context, msg);
	else std::cerr << qPrintable(msg) << std::endl;
}

/******************************************************************************
* Constructor.
******************************************************************************/
Application::Application() : _exitCode(0), _consoleMode(true), _headlessMode(true)
{
	// Set global application pointer.
	OVITO_ASSERT(_instance == nullptr);	// Only allowed to create one Application class instance.
	_instance = this;

	// Use all procesor cores by default.
	_idealThreadCount = std::max(1, QThread::idealThreadCount());
}

/******************************************************************************
* Destructor.
******************************************************************************/
Application::~Application()
{
	_instance = nullptr;
}

/******************************************************************************
* Returns the major version number of the application.
******************************************************************************/
int Application::applicationVersionMajor()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_MAJOR;
}

/******************************************************************************
* Returns the minor version number of the application.
******************************************************************************/
int Application::applicationVersionMinor()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_MINOR;
}

/******************************************************************************
* Returns the revision version number of the application.
******************************************************************************/
int Application::applicationVersionRevision()
{
	// This compile-time constant is defined by the CMake build script.
	return OVITO_VERSION_REVISION;
}

/******************************************************************************
* This is called on program startup.
******************************************************************************/
bool Application::initialize(int& argc, char** argv)
{
	// Install custom Qt error message handler to catch fatal errors in debug mode.
	defaultQtMessageHandler = qInstallMessageHandler(qtMessageOutput);

	// Install default exception handler.
	Exception::setExceptionHandler(consoleExceptionHandler);

#ifdef Q_OS_LINUX
	// Migrate settings file from old "Alexander Stukowski" directory to new default location.
	// This is for backward compatibility with OVITO version 2.4.4 and earlier.
	{
		QString oldConfigFile = QDir::homePath() + QStringLiteral("/.config/Alexander Stukowski/Ovito.conf");
		QString newConfigFile = QDir::homePath() + QStringLiteral("/.config/Ovito/Ovito.conf");
		if(QFile::exists(oldConfigFile) && !QFile::exists(newConfigFile)) {
			QDir configPath(newConfigFile + QStringLiteral("/.."));
			if(!configPath.mkpath("."))
				qDebug() << "Warning: Migrating the configuration file from" << oldConfigFile << "to" << newConfigFile << "failed. The destination directory could not be created.";
			if(!QFile::copy(oldConfigFile, newConfigFile))
				qDebug() << "Warning: Migrating the configuration file from" << oldConfigFile << "to" << newConfigFile << "failed. The copy operation failed.";
		}
	}
#endif

	// Set the application name.
	QCoreApplication::setApplicationName(tr("Ovito"));
	QCoreApplication::setOrganizationName(tr("Ovito"));
	QCoreApplication::setOrganizationDomain("ovito.org");
	QCoreApplication::setApplicationVersion(QStringLiteral(OVITO_VERSION_STRING));

	// Activate default "C" locale, which will be used to parse numbers in strings.
	std::setlocale(LC_NUMERIC, "C");

	// Suppress console messages "qt.network.ssl: QSslSocket: cannot resolve ..."
	qputenv("QT_LOGGING_RULES", "qt.network.ssl.warning=false");

	// Register our floating-point data type with the Qt type system.
	qRegisterMetaType<FloatType>("FloatType");

	// Register Qt stream operators for basic types.
	qRegisterMetaTypeStreamOperators<Vector2>("Ovito::Vector2");
	qRegisterMetaTypeStreamOperators<Vector3>("Ovito::Vector3");
	qRegisterMetaTypeStreamOperators<Vector4>("Ovito::Vector4");
	qRegisterMetaTypeStreamOperators<Point2>("Ovito::Point2");
	qRegisterMetaTypeStreamOperators<Point3>("Ovito::Point3");
	qRegisterMetaTypeStreamOperators<AffineTransformation>("Ovito::AffineTransformation");
	qRegisterMetaTypeStreamOperators<Matrix3>("Ovito::Matrix3");
	qRegisterMetaTypeStreamOperators<Matrix4>("Ovito::Matrix4");
	qRegisterMetaTypeStreamOperators<Box2>("Ovito::Box2");
	qRegisterMetaTypeStreamOperators<Box3>("Ovito::Box3");
	qRegisterMetaTypeStreamOperators<Rotation>("Ovito::Rotation");
	qRegisterMetaTypeStreamOperators<Scaling>("Ovito::Scaling");
	qRegisterMetaTypeStreamOperators<Quaternion>("Ovito::Quaternion");
	qRegisterMetaTypeStreamOperators<Color>("Ovito::Color");
	qRegisterMetaTypeStreamOperators<ColorA>("Ovito::ColorA");

	// Register Qt conversion operators for custom types.
	QMetaType::registerConverter<QColor, Color>();
	QMetaType::registerConverter<Color, QColor>();
	QMetaType::registerConverter<QColor, ColorA>();
	QMetaType::registerConverter<ColorA, QColor>();

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
		ex.showError();
		return false;
	}
	
	// Always use desktop OpenGL implementation (avoid ANGLE on Windows).
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);

	// Create Qt application object.
	createQtApplication(argc, argv);

	// Reactivate default "C" locale, which, in the meantime, might have been changed by QCoreApplication.
	std::setlocale(LC_NUMERIC, "C");

	try {
		// Initialize global objects in the right order.
		PluginManager::initialize();
		ControllerManager::initialize();
		FileManager::initialize(createFileManager());

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
		ex.showError();
		shutdown();
		return false;
	}
	return true;
}

/******************************************************************************
* Defines the program's command line parameters.
******************************************************************************/
void Application::registerCommandLineParameters(QCommandLineParser& parser)
{
	parser.addOption(QCommandLineOption(QStringList{{"h", "help"}}, tr("Shows this list of program options and exits.")));
	parser.addOption(QCommandLineOption(QStringList{{"v", "version"}}, tr("Prints the program version and exits.")));
	parser.addOption(QCommandLineOption(QStringList{{"nthreads"}}, tr("Sets the number of parallel threads to use for computations."), QStringLiteral("N")));
}

/******************************************************************************
* Interprets the command line parameters provided to the application.
******************************************************************************/
bool Application::processCommandLineParameters()
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
* Create the global instance of the right QCoreApplication derived class.
******************************************************************************/
void Application::createQtApplication(int& argc, char** argv)
{
	if(headlessMode()) {
#if defined(Q_OS_LINUX)
		// Determine font directory path.
		std::string applicationPath = argv[0];
		auto sepIndex = applicationPath.rfind('/');
		if(sepIndex != std::string::npos)
			applicationPath.resize(sepIndex + 1);
		std::string fontPath = applicationPath + "../share/ovito/fonts";

		// On Linux, use the 'minimal' QPA platform plugin instead of the standard XCB plugin when no X server is available.
		// Still create a Qt GUI application object, because otherwise we cannot use (offscreen) font rendering functions.
		qputenv("QT_QPA_PLATFORM", "minimal");
		// Enable rudimentary font rendering support, which is implemented by the 'minimal' platform plugin:
		qputenv("QT_DEBUG_BACKINGSTORE", "1");
		qputenv("QT_QPA_FONTDIR", fontPath.c_str());

		_app.reset(new QGuiApplication(argc, argv));
#else
		_app.reset(new QCoreApplication(argc, argv));
#endif
	}
}

/******************************************************************************
* Creates the global FileManager class instance.
******************************************************************************/
FileManager* Application::createFileManager()
{
	return new FileManager();
}

/******************************************************************************
* Starts the main event loop.
******************************************************************************/
int Application::runApplication()
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
void Application::shutdown()
{
	// Destroy auto-start objects.
	_autostartObjects.clear();

	// Shutdown global objects in reverse order they were initialized.
	FileManager::shutdown();
	ControllerManager::shutdown();
	PluginManager::shutdown();

	// Destroy Qt application object.
	_app.reset();
}

/******************************************************************************
* Returns a pointer to the main dataset container.
******************************************************************************/
DataSetContainer* Application::datasetContainer() const
{
	OVITO_ASSERT_MSG(!_datasetContainer.isNull(), "Application::datasetContainer()", "There is no global dataset container.");
	return _datasetContainer;
}

/******************************************************************************
* Executes the functions registered with the runOnceLater() function.
* This method is called after the events in the event queue have been processed.
******************************************************************************/
void Application::processRunOnceList()
{
	auto copy = _runOnceList;
	_runOnceList.clear();
	for(auto entry = copy.cbegin(); entry != copy.cend(); ++entry) {
		if(entry.key())
			entry.value()();
	}
}

/******************************************************************************
* Handler function for exceptions used in console mode.
******************************************************************************/
void Application::consoleExceptionHandler(const Exception& exception)
{
	for(int i = exception.messages().size() - 1; i >= 0; i--) {
		std::cerr << "ERROR: " << qPrintable(exception.messages()[i]) << std::endl;
	}
	std::cerr << std::flush;
}

}	// End of namespace
