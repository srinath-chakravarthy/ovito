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
#include <plugins/particles/gui/modifier/analysis/StructureListParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/properties/SubObjectParameterUI.h>
#include <gui/properties/BooleanGroupBoxParameterUI.h>
#include "../../modifier/grains/GrainSegmentationEngine.h"
#include "../../modifier/grains/GrainSegmentationModifier.h"
#include "GrainSegmentationModifierEditor.h"

#include <3rdparty/qwt/qwt_plot.h>
#include <3rdparty/qwt/qwt_plot_curve.h>
#include <3rdparty/qwt/qwt_plot_zoneitem.h>
#include <3rdparty/qwt/qwt_plot_grid.h>


namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_OVITO_OBJECT(GrainSegmentationModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(GrainSegmentationModifier, GrainSegmentationModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void GrainSegmentationModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Grain segmentation"), rolloutParams);

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* structureBox = new QGroupBox(tr("Input crystal structure"));
	layout->addWidget(structureBox);
	QGridLayout* sublayout1 = new QGridLayout(structureBox);
	sublayout1->setContentsMargins(4,4,4,4);
	sublayout1->setSpacing(4);
	sublayout1->setColumnStretch(1,1);
	VariantComboBoxParameterUI* crystalStructureUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::inputCrystalStructure));

	crystalStructureUI->comboBox()->addItem(tr("Face-centered cubic (FCC)"), QVariant::fromValue((int)GrainSegmentationEngine::FCC));
	crystalStructureUI->comboBox()->addItem(tr("Hexagonal close-packed (HCP)"), QVariant::fromValue((int)GrainSegmentationEngine::HCP));
	crystalStructureUI->comboBox()->addItem(tr("Body-centered cubic (BCC)"), QVariant::fromValue((int)GrainSegmentationEngine::BCC));
	crystalStructureUI->comboBox()->addItem(tr("Simple cubic (SC)"), QVariant::fromValue((int)GrainSegmentationEngine::SC));
	sublayout1->addWidget(crystalStructureUI->comboBox(), 0, 0, 1, 2);

#if 0
	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::_onlySelectedParticles));
	sublayout1->addWidget(onlySelectedUI->checkBox(), 1, 0, 1, 2);
#endif

	QGroupBox* paramsBox = new QGroupBox(tr("Parameters"));
	layout->addWidget(paramsBox);
	QGridLayout* sublayout2 = new QGridLayout(paramsBox);
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setSpacing(4);
	sublayout2->setColumnStretch(1, 1);

	FloatParameterUI* misorientationThresholdUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::misorientationThreshold));
	sublayout2->addWidget(misorientationThresholdUI->label(), 0, 0);
	sublayout2->addLayout(misorientationThresholdUI->createFieldLayout(), 0, 1);

	FloatParameterUI* rmsdCutoffUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::rmsdCutoff));
	sublayout2->addWidget(rmsdCutoffUI->label(), 1, 0);
	sublayout2->addLayout(rmsdCutoffUI->createFieldLayout(), 1, 1);

	IntegerParameterUI* minGrainAtomCountUI = new IntegerParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::minGrainAtomCount));
	sublayout2->addWidget(minGrainAtomCountUI->label(), 2, 0);
	sublayout2->addLayout(minGrainAtomCountUI->createFieldLayout(), 2, 1);

	paramsBox = new QGroupBox(tr("Advanced parameters"));
	layout->addWidget(paramsBox);
	sublayout2 = new QGridLayout(paramsBox);
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setSpacing(4);
	sublayout2->setColumnStretch(1, 1);

	IntegerParameterUI* numOrientationSmoothingIterationsUI = new IntegerParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::numOrientationSmoothingIterations));
	sublayout2->addWidget(numOrientationSmoothingIterationsUI->label(), 0, 0);
	sublayout2->addLayout(numOrientationSmoothingIterationsUI->createFieldLayout(), 0, 1);

	FloatParameterUI* orientationSmoothingWeightUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::orientationSmoothingWeight));
	sublayout2->addWidget(orientationSmoothingWeightUI->label(), 1, 0);
	sublayout2->addLayout(orientationSmoothingWeightUI->createFieldLayout(), 1, 1);

	QGroupBox* outputBox = new QGroupBox(tr("Output"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(outputBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(outputBox);

	// Output controls.
	BooleanParameterUI* outputOrientationUI = new BooleanParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::outputLocalOrientations));
	sublayout->addWidget(outputOrientationUI->checkBox());
	outputOrientationUI->checkBox()->setText(tr("Local lattice orientation"));

	BooleanGroupBoxParameterUI* generateMeshUI = new BooleanGroupBoxParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::outputPartitionMesh));
	generateMeshUI->groupBox()->setTitle(tr("Generate boundary mesh"));
	sublayout2 = new QGridLayout(generateMeshUI->childContainer());
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setColumnStretch(1, 1);
	layout->addWidget(generateMeshUI->groupBox());

	FloatParameterUI* radiusUI = new FloatParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::probeSphereRadius));
	sublayout2->addWidget(radiusUI->label(), 0, 0);
	sublayout2->addLayout(radiusUI->createFieldLayout(), 0, 1);

	IntegerParameterUI* smoothingLevelUI = new IntegerParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::smoothingLevel));
	sublayout2->addWidget(smoothingLevelUI->label(), 1, 0);
	sublayout2->addLayout(smoothingLevelUI->createFieldLayout(), 1, 1);

	// Status label.
	layout->addWidget(statusLabel());

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(structureTypesPUI->tableWidget());

	_plot = new QwtPlot();
	_plot->setMinimumHeight(240);
	_plot->setMaximumHeight(240);
	_plot->setCanvasBackground(Qt::white);
	_plot->setAxisTitle(QwtPlot::xBottom, tr("RMSD"));	
	_plot->setAxisTitle(QwtPlot::yLeft, tr("Count"));	

	layout->addSpacing(10);
	layout->addWidget(_plot);
	connect(this, &GrainSegmentationModifierEditor::contentsReplaced, this, &GrainSegmentationModifierEditor::plotHistogram);

	// Open a sub-editor for the mesh display object.
	new SubObjectParameterUI(this, PROPERTY_FIELD(GrainSegmentationModifier::meshDisplay), rolloutParams.after(rollout));
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool GrainSegmentationModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject() && (event->type() == ReferenceEvent::ObjectStatusChanged || event->type() == ReferenceEvent::TargetChanged)) {
		plotHistogramLater(this);
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the histogram computed by the modifier.
******************************************************************************/
void GrainSegmentationModifierEditor::plotHistogram()
{
	GrainSegmentationModifier* modifier = static_object_cast<GrainSegmentationModifier>(editObject());

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

}	// End of namespace
}	// End of namespace
}	// End of namespace

