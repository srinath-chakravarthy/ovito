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

#pragma once


#include <core/Core.h>
#include "Application.h"

namespace Ovito {

/**
 * \brief The application object used when running as a standalone application.
 */
class OVITO_CORE_EXPORT StandaloneApplication : public Application
{
	Q_OBJECT

public:

	/// \brief Returns the one and only instance of this class.
	inline static StandaloneApplication* instance() {
		return qobject_cast<StandaloneApplication*>(Application::instance());
	}

	/// \brief Initializes the application.
	/// \param argc The number of command line arguments.
	/// \param argv The command line arguments.
	/// \return \c true if the application was initialized successfully;
	///         \c false if an error occurred and the program should be terminated.
	///
	/// This is called on program startup.
	bool initialize(int& argc, char** argv);

	/// \brief Enters the main event loop.
	/// \return The program exit code.
	///
	/// If the application has been started in console mode then this method does nothing.
	int runApplication();

	/// \brief Releases everything.
	///
	/// This is called before the application exits.
	void shutdown();

	/// \brief Returns the command line options passed to the program.
	const QCommandLineParser& cmdLineParser() const { return _cmdLineParser; }

protected:

	/// Defines the program's command line parameters.
	virtual void registerCommandLineParameters(QCommandLineParser& parser);

	/// Interprets the command line parameters provided to the application.
	virtual bool processCommandLineParameters();

	/// Prepares application to start running.
	virtual bool startupApplication() = 0;

protected:

	/// The parser for the command line options passed to the program.
	QCommandLineParser _cmdLineParser;
};

}	// End of namespace


