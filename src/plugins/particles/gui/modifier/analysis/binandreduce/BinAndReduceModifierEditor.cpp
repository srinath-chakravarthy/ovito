///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/FloatParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <gui/properties/IntegerRadioButtonParameterUI.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include <gui/mainwin/MainWindow.h>
#include <plugins/particles/modifier/analysis/binandreduce/BinAndReduceModifier.h>
#include <plugins/particles/gui/util/ParticlePropertyParameterUI.h>
#include "BinAndReduceModifierEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, BinAndReduceModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(BinAndReduceModifier, BinAndReduceModifierEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void BinAndReduceModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Bin and reduce"), rolloutParams, "particles.modifiers.bin_and_reduce.html");

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	ParticlePropertyParameterUI* sourcePropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_sourceProperty));
	layout->addWidget(new QLabel(tr("Particle property:"), rollout));
	layout->addWidget(sourcePropertyUI->comboBox());

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->addWidget(new QLabel(tr("Reduction operation:"), rollout), 0, 0);
	VariantComboBoxParameterUI* reductionOperationPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_reductionOperation));
    reductionOperationPUI->comboBox()->addItem(tr("mean"), qVariantFromValue(BinAndReduceModifier::RED_MEAN));
    reductionOperationPUI->comboBox()->addItem(tr("sum"), qVariantFromValue(BinAndReduceModifier::RED_SUM));
    reductionOperationPUI->comboBox()->addItem(tr("sum divided by bin volume"), qVariantFromValue(BinAndReduceModifier::RED_SUM_VOL));
    reductionOperationPUI->comboBox()->addItem(tr("min"), qVariantFromValue(BinAndReduceModifier::RED_MIN));
    reductionOperationPUI->comboBox()->addItem(tr("max"), qVariantFromValue(BinAndReduceModifier::RED_MAX));
    gridlayout->addWidget(reductionOperationPUI->comboBox(), 0, 1);
    layout->addLayout(gridlayout);

	gridlayout = new QGridLayout();
	gridlayout->addWidget(new QLabel(tr("Binning direction:"), rollout), 0, 0);
	VariantComboBoxParameterUI* binDirectionPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_binDirection));
    binDirectionPUI->comboBox()->addItem("cell vector 1", qVariantFromValue(BinAndReduceModifier::CELL_VECTOR_1));
    binDirectionPUI->comboBox()->addItem("cell vector 2", qVariantFromValue(BinAndReduceModifier::CELL_VECTOR_2));
    binDirectionPUI->comboBox()->addItem("cell vector 3", qVariantFromValue(BinAndReduceModifier::CELL_VECTOR_3));
    binDirectionPUI->comboBox()->addItem("vectors 1 and 2", qVariantFromValue(BinAndReduceModifier::CELL_VECTORS_1_2));
    binDirectionPUI->comboBox()->addItem("vectors 1 and 3", qVariantFromValue(BinAndReduceModifier::CELL_VECTORS_1_3));
    binDirectionPUI->comboBox()->addItem("vectors 2 and 3", qVariantFromValue(BinAndReduceModifier::CELL_VECTORS_2_3));
    gridlayout->addWidget(binDirectionPUI->comboBox(), 0, 1);
    layout->addLayout(gridlayout);

	_firstDerivativePUI = new BooleanParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_firstDerivative));
	_firstDerivativePUI->setEnabled(false);
	layout->addWidget(_firstDerivativePUI->checkBox());

	gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);
	gridlayout->setColumnStretch(2, 1);

	// Number of bins parameters.
	IntegerParameterUI* numBinsXPUI = new IntegerParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_numberOfBinsX));
	gridlayout->addWidget(numBinsXPUI->label(), 0, 0);
	gridlayout->addLayout(numBinsXPUI->createFieldLayout(), 0, 1);
	_numBinsYPUI = new IntegerParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_numberOfBinsY));
	gridlayout->addLayout(_numBinsYPUI->createFieldLayout(), 0, 2);
	_numBinsYPUI->setEnabled(false);

	layout->addLayout(gridlayout);

	_averagesPlot = new QCustomPlot();
	_averagesPlot->setMinimumHeight(240);
    _averagesPlot->axisRect()->setRangeDrag(Qt::Vertical);
    _averagesPlot->axisRect()->setRangeZoom(Qt::Vertical);
	_averagesPlot->xAxis->setLabel("Position");
    connect(_averagesPlot->yAxis, SIGNAL(rangeChanged(const QCPRange&)), this, SLOT(updatePropertyAxisRange(const QCPRange&)));

	layout->addWidget(new QLabel(tr("Reduction:")));
	layout->addWidget(_averagesPlot);
	connect(this, &BinAndReduceModifierEditor::contentsReplaced, this, &BinAndReduceModifierEditor::plotData);

	QPushButton* saveDataButton = new QPushButton(tr("Save data"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &BinAndReduceModifierEditor::onSaveData);

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
    BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_fixPropertyAxisRange));
    axesSublayout->addWidget(rangeUI->checkBox());
        
    QHBoxLayout* hlayout = new QHBoxLayout();
    axesSublayout->addLayout(hlayout);
    FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_propertyAxisRangeStart));
    FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::_propertyAxisRangeEnd));
    hlayout->addWidget(new QLabel(tr("From:")));
    hlayout->addLayout(startPUI->createFieldLayout());
    hlayout->addSpacing(12);
    hlayout->addWidget(new QLabel(tr("To:")));
    hlayout->addLayout(endPUI->createFieldLayout());
    startPUI->setEnabled(false);
    endPUI->setEnabled(false);
    connect(rangeUI->checkBox(), &QCheckBox::toggled, startPUI, &FloatParameterUI::setEnabled);
    connect(rangeUI->checkBox(), &QCheckBox::toggled, endPUI, &FloatParameterUI::setEnabled);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	connect(this, &BinAndReduceModifierEditor::contentsChanged, this, &BinAndReduceModifierEditor::updateWidgets);
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool BinAndReduceModifierEditor::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(event->sender() == editObject()
			&& event->type() == ReferenceEvent::ObjectStatusChanged) {
		plotData();
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Plots the data computed by the modifier.
******************************************************************************/
void BinAndReduceModifierEditor::plotData()
{
	BinAndReduceModifier* modifier = static_object_cast<BinAndReduceModifier>(editObject());
	if(!modifier)
		return;

	int binDataSizeX = std::max(1, modifier->numberOfBinsX());
	int binDataSizeY = std::max(1, modifier->numberOfBinsY());
    if (modifier->is1D()) binDataSizeY = 1;
    size_t binDataSize = binDataSizeX*binDataSizeY;

    if (modifier->is1D()) {
        // If previous plot was a color map, delete and create graph.
        if (!_averagesGraph) {
            if (_averagesColorMap) {
                _averagesPlot->removePlottable(_averagesColorMap);
                _averagesColorMap = nullptr;
            }
            _averagesGraph = _averagesPlot->addGraph();
        }

        _averagesPlot->setInteraction(QCP::iRangeDrag, true);
        _averagesPlot->axisRect()->setRangeDrag(Qt::Vertical);
        _averagesPlot->setInteraction(QCP::iRangeZoom, true);
        _averagesPlot->axisRect()->setRangeZoom(Qt::Vertical);
        if(modifier->firstDerivative()) {
            _averagesPlot->yAxis->setLabel("d( "+modifier->sourceProperty().name()+" )/d( Position )");
        }
        else {
            _averagesPlot->yAxis->setLabel(modifier->sourceProperty().name());
        }

        if(modifier->binData().empty())
            return;

        QVector<double> xdata(binDataSize+2);
        QVector<double> ydata(binDataSize+2);
        double binSize = ( modifier->xAxisRangeEnd() - modifier->xAxisRangeStart() ) / binDataSize;
        for(int i = 0; i < binDataSize; i++) {
            xdata[i+1] = binSize * ((double)i + 0.5) + modifier->xAxisRangeStart();
            ydata[i+1] = modifier->binData()[i];
        }
        xdata.front() = modifier->xAxisRangeStart();
        ydata.front() = modifier->binData().front();
        xdata.back() = modifier->xAxisRangeEnd();
        ydata.back() = modifier->binData().back();
        _averagesPlot->graph()->setLineStyle(QCPGraph::lsStepCenter);
        _averagesPlot->graph()->setData(xdata, ydata);

        // Check if range is already correct, because setRange emits the rangeChanged signal
        // which is to be avoided if the range is not determined automatically.
        _rangeUpdate = false;
        _averagesPlot->xAxis->setRange(modifier->xAxisRangeStart(), modifier->xAxisRangeEnd());
        _averagesPlot->yAxis->setRange(modifier->propertyAxisRangeStart(), modifier->propertyAxisRangeEnd());
        _rangeUpdate = true;
    }
    else {
        // If previous plot was a graph, delete and create color map.
        if (!_averagesColorMap) {
            if (_averagesGraph) {
                _averagesPlot->removeGraph(_averagesGraph);
                _averagesGraph = nullptr;
            }
            _averagesColorMap = new QCPColorMap(_averagesPlot->xAxis, _averagesPlot->yAxis);
            _averagesPlot->addPlottable(_averagesColorMap);
        }

        _averagesPlot->setInteraction(QCP::iRangeDrag, false);
        _averagesPlot->setInteraction(QCP::iRangeZoom, false);
        _averagesPlot->yAxis->setLabel("Position");

        if(modifier->binData().empty())
            return;

        _averagesColorMap->setInterpolate(false);
        _averagesColorMap->setTightBoundary(false);
        _averagesColorMap->setGradient(QCPColorGradient::gpJet);

        _averagesColorMap->data()->setSize(binDataSizeX, binDataSizeY);
        _averagesColorMap->data()->setRange(QCPRange(modifier->xAxisRangeStart(), modifier->xAxisRangeEnd()),
                                            QCPRange(modifier->yAxisRangeStart(), modifier->yAxisRangeEnd()));

        _averagesPlot->xAxis->setRange(QCPRange(modifier->xAxisRangeStart(), modifier->xAxisRangeEnd()));
        _averagesPlot->yAxis->setRange(QCPRange(modifier->yAxisRangeStart(), modifier->yAxisRangeEnd()));

        // Copy data to QCPColorMapData object.
        for (int j = 0; j < binDataSizeY; j++) {
            for (int i = 0; i < binDataSizeX; i++) {
                _averagesColorMap->data()->setCell(i, j, modifier->binData()[j*binDataSizeX+i]);
            }
        }

        // Check if range is already correct, because setRange emits the rangeChanged signal
        // which is to be avoided if the range is not determined automatically.
        _rangeUpdate = false;
        _averagesColorMap->setDataRange(QCPRange(modifier->propertyAxisRangeStart(), modifier->propertyAxisRangeEnd()));
        _rangeUpdate = true;
    }
    
    _averagesPlot->replot(QCustomPlot::rpQueued);
}

/******************************************************************************
* Enable/disable the editor for number of y-bins and the first derivative
* button
******************************************************************************/
void BinAndReduceModifierEditor::updateWidgets()
{
	BinAndReduceModifier* modifier = static_object_cast<BinAndReduceModifier>(editObject());
	if(!modifier)
		return;

    _numBinsYPUI->setEnabled(!modifier->is1D());
    _firstDerivativePUI->setEnabled(modifier->is1D());
}

/******************************************************************************
* Keep y-axis range updated
******************************************************************************/
void BinAndReduceModifierEditor::updatePropertyAxisRange(const QCPRange &newRange)
{
	if (_rangeUpdate) {
		BinAndReduceModifier* modifier = static_object_cast<BinAndReduceModifier>(editObject());
		if(!modifier)
			return;
        if (!modifier->is1D())
            return;

		// Fix range if user modifies the range by a mouse action in QCustomPlot
		modifier->setFixPropertyAxisRange(true);
		modifier->setPropertyAxisRange(newRange.lower, newRange.upper);
	}
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void BinAndReduceModifierEditor::onSaveData()
{
	BinAndReduceModifier* modifier = static_object_cast<BinAndReduceModifier>(editObject());
	if(!modifier)
		return;

	if(modifier->binData().empty())
		return;

	QString fileName = QFileDialog::getSaveFileName(mainWindow(),
	    tr("Save Data"), QString(), 
        tr("Text files (*.txt);;All files (*)"));
	if(fileName.isEmpty())
		return;

	try {

		QFile file(fileName);
		if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
			throwException(tr("Could not open file for writing: %1").arg(file.errorString()));

		int binDataSizeX = std::max(1, modifier->numberOfBinsX());
		int binDataSizeY = std::max(1, modifier->numberOfBinsY());
        if (modifier->is1D()) binDataSizeY = 1;
		FloatType binSizeX = (modifier->xAxisRangeEnd() - modifier->xAxisRangeStart()) / binDataSizeX;
		FloatType binSizeY = (modifier->yAxisRangeEnd() - modifier->yAxisRangeStart()) / binDataSizeY;

		QTextStream stream(&file);
        if (binDataSizeY == 1) {
            stream << "# " << modifier->sourceProperty().name() << " bin size: " << binSizeX << endl;
			for(size_t i = 0; i < modifier->binData().size(); i++) {
                stream << (binSizeX * (FloatType(i) + 0.5f) + modifier->xAxisRangeStart()) << " " << modifier->binData()[i] << endl;
            }
        }
        else {
            stream << "# " << modifier->sourceProperty().name() << " bin size X: " << binDataSizeX << ", bin size Y: " << binDataSizeY << endl;
            for(int i = 0; i < binDataSizeY; i++) {
                for(int j = 0; j < binDataSizeX; j++) {
                    stream << modifier->binData()[i*binDataSizeX+j] << " ";
                }
                stream << endl;
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
