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

#include <gui/GUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/StringParameterUI.h>
#include <plugins/povray/renderer/POVRayRenderer.h>
#include "POVRayRendererEditor.h"

namespace Ovito { namespace POVRay { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(POVRayGui, POVRayRendererEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(POVRayRenderer, POVRayRendererEditor);

/******************************************************************************
* Creates the UI controls for the editor.
******************************************************************************/
void POVRayRendererEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("POV-Ray settings"), rolloutParams);

	QVBoxLayout* mainLayout = new QVBoxLayout(rollout);
	mainLayout->setContentsMargins(4,4,4,4);

	QGroupBox* generalGroupBox = new QGroupBox(tr("Rendering quality"));
	mainLayout->addWidget(generalGroupBox);

	QGridLayout* layout = new QGridLayout(generalGroupBox);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);
	layout->setColumnStretch(1, 1);

	// Quality level
	IntegerParameterUI* qualityLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_qualityLevel));
	layout->addWidget(qualityLevelUI->label(), 0, 0);
	layout->addLayout(qualityLevelUI->createFieldLayout(), 0, 1);
	
	// Antialiasing
	BooleanGroupBoxParameterUI* enableAntialiasingUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_enableAntialiasing));
	QGroupBox* aaGroupBox = enableAntialiasingUI->groupBox();
	mainLayout->addWidget(aaGroupBox);

	layout = new QGridLayout(enableAntialiasingUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);
	layout->setColumnStretch(1, 1);

	// Sampling Method
	IntegerRadioButtonParameterUI* samplingMethodUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_samplingMethod));
	layout->addWidget(samplingMethodUI->addRadioButton(1, tr("Non-recursive sampling")), 1, 0, 1, 2);
	layout->addWidget(samplingMethodUI->addRadioButton(2, tr("Recursive sampling")), 2, 0, 1, 2);

	// AA Threshold
	FloatParameterUI* AAThresholdUI = new FloatParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_AAThreshold));
	layout->addWidget(AAThresholdUI->label(), 3, 0);
	layout->addLayout(AAThresholdUI->createFieldLayout(), 3, 1);

	// AA Depth
	IntegerParameterUI* AADepthUI = new IntegerParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_antialiasDepth));
	layout->addWidget(AADepthUI->label(), 4, 0);
	layout->addLayout(AADepthUI->createFieldLayout(), 4, 1);

	// Jitter
	BooleanParameterUI* enableJitterUI = new BooleanParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_enableJitter));
	layout->addWidget(enableJitterUI->checkBox(), 5, 0, 1, 2);

	// Radiosity
	BooleanGroupBoxParameterUI* enableRadiosityUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_enableRadiosity));
	QGroupBox* radiosityGroupBox = enableRadiosityUI->groupBox();
	mainLayout->addWidget(radiosityGroupBox);

	layout = new QGridLayout(enableRadiosityUI->childContainer());
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(2);
	layout->setColumnStretch(1, 1);

	// Ray count
	IntegerParameterUI* radiosityRayCountUI = new IntegerParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_radiosityRayCount));
	layout->addWidget(radiosityRayCountUI->label(), 0, 0);
	layout->addLayout(radiosityRayCountUI->createFieldLayout(), 0, 1);

	// Recursion limit
	IntegerParameterUI* radiosityRecursionLimitUI = new IntegerParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_radiosityRecursionLimit));
	layout->addWidget(radiosityRecursionLimitUI->label(), 1, 0);
	layout->addLayout(radiosityRecursionLimitUI->createFieldLayout(), 1, 1);

	// Error bound
	FloatParameterUI* radiosityErrorBoundUI = new FloatParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_radiosityErrorBound));
	layout->addWidget(radiosityErrorBoundUI->label(), 2, 0);
	layout->addLayout(radiosityErrorBoundUI->createFieldLayout(), 2, 1);

	// Preferences group
	QGroupBox* preferencesGroupBox = new QGroupBox(tr("Preferences"));
	mainLayout->addWidget(preferencesGroupBox);
	layout = new QGridLayout(preferencesGroupBox);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(0);
	layout->setColumnStretch(0, 1);

	// POV-Ray executable path
	layout->addWidget(new QLabel(tr("POV-Ray executable:")), 0, 0, 1, 2);
	
	StringParameterUI* povrayExecutablePUI = new StringParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_povrayExecutable));
	layout->addWidget(new QLabel(tr("POV-Ray executable:")), 0, 0);
	static_cast<QLineEdit*>(povrayExecutablePUI->textBox())->setPlaceholderText(QStringLiteral("povray"));
	layout->addWidget(povrayExecutablePUI->textBox(), 1, 0);
	QPushButton* selectExecutablePathButton = new QPushButton("...");
	connect(selectExecutablePathButton, &QPushButton::clicked, this, [this]{
		try {
			POVRayRenderer* renderer = static_object_cast<POVRayRenderer>(editObject());
			if(!renderer) return;			
			QString path = QFileDialog::getOpenFileName(container(), tr("Select POV-Ray Executable"), renderer->povrayExecutable());
			if(!path.isEmpty()) {
				UndoableTransaction::handleExceptions(renderer->dataset()->undoStack(), tr("Set executable path"), [renderer, &path]() {
					renderer->setPovrayExecutable(path);
				});
			}
		}
		catch(const Exception& ex) {
			ex.showError();
		}			
	});
	layout->addWidget(selectExecutablePathButton, 1, 1);

	// Show POV-Ray window
	BooleanParameterUI* povrayDisplayEnabledUI = new BooleanParameterUI(this, PROPERTY_FIELD(POVRayRenderer::_povrayDisplayEnabled));
	layout->addWidget(povrayDisplayEnabledUI->checkBox(), 2, 0, 1, 2);
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
