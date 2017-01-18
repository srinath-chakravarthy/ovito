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

#include <plugins/particles/Particles.h>
#include <core/utilities/units/UnitsManager.h>
#include "SimulationCellObject.h"
#include "SimulationCellDisplay.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(SimulationCellObject, DataObject);
DEFINE_PROPERTY_FIELD(SimulationCellObject, _cellVector1, "CellVector1");
DEFINE_PROPERTY_FIELD(SimulationCellObject, _cellVector2, "CellVector2");
DEFINE_PROPERTY_FIELD(SimulationCellObject, _cellVector3, "CellVector3");
DEFINE_PROPERTY_FIELD(SimulationCellObject, _cellOrigin, "CellTranslation");
DEFINE_PROPERTY_FIELD(SimulationCellObject, _pbcX, "PeriodicX");
DEFINE_PROPERTY_FIELD(SimulationCellObject, _pbcY, "PeriodicY");
DEFINE_PROPERTY_FIELD(SimulationCellObject, _pbcZ, "PeriodicZ");
DEFINE_PROPERTY_FIELD(SimulationCellObject, _is2D, "Is2D");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, _cellVector1, "Cell vector 1");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, _cellVector2, "Cell vector 2");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, _cellVector3, "Cell vector 3");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, _cellOrigin, "Cell origin");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, _pbcX, "Periodic boundary conditions (X)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, _pbcY, "Periodic boundary conditions (Y)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, _pbcZ, "Periodic boundary conditions (Z)");
SET_PROPERTY_FIELD_LABEL(SimulationCellObject, _is2D, "2D");
SET_PROPERTY_FIELD_UNITS(SimulationCellObject, _cellVector1, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(SimulationCellObject, _cellVector2, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(SimulationCellObject, _cellVector3, WorldParameterUnit);
SET_PROPERTY_FIELD_UNITS(SimulationCellObject, _cellOrigin, WorldParameterUnit);

/******************************************************************************
* Creates the storage for the internal parameters.
******************************************************************************/
void SimulationCellObject::init(DataSet* dataset)
{
	INIT_PROPERTY_FIELD(SimulationCellObject::_cellVector1);
	INIT_PROPERTY_FIELD(SimulationCellObject::_cellVector2);
	INIT_PROPERTY_FIELD(SimulationCellObject::_cellVector3);
	INIT_PROPERTY_FIELD(SimulationCellObject::_cellOrigin);
	INIT_PROPERTY_FIELD(SimulationCellObject::_pbcX);
	INIT_PROPERTY_FIELD(SimulationCellObject::_pbcY);
	INIT_PROPERTY_FIELD(SimulationCellObject::_pbcZ);
	INIT_PROPERTY_FIELD(SimulationCellObject::_is2D);

	// Attach a display object.
	addDisplayObject(new SimulationCellDisplay(dataset));
}

}	// End of namespace
}	// End of namespace
