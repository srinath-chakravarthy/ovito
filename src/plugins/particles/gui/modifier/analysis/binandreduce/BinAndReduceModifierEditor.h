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

#pragma once


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>
#include <gui/properties/BooleanParameterUI.h>
#include <gui/properties/IntegerParameterUI.h>
#include <core/utilities/DeferredMethodInvocation.h>

class QwtPlot;
class QwtPlotCurve;
class QwtPlotSpectrogram;
class QwtMatrixRasterData;
class QwtPlotGrid;

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the BinAndReduceModifier class.
 */
class BinAndReduceModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE BinAndReduceModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Plots the data computed by the modifier.
	void plotData();

    /// Enable/disable the editor for number of y-bins and the first derivative button.
    void updateWidgets();

	/// This is called when the user has clicked the "Save Data" button.
	void onSaveData();

private:

    /// Widget controlling the number of y-bins.
    BooleanParameterUI* _firstDerivativePUI;

    /// Widget controlling the number of y-bins.
    IntegerParameterUI* _numBinsYPUI;

	/// The graph widget to display the data.
	QwtPlot* _plot;

	/// The plot item for the graph.
    QwtPlotCurve* _plotCurve = nullptr;

	/// The plot item for the 2D color plot.
	QwtPlotSpectrogram* _plotRaster = nullptr;

	/// The data storage for the 2D color plot.
	QwtMatrixRasterData* _rasterData = nullptr;

	/// The grid.
	QwtPlotGrid* _plotGrid = nullptr;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<BinAndReduceModifierEditor, &BinAndReduceModifierEditor::plotData> plotLater;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


