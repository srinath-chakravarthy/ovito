///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_spectrocurve.h>
#include <qwt/qwt_plot_zoneitem.h>
#include <qwt/qwt_plot_grid.h>
#include <qwt/qwt_color_map.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ScatterPlotModifierEditor, ParticleModifierEditor);
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

	ParticlePropertyParameterUI* xPropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::xAxisProperty));
	layout->addWidget(new QLabel(tr("X-axis property:"), rollout));
	layout->addWidget(xPropertyUI->comboBox());
	ParticlePropertyParameterUI* yPropertyUI = new ParticlePropertyParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::yAxisProperty));
	layout->addWidget(new QLabel(tr("Y-axis property:"), rollout));
	layout->addWidget(yPropertyUI->comboBox());

	_plot = new QwtPlot();
	_plot->setMinimumHeight(240);
	_plot->setMaximumHeight(240);
	_plot->setCanvasBackground(Qt::white);

	layout->addWidget(new QLabel(tr("Scatter plot:")));
	layout->addWidget(_plot);
	connect(this, &ScatterPlotModifierEditor::contentsReplaced, this, &ScatterPlotModifierEditor::plotScatterPlot);

	QPushButton* saveDataButton = new QPushButton(tr("Save scatter plot data"));
	layout->addWidget(saveDataButton);
	connect(saveDataButton, &QPushButton::clicked, this, &ScatterPlotModifierEditor::onSaveData);

	// Selection.
	QGroupBox* selectionBox = new QGroupBox(tr("Selection"), rollout);
	QVBoxLayout* sublayout = new QVBoxLayout(selectionBox);
	sublayout->setContentsMargins(4,4,4,4);
	layout->addWidget(selectionBox);

	BooleanParameterUI* selectInRangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectXAxisInRange));
	sublayout->addWidget(selectInRangeUI->checkBox());

	QHBoxLayout* hlayout = new QHBoxLayout();
	sublayout->addLayout(hlayout);
	FloatParameterUI* selRangeStartPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectionXAxisRangeStart));
	FloatParameterUI* selRangeEndPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectionXAxisRangeEnd));
	hlayout->addWidget(new QLabel(tr("From:")));
	hlayout->addLayout(selRangeStartPUI->createFieldLayout());
	hlayout->addSpacing(12);
	hlayout->addWidget(new QLabel(tr("To:")));
	hlayout->addLayout(selRangeEndPUI->createFieldLayout());
	selRangeStartPUI->setEnabled(false);
	selRangeEndPUI->setEnabled(false);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeStartPUI, &FloatParameterUI::setEnabled);
	connect(selectInRangeUI->checkBox(), &QCheckBox::toggled, selRangeEndPUI, &FloatParameterUI::setEnabled);

	selectInRangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectYAxisInRange));
	sublayout->addWidget(selectInRangeUI->checkBox());

	hlayout = new QHBoxLayout();
	sublayout->addLayout(hlayout);
	selRangeStartPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectionYAxisRangeStart));
	selRangeEndPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::selectionYAxisRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::fixXAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::xAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::xAxisRangeEnd));
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
		BooleanParameterUI* rangeUI = new BooleanParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::fixYAxisRange));
		axesSublayout->addWidget(rangeUI->checkBox());

		QHBoxLayout* hlayout = new QHBoxLayout();
		axesSublayout->addLayout(hlayout);
		FloatParameterUI* startPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::yAxisRangeStart));
		FloatParameterUI* endPUI = new FloatParameterUI(this, PROPERTY_FIELD(ScatterPlotModifier::yAxisRangeEnd));
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
		plotLater(this);
	}
	return ParticleModifierEditor::referenceEvent(source, event);
}

