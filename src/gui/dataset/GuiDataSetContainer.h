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
#include <core/dataset/DataSetContainer.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem)

/**
 * \brief Manages the DataSet being edited.
 */
class OVITO_GUI_EXPORT GuiDataSetContainer : public DataSetContainer
{
public:

	/// \brief Constructor.
	GuiDataSetContainer(MainWindow* mainWindow = nullptr);

	/// \brief Returns the window this dataset container is linked to (may be NULL).
	MainWindow* mainWindow() const { return _mainWindow; }

	/// \brief Imports a given file into the current dataset.
	/// \param url The location of the file to import.
	/// \param importerType The FileImporter type to use. If NULL, the file format will be automatically detected.
	/// \return true if the file was successfully imported; false if operation has been canceled by the user.
	/// \throw Exception on error.
	bool importFile(const QUrl& url, const OvitoObjectType* importerType = nullptr);

	/// \brief Creates an empty dataset and makes it the current dataset.
	/// \return \c true if the operation was completed; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	bool fileNew();

	/// \brief Loads the given file and makes it the current dataset.
	/// \return \c true if the file has been successfully loaded; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	bool fileLoad(const QString& filename);

	/// \brief Save the current dataset.
	/// \return \c true, if the dataset has been saved; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	/// 
	/// If the current dataset has not been assigned a file path, then this method
	/// displays a file selector dialog by calling fileSaveAs() to let the user select a file path.
	bool fileSave();

	/// \brief Lets the user select a new destination filename for the current dataset. Then saves the dataset by calling fileSave().
	/// \param filename If \a filename is an empty string that this method asks the user for a filename. Otherwise
	///                 the provided filename is used.
	/// \return \c true, if the dataset has been saved; \c false if the operation has been canceled by the user.
	/// \throw Exception on error.
	bool fileSaveAs(const QString& filename = QString());

	/// \brief Asks the user if changes made to the dataset should be saved.
	/// \return \c false if the operation has been canceled by the user; \c true on success.
	/// \throw Exception on error.
	/// 
	/// If the current dataset has been changed, this method asks the user if changes should be saved.
	/// If yes, then the dataset is saved by calling fileSave().
	bool askForSaveChanges();

private Q_SLOTS:

	/// Is called whenever a local event loop is entered to wait for a task to finish.
	void localEventLoopEntered();

	/// Is called whenever a local event loop was exited after waiting for a task to finish.
	void localEventLoopExited();

private:

	/// The window this dataset container is linked to (may be NULL).
	MainWindow* _mainWindow;

	/// Counts how many times viewport repaints have been disabled so that we can
	/// re-enable them again the same number of times.
	int _viewportRepaintsDisabled = 0;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace


