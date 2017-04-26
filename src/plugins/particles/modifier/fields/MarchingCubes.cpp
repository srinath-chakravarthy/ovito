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

#include <plugins/particles/Particles.h>
#include "MarchingCubes.h"
#include "MarchingCubesLookupTable.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Fields)

/******************************************************************************
* Constructor.
******************************************************************************/
MarchingCubes::MarchingCubes(int size_x, int size_y, int size_z, const FloatType* data, size_t stride, HalfEdgeMesh<>& outputMesh) :
    _size_x(size_x), _size_y(size_y), _size_z(size_z),
    _data(data), _dataStride(stride), _outputMesh(outputMesh),
    _cubeVerts(size_x * size_y * size_z * 3, nullptr),
    _isCompletelySolid(false)
{
    OVITO_ASSERT(stride >= 1);
}

/******************************************************************************
* Main method that constructs the isosurface mesh.
******************************************************************************/
bool MarchingCubes::generateIsosurface(FloatType isolevel, PromiseBase& promise)
{
    promise.setProgressMaximum(_size_z*2);
    promise.setProgressValue(0);
    computeIntersectionPoints(isolevel, promise);
    if(promise.isCanceled()) return false;

    for(int k = 0; k < _size_z && !promise.isCanceled(); k++, promise.incrementProgressValue()) {
        for(int j = 0; j < _size_y; j++) {
            for(int i = 0; i < _size_x; i++) {
                _lut_entry = 0;
                for(int p = 0; p < 8; ++p) {
                    _cube[p] = getFieldValue(i+((p^(p>>1))&1), j+((p>>1)&1), k+((p>>2)&1)) - isolevel;
                    if(std::abs(_cube[p]) < _epsilon) _cube[p] = _epsilon;
                    if(_cube[p] > 0) _lut_entry += 1 << p;
                }
                processCube(i,j,k);
            }
        }
    }
    return !promise.isCanceled();
}

/******************************************************************************
* Compute the intersection points with the isosurface along the cube edges.
******************************************************************************/
void MarchingCubes::computeIntersectionPoints(FloatType isolevel, PromiseBase& promise)
{
    _isCompletelySolid = true;
    for(int k = 0; k < _size_z && !promise.isCanceled(); k++, promise.incrementProgressValue()) {
        for(int j = 0; j < _size_y; j++) {
            for(int i = 0; i < _size_x; i++) {
                FloatType cube[8];
                cube[0] = getFieldValue(i,   j,   k  ) - isolevel;
                cube[1] = getFieldValue(i+1, j,   k  ) - isolevel;
                cube[3] = getFieldValue(i,   j+1, k  ) - isolevel;
                cube[4] = getFieldValue(i,   j,   k+1) - isolevel;

                if(std::abs(cube[0]) < _epsilon) cube[0] = _epsilon;
                if(std::abs(cube[1]) < _epsilon) cube[1] = _epsilon;
                if(std::abs(cube[3]) < _epsilon) cube[3] = _epsilon;
                if(std::abs(cube[4]) < _epsilon) cube[4] = _epsilon;

                if(cube[0] < 0) _isCompletelySolid = false;
                if(cube[1]*cube[0] < 0) createEdgeVertexX(i,j,k, cube[0] / (cube[0] - cube[1]));
                if(cube[3]*cube[0] < 0) createEdgeVertexY(i,j,k, cube[0] / (cube[0] - cube[3]));
                if(cube[4]*cube[0] < 0) createEdgeVertexZ(i,j,k, cube[0] / (cube[0] - cube[4]));
            }
        }
    }
}

