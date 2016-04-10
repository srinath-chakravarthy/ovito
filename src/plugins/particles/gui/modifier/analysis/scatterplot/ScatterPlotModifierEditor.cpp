///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
//  Copyright (2014) Lars Pastewka
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
#include <plugins/particles/modifier/analysis/scatterplot/ScatterPlotModifier.h>
#include <plugins/particles/gui/util/ParticlePropertyParameterUI.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/mainwin/MainWindow.h>
#include "ScatterPlotModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, ScatterPlotModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ScatterPlotModifier, ScatterPlotModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ScatterPlotModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Scatter plot"), rolloutParams, "particles.modifiers.scatter_plot.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	ParticlePropertyParameterUI* xPropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_xAxisProperty));
	layout->addWidget(new QLabel(tr("X-axis property:"), rollout));
	layout->addWidget(xPropertyUI->comboBox());
	ParticlePropertyParameterUI* yPropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_yAxisProperty));
	layout->addWidget(new QLabel(tr("Y-axis property:"), rollout));
	layout->addWidget(yPropertyUI->comboBox());

	_scatterPlot = new QCustomPlot();
	_scatterPlot->setMinimumHeight(240);
	_scatterPlot->setInteraction(QCP::iRangeDrag, true);
	_scatterPlot->axisRect()->setRangeDrag(Qt::Orientations(Qt::Horizontal | Qt::Vertical));
	_scatterPlot->setInteraction(QCP::iRangeZoom, true);
	_scatterPlot->axisRect()->setRangeZoom(Qt::Orientations(Qt::Horizontal | Qt::Vertical));

	QPen markerPen;
	markerPen.setColor(QColor(255, 40, 30));
	markerPen.setStyle(Qt::DotLine);
	markerPen.setWidth(2);
	_selectionXAxisRangeStartMarker = new QCPItemStraightLine(_scatterPlot);
	_selectionXAxisRangeEndMarker = new QCPItemStraightLine(_scatterPlot);
	_selectionXAxisRangeStartMarker->setVisible(false);
	_selectionXAxisRangeEndMarker->setVisible(false);
	_selectionXAxisRangeStartMarker->setPen(markerPen);
	_selectionXAxisRangeEndMarker->setPen(markerPen);
	_scatterPlot->addItem(_selectionXAxisRangeStartMarker);
	_scatterPlot->addItem(_selectionXAxisRangeEndMarker);
	_selectionYAxisRangeStartMarker = new QCPItemStraightLine(_scatterPlot);
	_selectionYAxisRangeEndMarker = new QCPItemStraightLine(_scatterPlot);
	_selectionYAxisRangeStartMarker->setVisible(false);
	_selectionYAxisRangeEndMarker->setVisible(false);
	_selectionYAxisRangeStartMarker->setPen(markerPen);
	_selectionYAxisRangeEndMarker->setPen(markerPen);
	_scatterPlot->addItem(_selectionYAxisRangeStartMarker);
	_scatterPlot->addItem(_selectionYAxisRangeEndMarker);
	connect(_scatterPlot->xAxis, SIGNAL(rangeChanged(const QCPRange&)), this, SLOT(updateXAxisRange(const QCPRange&)));
	connect(_scatterPlot->yAxis, SIGNAL(rangeChanged(const QCPRange&)), this, SLOT(updateYAxisRange(const QCPRange&)));

	layout->addWidget(new QLabel(tr("Scatter plot:")));
	layout->addWidget(_scatterPlot);
	connect(this, &ScatterPlotModifierEditor::contentsReplaced, this, &ScatterPlotModifierEditor::plotScatterPlot);

	QPushButton* saveDataButton = new QPushButton(tr("Save scatter plot data"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &ScatterPlotModifierEditor::onSaveData);

	// Selection.
	QGroupBox* selectionBox = new QGroupBox(tr("Selection"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(selectionBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(selectionBox);

	BooleanParameterUI* selectInRangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_selectXAxisInRange));
	sublayout->addWidget(selectInRangeUI->checkBox());

	QHBoxLayout* hlayout = new QHBoxLayout();
	sublayout->addLayout(hlayout);
	FloatParameterUI* selRangeStartPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_selectionXAxisRangeStart));
	FloatParameterUI* selRangeEndPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_selectionXAxisRangeEnd));
	hlayout->addWidget(new QLabel(tr("From:")));
	hlayout->addLayout(selRangeStartPUI->createFieldLayout());
	hlayout->addSpacing(12);
	hlayout->addWidget(new QLabel(tr("To:")));
	hlayout->addLayout(selRangeEndPUI->createFieldLayout());
	selRangeStartPUI->setEnabled(false);
	selRangeEndPUI->setEnabled(false);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeStartPUI, &FloatParameterUI::setEnabled);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeEndPUI, &FloatParameterUI::setEnabled);

	selectInRangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_selectYAxisInRange));
	sublayout->addWidget(selectInRangeUI->checkBox());

	hlayout = new QHBoxLayout();
	sublayout->addLayout(hlayout);
	selRangeStartPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_selectionYAxisRangeStart));
	selRangeEndPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_selectionYAxisRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_fixXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_xAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_xAxisRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_fixYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_yAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::_yAxisRangeEnd));
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
bool ScatterPlotModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		plotScatterPlot();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the scatter plot computed by the modifier.
******************************************************************************/
void ScatterPlotModifierEditor::plotScatterPlot()
{
	ScatterPlotModifier* modifier = static_object_cast<ScatterPlotModifier>(editObject());
	if(!modifier)
		return;

	_scatterPlot->xAxis->setLabel(modifier->xAxisProperty().name());
	_scatterPlot->yAxis->setLabel(modifier->yAxisProperty().name());

	if(modifier->numberOfParticleTypeIds() == 0)
		return;

	// Make sure we have the correct number of graphs. (One graph per particle id.)
	while (_scatterPlot->graphCount() > modifier->numberOfParticleTypeIds()) {
		_scatterPlot->removeGraph(_scatterPlot->graph(0));
	}
	while (_scatterPlot->graphCount() < modifier->numberOfParticleTypeIds()) {
		_scatterPlot->addGraph();
		_scatterPlot->graph()->setLineStyle(QCPGraph::lsNone);
	}

	for (int i = 0; i < modifier->numberOfParticleTypeIds(); i++) {
		if (modifier->hasColor(i)) {
			_scatterPlot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc,
																	modifier->color(i), 5.0));
		}
		else {
			_scatterPlot->graph(i)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDisc, 5.0));
		}
		_scatterPlot->graph(i)->setData(modifier->xData(i), modifier->yData(i));
	}

	// Check if range is already correct, because setRange emits the rangeChanged signa
	// which is to be avoided if the range is not determined automatically.
	_rangeUpdate = false;
	_scatterPlot->xAxis->setRange(modifier->xAxisRangeStart(), modifier->xAxisRangeEnd());
	_scatterPlot->yAxis->setRange(modifier->yAxisRangeStart(), modifier->yAxisRangeEnd());
	_rangeUpdate = true;

	if(modifier->selectXAxisInRange()) {
		_selectionXAxisRangeStartMarker->setVisible(true);
		_selectionXAxisRangeEndMarker->setVisible(true);
		_selectionXAxisRangeStartMarker->point1->setCoords(modifier->selectionXAxisRangeStart(), 0);
		_selectionXAxisRangeStartMarker->point2->setCoords(modifier->selectionXAxisRangeStart(), 1);
		_selectionXAxisRangeEndMarker->point1->setCoords(modifier->selectionXAxisRangeEnd(), 0);
		_selectionXAxisRangeEndMarker->point2->setCoords(modifier->selectionXAxisRangeEnd(), 1);
	}
	else {
		_selectionXAxisRangeStartMarker->setVisible(false);
		_selectionXAxisRangeEndMarker->setVisible(false);
	}

	if(modifier->selectYAxisInRange()) {
		_selectionYAxisRangeStartMarker->setVisible(true);
		_selectionYAxisRangeEndMarker->setVisible(true);
		_selectionYAxisRangeStartMarker->point1->setCoords(0, modifier->selectionYAxisRangeStart());
		_selectionYAxisRangeStartMarker->point2->setCoords(1, modifier->selectionYAxisRangeStart());
		_selectionYAxisRangeEndMarker->point1->setCoords(0, modifier->selectionYAxisRangeEnd());
		_selectionYAxisRangeEndMarker->point2->setCoords(1, modifier->selectionYAxisRangeEnd());
	}
	else {
		_selectionYAxisRangeStartMarker->setVisible(false);
		_selectionYAxisRangeEndMarker->setVisible(false);
	}

	_scatterPlot->replot(QCustomPlot::rpQueued);
}

