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

#pragma once


#include <plugins/particles/gui/ParticlesGui.h>
#include <gui/properties/PropertiesEditor.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the SimulationCellObject class.
 */
class SimulationCellEditor : public PropertiesEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE SimulationCellEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Is called when a spinner's value has changed.
	void onSizeSpinnerValueChanged(int dim);

	/// Is called when the user begins dragging a spinner interactively.
	void onSizeSpinnerDragStart(int dim);

	/// Is called when the user stops dragging a spinner interactively.
	void onSizeSpinnerDragStop(int dim);

	/// Is called when the user aborts dragging a spinner interactively.
	void onSizeSpinnerDragAbort(int dim);

	/// After the simulation cell size has changed, updates the UI controls.
	void updateSimulationBoxSize();

private:

	/// After the user has changed a spinner's value, this method changes the
	/// simulation cell geometry.
	void changeSimulationBoxSize(int dim);

	SpinnerWidget* simCellSizeSpinners[3];
	BooleanParameterUI* pbczPUI;
	Vector3ParameterUI* zvectorPUI[3];
	Vector3ParameterUI* zoriginPUI;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