/******************************************************************************
* Test a face.
* if face>0 return true if the face contains a part of the surface
******************************************************************************/
bool MarchingCubes::testFace(char face)
{
    FloatType A,B,C,D;

    switch(face)
    {
    case -1: case 1:  A = _cube[0];  B = _cube[4];  C = _cube[5];  D = _cube[1];  break;
    case -2: case 2:  A = _cube[1];  B = _cube[5];  C = _cube[6];  D = _cube[2];  break;
    case -3: case 3:  A = _cube[2];  B = _cube[6];  C = _cube[7];  D = _cube[3];  break;
    case -4: case 4:  A = _cube[3];  B = _cube[7];  C = _cube[4];  D = _cube[0];  break;
    case -5: case 5:  A = _cube[0];  B = _cube[3];  C = _cube[2];  D = _cube[1];  break;
    case -6: case 6:  A = _cube[4];  B = _cube[7];  C = _cube[6];  D = _cube[5];  break;
    default: OVITO_ASSERT_MSG(false, "Marching cubes", "Invalid face code");
    };

    if(std::abs(A*C - B*D) < _epsilon)
        return face >= 0;
    return face * A * (A*C - B*D) >= 0;  // face and A invert signs
}


/******************************************************************************
* Test the interior of a cube.
* if s == 7, return true  if the interior is empty
* if s ==-7, return false if the interior is empty
******************************************************************************/
bool MarchingCubes::testInterior(char s)
{
    FloatType t, At=0, Bt=0, Ct=0, Dt=0, a, b;
    char  test =  0;
    char  edge = -1; // reference edge of the triangulation

    switch( _case )
    {
    case  4 :
    case 10 :
        a = ( _cube[4] - _cube[0]) * ( _cube[6] - _cube[2]) - ( _cube[7] - _cube[3]) * ( _cube[5] - _cube[1]);
        b =  _cube[2] * ( _cube[4] - _cube[0]) + _cube[0] * ( _cube[6] - _cube[2] )
                - _cube[1] * ( _cube[7] - _cube[3]) - _cube[3] * ( _cube[5] - _cube[1]);
        t = - b / (2*a);
        if(t<0 || t>1) return s>0;

        At = _cube[0] + ( _cube[4] - _cube[0]) * t;
        Bt = _cube[3] + ( _cube[7] - _cube[3]) * t;
        Ct = _cube[2] + ( _cube[6] - _cube[2]) * t;
        Dt = _cube[1] + ( _cube[5] - _cube[1]) * t;
        break;

    case  6 :
    case  7 :
    case 12 :
    case 13 :
        switch( _case )
        {
        case  6: edge = test6 [_config][2]; break;
        case  7: edge = test7 [_config][4]; break;
        case 12: edge = test12[_config][3]; break;
        case 13: edge = tiling13_5_1[_config][_subconfig][0]; break;
        }
        switch( edge )
        {
        case  0 :
          t  = _cube[0] / ( _cube[0] - _cube[1]);
          At = 0;
          Bt = _cube[3] + ( _cube[2] - _cube[3]) * t;
          Ct = _cube[7] + ( _cube[6] - _cube[7]) * t;
          Dt = _cube[4] + ( _cube[5] - _cube[4]) * t;
          break;
        case  1 :
          t  = _cube[1] / ( _cube[1] - _cube[2]);
          At = 0;
          Bt = _cube[0] + ( _cube[3] - _cube[0]) * t;
          Ct = _cube[4] + ( _cube[7] - _cube[4]) * t;
          Dt = _cube[5] + ( _cube[6] - _cube[5]) * t;
          break;
        case  2 :
          t  = _cube[2] / ( _cube[2] - _cube[3]);
          At = 0;
          Bt = _cube[1] + ( _cube[0] - _cube[1]) * t;
          Ct = _cube[5] + ( _cube[4] - _cube[5]) * t;
          Dt = _cube[6] + ( _cube[7] - _cube[6]) * t;
          break;
        case  3 :
          t  = _cube[3] / ( _cube[3] - _cube[0]);
          At = 0;
          Bt = _cube[2] + ( _cube[1] - _cube[2]) * t;
          Ct = _cube[6] + ( _cube[5] - _cube[6]) * t;
          Dt = _cube[7] + ( _cube[4] - _cube[7]) * t;
          break;
        case  4 :
          t  = _cube[4] / ( _cube[4] - _cube[5]);
          At = 0;
          Bt = _cube[7] + ( _cube[6] - _cube[7]) * t;
          Ct = _cube[3] + ( _cube[2] - _cube[3]) * t;
          Dt = _cube[0] + ( _cube[1] - _cube[0]) * t;
          break;
        case  5 :
          t  = _cube[5] / ( _cube[5] - _cube[6]);
          At = 0;
          Bt = _cube[4] + ( _cube[7] - _cube[4]) * t;
          Ct = _cube[0] + ( _cube[3] - _cube[0]) * t;
          Dt = _cube[1] + ( _cube[2] - _cube[1]) * t;
          break;
        case  6 :
          t  = _cube[6] / ( _cube[6] - _cube[7]);
          At = 0;
          Bt = _cube[5] + ( _cube[4] - _cube[5]) * t;
          Ct = _cube[1] + ( _cube[0] - _cube[1]) * t;
          Dt = _cube[2] + ( _cube[3] - _cube[2]) * t;
          break;
        case  7 :
          t  = _cube[7] / ( _cube[7] - _cube[4]);
          At = 0;
          Bt = _cube[6] + ( _cube[5] - _cube[6]) * t;
          Ct = _cube[2] + ( _cube[1] - _cube[2]) * t;
          Dt = _cube[3] + ( _cube[0] - _cube[3]) * t;
          break;
        case  8 :
          t  = _cube[0] / ( _cube[0] - _cube[4]);
          At = 0;
          Bt = _cube[3] + ( _cube[7] - _cube[3]) * t;
          Ct = _cube[2] + ( _cube[6] - _cube[2]) * t;
          Dt = _cube[1] + ( _cube[5] - _cube[1]) * t;
          break;
        case  9 :
          t  = _cube[1] / ( _cube[1] - _cube[5]);
          At = 0;
          Bt = _cube[0] + ( _cube[4] - _cube[0]) * t;
          Ct = _cube[3] + ( _cube[7] - _cube[3]) * t;
          Dt = _cube[2] + ( _cube[6] - _cube[2]) * t;
          break;
        case 10 :
          t  = _cube[2] / ( _cube[2] - _cube[6]);
          At = 0;
          Bt = _cube[1] + ( _cube[5] - _cube[1]) * t;
          Ct = _cube[0] + ( _cube[4] - _cube[0]) * t;
          Dt = _cube[3] + ( _cube[7] - _cube[3]) * t;
          break;
        case 11 :
          t  = _cube[3] / ( _cube[3] - _cube[7]);
          At = 0;
          Bt = _cube[2] + ( _cube[6] - _cube[2]) * t;
          Ct = _cube[1] + ( _cube[5] - _cube[1]) * t;
          Dt = _cube[0] + ( _cube[4] - _cube[0]) * t;
          break;
        default: OVITO_ASSERT_MSG(false, "Marching cubes", "Invalid edge"); break;
        }
        break;

    default: OVITO_ASSERT_MSG(false, "Marching cubes", "Invalid ambiguous case");  break;
    }

    if(At >= 0) test ++;
    if(Bt >= 0) test += 2;
    if(Ct >= 0) test += 4;
    if(Dt >= 0) test += 8;
    switch( test )
    {
    case  0: return s>0;
    case  1: return s>0;
    case  2: return s>0;
    case  3: return s>0;
    case  4: return s>0;
    case  5: if(At * Ct - Bt * Dt <  _epsilon) return s>0; break;
    case  6: return s>0;
    case  7: return s<0;
    case  8: return s>0;
    case  9: return s>0;
    case 10: if(At * Ct - Bt * Dt >= _epsilon) return s>0; break;
    case 11: return s<0;
    case 12: return s>0;
    case 13: return s<0;
    case 14: return s<0;
    case 15: return s<0;
    }

    return s<0;
}

