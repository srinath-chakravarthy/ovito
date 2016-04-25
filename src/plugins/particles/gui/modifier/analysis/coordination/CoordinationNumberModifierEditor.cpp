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
#include <plugins/particles/modifier/analysis/coordination/CoordinationNumberModifier.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include "CoordinationNumberModifierEditor.h"
#include <qcustomplot.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, CoordinationNumberModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(CoordinationNumberModifier, CoordinationNumberModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CoordinationNumberModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Coordination analysis"), rolloutParams, "particles.modifiers.coordination_analysis.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CoordinationNumberModifier::_cutoff));
	gridlayout->addWidget(cutoffRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);

	// Number of bins parameter.
	IntegerParameterUI* numBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CoordinationNumberModifier::_numberOfBins));
	gridlayout->addWidget(numBinsPUI->label(), 1, 0);
	gridlayout->addLayout(numBinsPUI->createFieldLayout(), 1, 1);

	layout->addLayout(gridlayout);

	_rdfPlot = new QCustomPlot();
	_rdfPlot->setMinimumHeight(180);
	_rdfPlot->xAxis->setLabel("Pair separation distance");
	_rdfPlot->yAxis->setLabel("g(r)");
	_rdfPlot->addGraph();

	layout->addWidget(new QLabel(tr("Radial distribution function:")));
	layout->addWidget(_rdfPlot);
	connect(this, &CoordinationNumberModifierEditor::contentsReplaced, this, &CoordinationNumberModifierEditor::plotRDF);

	QPushButton* saveDataButton = new QPushButton(tr("Export data to text file"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &CoordinationNumberModifierEditor::onSaveData);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool CoordinationNumberModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		plotRDF();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the plot of the RDF computed by the modifier.
******************************************************************************/
void CoordinationNumberModifierEditor::plotRDF()
{
	CoordinationNumberModifier* modifier = static_object_cast<CoordinationNumberModifier>(editObject());
	if(!modifier)
		return;

	if(modifier->rdfX().empty())
		return;

	_rdfPlot->graph()->setData(modifier->rdfX(), modifier->rdfY());
	_rdfPlot->graph()->rescaleAxes();

	// Determine lower X bound where the histogram is non-zero.
	double maxx = modifier->rdfX().back();
	for(int i = 0; i < modifier->rdfX().size(); i++) {
		if(modifier->rdfY()[i] != 0) {
			double minx = std::floor(modifier->rdfX()[i] * 9.0 / maxx) / 10.0 * maxx;
			_rdfPlot->xAxis->setRange(minx, maxx);
			break;
		}
	}

	_rdfPlot->replot(QCustomPlot::rpQueued);
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void CoordinationNumberModifierEditor::onSaveData()
{
	CoordinationNumberModifier* modifier = static_object_cast<CoordinationNumberModifier>(editObject());
	if(!modifier)
		return;

	if(modifier->rdfX().empty())
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save RDF Data"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {

		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			modifier->throwException(tr("Could not open file for writing: %1").arg(file.errorString()));

		QTextStream stream(&file);

		stream << "# 1: Bin number" << endl;
		stream << "# 2: r" << endl;
		stream << "# 3: g(r)" << endl;
		for(int i = 0; i < modifier->rdfX().size(); i++) {
			stream << i << "\t" << modifier->rdfX()[i] << "\t" << modifier->rdfY()[i] << endl;
		}
	}
	catch(const Exception& ex) {
		ex.showError();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
