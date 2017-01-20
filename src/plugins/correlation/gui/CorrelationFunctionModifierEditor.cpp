///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
//  Copyright (2017) Lars Pastewka
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

	// FFT grid spacing parameter.
	FloatParameterUI* fftGridSpacingRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_fftGridSpacing));
	gridlayout->addWidget(fftGridSpacingRadiusPUI->label(), 0, 0);
	gridlayout->addLayout(fftGridSpacingRadiusPUI->createFieldLayout(), 0, 1);

	// Neighbor cutoff parameter.
	FloatParameterUI *shortRangedCutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_shortRangedCutoff));
	gridlayout->addWidget(shortRangedCutoffRadiusPUI->label(), 1, 0);
	gridlayout->addLayout(shortRangedCutoffRadiusPUI->createFieldLayout(), 1, 1);

	// Number of bins parameter.
	IntegerParameterUI* numBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_numberOfBinsForShortRangedCalculation));
	gridlayout->addWidget(numBinsPUI->label(), 2, 0);
	gridlayout->addLayout(numBinsPUI->createFieldLayout(), 2, 1);

	layout->addLayout(gridlayout);

	_realSpacePlot = new QwtPlot();
	_realSpacePlot->setMinimumHeight(200);
	_realSpacePlot->setMaximumHeight(200);
	_realSpacePlot->setCanvasBackground(Qt::white);
	_realSpacePlot->setAxisTitle(QwtPlot::xBottom, tr("Distance r"));
	_realSpacePlot->setAxisTitle(QwtPlot::yLeft, tr("C(r)"));

	layout->addWidget(new QLabel(tr("Real-space correlation function:")));
	layout->addWidget(_realSpacePlot);

	_reciprocalSpacePlot = new QwtPlot();
	_reciprocalSpacePlot->setMinimumHeight(200);
	_reciprocalSpacePlot->setMaximumHeight(200);
	_reciprocalSpacePlot->setCanvasBackground(Qt::white);
	_reciprocalSpacePlot->setAxisTitle(QwtPlot::xBottom, tr("Wavevector q"));
	_reciprocalSpacePlot->setAxisTitle(QwtPlot::yLeft, tr("C(q)"));

	layout->addWidget(new QLabel(tr("Reciprocal-space correlation function:")));
	layout->addWidget(_reciprocalSpacePlot);

	connect(this, &CorrelationFunctionModifierEditor::contentsReplaced, this, &CorrelationFunctionModifierEditor::plotAllData);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool CorrelationFunctionModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{

	if(event->sender() == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		plotAllDataLater(this);
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Plot correlation function.
******************************************************************************/
void CorrelationFunctionModifierEditor::plotData(const QVector<FloatType> &xData,
											     const QVector<FloatType> &yData,
												 QwtPlot *plot,
												 QwtPlotCurve *&curve)
{
	if (xData.size() != yData.size())
		throwException("Argument to plotData must have same size.");

	if(!curve) {
		curve = new QwtPlotCurve();
		curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
		curve->setBrush(Qt::lightGray);
		curve->attach(plot);
		QwtPlotGrid* plotGrid = new QwtPlotGrid();
		plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
		plotGrid->attach(plot);
	}

	// Set data to plot.
	size_t numberOfDataPoints = yData.size();
	QVector<QPointF> plotData(numberOfDataPoints);
	FloatType minx, maxx;
	minx = maxx = xData[0];
	for (int i = 1; i < numberOfDataPoints; i++) {
		FloatType xValue = xData[i];
		plotData[i].rx() = xValue;
		plotData[i].ry() = yData[i];
		minx = std::min(minx, xValue);
		maxx = std::max(maxx, xValue);
	}
	curve->setSamples(plotData);

	// Determine lower X bound where the correlation function is non-zero.
	plot->setAxisAutoScale(QwtPlot::xBottom);
	plot->setAxisScale(QwtPlot::xBottom, 0.0, maxx);
	plot->replot();
}

/******************************************************************************
* Updates the plot of the RDF computed by the modifier.
******************************************************************************/
void CorrelationFunctionModifierEditor::plotAllData()
{
	CorrelationFunctionModifier* modifier = static_object_cast<CorrelationFunctionModifier>(editObject());
	if(!modifier)
		return;

	// Plot real-space correlation function
	if(!modifier->realSpaceCorrelationFunctionX().empty() &&
	   !modifier->realSpaceCorrelationFunction().empty()) {
		plotData(modifier->realSpaceCorrelationFunctionX(),
				 modifier->realSpaceCorrelationFunction(),
				 _realSpacePlot,
				 _realSpaceCurve);
	}

	if(!modifier->shortRangedRealSpaceCorrelationFunctionX().empty() &&
	   !modifier->shortRangedRealSpaceCorrelationFunction().empty()) {
		if(!_shortRangedRealSpaceCurve) {
			_shortRangedRealSpaceCurve = new QwtPlotCurve();
			_shortRangedRealSpaceCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
			_shortRangedRealSpaceCurve->setPen(Qt::red);
			_shortRangedRealSpaceCurve->attach(_realSpacePlot);
		}

		// Set data to plot.
		auto &xData = modifier->shortRangedRealSpaceCorrelationFunctionX();
		auto &yData = modifier->shortRangedRealSpaceCorrelationFunction();
		size_t numberOfDataPoints = yData.size();
		QVector<QPointF> plotData(numberOfDataPoints);
		FloatType minx, maxx;
		minx = maxx = xData[0];
		for (int i = 1; i < numberOfDataPoints; i++) {
			FloatType xValue = xData[i];
			plotData[i].rx() = xValue;
			plotData[i].ry() = yData[i];
			minx = std::min(minx, xValue);
			maxx = std::max(maxx, xValue);
		}
		_shortRangedRealSpaceCurve->setSamples(plotData);
	}

	// Plot reciprocal-space correlation function
	if(!modifier->reciprocalSpaceCorrelationFunctionX().empty() &&
	   !modifier->reciprocalSpaceCorrelationFunction().empty()) {
		plotData(modifier->reciprocalSpaceCorrelationFunctionX(),
				 modifier->reciprocalSpaceCorrelationFunction(),
				 _reciprocalSpacePlot,
				 _reciprocalSpaceCurve);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
