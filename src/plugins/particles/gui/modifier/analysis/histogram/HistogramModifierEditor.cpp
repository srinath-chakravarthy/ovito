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
#include <plugins/particles/modifier/analysis/histogram/HistogramModifier.h>
#include <plugins/particles/gui/util/ParticlePropertyParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/mainwin/MainWindow.h>
#include "HistogramModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, HistogramModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(HistogramModifier, HistogramModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void HistogramModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Histogram"), rolloutParams, "particles.modifiers.histogram.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	ParticlePropertyParameterUI* sourcePropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(HistogramModifier::_sourceProperty));
	layout->addWidget(new QLabel(tr("Property:"), rollout));
	layout->addWidget(sourcePropertyUI->comboBox());

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setColumnStretch(1, 1);

	// Number of bins parameter.
	IntegerParameterUI* numBinsPUI = new IntegerParameterUI(this, PROPERTY_FIELD(HistogramModifier::_numberOfBins));
	gridlayout->addWidget(numBinsPUI->label(), 0, 0);
	gridlayout->addLayout(numBinsPUI->createFieldLayout(), 0, 1);

	layout->addLayout(gridlayout);

	_histogramPlot = new QCustomPlot();
	_histogramPlot->setMinimumHeight(240);
	_histogramPlot->setInteraction(QCP::iRangeDrag, true);
	_histogramPlot->axisRect()->setRangeDrag(Qt::Horizontal);
	_histogramPlot->setInteraction(QCP::iRangeZoom, true);
	_histogramPlot->axisRect()->setRangeZoom(Qt::Horizontal);
	_histogramPlot->yAxis->setLabel("Particle count");
	_histogramPlot->addGraph();
	_histogramPlot->graph()->setBrush(QBrush(QColor(255, 160, 100)));

	_selectionRangeStartMarker = new QCPItemStraightLine(_histogramPlot);
	_selectionRangeEndMarker = new QCPItemStraightLine(_histogramPlot);
	_selectionRangeStartMarker->setVisible(false);
	_selectionRangeEndMarker->setVisible(false);
	QPen markerPen;
	markerPen.setColor(QColor(255, 40, 30));
	markerPen.setStyle(Qt::DotLine);
	markerPen.setWidth(2);
	_selectionRangeStartMarker->setPen(markerPen);
	_selectionRangeEndMarker->setPen(markerPen);
	_histogramPlot->addItem(_selectionRangeStartMarker);
	_histogramPlot->addItem(_selectionRangeEndMarker);
	connect(_histogramPlot->xAxis, SIGNAL(rangeChanged(const QCPRange&)), this, SLOT(updateXAxisRange(const QCPRange&)));

	layout->addWidget(new QLabel(tr("Histogram:")));
	layout->addWidget(_histogramPlot);
	connect(this, &HistogramModifierEditor::contentsReplaced, this, &HistogramModifierEditor::plotHistogram);

	QPushButton* saveDataButton = new QPushButton(tr("Save histogram data"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &HistogramModifierEditor::onSaveData);

	// Input.
	QGroupBox* inputBox = new QGroupBox(tr("Input"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(inputBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(inputBox);

	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(HistogramModifier::_onlySelected));
	sublayout->addWidget(onlySelectedUI->checkBox());

	// Create selection.
	QGroupBox* selectionBox = new QGroupBox(tr("Create selection"), rollout);
	sublayout = new QVBoxLayout(selectionBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(selectionBox);

	BooleanParameterUI* selectInRangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(HistogramModifier::_selectInRange));
	sublayout->addWidget(selectInRangeUI->checkBox());

	QHBoxLayout* hlayout = new QHBoxLayout();
	sublayout->addLayout(hlayout);
	FloatParameterUI* selRangeStartPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::_selectionRangeStart));
	FloatParameterUI* selRangeEndPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::_selectionRangeEnd));
	hlayout->addWidget(new QLabel(tr("From:")));
	hlayout->addLayout(selRangeStartPUI->createFieldLayout());
	hlayout->addSpacing(12);
	hlayout->addWidget(new QLabel(tr("To:")));
	hlayout->addLayout(selRangeEndPUI->createFieldLayout());
	selRangeStartPUI->setEnabled(false);
	selRangeEndPUI->setEnabled(false);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeStartPUI, &FloatParameterUI::setEnabled);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeEndPUI, &FloatParameterUI::setEnabled);

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
	// x-axis.
	{
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(HistogramModifier::_fixXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::_xAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::_xAxisRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(HistogramModifier::_fixYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::_yAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(HistogramModifier::_yAxisRangeEnd));
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

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool HistogramModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		plotHistogram();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void HistogramModifierEditor::plotHistogram()
{
	HistogramModifier* modifier = static_object_cast<HistogramModifier>(editObject());
	if(!modifier)
		return;

	_histogramPlot->xAxis->setLabel(modifier->sourceProperty().name());

	if(modifier->histogramData().empty())
		return;

	QVector<double> xdata(modifier->histogramData().size());
	QVector<double> ydata(modifier->histogramData().size());
	double binSize = (modifier->xAxisRangeEnd() - modifier->xAxisRangeStart()) / xdata.size();
	double maxHistogramData = 0.0;
	for(int i = 0; i < xdata.size(); i++) {
		xdata[i] = binSize * ((double)i + 0.5) + modifier->xAxisRangeStart();
		ydata[i] = modifier->histogramData()[i];
		maxHistogramData = std::max(maxHistogramData, ydata[i]);
	}
	_histogramPlot->graph()->setLineStyle(QCPGraph::lsStepCenter);
	_histogramPlot->graph()->setData(xdata, ydata);

	// Check if range is already correct, because setRange emits the rangeChanged signa
	// which is to be avoided if the range is not determined automatically.
	_rangeUpdate = false;
	_histogramPlot->xAxis->setRange(modifier->xAxisRangeStart(), modifier->xAxisRangeEnd());
	_histogramPlot->yAxis->setRange(modifier->yAxisRangeStart(), modifier->yAxisRangeEnd());
	_rangeUpdate = true;

	if(modifier->selectInRange()) {
		_selectionRangeStartMarker->setVisible(true);
		_selectionRangeEndMarker->setVisible(true);
		_selectionRangeStartMarker->point1->setCoords(modifier->selectionRangeStart(), 0);
		_selectionRangeStartMarker->point2->setCoords(modifier->selectionRangeStart(), 1);
		_selectionRangeEndMarker->point1->setCoords(modifier->selectionRangeEnd(), 0);
		_selectionRangeEndMarker->point2->setCoords(modifier->selectionRangeEnd(), 1);
	}
	else {
		_selectionRangeStartMarker->setVisible(false);
		_selectionRangeEndMarker->setVisible(false);
	}

	_histogramPlot->replot(QCustomPlot::rpQueued);
}

/******************************************************************************
* Keep x-axis range updated
******************************************************************************/
void HistogramModifierEditor::updateXAxisRange(const QCPRange &newRange)
{
	if (_rangeUpdate) {
		HistogramModifier* modifier = static_object_cast<HistogramModifier>(editObject());
		if(!modifier)
			return;

		// Fix range if user modifies the range by a mouse action in QCustomPlot
		modifier->setFixXAxisRange(true);
		modifier->setXAxisRange(newRange.lower, newRange.upper);
	}
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void HistogramModifierEditor::onSaveData()
{
	HistogramModifier* modifier = static_object_cast<HistogramModifier>(editObject());
	if(!modifier)
		return;

	if(modifier->histogramData().empty())
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save Histogram"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {

		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			modifier->throwException(tr("Could not open file for writing: %1").arg(file.errorString()));

		QTextStream stream(&file);

		FloatType binSize = (modifier->xAxisRangeEnd() - modifier->xAxisRangeStart()) / modifier->histogramData().size();
		stream << "# " << modifier->sourceProperty().name() << " histogram (bin size: " << binSize << ")" << endl;
		for(int i = 0; i < modifier->histogramData().size(); i++) {
			stream << (binSize * (FloatType(i) + FloatType(0.5)) + modifier->xAxisRangeStart()) << " " <<
					modifier->histogramData()[i] << endl;
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
