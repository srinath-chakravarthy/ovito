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
#include <plugins/particles/modifier/analysis/ptm/PolyhedralTemplateMatchingModifier.h>
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include "PolyhedralTemplateMatchingModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_zoneitem.h>
#include <qwt/qwt_plot_grid.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(PolyhedralTemplateMatchingModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(PolyhedralTemplateMatchingModifier, PolyhedralTemplateMatchingModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void PolyhedralTemplateMatchingModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Polyhedral template matching"), rolloutParams, "particles.modifiers.polyhedral_template_matching.html");

    // Create the rollout contents.
	QVBoxLayout* layout1 = new QVBoxLayout(rollout);
	layout1->setContentsMargins(4,4,4,4);
	layout1->setSpacing(6);

	QGroupBox* paramsBox = new QGroupBox(tr("Parameters"), rollout);
	QGridLayout* gridlayout = new QGridLayout(paramsBox);
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);
	layout1->addWidget(paramsBox);

	// RMSD cutoff parameter.
	FloatParameterUI* rmsdCutoffPUI = new FloatParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_rmsdCutoff));
	gridlayout->addWidget(rmsdCutoffPUI->label(), 0, 0);
	gridlayout->addLayout(rmsdCutoffPUI->createFieldLayout(), 0, 1);

	// Use only selected particles.
	BooleanParameterUI* onlySelectedParticlesUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::_onlySelectedParticles));
	gridlayout->addWidget(onlySelectedParticlesUI->checkBox(), 1, 0, 1, 2);

	QGroupBox* outputBox = new QGroupBox(tr("Output"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(outputBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout1->addWidget(outputBox);

	// Output controls.
	BooleanParameterUI* outputRmsdUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputRmsd));
	sublayout->addWidget(outputRmsdUI->checkBox());
	outputRmsdUI->checkBox()->setText(tr("RMSD value"));
	BooleanParameterUI* outputInteratomicDistanceUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputInteratomicDistance));
	sublayout->addWidget(outputInteratomicDistanceUI->checkBox());
	outputInteratomicDistanceUI->checkBox()->setText(tr("Interatomic distance"));
	BooleanParameterUI* outputOrientationUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputOrientation));
	sublayout->addWidget(outputOrientationUI->checkBox());
	outputOrientationUI->checkBox()->setText(tr("Lattice orientation"));
	BooleanParameterUI* outputDeformationGradientUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputDeformationGradient));
	sublayout->addWidget(outputDeformationGradientUI->checkBox());
	outputDeformationGradientUI->checkBox()->setText(tr("Elastic deformation gradient"));
	BooleanParameterUI* outputAlloyTypesUI = new BooleanParameterUI(this, PROPERTY_FIELD(PolyhedralTemplateMatchingModifier::_outputAlloyTypes));
	sublayout->addWidget(outputAlloyTypesUI->checkBox());
	outputAlloyTypesUI->checkBox()->setText(tr("Alloy type"));

	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this, true);
	layout1->addSpacing(10);
	layout1->addWidget(new QLabel(tr("Structure types:")));
	layout1->addWidget(structureTypesPUI->tableWidget());
	QLabel* label = new QLabel(tr("<p style=\"font-size: small;\">Double-click to change colors. Defaults can be set in the application settings.</p>"));
	label->setWordWrap(true);
	layout1->addWidget(label);

	_plot = new QwtPlot();
	_plot->setMinimumHeight(240);
	_plot->setMaximumHeight(240);
	_plot->setCanvasBackground(Qt::white);
	_plot->setAxisTitle(QwtPlot::xBottom, tr("RMSD"));	
	_plot->setAxisTitle(QwtPlot::yLeft, tr("Count"));	

	layout1->addSpacing(10);
	layout1->addWidget(_plot);
	connect(this, &PolyhedralTemplateMatchingModifierEditor::contentsReplaced, this, &PolyhedralTemplateMatchingModifierEditor::plotHistogram);

	// Status label.
	layout1->addSpacing(10);
	layout1->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool PolyhedralTemplateMatchingModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject() && (event->type() == ReferenceEvent::ObjectStatusChanged || event->type() == ReferenceEvent::TargetChanged)) {
		plotHistogramLater(this);
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void PolyhedralTemplateMatchingModifierEditor::plotHistogram()
{
	PolyhedralTemplateMatchingModifier* modifier = static_object_cast<PolyhedralTemplateMatchingModifier>(editObject());
	
	if(!modifier || modifier->rmsdHistogramData().empty()) {
		if(_plotCurve) _plotCurve->hide();
		return;
	}

	QVector<QPointF> plotData(modifier->rmsdHistogramData().size());
	double binSize = modifier->rmsdHistogramBinSize();
	double maxHistogramData = 0;
	for(int i = 0; i < modifier->rmsdHistogramData().size(); i++) {
		plotData[i].rx() = binSize * ((double)i + 0.5);
		plotData[i].ry() = modifier->rmsdHistogramData()[i];
		maxHistogramData = std::max(maxHistogramData, plotData[i].y());
	}

	if(!_plotCurve) {
		_plotCurve = new QwtPlotCurve();
	    _plotCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
		_plotCurve->setBrush(QColor(255, 160, 100));
		_plotCurve->attach(_plot);
		QwtPlotGrid* plotGrid = new QwtPlotGrid();
		plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
		plotGrid->attach(_plot);
	}
    _plotCurve->setSamples(plotData);

	if(modifier->rmsdCutoff() > 0) {
		if(!_rmsdRange) {
			_rmsdRange = new QwtPlotZoneItem();
			_rmsdRange->setOrientation(Qt::Vertical);
			_rmsdRange->setZ(_plotCurve->z() + 1);
			_rmsdRange->attach(_plot);
		}
		_rmsdRange->show();
		_rmsdRange->setInterval(0, modifier->rmsdCutoff());
	}
	else if(_rmsdRange) {
		_rmsdRange->hide();
	}

	_plot->replot();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
