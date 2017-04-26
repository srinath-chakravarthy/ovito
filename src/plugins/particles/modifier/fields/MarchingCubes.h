///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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


#include <plugins/particles/Particles.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include <core/utilities/concurrent/Promise.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Fields)

/** 
* The Marching Cubes algorithm for constructing isosurfaces from grid data.
*/ 
class MarchingCubes
{
public:

    // Constructor
    MarchingCubes(int size_x, int size_y, int size_z, const FloatType* fielddata, size_t stride, HalfEdgeMesh<>& outputMesh);

    /// Returns the field value in a specific cube of the grid.
    /// Takes into account periodic boundary conditions.
    inline const FloatType getFieldValue(int i, int j, int k) const { 
        if(i == _size_x) i = 0;
        if(j == _size_y) j = 0;
        if(k == _size_z) k = 0;
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        return _data[(i + j*_size_x + k*_size_x*_size_y) * _dataStride];
    }
  
    bool isCompletelySolid() const { return _isCompletelySolid; }

    bool generateIsosurface(FloatType iso, PromiseBase& promise);

protected:

    /// Tessellates one cube.
    void processCube(int i, int j, int k);

    /// tTests if the components of the tessellation of the cube should be 
    /// connected by the interior of an ambiguous face.
    bool testFace(char face);

    /// Tests if the components of the tessellation of the cube should be 
    /// connected through the interior of the cube.
    bool testInterior(char s);

    /// Computes almost all the vertices of the mesh by interpolation along the cubes edges.
    void computeIntersectionPoints(FloatType iso, PromiseBase& promise);

    /// Adds triangles to the mesh.
    void addTriangle(int i, int j, int k, const char* trig, char n, HalfEdgeMesh<>::Vertex* v12 = nullptr);

    /// Adds a vertex on the current horizontal edge.
    HalfEdgeMesh<>::Vertex* createEdgeVertexX(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i + u, j, k));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 0] = v; 
        return v;
    }
    
    /// Adds a vertex on the current longitudinal edge.
    HalfEdgeMesh<>::Vertex* createEdgeVertexY(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i, j + u, k));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 1] = v; 
        return v;
    }

    /// Adds a vertex on the current vertical edge.
    HalfEdgeMesh<>::Vertex* createEdgeVertexZ(int i, int j, int k, FloatType u) {
        OVITO_ASSERT(i >= 0 && i < _size_x);
        OVITO_ASSERT(j >= 0 && j < _size_y);
        OVITO_ASSERT(k >= 0 && k < _size_z);
        auto v = _outputMesh.createVertex(Point3(i, j, k + u));
        _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + 2] = v; 
        return v;
    }
    
    /// Adds a vertex inside the current cube.
    HalfEdgeMesh<>::Vertex* createCenterVertex(int i, int j, int k);

    /// Accesses the pre-computed vertex on a lower edge of a specific cube.
    HalfEdgeMesh<>::Vertex* getEdgeVert(int i, int j, int k, int axis) const {
        OVITO_ASSERT(i >= 0 && i <= _size_x);
        OVITO_ASSERT(j >= 0 && j <= _size_y);
        OVITO_ASSERT(k >= 0 && k <= _size_z);
        OVITO_ASSERT(axis >= 0 && axis < 3);
        if(i == _size_x) i = 0;
        if(j == _size_y) j = 0;
        if(k == _size_z) k = 0;
        return _cubeVerts[(i + j*_size_x + k*_size_x*_size_y)*3 + axis]; 
    }

protected :
    int _size_x;  ///< width  of the grid
    int _size_y;  ///< depth  of the grid
    int _size_z;  ///< height of the grid
    const FloatType* _data;  ///< implicit function values sampled on the grid
    size_t _dataStride;

    /// Vertices created along cube edges.
    std::vector<HalfEdgeMesh<>::Vertex*> _cubeVerts;

    FloatType     _cube[8];   ///< values of the implicit function on the active cube
    unsigned char _lut_entry; ///< cube sign representation in [0..255]
    unsigned char _case;      ///< case of the active cube in [0..15]
    unsigned char _config;    ///< configuration of the active cube
    unsigned char _subconfig; ///< subconfiguration of the active cube

    /// The generated mesh.
    HalfEdgeMesh<>& _outputMesh;

    /// Flag that indicates whether all cube cells are on one side of the isosurface.
    bool _isCompletelySolid;

#ifdef FLOATTYPE_FLOAT
    static constexpr FloatType _epsilon = FloatType(1e-12);
#else
    static constexpr FloatType _epsilon = FloatType(1e-18);
#endif
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
