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
#include "LoadImageFileDialog.h"

namespace Ovito { namespace Gui { namespace Dialogs {

/******************************************************************************
* Constructs the dialog window.
******************************************************************************/
LoadImageFileDialog::LoadImageFileDialog(QWidget* parent, const QString& caption, const ImageInfo& imageInfo) :
	HistoryFileDialog("load_image", parent, caption), _imageInfo(imageInfo)
{
	connect(this, &QFileDialog::fileSelected, this, &LoadImageFileDialog::onFileSelected);
	setAcceptMode(QFileDialog::AcceptOpen);
	setNameFilter(tr("Image files (*.png *.jpg *.jpeg)"));
	if(_imageInfo.filename().isEmpty() == false)
		selectFile(_imageInfo.filename());
}

/******************************************************************************
* This is called when the user has pressed the OK button of the dialog.
******************************************************************************/
void LoadImageFileDialog::onFileSelected(const QString& file)
{
	_imageInfo.setFilename(file);
}

}}}	// End of namespace