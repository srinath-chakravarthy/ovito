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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/TrajectoryGeneratorObject.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/BooleanRadioButtonParameterUI.h>
#include <gui/widgets/general/ElidedTextLabel.h>
#include <gui/utilities/concurrent/ProgressDialog.h>
#include <core/scene/ObjectNode.h>
#include "TrajectoryGeneratorObjectEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(TrajectoryGeneratorObjectEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(TrajectoryGeneratorObject, TrajectoryGeneratorObjectEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void TrajectoryGeneratorObjectEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Generate trajectory"), rolloutParams, "howto.visualize_particle_trajectories.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	// Particle set
	{
		QGroupBox* groupBox = new QGroupBox(tr("Input particles"));
		layout->addWidget(groupBox);

		QGridLayout* layout2 = new QGridLayout(groupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(4);
		layout2->setColumnStretch(1, 1);
		layout2->setColumnMinimumWidth(0, 15);

		layout2->addWidget(new QLabel(tr("Source:")), 0, 0, 1, 2);
		QLabel* dataSourceLabel = new ElidedTextLabel();
		layout2->addWidget(dataSourceLabel, 1, 1);

	    connect(this, &PropertiesEditor::contentsChanged, [dataSourceLabel](RefTarget* editObject) {
	    	if(TrajectoryGeneratorObject* trajObj = static_object_cast<TrajectoryGeneratorObject>(editObject)) {
	    		if(trajObj->source()) {
	    			dataSourceLabel->setText(trajObj->source()->objectTitle());
	    			return;
	    		}
	    	}
			dataSourceLabel->setText(QString());
	    });

		layout2->addWidget(new QLabel(tr("Generate trajectories for:")), 2, 0, 1, 2);

		BooleanRadioButtonParameterUI* onlySelectedParticlesUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(TrajectoryGeneratorObject::onlySelectedParticles));

		QRadioButton* allParticlesButton = onlySelectedParticlesUI->buttonFalse();
		allParticlesButton->setText(tr("All particles"));
		layout2->addWidget(allParticlesButton, 3, 1);

		QRadioButton* selectedParticlesButton = onlySelectedParticlesUI->buttonTrue();
		selectedParticlesButton->setText(tr("Selected particles"));
		layout2->addWidget(selectedParticlesButton, 4, 1);
	}

	// Periodic boundaries
	{
		QGroupBox* groupBox = new QGroupBox(tr("Periodic boundary conditions"));
		layout->addWidget(groupBox);

		QGridLayout* layout2 = new QGridLayout(groupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(2);

		BooleanParameterUI* unwrapTrajectoriesUI = new BooleanParameterUI(this, PROPERTY_FIELD(TrajectoryGeneratorObject::unwrapTrajectories));
		layout2->addWidget(unwrapTrajectoriesUI->checkBox(), 0, 0);
	}

	// Time range
	{
		QGroupBox* groupBox = new QGroupBox(tr("Time range"));
		layout->addWidget(groupBox);

		QVBoxLayout* layout2 = new QVBoxLayout(groupBox);
		layout2->setContentsMargins(4,4,4,4);
		layout2->setSpacing(2);
		QGridLayout* layout2c = new QGridLayout();
		layout2c->setContentsMargins(0,0,0,0);
		layout2c->setSpacing(2);
		layout2->addLayout(layout2c);

		BooleanRadioButtonParameterUI* useCustomIntervalUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(TrajectoryGeneratorObject::useCustomInterval));

		QRadioButton* animationIntervalButton = useCustomIntervalUI->buttonFalse();
		animationIntervalButton->setText(tr("Complete trajectory"));
		layout2c->addWidget(animationIntervalButton, 0, 0, 1, 5);

		QRadioButton* customIntervalButton = useCustomIntervalUI->buttonTrue();
		customIntervalButton->setText(tr("Frame interval:"));
		layout2c->addWidget(customIntervalButton, 1, 0, 1, 5);

		IntegerParameterUI* customRangeStartUI = new IntegerParameterUI(this, PROPERTY_FIELD(TrajectoryGeneratorObject::customIntervalStart));
		customRangeStartUI->setEnabled(false);
		layout2c->addLayout(customRangeStartUI->createFieldLayout(), 2, 1);
		layout2c->addWidget(new QLabel(tr("to")), 2, 2);
		IntegerParameterUI* customRangeEndUI = new IntegerParameterUI(this, PROPERTY_FIELD(TrajectoryGeneratorObject::customIntervalEnd));
		customRangeEndUI->setEnabled(false);
		layout2c->addLayout(customRangeEndUI->createFieldLayout(), 2, 3);
		layout2c->setColumnMinimumWidth(0, 30);
		layout2c->setColumnStretch(4, 1);
		connect(customIntervalButton, &QRadioButton::toggled, customRangeStartUI, &IntegerParameterUI::setEnabled);
		connect(customIntervalButton, &QRadioButton::toggled, customRangeEndUI, &IntegerParameterUI::setEnabled);

		QGridLayout* layout2a = new QGridLayout();
		layout2a->setContentsMargins(0,6,0,0);
		layout2a->setSpacing(2);
		layout2->addLayout(layout2a);
		IntegerParameterUI* everyNthFrameUI = new IntegerParameterUI(this, PROPERTY_FIELD(TrajectoryGeneratorObject::everyNthFrame));
		layout2a->addWidget(everyNthFrameUI->label(), 0, 0);
		layout2a->addLayout(everyNthFrameUI->createFieldLayout(), 0, 1);
		layout2a->setColumnStretch(2, 1);
	}

	QPushButton* createTrajectoryButton = new QPushButton(tr("Regenerate trajectory lines"));
	layout->addWidget(createTrajectoryButton);
	connect(createTrajectoryButton, &QPushButton::clicked, this, &TrajectoryGeneratorObjectEditor::onRegenerateTrajectory);
}

/******************************************************************************
* Is called when the user clicks the 'Regenerate trajectory' button.
******************************************************************************/
void TrajectoryGeneratorObjectEditor::onRegenerateTrajectory()
{
	TrajectoryGeneratorObject* trajObj = static_object_cast<TrajectoryGeneratorObject>(editObject());
	if(!trajObj) return;

	undoableTransaction(tr("Generate trajectory"), [this,trajObj]() {

		// Show progress dialog.
		ProgressDialog progressDialog(container(), trajObj->dataset()->container()->taskManager(), tr("Generating trajectory lines"));

		trajObj->generateTrajectories(progressDialog.taskManager());
	});
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