/******************************************************************************
* Replots the scatter plot computed by the modifier.
******************************************************************************/
void ScatterPlotModifierEditor::plotScatterPlot()
{
	ScatterPlotModifier* modifier = static_object_cast<ScatterPlotModifier>(editObject());

	if(!modifier) {
		if(_plotCurve) _plotCurve->hide();
		return;
	}

	_plot->setAxisTitle(QwtPlot::xBottom, modifier->xAxisProperty().nameWithComponent());
	_plot->setAxisTitle(QwtPlot::yLeft, modifier->yAxisProperty().nameWithComponent());

	if(!_plotCurve) {
		_plotCurve = new QwtPlotSpectroCurve();
	    _plotCurve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
		_plotCurve->setPenWidth(3);
		_plotCurve->attach(_plot);
		QwtPlotGrid* plotGrid = new QwtPlotGrid();
		plotGrid->setPen(Qt::gray, 0, Qt::DotLine);
		plotGrid->attach(_plot);
	}
	const auto& xyData = modifier->xyData();
	const auto& typeData = modifier->typeData();
	QVector<QwtPoint3D> plotData(xyData.size());
	for(int i = 0; i < plotData.size(); i++) {
		plotData[i].rx() = xyData[i].x();
		plotData[i].ry() = xyData[i].y();
		plotData[i].rz() = typeData.empty() ? 0 : typeData[i];
	}
	_plotCurve->setSamples(plotData);

	class ColorMap : public QwtColorMap {
		std::unordered_map<int,QRgb> _map;
	public:
		ColorMap(const std::map<int,Color>& map) {
			for(const auto& e : map) {
				int r = 255 * e.second.r();
				int g = 255 * e.second.g();
				int b = 255 * e.second.b();
				_map.insert(std::make_pair(e.first, qRgb(r,g,b)));
			}
		}
		virtual unsigned char colorIndex(const QwtInterval& interval, double value) const override { return 0; }
		virtual QRgb rgb(const QwtInterval& interval, double value) const override {
			auto iter = _map.find((int)value);
			if(iter != _map.end()) return iter->second;
			else return qRgb(0,0,200);
		}
	};
	_plotCurve->setColorMap(new ColorMap(modifier->colorMap()));

	if(modifier->selectXAxisInRange()) {
		if(!_selectionRangeX) {
			_selectionRangeX = new QwtPlotZoneItem();
			_selectionRangeX->setOrientation(Qt::Vertical);
			_selectionRangeX->setZ(_plotCurve->z() + 1);
			_selectionRangeX->attach(_plot);
		}
		_selectionRangeX->show();
		auto minmax = std::minmax(modifier->selectionXAxisRangeStart(), modifier->selectionXAxisRangeEnd());
		_selectionRangeX->setInterval(minmax.first, minmax.second);
	}
	else if(_selectionRangeX) {
		_selectionRangeX->hide();
	}

	if(modifier->selectYAxisInRange()) {
		if(!_selectionRangeY) {
			_selectionRangeY = new QwtPlotZoneItem();
			_selectionRangeY->setOrientation(Qt::Horizontal);
			_selectionRangeY->setZ(_plotCurve->z() + 2);
			_selectionRangeY->attach(_plot);
		}
		_selectionRangeY->show();
		auto minmax = std::minmax(modifier->selectionYAxisRangeStart(), modifier->selectionYAxisRangeEnd());
		_selectionRangeY->setInterval(minmax.first, minmax.second);
	}
	else if(_selectionRangeY) {
		_selectionRangeY->hide();
	}

	if(!modifier->fixXAxisRange())
		_plot->setAxisAutoScale(QwtPlot::xBottom);
	else
		_plot->setAxisScale(QwtPlot::xBottom, modifier->xAxisRangeStart(), modifier->xAxisRangeEnd());

	if(!modifier->fixYAxisRange())
		_plot->setAxisAutoScale(QwtPlot::yLeft);
	else
		_plot->setAxisScale(QwtPlot::yLeft, modifier->yAxisRangeStart(), modifier->yAxisRangeEnd());

	_plot->replot();
}

/******************************************************************************
* This is called when the user has clicked the "Save Data" button.
******************************************************************************/
void ScatterPlotModifierEditor::onSaveData()
{
	ScatterPlotModifier* modifier = static_object_cast<ScatterPlotModifier>(editObject());
	if(!modifier)
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

		if(modifier->typeData().empty()) {
			stream << "# " << modifier->xAxisProperty().nameWithComponent() << " " << modifier->yAxisProperty().nameWithComponent() << "\n";
			for(const QPointF& p : modifier->xyData())
				stream << p.x() << " " << p.y() << "\n";
		}
		else {
			stream << "# " << modifier->xAxisProperty().nameWithComponent() << " " << modifier->yAxisProperty().nameWithComponent() << " type\n";
			OVITO_ASSERT(modifier->typeData().size() == modifier->xyData().size());
			auto t = modifier->typeData().constBegin();
			for(const QPointF& p : modifier->xyData())
				stream << p.x() << " " << p.y() << " " << *t++ << "\n";
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