/******************************************************************************
* Processes a single cube.
******************************************************************************/
void MarchingCubes::processCube(int i, int j, int k)
{
    HalfEdgeMesh<>::Vertex* v12 = nullptr;
    _case   = cases[_lut_entry][0];
    _config = cases[_lut_entry][1];
    _subconfig = 0;

    switch(_case) {
    case  0 :
        break;

    case  1 :
        addTriangle(i,j,k, tiling1[_config], 1);
        break;

    case  2 :
        addTriangle(i,j,k, tiling2[_config], 2);
        break;

    case  3 :
        if(testFace(test3[_config]) )
            addTriangle(i,j,k, tiling3_2[_config], 4); // 3.2
        else
            addTriangle(i,j,k, tiling3_1[_config], 2); // 3.1
        break;

    case  4 :
        if(testInterior(test4[_config]) )
            addTriangle(i,j,k, tiling4_1[_config], 2); // 4.1.1
        else
            addTriangle(i,j,k, tiling4_2[_config], 6); // 4.1.2
        break;

    case  5 :
        addTriangle(i,j,k, tiling5[_config], 3);
        break;

    case  6 :
        if(testFace(test6[_config][0]) )
            addTriangle(i,j,k, tiling6_2[_config], 5); // 6.2
        else
        {
            if(testInterior(test6[_config][1]) )
                addTriangle(i,j,k, tiling6_1_1[_config], 3); // 6.1.1
            else
            {
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling6_1_2[_config], 9 , v12); // 6.1.2
            }
        }
        break;

    case  7 :
        if(testFace(test7[_config][0])) _subconfig +=  1;
        if(testFace(test7[_config][1])) _subconfig +=  2;
        if(testFace(test7[_config][2])) _subconfig +=  4;
        switch( _subconfig )
        {
        case 0 :
            addTriangle(i,j,k, tiling7_1[_config], 3); break;
        case 1 :
            addTriangle(i,j,k, tiling7_2[_config][0], 5); break;
        case 2 :
            addTriangle(i,j,k, tiling7_2[_config][1], 5); break;
        case 3 :
            v12 = createCenterVertex(i,j,k);
            addTriangle(i,j,k, tiling7_3[_config][0], 9, v12); break;
        case 4 :
            addTriangle(i,j,k, tiling7_2[_config][2], 5); break;
        case 5 :
            v12 = createCenterVertex(i,j,k);
            addTriangle(i,j,k, tiling7_3[_config][1], 9, v12); break;
        case 6 :
            v12 = createCenterVertex(i,j,k);
            addTriangle(i,j,k, tiling7_3[_config][2], 9, v12); break;
        case 7 :
            if(testInterior(test7[_config][3]) )
                addTriangle(i,j,k, tiling7_4_2[_config], 9);
            else
                addTriangle(i,j,k, tiling7_4_1[_config], 5);
            break;
        };
        break;

    case  8 :
        addTriangle(i,j,k, tiling8[_config], 2);
        break;

    case  9 :
        addTriangle(i,j,k, tiling9[_config], 4);
        break;

    case 10 :
        if(testFace(test10[_config][0]) )
        {
            if(testFace(test10[_config][1]) )
                addTriangle(i,j,k, tiling10_1_1_[_config], 4); // 10.1.1
            else
            {
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling10_2[_config], 8, v12); // 10.2
            }
        }
        else
        {
            if(testFace(test10[_config][1]) )
            {
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling10_2_[_config], 8, v12); // 10.2
            }
            else
            {
                if(testInterior(test10[_config][2]) )
                    addTriangle(i,j,k, tiling10_1_1[_config], 4); // 10.1.1
                else
                    addTriangle(i,j,k, tiling10_1_2[_config], 8); // 10.1.2
            }
        }
        break;

    case 11 :
        addTriangle(i,j,k, tiling11[_config], 4);
        break;

    case 12 :
        if(testFace(test12[_config][0]) )
        {
            if(testFace(test12[_config][1]) )
                addTriangle(i,j,k, tiling12_1_1_[_config], 4); // 12.1.1
            else
            {
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling12_2[_config], 8, v12); // 12.2
            }
        }
        else
        {
            if(testFace(test12[_config][1]) )
            {
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling12_2_[_config], 8, v12); // 12.2
            }
            else
            {
                if(testInterior(test12[_config][2]) )
                    addTriangle(i,j,k, tiling12_1_1[_config], 4); // 12.1.1
                else
                    addTriangle(i,j,k, tiling12_1_2[_config], 8); // 12.1.2
            }
        }
        break;

    case 13 :
        if(testFace(test13[_config][0])) _subconfig +=  1;
        if(testFace(test13[_config][1])) _subconfig +=  2;
        if(testFace(test13[_config][2])) _subconfig +=  4;
        if(testFace(test13[_config][3])) _subconfig +=  8;
        if(testFace(test13[_config][4])) _subconfig += 16;
        if(testFace(test13[_config][5])) _subconfig += 32;
        switch(subconfig13[_subconfig]) {
            case 0 :/* 13.1 */
                addTriangle(i,j,k, tiling13_1[_config], 4); break;

            case 1 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2[_config][0], 6); break;
            case 2 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2[_config][1], 6); break;
            case 3 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2[_config][2], 6); break;
            case 4 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2[_config][3], 6); break;
            case 5 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2[_config][4], 6); break;
            case 6 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2[_config][5], 6); break;

            case 7 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][0], 10, v12); break;
            case 8 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][1], 10, v12); break;
            case 9 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][2], 10, v12); break;
            case 10 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][3], 10, v12); break;
            case 11 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][4], 10, v12); break;
            case 12 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][5], 10, v12); break;
            case 13 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][6], 10, v12); break;
            case 14 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][7], 10, v12); break;
            case 15 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][8], 10, v12); break;
            case 16 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][9], 10, v12); break;
            case 17 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][10], 10, v12); break;
            case 18 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3[_config][11], 10, v12); break;

            case 19 :/* 13.4 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_4[_config][0], 12, v12); break;
            case 20 :/* 13.4 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_4[_config][1], 12, v12); break;
            case 21 :/* 13.4 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_4[_config][2], 12, v12); break;
            case 22 :/* 13.4 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_4[_config][3], 12, v12); break;

            case 23 :/* 13.5 */
                _subconfig = 0;
                if(testInterior(test13[_config][6]) )
                    addTriangle(i,j,k, tiling13_5_1[_config][0], 6);
                else
                    addTriangle(i,j,k, tiling13_5_2[_config][0], 10);
                break;
            case 24 :/* 13.5 */
                _subconfig = 1;
                if(testInterior(test13[_config][6]) )
                    addTriangle(i,j,k, tiling13_5_1[_config][1], 6);
                else
                    addTriangle(i,j,k, tiling13_5_2[_config][1], 10);
                break;
            case 25 :/* 13.5 */
                _subconfig = 2;
                if(testInterior(test13[_config][6]) )
                    addTriangle(i,j,k, tiling13_5_1[_config][2], 6);
                else
                    addTriangle(i,j,k, tiling13_5_2[_config][2], 10);
                break;
            case 26 :/* 13.5 */
                _subconfig = 3;
                if(testInterior(test13[_config][6]) )
                    addTriangle(i,j,k, tiling13_5_1[_config][3], 6);
                else
                    addTriangle(i,j,k, tiling13_5_2[_config][3], 10);
                break;

            case 27 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][0], 10, v12); break;
            case 28 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][1], 10, v12); break;
            case 29 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][2], 10, v12); break;
            case 30 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][3], 10, v12); break;
            case 31 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][4], 10, v12); break;
            case 32 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][5], 10, v12); break;
            case 33 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][6], 10, v12); break;
            case 34 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][7], 10, v12); break;
            case 35 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][8], 10, v12); break;
            case 36 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][9], 10, v12); break;
            case 37 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][10], 10, v12); break;
            case 38 :/* 13.3 */
                v12 = createCenterVertex(i,j,k);
                addTriangle(i,j,k, tiling13_3_[_config][11], 10, v12); break;

            case 39 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2_[_config][0], 6); break;
            case 40 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2_[_config][1], 6); break;
            case 41 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2_[_config][2], 6); break;
            case 42 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2_[_config][3], 6); break;
            case 43 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2_[_config][4], 6); break;
            case 44 :/* 13.2 */
                addTriangle(i,j,k, tiling13_2_[_config][5], 6); break;

            case 45 :/* 13.1 */
                addTriangle(i,j,k, tiling13_1_[_config], 4); break;

            default :
                OVITO_ASSERT_MSG(false, "Marching cubes", "Impossible case 13?");
          }
          break;

      case 14 :
          addTriangle(i,j,k, tiling14[_config], 4);
          break;
    };
}

