///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>
#include <qwt/qwt_plot_spectrogram.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_scale_engine.h>
#include <qwt/qwt_matrix_raster_data.h>
#include <qwt/qwt_color_map.h>
#include <qwt/qwt_scale_widget.h>
#include <qwt/qwt_plot_layout.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(BinAndReduceModifierEditor, ParticleModifierEditor);
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

	ParticlePropertyParameterUI* sourcePropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::sourceProperty));
	layout->addWidget(new QLabel(tr("Particle property:"), rollout));
	layout->addWidget(sourcePropertyUI->comboBox());

	QGridLayout* gridlayout = new QGridLayout();
	gridlayout->addWidget(new QLabel(tr("Reduction operation:"), rollout), 0, 0);
	VariantComboBoxParameterUI* reductionOperationPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::reductionOperation));
    reductionOperationPUI->comboBox()->addItem(tr("mean"), qVariantFromValue(BinAndReduceModifier::RED_MEAN));
    reductionOperationPUI->comboBox()->addItem(tr("sum"), qVariantFromValue(BinAndReduceModifier::RED_SUM));
    reductionOperationPUI->comboBox()->addItem(tr("sum divided by bin volume"), qVariantFromValue(BinAndReduceModifier::RED_SUM_VOL));
    reductionOperationPUI->comboBox()->addItem(tr("min"), qVariantFromValue(BinAndReduceModifier::RED_MIN));
    reductionOperationPUI->comboBox()->addItem(tr("max"), qVariantFromValue(BinAndReduceModifier::RED_MAX));
    gridlayout->addWidget(reductionOperationPUI->comboBox(), 0, 1);
    layout->addLayout(gridlayout);

	gridlayout = new QGridLayout();
	gridlayout->addWidget(new QLabel(tr("Binning direction:"), rollout), 0, 0);
	VariantComboBoxParameterUI* binDirectionPUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::binDirection));
    binDirectionPUI->comboBox()->addItem("cell vector 1", qVariantFromValue(BinAndReduceModifier::CELL_VECTOR_1));
    binDirectionPUI->comboBox()->addItem("cell vector 2", qVariantFromValue(BinAndReduceModifier::CELL_VECTOR_2));
    binDirectionPUI->comboBox()->addItem("cell vector 3", qVariantFromValue(BinAndReduceModifier::CELL_VECTOR_3));
    binDirectionPUI->comboBox()->addItem("vectors 1 and 2", qVariantFromValue(BinAndReduceModifier::CELL_VECTORS_1_2));
    binDirectionPUI->comboBox()->addItem("vectors 1 and 3", qVariantFromValue(BinAndReduceModifier::CELL_VECTORS_1_3));
    binDirectionPUI->comboBox()->addItem("vectors 2 and 3", qVariantFromValue(BinAndReduceModifier::CELL_VECTORS_2_3));
    gridlayout->addWidget(binDirectionPUI->comboBox(), 0, 1);
    layout->addLayout(gridlayout);

	_firstDerivativePUI = new BooleanParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::firstDerivative));
	_firstDerivativePUI->setEnabled(false);
	layout->addWidget(_firstDerivativePUI->checkBox());

	gridlayout = new QGridLayout();
	gridlayout->setContentsMargins(0,0,0,0);
	gridlayout->setColumnStretch(1, 1);
	gridlayout->setColumnStretch(2, 1);

	// Number of bins parameters.
	IntegerParameterUI* numBinsXPUI = new IntegerParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::numberOfBinsX));
	gridlayout->addWidget(numBinsXPUI->label(), 0, 0);
	gridlayout->addLayout(numBinsXPUI->createFieldLayout(), 0, 1);
	_numBinsYPUI = new IntegerParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::numberOfBinsY));
	gridlayout->addLayout(_numBinsYPUI->createFieldLayout(), 0, 2);
	_numBinsYPUI->setEnabled(false);

	layout->addLayout(gridlayout);

	_plot = new QwtPlot();
	_plot->setMinimumHeight(240);
	_plot->setMaximumHeight(240);
	_plot->setCanvasBackground(Qt::white);
    _plot->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Floating);

	layout->addWidget(new QLabel(tr("Reduction:")));
	layout->addWidget(_plot);
	connect(this, &BinAndReduceModifierEditor::contentsReplaced, this, &BinAndReduceModifierEditor::plotData);

	QPushButton* saveDataButton = new QPushButton(tr("Save data"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &BinAndReduceModifierEditor::onSaveData);

	// Input.
	QGroupBox* inputBox = new QGroupBox(tr("Input"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(inputBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(inputBox);

	BooleanParameterUI* onlySelectedUI = new BooleanParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::onlySelected));
	sublayout->addWidget(onlySelectedUI->checkBox());

	// Axes.
	QGroupBox* axesBox = new QGroupBox(tr("Plot axes"), rollout);
	QVBoxLayout* axesSublayout = new QVBoxLayout(axesBox);
	axesSublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(axesBox);
    BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::fixPropertyAxisRange));
    axesSublayout->addWidget(rangeUI->checkBox());
        
    QHBoxLayout* hlayout = new QHBoxLayout();
    axesSublayout->addLayout(hlayout);
    FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::propertyAxisRangeStart));
    FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(BinAndReduceModifier::propertyAxisRangeEnd));
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
	if(event->sender() == editObject() && event->type() == ReferenceEvent::ObjectStatusChanged) {
		plotLater(this);
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
    if(modifier->is1D()) binDataSizeY = 1;
    size_t binDataSize = binDataSizeX * binDataSizeY;

    if(modifier->is1D()) {
    	_plot->setAxisTitle(QwtPlot::yRight, QString());
        _plot->enableAxis(QwtPlot::yRight, false);
    	_plot->setAxisTitle(QwtPlot::xBottom, tr("Position"));
        if(modifier->firstDerivative()) {
        	_plot->setAxisTitle(QwtPlot::yLeft, QStringLiteral("d(%1)/d(Position)").arg(modifier->sourceProperty().nameWithComponent()));
        }
        else {
        	_plot->setAxisTitle(QwtPlot::yLeft, modifier->sourceProperty().nameWithComponent());
        }

        if(!_plotCurve) {
            _plotCurve = new QwtPlotCurve();
            _plotCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
            _plotCurve->setStyle(QwtPlotCurve::Steps);
            _plotCurve->attach(_plot);
            _plotGrid = new QwtPlotGrid();
            _plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
            _plotGrid->attach(_plot);
        }
        _plotGrid->show();

        if(_plotRaster) _plotRaster->hide();

        if(modifier->binData().empty()) {
            _plotCurve->hide();
            return;
        }
        _plotCurve->show();

        QVector<QPointF> plotData(binDataSize + 1);
        double binSize = (modifier->xAxisRangeEnd() - modifier->xAxisRangeStart()) / binDataSize;
        for(int i = 1; i <= binDataSize; i++) {
            plotData[i].rx() = binSize * i + modifier->xAxisRangeStart();
            plotData[i].ry() = modifier->binData()[i-1];
        }
        plotData.front().rx() = modifier->xAxisRangeStart();
        plotData.front().ry() = modifier->binData().front();
        _plotCurve->setSamples(plotData);

		_plot->setAxisAutoScale(QwtPlot::xBottom);
        if(!modifier->fixPropertyAxisRange())
            _plot->setAxisAutoScale(QwtPlot::yLeft);
        else
            _plot->setAxisScale(QwtPlot::yLeft, modifier->propertyAxisRangeStart(), modifier->propertyAxisRangeEnd());
    }
    else {
        if(_plotCurve) _plotCurve->hide();
        if(_plotGrid) _plotGrid->hide();

        class ColorMap: public QwtLinearColorMap
        {
        public:
            ColorMap() : QwtLinearColorMap(Qt::darkBlue, Qt::darkRed) {
                addColorStop(0.2, Qt::blue);
                addColorStop(0.4, Qt::cyan);
                addColorStop(0.6, Qt::yellow);
                addColorStop(0.8, Qt::red);
            }
        };
    	_plot->setAxisTitle(QwtPlot::xBottom, tr("Position"));
    	_plot->setAxisTitle(QwtPlot::yLeft, tr("Position"));

        if(!_plotRaster) {
            _plotRaster = new QwtPlotSpectrogram();
            _plotRaster->attach(_plot);
            _plotRaster->setColorMap(new ColorMap());
            _rasterData = new QwtMatrixRasterData();
            _plotRaster->setData(_rasterData);

            QwtScaleWidget* rightAxis = _plot->axisWidget(QwtPlot::yRight);
            rightAxis->setColorBarEnabled(true);
            rightAxis->setColorBarWidth(20);
            _plot->plotLayout()->setAlignCanvasToScales(true);
        }

        if(modifier->binData().empty()) {
            _plotRaster->hide();
            return;
        }
        _plotRaster->show();

        _plot->enableAxis(QwtPlot::yRight);
        _rasterData->setValueMatrix(modifier->binData(), binDataSizeX);
        _rasterData->setInterval(Qt::XAxis, QwtInterval(modifier->xAxisRangeStart(), modifier->xAxisRangeEnd(), QwtInterval::ExcludeMaximum));
        _rasterData->setInterval(Qt::YAxis, QwtInterval(modifier->yAxisRangeStart(), modifier->yAxisRangeEnd(), QwtInterval::ExcludeMaximum));
        QwtInterval zInterval;
        if(!modifier->fixPropertyAxisRange()) {
            auto minmax = std::minmax_element(modifier->binData().begin(), modifier->binData().end());
            zInterval = QwtInterval(*minmax.first, *minmax.second, QwtInterval::ExcludeMaximum);
        }
        else {
            zInterval = QwtInterval(modifier->propertyAxisRangeStart(), modifier->propertyAxisRangeEnd(), QwtInterval::ExcludeMaximum);
        }
        _plot->axisScaleEngine(QwtPlot::yRight)->setAttribute(QwtScaleEngine::Inverted, zInterval.minValue() > zInterval.maxValue());
        _rasterData->setInterval(Qt::ZAxis, zInterval.normalized());
		_plot->setAxisScale(QwtPlot::xBottom, modifier->xAxisRangeStart(), modifier->xAxisRangeEnd());
		_plot->setAxisScale(QwtPlot::yLeft, modifier->yAxisRangeStart(), modifier->yAxisRangeEnd());
        _plot->axisWidget(QwtPlot::yRight)->setColorMap(zInterval.normalized(), new ColorMap());
        _plot->setAxisScale(QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue());
        _plot->setAxisTitle(QwtPlot::yRight, modifier->sourceProperty().name());
    }
 
    _plot->replot();
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
            stream << "# " << modifier->sourceProperty().nameWithComponent() << " bin size: " << binSizeX << endl;
			for(size_t i = 0; i < modifier->binData().size(); i++) {
                stream << (binSizeX * (FloatType(i) + 0.5f) + modifier->xAxisRangeStart()) << " " << modifier->binData()[i] << endl;
            }
        }
        else {
            stream << "# " << modifier->sourceProperty().nameWithComponent() << " bin size X: " << binDataSizeX << ", bin size Y: " << binDataSizeY << endl;
            for(int i = 0; i < binDataSizeY; i++) {
                for(int j = 0; j < binDataSizeX; j++) {
                    stream << modifier->binData()[i*binDataSizeX+j] << " ";
                }
                stream << endl;
            }
        }
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
