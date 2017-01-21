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
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <plugins/particles/gui/util/ParticlePropertyParameterUI.h>
#include "CorrelationFunctionModifierEditor.h"

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_scale_engine.h>

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
	FloatParameterUI *neighCutoffRadiusPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_neighCutoff));
	gridlayout->addWidget(neighCutoffRadiusPUI->label(), 1, 0);
	gridlayout->addLayout(neighCutoffRadiusPUI->createFieldLayout(), 1, 1);

	// Number of bins parameter.
	IntegerParameterUI* numberOfNeighBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_numberOfNeighBins));
	gridlayout->addWidget(numberOfNeighBinsPUI->label(), 2, 0);
	gridlayout->addLayout(numberOfNeighBinsPUI->createFieldLayout(), 2, 1);

	layout->addLayout(gridlayout);

	QGroupBox* realSpaceGroupBox = new QGroupBox(tr("Real-space correlation function"));
	layout->addWidget(realSpaceGroupBox);

	BooleanParameterUI* normalizeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_normalize));

	QGridLayout* typeOfRealSpacePlotLayout = new QGridLayout();
	IntegerRadioButtonParameterUI *typeOfRealSpacePlotPUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_typeOfRealSpacePlot));
	typeOfRealSpacePlotLayout->addWidget(new QLabel(tr("Display as:")), 0, 0);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(0, tr("lin-lin")), 0, 1);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(1, tr("log-lin")), 0, 2);
	typeOfRealSpacePlotLayout->addWidget(typeOfRealSpacePlotPUI->addRadioButton(3, tr("log-log")), 0, 3);

	_realSpacePlot = new QwtPlot();
	_realSpacePlot->setMinimumHeight(200);
	_realSpacePlot->setMaximumHeight(200);
	_realSpacePlot->setCanvasBackground(Qt::white);
	_realSpacePlot->setAxisTitle(QwtPlot::xBottom, tr("Distance r"));
	_realSpacePlot->setAxisTitle(QwtPlot::yLeft, tr("C(r)"));

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
	// x-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_fixRealSpaceXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_realSpaceXAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_realSpaceXAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}
	// y-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_fixRealSpaceYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_realSpaceYAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_realSpaceYAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}

	QVBoxLayout* realSpaceLayout = new QVBoxLayout(realSpaceGroupBox);
	realSpaceLayout->addWidget(normalizeUI->checkBox());
	realSpaceLayout->addLayout(typeOfRealSpacePlotLayout);
	realSpaceLayout->addWidget(_realSpacePlot);
	realSpaceLayout->addWidget(axesBox);

	QGroupBox* reciprocalSpaceGroupBox = new QGroupBox(tr("Reciprocal-space correlation function"));
	layout->addWidget(reciprocalSpaceGroupBox);

	QGridLayout* typeOfReciprocalSpacePlotLayout = new QGridLayout();
	IntegerRadioButtonParameterUI *typeOfReciprocalSpacePlotPUI = new IntegerRadioButtonParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_typeOfReciprocalSpacePlot));
	typeOfReciprocalSpacePlotLayout->addWidget(new QLabel(tr("Display as:")), 0, 0);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(0, tr("lin-lin")), 0, 1);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(1, tr("log-lin")), 0, 2);
	typeOfReciprocalSpacePlotLayout->addWidget(typeOfReciprocalSpacePlotPUI->addRadioButton(3, tr("log-log")), 0, 3);

	_reciprocalSpacePlot = new QwtPlot();
	_reciprocalSpacePlot->setMinimumHeight(200);
	_reciprocalSpacePlot->setMaximumHeight(200);
	_reciprocalSpacePlot->setCanvasBackground(Qt::white);
	_reciprocalSpacePlot->setAxisTitle(QwtPlot::xBottom, tr("Wavevector q"));
	_reciprocalSpacePlot->setAxisTitle(QwtPlot::yLeft, tr("C(q)"));

	// Axes.
	axesBox = new QGroupBox(tr("Plot axes"), rollout);
	axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
	// x-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_fixReciprocalSpaceXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_reciprocalSpaceXAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_reciprocalSpaceXAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}
	// y-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_fixReciprocalSpaceYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_reciprocalSpaceYAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(CorrelationFunctionModifier::_reciprocalSpaceYAxisRangeEnd));
		hlayout->addWidget(new QLabel(tr("From:")));
		hlayout->addLayout(startPUI->createFieldLayout());
		hlayout->addSpacing(12);
		hlayout->addWidget(new QLabel(tr("To:")));
		hlayout->addLayout(endPUI->createFieldLayout());
		startPUI->setEnabled(false);
		endPUI->setEnabled(false);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
		connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);
	}

	QVBoxLayout* reciprocalSpaceLayout = new QVBoxLayout(reciprocalSpaceGroupBox);
	reciprocalSpaceLayout->addLayout(typeOfReciprocalSpacePlotLayout);
	reciprocalSpaceLayout->addWidget(_reciprocalSpacePlot);
	reciprocalSpaceLayout->addWidget(axesBox);

	connect(this, &CorrelationFunctionModifierEditor::contentsReplaced, this, &CorrelationFunctionModifierEditor::plotAllData);

	layout->addSpacing(12);
	QPushButton* saveDataButton = new QPushButton(tr("Export data to text file"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &CorrelationFunctionModifierEditor::onSaveData);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool CorrelationFunctionModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if (event->sender() == editObject() && 
		(event->type() == ReferenceEvent::TargetChanged || 
		 event->type() == ReferenceEvent::ObjectStatusChanged)) {
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
												 QwtPlotCurve *&curve,
												 FloatType &minX, FloatType &maxX,
												 FloatType &minY, FloatType &maxY,
												 FloatType offset, FloatType fac)
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
	int startAt = 1;
	QVector<QPointF> plotData(numberOfDataPoints-startAt);
	minX = minY = 1e20;
	maxX = maxY = -1e20;
	for (int i = startAt; i < numberOfDataPoints; i++) {
		FloatType xValue = xData[i];
		FloatType yValue = fac*(yData[i]-offset);
		plotData[i-startAt].rx() = xValue;
		plotData[i-startAt].ry() = yValue;
		if (xValue != 0) {
			minX = std::min(minX, xValue);
			maxX = std::max(maxX, xValue);
		}
		if (yValue != 0) {
			minY = std::min(minY, yValue);
			maxY = std::max(maxY, yValue);
		}
	}
	curve->setSamples(plotData);
}

