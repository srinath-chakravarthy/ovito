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
#include <core/dataset/importexport/FileExporter.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief This dialog box lets the user adjust the settings of a FileExporter.
 */
class FileExporterSettingsDialog : public QDialog
{
	Q_OBJECT
	
public:

	/// Constructor.
	FileExporterSettingsDialog(MainWindow* parent, FileExporter* exporter);

protected Q_SLOTS:

	/// This is called when the user has pressed the OK button.
	virtual void onOk();

protected:

	QVBoxLayout* _mainLayout;
	OORef<FileExporter> _exporter;
	SpinnerWidget* _startTimeSpinner;
	SpinnerWidget* _endTimeSpinner;
	SpinnerWidget* _nthFrameSpinner;
	QLineEdit* _wildcardTextbox;
	QButtonGroup* _fileGroupButtonGroup;
	QButtonGroup* _rangeButtonGroup;
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


