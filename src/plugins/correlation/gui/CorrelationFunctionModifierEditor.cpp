///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//  Copyright (2016) Lars Pastewka
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
#include <plugins/correlation/CorrelationFunctionModifier.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <plugins/particles/gui/util/ParticlePropertyParameterUI.h>
#include "CorrelationFunctionModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(CorrelationFunctionModifierPluginGui, CorrelationFunctionModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(CorrelationFunctionModifier, CorrelationFunctionModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void CorrelationFunctionModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	qDebug() << "CorrelationFunctionModifierEditor::createUI";

	// Create a rollout.
	QWidget* rollout = createRollout(tr("Correlation function"), rolloutParams, "particles.modifiers.correlation_function.html");

	// Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	ParticlePropertyParameterUI* sourceProperty1UI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty1));
	layout->addWidget(new QLabel(tr("First property:"), rollout));
	layout->addWidget(sourceProperty1UI->comboBox());

	ParticlePropertyParameterUI* sourceProperty2UI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_sourceProperty2));
	layout->addWidget(new QLabel(tr("Second property:"), rollout));
	layout->addWidget(sourceProperty2UI->comboBox());

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Cutoff parameter.
	FloatParameterUI* cutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_cutoff));
	gridlayout->addWidget(cutoffRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(cutoffRadiusPUI->createFieldLayout(), 0, 1);

	// Number of bins parameter.
	IntegerParameterUI* numBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_numberOfBins));
	gridlayout->addWidget(numBinsPUI->label(), 1, 0);
	gridlayout->addLayout(numBinsPUI->createFieldLayout(), 1, 1);

	layout->addLayout(gridlayout);

	_realSpacePlot = new QwtPlot();
	_realSpacePlot->setMinimumHeight(200);
	_realSpacePlot->setMaximumHeight(200);
	_realSpacePlot->setCanvasBackground(Qt::white);
	_realSpacePlot->setAxisTitle(QwtPlot::xBottom, tr("Pair separation distance"));
	_realSpacePlot->setAxisTitle(QwtPlot::yLeft, tr("C(r)"));

	layout->addWidget(new QLabel(tr("Correlation function:")));
	layout->addWidget(_realSpacePlot);
	connect(this, &CorrelationFunctionModifierEditor::contentsReplaced, this, &CorrelationFunctionModifierEditor::plotData);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool CorrelationFunctionModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	qDebug() << "CorrelationFunctionModifierEditor::referenceEvent";

	if(event->sender() == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		plotDataLater(this);
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Updates the plot of the RDF computed by the modifier.
******************************************************************************/
void CorrelationFunctionModifierEditor::plotData()
{
	qDebug() << "CorrelationFunctionModifierEditor::plotData";

	CorrelationFunctionModifier* modifier = static_object_cast<CorrelationFunctionModifier>(editObject());
	if(!modifier)
		return;

	if(modifier->realSpaceCorrelationFunctionX().empty())
		return;
	if(modifier->realSpaceCorrelationFunction().empty())
		return;

	qDebug() << modifier->realSpaceCorrelationFunctionX().size();
	qDebug() << modifier->realSpaceCorrelationFunction().size();
	qDebug() << modifier->realSpaceCorrelationFunctionX();
	qDebug() << modifier->realSpaceCorrelationFunction();

	if(!_realSpaceCurve) {
		qDebug() << "Allocating QwtPlotCurve";
		_realSpaceCurve = new QwtPlotCurve();
		_realSpaceCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
		_realSpaceCurve->setBrush(Qt::lightGray);
		_realSpaceCurve->attach(_realSpacePlot);
		QwtPlotGrid* plotGrid = new QwtPlotGrid();
		plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
		plotGrid->attach(_realSpacePlot);
	}

	// Set data to plot.
	size_t numberOfDataPoints = modifier->reciprocalSpaceCorrelationFunction().size();
	QVector<QPointF> plotData(numberOfDataPoints);
	for (int i = 0; i < numberOfDataPoints; i++) {
		plotData[i].rx() = modifier->reciprocalSpaceCorrelationFunctionX()[i];
		plotData[i].ry() = modifier->reciprocalSpaceCorrelationFunction()[i];
	}
	_realSpaceCurve->setSamples(plotData);

	// Determine lower X bound where the correlation function is non-zero.
	_realSpacePlot->setAxisAutoScale(QwtPlot::xBottom);
	_realSpacePlot->setAxisScale(QwtPlot::xBottom, 0, 20);
	_realSpacePlot->replot();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
