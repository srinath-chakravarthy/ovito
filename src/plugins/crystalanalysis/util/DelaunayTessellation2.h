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

#ifndef __OVITO_DELAUNAY_TESSELLATION2_H
#define __OVITO_DELAUNAY_TESSELLATION2_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/data/SimulationCell.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <core/utilities/concurrent/FutureInterface.h>

#include <geogram/delaunay/delaunay_3d.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * Generates a Delaunay tessellation of a particle system.
 */
class OVITO_CRYSTALANALYSIS_EXPORT DelaunayTessellation
{
public:

	typedef GEO::index_t size_type;
	typedef GEO::index_t CellHandle;
	typedef GEO::index_t VertexHandle;
	typedef size_type CellIterator;

	/// Data structure attached to each tessellation cell.
	struct CellInfo {
		bool isGhost;	// Indicates whether this is a ghost tetrahedron.
		int userField;	// An additional field that can be used by client code.
		int index;		// An index assigned to the cell.
	};

	/// Generates the Delaunay tessellation.
	bool generateTessellation(const SimulationCell& simCell, const Point3* positions, size_t numPoints, FloatType ghostLayerSize, const int* selectedPoints = nullptr, FutureInterfaceBase* progress = nullptr);

	/// Returns the total number of tetrahedra in the tessellation.
	size_type numberOfTetrahedra() const { return _dt->nb_cells(); }

	/// Returns the number of finite cells in the primary image of the simulation cell.
	size_type numberOfPrimaryTetrahedra() const { return _numPrimaryTetrahedra; }

	CellIterator begin_cells() const { return 0; }
	CellIterator end_cells() const { return _dt->nb_cells(); }

	void setCellIndex(CellHandle cell, int value) {
		_cellInfo[cell].index = value;
	}

	int getCellIndex(CellHandle cell) const {
		return _cellInfo[cell].index;
	}

	void setUserField(CellHandle cell, int value) {
		_cellInfo[cell].userField = value;
	}

	int getUserField(CellHandle cell) const {
		return _cellInfo[cell].userField;
	}

	/// Returns whether the given tessellation cell connects four physical vertices.
	/// Returns false if one of the four vertices is the infinite vertex.
	bool isValidCell(CellHandle cell) const {
		return _dt->cell_is_finite(cell);
	}

	bool isGhost(CellHandle cell) const {
		return _cellInfo[cell].isGhost;
	}

	VertexHandle cellVertex(CellHandle cell, size_type localIndex) const {
		return _dt->cell_vertex(cell, localIndex);
	}

	Point3 vertexPosition(VertexHandle vertex) const {
		const double* xyz = _dt->vertex_ptr(vertex);
		return Point3((FloatType)xyz[0], (FloatType)xyz[1], (FloatType)xyz[2]);
	}

	static int vertexIndex(VertexHandle vertex) {
		return vertex;
	}

	/// Returns the simulation cell geometry.
	const SimulationCell& simCell() const { return _simCell; }

private:

	/// The internal Delaunay generator object.
	GEO::SmartPointer<GEO::Delaunay3d> _dt;

	std::vector<CellInfo> _cellInfo;

	/// The number of finite cells in the primary image of the simulation cell.
	size_type _numPrimaryTetrahedra = 0;

	/// The simulation cell geometry.
	SimulationCell _simCell;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


#endif // __OVITO_DELAUNAY_TESSELLATION2_H
