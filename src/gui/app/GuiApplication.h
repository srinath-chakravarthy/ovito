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

#pragma once


#include <gui/GUI.h>
#include <core/utilities/Exception.h>
#include <core/app/StandaloneApplication.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui)

/**
 * \brief The main application with a graphical user interface.
 */
class OVITO_GUI_EXPORT GuiApplication : public StandaloneApplication
{
public:

	/// Create the global instance of the right QCoreApplication derived class.
	virtual void createQtApplication(int& argc, char** argv) override;

	/// Handler function for exceptions.
	virtual void reportError(const Exception& exception, bool blocking) override;

protected:

	/// Defines the program's command line parameters.
	virtual void registerCommandLineParameters(QCommandLineParser& parser) override;

	/// Interprets the command line parameters provided to the application.
	virtual bool processCommandLineParameters() override;

	/// Prepares application to start running.
	virtual bool startupApplication() override;

	/// Creates the global FileManager class instance.
	virtual FileManager* createFileManager() override;

private Q_SLOTS:

	/// Displays an error message box. This slot is called by reportError().
	void showErrorMessages();

private:

	/// Initializes the graphical user interface of the application.
	void initializeGUI();

	/// List of errors to be displayed by showErrorMessages().
	std::deque<Exception> _errorList;

	Q_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