/******************************************************************************
* Adds triangles to the mesh.
******************************************************************************/
void MarchingCubes::addTriangle(int i, int j, int k, const char* trig, char n, HalfEdgeMesh<>::Vertex* v12)
{
    HalfEdgeMesh<>::Vertex*    tv[3];

    for(int t = 0; t < 3*n; t++) {
        switch(trig[t]) {
            case  0: tv[t % 3] = getEdgeVert(i  , j  , k,  0); break;
            case  1: tv[t % 3] = getEdgeVert(i+1, j  , k,  1); break;
            case  2: tv[t % 3] = getEdgeVert(i  , j+1, k,  0); break;
            case  3: tv[t % 3] = getEdgeVert(i  , j  , k,  1); break;
            case  4: tv[t % 3] = getEdgeVert(i  , j  , k+1,0); break;
            case  5: tv[t % 3] = getEdgeVert(i+1, j  , k+1,1); break;
            case  6: tv[t % 3] = getEdgeVert(i  , j+1, k+1,0); break;
            case  7: tv[t % 3] = getEdgeVert(i  , j  , k+1,1); break;
            case  8: tv[t % 3] = getEdgeVert(i  , j  , k,  2); break;
            case  9: tv[t % 3] = getEdgeVert(i+1, j  , k,  2); break;
            case 10: tv[t % 3] = getEdgeVert(i+1, j+1, k,  2); break;
            case 11: tv[t % 3] = getEdgeVert(i  , j+1, k,  2); break;
            case 12: tv[t % 3] = v12; break;
            default: break;
        }
        OVITO_ASSERT_MSG(tv[t%3] != nullptr, "Marching cubes", "invalid triangle");

        if(t%3 == 2)
            _outputMesh.createFace({tv[2], tv[1], tv[0]});
    }
}