/******************************************************************************
* Updates the plot of the RDF computed by the modifier.
******************************************************************************/
void CorrelationFunctionModifierEditor::plotAllData()
{
	CorrelationFunctionModifier* modifier = static_object_cast<CorrelationFunctionModifier>(editObject());
	if(!modifier)
		return;

	FloatType offset = 0.0;
	FloatType fac = 1.0;
	if (modifier->normalize()) {
		offset = modifier->mean1()*modifier->mean2();
		fac = 1.0/(modifier->covariance()-offset);
	}

	FloatType realSpaceMinX = 1e20, realSpaceMaxX = -1e20, realSpaceMinY = 1e20, realSpaceMaxY = -1e20;
	// Plot real-space correlation function
	if(!modifier->realSpaceCorrelationX().empty() &&
	   !modifier->realSpaceCorrelation().empty()) {
		plotData(modifier->realSpaceCorrelationX(),
				 modifier->realSpaceCorrelation(),
				 _realSpacePlot,
				 _realSpaceCurve,
				 realSpaceMinX, realSpaceMaxX,
				 realSpaceMinY, realSpaceMaxY,
				 offset, fac);
	}

	FloatType neighMinX = 1e20, neighMaxX = -1e20, neighMinY = 1e20, neighMaxY = -1e20;
	if(!modifier->neighCorrelationX().empty() &&
	   !modifier->neighCorrelation().empty()) {
		if(!_neighCurve) {
			_neighCurve = new QwtPlotCurve();
			_neighCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
			_neighCurve->setPen(Qt::red);
			_neighCurve->attach(_realSpacePlot);
		}

		// Set data to plot.
		auto &xData = modifier->neighCorrelationX();
		auto &yData = modifier->neighCorrelation();
		size_t numberOfDataPoints = yData.size();
		QVector<QPointF> plotData(numberOfDataPoints);
		for (int i = 0; i < numberOfDataPoints; i++) {
			FloatType xValue = xData[i];
			FloatType yValue = fac*(yData[i]-offset);
			plotData[i].rx() = xValue;
			plotData[i].ry() = yValue;
			if (xValue != 0) {
				neighMinX = std::min(neighMinX, xValue);
				neighMaxX = std::max(neighMaxX, xValue);
			}
			if (yValue != 0) {
				neighMinY = std::min(neighMinY, yValue);
				neighMaxY = std::max(neighMaxY, yValue);
			}
		}
		_neighCurve->setSamples(plotData);
	}

	// Plot reciprocal-space correlation function
	FloatType reciprocalSpaceMinX = 1e20, reciprocalSpaceMaxX = -1e20, reciprocalSpaceMinY = 1e20, reciprocalSpaceMaxY = -1e20;
	if(!modifier->reciprocalSpaceCorrelationX().empty() &&
	   !modifier->reciprocalSpaceCorrelation().empty()) {
		plotData(modifier->reciprocalSpaceCorrelationX(),
				 modifier->reciprocalSpaceCorrelation(),
				 _reciprocalSpacePlot,
				 _reciprocalSpaceCurve,
				 reciprocalSpaceMinX, reciprocalSpaceMaxX,
				 reciprocalSpaceMinY, reciprocalSpaceMaxY);
	}

	// Set type of plot.
	if (modifier->typeOfRealSpacePlot() & 1) 
		_realSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
	else
		_realSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
	if (modifier->typeOfRealSpacePlot() & 2)
		_realSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
	else
		_realSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());

	if (modifier->typeOfReciprocalSpacePlot() & 1)
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
	else
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
	if (modifier->typeOfReciprocalSpacePlot() & 2)
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());
	else
		_reciprocalSpacePlot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine());

	_realSpacePlot->setAxisScale(QwtPlot::xBottom,
								 std::min(realSpaceMinX, neighMinX),
								 std::max(realSpaceMaxX, neighMaxX));
	_realSpacePlot->setAxisScale(QwtPlot::yLeft,
								 std::min(realSpaceMinY, neighMinY),
								 std::max(realSpaceMaxY, neighMaxY));
	_reciprocalSpacePlot->setAxisScale(QwtPlot::xBottom, reciprocalSpaceMinX, reciprocalSpaceMaxX);
	_reciprocalSpacePlot->setAxisScale(QwtPlot::yLeft, reciprocalSpaceMinY, reciprocalSpaceMaxY);

	_realSpacePlot->replot();
	_reciprocalSpacePlot->replot();
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void CorrelationFunctionModifierEditor::onSaveData()
{
	CorrelationFunctionModifier* modifier = static_object_cast<CorrelationFunctionModifier>(editObject());
	if(!modifier)
		return;

	if(modifier->realSpaceCorrelation().empty() && modifier->neighCorrelation().empty() && modifier->reciprocalSpaceCorrelation().empty())
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
		tr("Save Correlation Data"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {
		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			modifier->throwException(tr("Could not open file for writing: %1").arg(file.errorString()));

		QTextStream stream(&file);

		stream << "# This file contains the correlation between the following property:" << endl;
		stream << "# " << modifier->sourceProperty1().name() << " with mean value " << modifier->mean1() << endl;
		stream << "# " << modifier->sourceProperty2().name() << " with mean value " << modifier->mean2() << endl;
		stream << "# Covariance is " << modifier->covariance() << endl << endl;

		if (!modifier->realSpaceCorrelation().empty()) {
			stream << "# Real-space correlation function from FFT follows." << endl;
			stream << "# 1: Bin number" << endl;
			stream << "# 2: Distance r" << endl;
			stream << "# 3: Correlation function C(r)" << endl;
			for(int i = 0; i < modifier->realSpaceCorrelation().size(); i++) {
				stream << i << "\t" << modifier->realSpaceCorrelationX()[i] << "\t" << modifier->realSpaceCorrelation()[i] << endl;
			}
			stream << endl;
		}

		if (!modifier->neighCorrelation().empty()) {
			stream << "# Real-space correlation function from direct sum over neighbors follows." << endl;
			stream << "# 1: Bin number" << endl;
			stream << "# 2: Distance r" << endl;
			stream << "# 3: Correlation function C(r)" << endl;
			for(int i = 0; i < modifier->neighCorrelation().size(); i++) {
				stream << i << "\t" << modifier->neighCorrelationX()[i] << "\t" << modifier->neighCorrelation()[i] << endl;
			}
			stream << endl;
		}

		if (!modifier->reciprocalSpaceCorrelation().empty()) {
			stream << "# Reciprocal-space correlation function from FFT follows." << endl;
			stream << "# 1: Bin number" << endl;
			stream << "# 2: Wavevector q (includes a factor of 2*pi)" << endl;
			stream << "# 3: Correlation function C(q)" << endl;
			for(int i = 0; i < modifier->reciprocalSpaceCorrelation().size(); i++) {
				stream << i << "\t" << modifier->reciprocalSpaceCorrelationX()[i] << "\t" << modifier->reciprocalSpaceCorrelation()[i] << endl;
			}
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
