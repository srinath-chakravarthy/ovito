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

#include <gui/GUI.h>
#include <core/dataset/DataSet.h>
#include <core/dataset/DataSetContainer.h>
#include <core/viewport/Viewport.h>
#include <gui/properties/PropertiesPanel.h>
#include "VRWindow.h"

namespace VRPlugin {

/******************************************************************************
* Constructor.
******************************************************************************/
VRWindow::VRWindow(QWidget* parentWidget, GuiDataSetContainer* datasetContainer) : QMainWindow(parentWidget)
{
	// Use a default window size.
	resize(800, 600);

    // Set title.
	setWindowTitle(tr("Ovito - Virtual Reality Module"));

    // Create the widget for rendering.
	_glWidget = new VRRenderingWidget(this, datasetContainer->currentSet());
	setCentralWidget(_glWidget);

    // Create settings panel.
    PropertiesPanel* propPanel = new PropertiesPanel(this, datasetContainer->mainWindow());
    propPanel->setEditObject(_glWidget->settings());
	QDockWidget* dockWidget = new QDockWidget(tr("Settings"), this);
	dockWidget->setObjectName("SettingsPanel");
	dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	dockWidget->setFeatures(QDockWidget::DockWidgetClosable);
	dockWidget->setWidget(propPanel);
	dockWidget->setTitleBarWidget(new QWidget());
	addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    // Close the VR window immediately when OVITO is closed or if another DataSet is loaded.
    connect(datasetContainer, &DataSetContainer::dataSetChanged, this, [this] {
        delete this;
    });

    // Delete window when it is being closed by the user.
    setAttribute(Qt::WA_DeleteOnClose);
}

};