/******************************************************************************
* Keep x-axis range updated
******************************************************************************/
void ScatterPlotModifierEditor::updateXAxisRange(const QCPRange &newRange)
{
	if (_rangeUpdate) {
		ScatterPlotModifier* modifier = static_object_cast<ScatterPlotModifier>(editObject());
		if(!modifier)
			return;

		// Fix range if user modifies the range by a mouse action in QCustomPlot
		modifier->setFixXAxisRange(true);
		modifier->setXAxisRange(newRange.lower, newRange.upper);
	}
}

/******************************************************************************
* Keep y-axis range updated
******************************************************************************/
void ScatterPlotModifierEditor::updateYAxisRange(const QCPRange &newRange)
{
	if (_rangeUpdate) {
		ScatterPlotModifier* modifier = static_object_cast<ScatterPlotModifier>(editObject());
		if(!modifier)
			return;

		// Fix range if user modifies the range by a mouse action in QCustomPlot
		modifier->setFixYAxisRange(true);
		modifier->setYAxisRange(newRange.lower, newRange.upper);
	}
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void ScatterPlotModifierEditor::onSaveData()
{
	ScatterPlotModifier* modifier = static_object_cast<ScatterPlotModifier>(editObject());
	if(!modifier)
		return;

	if(modifier->numberOfParticleTypeIds() == 0)
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save Scatter Plot"), QString(), tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {

		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			modifier->throwException(tr("Could not open file for writing: %1").arg(file.errorString()));

		QTextStream stream(&file);

		stream << "# " << modifier->xAxisProperty().name() << " " << modifier->yAxisProperty().name() << endl;
		for(int typeId = 0; typeId < modifier->numberOfParticleTypeIds(); typeId++) {
			stream << "# Data for particle type id " << typeId << " follow." << endl;
			for(int i = 0; i < modifier->xData(typeId).size(); i++) {
				stream << modifier->xData(typeId)[i] << " " << modifier->yData(typeId)[i] << endl;
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
