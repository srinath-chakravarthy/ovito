///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/scene/ObjectNode.h>
#include <core/scene/SelectionSet.h>
#include "DislocationNetworkObject.h"
#if 0
#include "DislocationInspector.h"
#endif

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, DislocationNetworkObject, DataObject);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, DislocationNetworkObjectEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(DislocationNetworkObject, DislocationNetworkObjectEditor);

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationNetworkObject::DislocationNetworkObject(DataSet* dataset, DislocationNetwork* network)
	: DataObjectWithSharedStorage(dataset, network ? network : new DislocationNetwork(new ClusterGraph()))
{
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DislocationNetworkObjectEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Dislocations"), rolloutParams);
	QVBoxLayout* rolloutLayout = new QVBoxLayout(rollout);

#if 0
	QPushButton* openInspectorButton = new QPushButton(tr("Open Dislocation Inspector"), rollout);
	rolloutLayout->addWidget(openInspectorButton);
	connect(openInspectorButton, &QPushButton::clicked, this, &DislocationNetworkObjectEditor::onOpenInspector);
#endif
}

/******************************************************************************
* Is called when the user presses the "Open Inspector" button.
******************************************************************************/
void DislocationNetworkObjectEditor::onOpenInspector()
{
	DislocationNetworkObject* dislocationsObj = static_object_cast<DislocationNetworkObject>(editObject());
	if(!dislocationsObj) return;

#if 0
	QMainWindow* inspectorWindow = new QMainWindow(container()->window(), (Qt::WindowFlags)(Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint));
	inspectorWindow->setWindowTitle(tr("Dislocation Inspector"));
	PropertiesPanel* propertiesPanel = new PropertiesPanel(inspectorWindow);
	propertiesPanel->hide();

	QWidget* mainPanel = new QWidget(inspectorWindow);
	QVBoxLayout* mainPanelLayout = new QVBoxLayout(mainPanel);
	mainPanelLayout->setStretch(0,1);
	mainPanelLayout->setContentsMargins(0,0,0,0);
	inspectorWindow->setCentralWidget(mainPanel);

	ObjectNode* node = dynamic_object_cast<ObjectNode>(dataset()->selection()->front());
	DislocationInspector* inspector = new DislocationInspector(node);
	connect(inspector, &QObject::destroyed, inspectorWindow, &QMainWindow::close);
	inspector->setParent(propertiesPanel);
	inspector->initialize(propertiesPanel, mainWindow(), RolloutInsertionParameters().insertInto(mainPanel));
	inspector->setEditObject(dislocationsObj);
	inspectorWindow->setAttribute(Qt::WA_DeleteOnClose);
	inspectorWindow->resize(1000, 350);
	inspectorWindow->show();
#endif
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
