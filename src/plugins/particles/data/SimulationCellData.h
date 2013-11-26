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

#ifndef __OVITO_SIMULATION_CELL_DATA_H
#define __OVITO_SIMULATION_CELL_DATA_H

#include <plugins/particles/Particles.h>

namespace Particles {

using namespace Ovito;

/**
* \brief Stores the geometry and boundary conditions of a simulation box.
 *
 * The simulation box geometry is a parallelepiped defined by three edge vectors.
 * A fourth vector specifies the origin of the simulation box in space.
 */
class OVITO_PARTICLES_EXPORT SimulationCellData
{
public:

	/// Returns the current simulation cell matrix.
	const AffineTransformation& matrix() const { return _simulationCell; }

	/// Sets the simulation cell matrix.
	void setMatrix(const AffineTransformation& cellMatrix) {
		_simulationCell = cellMatrix;
		if(!cellMatrix.inverse(_reciprocalSimulationCell))
			_reciprocalSimulationCell.setIdentity();
	}

	/// Returns the PBC flags.
	const std::array<bool,3>& pbcFlags() const { return _pbcFlags; }

	/// Sets the PBC flags.
	void setPbcFlags(const std::array<bool,3>& flags) { _pbcFlags = flags; }

	/// Sets the PBC flags.
	void setPbcFlags(bool pbcX, bool pbcY, bool pbcZ) { _pbcFlags[0] = pbcX; _pbcFlags[1] = pbcY; _pbcFlags[2] = pbcZ; }

	/// Computes the (positive) volume of the cell.
	FloatType volume() const {
		return std::abs(_simulationCell.determinant());
	}

	/// Checks if two simulation cells are identical.
	bool operator==(const SimulationCellData& other) const {
		return (_simulationCell == other._simulationCell && _pbcFlags == other._pbcFlags);
	}

	/// Converts a point given in reduced cell coordinates to a point in absolute coordinates.
	Point3 reducedToAbsolute(const Point3& reducedPoint) const { return _simulationCell * reducedPoint; }

	/// Converts a point given in absolute coordinates to a point in reduced cell coordinates.
	Point3 absoluteToReduced(const Point3& absPoint) const { return _reciprocalSimulationCell * absPoint; }

	/// Converts a vector given in reduced cell coordinates to a vector in absolute coordinates.
	Vector3 reducedToAbsolute(const Vector3& reducedVec) const { return _simulationCell * reducedVec; }

	/// Converts a vector given in absolute coordinates to a point in vector cell coordinates.
	Vector3 absoluteToReduced(const Vector3& absVec) const { return _reciprocalSimulationCell * absVec; }

private:

	/// The geometry of the cell.
	AffineTransformation _simulationCell = AffineTransformation::Zero();

	/// The reciprocal cell matrix.
	AffineTransformation _reciprocalSimulationCell = AffineTransformation::Zero();

	/// PBC flags.
	std::array<bool,3> _pbcFlags = {{ true, true, true }};
};

};

#endif // __OVITO_SIMULATION_CELL_DATA_H