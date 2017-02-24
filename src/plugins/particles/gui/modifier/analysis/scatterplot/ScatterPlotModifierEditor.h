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

#pragma once


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>
#include <core/utilities/DeferredMethodInvocation.h>

class QwtPlot;
class QwtPlotSpectroCurve;
class QwtPlotZoneItem;

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor for the ScatterPlotModifier class.
 */
class ScatterPlotModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE ScatterPlotModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

protected Q_SLOTS:

	/// Replots the scatter plot computed by the modifier.
	void plotScatterPlot();

	/// This is called when the user has clicked the "Save Data" button.
	void onSaveData();

private:

	/// The graph widget to display the plot.
	QwtPlot* _plot;

	/// The plot item for the points.
    QwtPlotSpectroCurve* _plotCurve = nullptr;

	/// Marks the range of selected points in the X direction.
	QwtPlotZoneItem* _selectionRangeX = nullptr;

	/// Marks the range of selected points in the Y direction.
	QwtPlotZoneItem* _selectionRangeY = nullptr;

	/// For deferred invocation of the plot repaint function.
	DeferredMethodInvocation<ScatterPlotModifierEditor, &ScatterPlotModifierEditor::plotScatterPlot> plotLater;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


