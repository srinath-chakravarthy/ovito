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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/modifier/grains2/GrainSegmentationModifier2.h>
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include "GrainSegmentationModifierEditor2.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(CrystalAnalysisGui, GrainSegmentationModifierEditor2, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(GrainSegmentationModifier2, GrainSegmentationModifierEditor2);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void GrainSegmentationModifierEditor2::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Grain segmentation"), rolloutParams);

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* structureBox = new QGroupBox(tr("Input"));
	layout->addWidget(structureBox);
	QGridLayout* sublayout1 = new QGridLayout(structureBox);
	sublayout1->setContentsMargins(4,4,4,4);
	sublayout1->setSpacing(4);
	sublayout1->setColumnStretch(1,1);

	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_onlySelectedParticles));
	sublayout1->addWidget(onlySelectedUI->checkBox(), 0, 0, 1, 2);

	QGroupBox* paramsBox = new QGroupBox(tr("Parameters"));
	layout->addWidget(paramsBox);
	QGridLayout* sublayout2 = new QGridLayout(paramsBox);
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setSpacing(4);
	sublayout2->setColumnStretch(1, 1);

	FloatParameterUI* misorientationThresholdUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_misorientationThreshold));
	sublayout2->addWidget(misorientationThresholdUI->label(), 0, 0);
	sublayout2->addLayout(misorientationThresholdUI->createFieldLayout(), 0, 1);

	FloatParameterUI* rmsdCutoffUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_rmsdCutoff));
	sublayout2->addWidget(rmsdCutoffUI->label(), 1, 0);
	sublayout2->addLayout(rmsdCutoffUI->createFieldLayout(), 1, 1);

	IntegerParameterUI* minGrainAtomCountUI = new IntegerParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_minGrainAtomCount));
	sublayout2->addWidget(minGrainAtomCountUI->label(), 2, 0);
	sublayout2->addLayout(minGrainAtomCountUI->createFieldLayout(), 2, 1);

	QGroupBox* outputBox = new QGroupBox(tr("Output"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(outputBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(outputBox);

	// Output controls.
	BooleanParameterUI* outputOrientationUI = new BooleanParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_outputLocalOrientations));
	sublayout->addWidget(outputOrientationUI->checkBox());
	outputOrientationUI->checkBox()->setText(tr("Local lattice orientation"));

	BooleanGroupBoxParameterUI* generateMeshUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_outputPartitionMesh));
	generateMeshUI->groupBox()->setTitle(tr("Generate boundary mesh"));
	sublayout2 = new QGridLayout(generateMeshUI->childContainer());
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setColumnStretch(1, 1);
	layout->addWidget(generateMeshUI->groupBox());

	FloatParameterUI* radiusUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_probeSphereRadius));
	sublayout2->addWidget(radiusUI->label(), 0, 0);
	sublayout2->addLayout(radiusUI->createFieldLayout(), 0, 1);

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_smoothingLevel));
	sublayout2->addWidget(smoothingLevelUI->label(), 1, 0);
	sublayout2->addLayout(smoothingLevelUI->createFieldLayout(), 1, 1);

	// Status label.
	layout->addWidget(statusLabel());

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this, true);
	layout->addSpacing(10);
	layout->addWidget(structureTypesPUI->tableWidget());

	_histogramPlot = new QCustomPlot();
	_histogramPlot->setMinimumHeight(240);
	_histogramPlot->setInteraction(QCP::iRangeDrag, true);
	_histogramPlot->axisRect()->setRangeDrag(Qt::Horizontal);
	_histogramPlot->setInteraction(QCP::iRangeZoom, true);
	_histogramPlot->axisRect()->setRangeZoom(Qt::Horizontal);
	_histogramPlot->xAxis->setLabel(tr("RMSD"));
	_histogramPlot->yAxis->setLabel(tr("Count"));
	_histogramPlot->addGraph();
	_histogramPlot->graph()->setBrush(QBrush(QColor(255, 160, 100)));

	_rmsdCutoffMarker = new QCPItemStraightLine(_histogramPlot);
	_rmsdCutoffMarker->setVisible(false);
	QPen markerPen;
	markerPen.setColor(QColor(255, 40, 30));
	markerPen.setStyle(Qt::DotLine);
	markerPen.setWidth(2);
	_rmsdCutoffMarker->setPen(markerPen);
	_histogramPlot->addItem(_rmsdCutoffMarker);

	layout->addSpacing(10);
	layout->addWidget(_histogramPlot);
	connect(this, &GrainSegmentationModifierEditor2::contentsReplaced, this, &GrainSegmentationModifierEditor2::plotHistogram);

	// Open a sub-editor for the mesh display object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier2::_meshDisplay), rolloutParams.after(rollout));
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool GrainSegmentationModifierEditor2::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject() && (event->type() == ReferenceEvent::ObjectStatusChanged || event->type() == ReferenceEvent::TargetChanged)) {
		plotHistogram();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void GrainSegmentationModifierEditor2::plotHistogram()
{
	GrainSegmentationModifier2* modifier = static_object_cast<GrainSegmentationModifier2>(editObject());
	if(!modifier)
		return;

	if(modifier->rmsdHistogramData().empty())
		return;

	QVector<double> xdata(modifier->rmsdHistogramData().size());
	QVector<double> ydata(modifier->rmsdHistogramData().size());
	double binSize = modifier->rmsdHistogramBinSize();
	double maxHistogramData = 0;
	for(int i = 0; i < xdata.size(); i++) {
		xdata[i] = binSize * ((double)i + 0.5);
		ydata[i] = modifier->rmsdHistogramData()[i];
		maxHistogramData = std::max(maxHistogramData, ydata[i]);
	}
	_histogramPlot->graph()->setLineStyle(QCPGraph::lsStepCenter);
	_histogramPlot->graph()->setData(xdata, ydata);

	if(modifier->rmsdCutoff() > 0) {
		_rmsdCutoffMarker->setVisible(true);
		_rmsdCutoffMarker->point1->setCoords(modifier->rmsdCutoff(), 0);
		_rmsdCutoffMarker->point2->setCoords(modifier->rmsdCutoff(), 1);
	}
	else {
		_rmsdCutoffMarker->setVisible(false);
	}

	_histogramPlot->xAxis->setRange(0, binSize * xdata.size());
	_histogramPlot->yAxis->setRange(0, maxHistogramData);
	_histogramPlot->replot(QCustomPlot::rpQueued);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

