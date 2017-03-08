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
bool Application::initialize()
{
	// Install custom Qt error message handler to catch fatal errors in debug mode.
	defaultQtMessageHandler = qInstallMessageHandler(qtMessageOutput);

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

	// Create global FileManager object.
	_fileManager.reset(createFileManager());

	return true;
}

void test_func() {}

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
		if(!QDir(QString::fromStdString(fontPath)).exists())
			fontPath = "/usr/share/fonts";

		// On Linux, use the 'minimal' QPA platform plugin instead of the standard XCB plugin when no X server is available.
		// Still create a Qt GUI application object, because otherwise we cannot use (offscreen) font rendering functions.
		qputenv("QT_QPA_PLATFORM", "minimal");
		// Enable rudimentary font rendering support, which is implemented by the 'minimal' platform plugin:
		qputenv("QT_DEBUG_BACKINGSTORE", "1");
		qputenv("QT_QPA_FONTDIR", fontPath.c_str());

		new QGuiApplication(argc, argv);
#elif defined(Q_OS_MAC)
		new QGuiApplication(argc, argv);
#else
		new QCoreApplication(argc, argv);
#endif
	}
	else {
		new QGuiApplication(argc, argv);
	}
}

/******************************************************************************
* Returns a pointer to the main dataset container.
******************************************************************************/
DataSetContainer* Application::datasetContainer() const
{
	return _datasetContainer;
}

/******************************************************************************
* Creates the global FileManager class instance.
******************************************************************************/
FileManager* Application::createFileManager()
{
	return new FileManager();
}

/******************************************************************************
* Executes the functions registered with the runOnceLater() function.
* This method is called after the events in the event queue have been processed.
******************************************************************************/
void Application::processRunOnceList()
{
	QMap<QPointer<QObject>,std::function<void()>> listCopy;
	_runOnceList.swap(listCopy);
	for(auto entry = listCopy.cbegin(); entry != listCopy.cend(); ++entry) {
		if(entry.key())
			entry.value()();
	}
}

/******************************************************************************
* Handler function for exceptions.
******************************************************************************/
void Application::reportError(const Exception& exception, bool blocking)
{
	for(int i = exception.messages().size() - 1; i >= 0; i--) {
		std::cerr << "ERROR: " << qPrintable(exception.messages()[i]) << std::endl;
	}
	std::cerr << std::flush;
}

}	// End of namespace