/******************************************************************************
* Adds a vertex inside the current cube.
******************************************************************************/
HalfEdgeMesh<>::Vertex* MarchingCubes::createCenterVertex(int i, int j, int k)
{
    int u = 0;
    Point3 p = Point3::Origin();

    // Computes the average of the intersection points of the cube
    auto addPosition = [&p, &u](const HalfEdgeMesh<>::Vertex* v) {
        if(v) {
            ++u;
            p.x() += v->pos().x();
            p.y() += v->pos().y();
            p.z() += v->pos().z();
        }
    };
    addPosition(getEdgeVert(i  , j  , k,  0));
    addPosition(getEdgeVert(i+1, j  , k,  1));
    addPosition(getEdgeVert(i  , j+1, k,  0));
    addPosition(getEdgeVert(i  , j  , k,  1));
    addPosition(getEdgeVert(i  , j  , k+1,0));
    addPosition(getEdgeVert(i+1, j  , k+1,1));
    addPosition(getEdgeVert(i  , j+1, k+1,0));
    addPosition(getEdgeVert(i  , j  , k+1,1));
    addPosition(getEdgeVert(i  , j  , k,  2));
    addPosition(getEdgeVert(i+1, j  , k,  2));
    addPosition(getEdgeVert(i+1, j+1, k,  2));
    addPosition(getEdgeVert(i  , j+1, k,  2));

    p.x()  /= u;
    p.y()  /= u;
    p.z()  /= u;

    return _outputMesh.createVertex(p);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
