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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/particles/data/SimulationCell.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <core/utilities/concurrent/Promise.h>

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

	typedef std::pair<CellHandle, int> Facet;

	class FacetCirculator {
	public:
		FacetCirculator(const DelaunayTessellation& tess, CellHandle cell, int s, int t, CellHandle start, int f) :
			_tess(tess), _s(tess.cellVertex(cell, s)), _t(tess.cellVertex(cell, t)) {
		    int i = tess._dt->index(start, _s);
		    int j = tess._dt->index(start, _t);

		    OVITO_ASSERT( f!=i && f!=j );

		    if(f == next_around_edge(i,j))
		    	_pos = start;
		    else
		    	_pos = tess._dt->cell_adjacent(start, f); // other cell with same facet
		}
		FacetCirculator& operator--() {
			_pos = _tess._dt->cell_adjacent(_pos, next_around_edge(_tess._dt->index(_pos, _t), _tess._dt->index(_pos, _s)));
			return *this;
		}
		FacetCirculator operator--(int) {
			FacetCirculator tmp(*this);
			--(*this);
			return tmp;
		}
		FacetCirculator & operator++() {
			_pos = _tess._dt->cell_adjacent(_pos, next_around_edge(_tess._dt->index(_pos, _s), _tess._dt->index(_pos, _t)));
			return *this;
		}
		FacetCirculator operator++(int) {
			FacetCirculator tmp(*this);
			++(*this);
			return tmp;
		}
		Facet operator*() const {
			return Facet(_pos, next_around_edge(_tess._dt->index(_pos, _s), _tess._dt->index(_pos, _t)));
		}
		Facet operator->() const {
			return Facet(_pos, next_around_edge(_tess._dt->index(_pos, _s), _tess._dt->index(_pos, _t)));
		}
		bool operator==(const FacetCirculator& ccir) const
		{
			return _pos == ccir._pos && _s == ccir._s && _t == ccir._t;
		}
		bool operator!=(const FacetCirculator& ccir) const
		{
			return ! (*this == ccir);
		}

	private:
		const DelaunayTessellation& _tess;
		VertexHandle _s;
		VertexHandle _t;
		CellHandle _pos;
		static int next_around_edge(int i, int j) {
			static const int tab_next_around_edge[4][4] = {
			      {5, 2, 3, 1},
			      {3, 5, 0, 2},
			      {1, 3, 5, 0},
			      {2, 0, 1, 5} };
			return tab_next_around_edge[i][j];
		}
	};

	/// Generates the Delaunay tessellation.
	bool generateTessellation(const SimulationCell& simCell, const Point3* positions, size_t numPoints, FloatType ghostLayerSize, const int* selectedPoints, PromiseBase& promise);

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

	bool isGhostCell(CellHandle cell) const {
		return _cellInfo[cell].isGhost;
	}

	bool isGhostVertex(VertexHandle vertex) const {
		return vertex >= _primaryVertexCount;
	}

	VertexHandle cellVertex(CellHandle cell, size_type localIndex) const {
		return _dt->cell_vertex(cell, localIndex);
	}

	Point3 vertexPosition(VertexHandle vertex) const {
		const double* xyz = _dt->vertex_ptr(vertex);
		return Point3((FloatType)xyz[0], (FloatType)xyz[1], (FloatType)xyz[2]);
	}

	bool alphaTest(CellHandle cell, FloatType alpha) const;

	int vertexIndex(VertexHandle vertex) const {
		OVITO_ASSERT(vertex < _particleIndices.size());
		return _particleIndices[vertex];
	}

	Facet mirrorFacet(CellHandle cell, int face) const {
		GEO::signed_index_t adjacentCell = _dt->cell_adjacent(cell, face);
		OVITO_ASSERT(adjacentCell >= 0);
		return Facet(adjacentCell, _dt->adjacent_index(adjacentCell, cell));
	}

	Facet mirrorFacet(const Facet& facet) const {
		return mirrorFacet(facet.first, facet.second);
	}

	/// Returns the cell vertex for the given triangle vertex of the given cell facet.
	static inline int cellFacetVertexIndex(int cellFacetIndex, int facetVertexIndex) {
		static const int tab_vertex_triple_index[4][3] = {
		 {1, 3, 2},
		 {0, 2, 3},
		 {0, 3, 1},
		 {0, 1, 2}
		};
		OVITO_ASSERT(cellFacetIndex >= 0 && cellFacetIndex < 4);
		OVITO_ASSERT(facetVertexIndex >= 0 && facetVertexIndex < 3);
		return tab_vertex_triple_index[cellFacetIndex][facetVertexIndex];
	}

	FacetCirculator incident_facets(CellHandle cell, int i, int j, CellHandle start, int f) const {
		return FacetCirculator(*this, cell, i, j, start, f);
	}

	/// Returns the simulation cell geometry.
	const SimulationCell& simCell() const { return _simCell; }

private:

	/// Determines whether the given tetrahedral cell is a ghost cell (or an invalid cell).
	bool classifyGhostCell(CellHandle cell) const;

	/// The internal Delaunay generator object.
	GEO::SmartPointer<GEO::Delaunay3d> _dt;

	/// Stores the coordinates of the input points.
	std::vector<double> _pointData;

	/// Stores per-cell auxiliary data.
	std::vector<CellInfo> _cellInfo;

	/// Mapping of Delaunay points to input particles.
	std::vector<int> _particleIndices;

	/// The number of primary (non-ghost) vertices.
	size_type _primaryVertexCount;

	/// The number of finite cells in the primary image of the simulation cell.
	size_type _numPrimaryTetrahedra = 0;

	/// The simulation cell geometry.
	SimulationCell _simCell;
};

}	// End of namespace
}	// End of namespace
}	// End of namespace



