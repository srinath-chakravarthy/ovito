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

#include <core/Core.h>
#include <core/utilities/io/CompressedTextWriter.h>
#include "TriMesh.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(Mesh)

/******************************************************************************
* Constructs an empty mesh.
******************************************************************************/
TriMesh::TriMesh() : _hasVertexColors(false), _hasFaceColors(false), _hasNormals(false)
{
}

/******************************************************************************
* Clears all vertices and faces.
******************************************************************************/
void TriMesh::clear()
{
	_vertices.clear();
	_faces.clear();
	_vertexColors.clear();
	_faceColors.clear();
	_boundingBox.setEmpty();
	_hasVertexColors = false;
	_hasFaceColors = false;
	_hasNormals = false;
}

/******************************************************************************
* Sets the number of vertices in this mesh.
******************************************************************************/
void TriMesh::setVertexCount(int n)
{
	_vertices.resize(n);
	if(_hasVertexColors)
		_vertexColors.resize(n);
}

/******************************************************************************
* Sets the number of faces in this mesh.
******************************************************************************/
void TriMesh::setFaceCount(int n)
{
	_faces.resize(n);
	if(_hasFaceColors)
		_faceColors.resize(n);
	if(_hasNormals)
		_normals.resize(n * 3);
}

/******************************************************************************
* Adds a new triangle face and returns a reference to it.
* The new face is NOT initialized by this method.
******************************************************************************/
TriMeshFace& TriMesh::addFace()
{
	setFaceCount(faceCount() + 1);
	return _faces.back();
}

/******************************************************************************
* Saves the mesh to the given stream.
******************************************************************************/
void TriMesh::saveToStream(SaveStream& stream)
{
	stream.beginChunk(0x03);

	// Save vertices.
	stream << _vertices;

	// Save vertex colors.
	stream << _hasVertexColors;
	stream << _vertexColors;

	// Save face colors.
	stream << _hasFaceColors;
	stream << _faceColors;

	// Save face normals.
	stream << _hasNormals;
	stream << _normals;

	// Save faces.
	stream << (int)faceCount();
	for(auto face = faces().constBegin(); face != faces().constEnd(); ++face) {
		stream << face->_flags;
		stream << face->_vertices[0];
		stream << face->_vertices[1];
		stream << face->_vertices[2];
		stream << face->_smoothingGroups;
		stream << face->_materialIndex;
	}

	stream.endChunk();
}

/******************************************************************************
* Loads the mesh from the given stream.
******************************************************************************/
void TriMesh::loadFromStream(LoadStream& stream)
{
	int formatVersion = stream.expectChunkRange(0x00, 0x03);

	// Reset mesh.
	clear();

	// Load vertices.
	stream >> _vertices;

	// Load vertex colors.
	stream >> _hasVertexColors;
	stream >> _vertexColors;
	OVITO_ASSERT(_vertexColors.size() == _vertices.size() || !_hasVertexColors);

	if(formatVersion >= 2) {
		// Load face colors.
		stream >> _hasFaceColors;
		stream >> _faceColors;
	}

	if(formatVersion >= 3) {
		// Load normals.
		stream >> _hasNormals;
		stream >> _normals;
	}

	// Load faces.
	int nFaces;
	stream >> nFaces;
	_faces.resize(nFaces);
	for(auto face = faces().begin(); face != faces().end(); ++face) {
		stream >> face->_flags;
		stream >> face->_vertices[0];
		stream >> face->_vertices[1];
		stream >> face->_vertices[2];
		stream >> face->_smoothingGroups;
		stream >> face->_materialIndex;
	}

	stream.closeChunk();
}

/******************************************************************************
* Flip the orientation of all faces.
******************************************************************************/
void TriMesh::flipFaces()
{
	for(TriMeshFace& face : faces()) {
		face.setVertices(face.vertex(2), face.vertex(1), face.vertex(0));
		face.setEdgeVisibility(face.edgeVisible(2), face.edgeVisible(1), face.edgeVisible(0));
	}
	if(_hasNormals) {
		for(Vector3& n : _normals)
			n = -n;
	}
	invalidateFaces();
}

/******************************************************************************
* Performs a ray intersection calculation.
******************************************************************************/
bool TriMesh::intersectRay(const Ray3& ray, FloatType& t, Vector3& normal, int& faceIndex, bool backfaceCull) const
{
	FloatType bestT = FLOATTYPE_MAX;
	int index = 0;
	for(auto face = faces().constBegin(); face != faces().constEnd(); ++face) {

		Point3 v0 = vertex(face->vertex(0));
		Vector3 e1 = vertex(face->vertex(1)) - v0;
		Vector3 e2 = vertex(face->vertex(2)) - v0;

		Vector3 h = ray.dir.cross(e2);
		FloatType a = e1.dot(h);

		if(std::fabs(a) < FLOATTYPE_EPSILON)
			continue;

		FloatType f = 1 / a;
		Vector3 s = ray.base - v0;
		FloatType u = f * s.dot(h);

		if(u < FloatType(0) || u > FloatType(1))
			continue;

		Vector3 q = s.cross(e1);
		FloatType v = f * ray.dir.dot(q);

		if(v < FloatType(0) || u + v > FloatType(1))
			continue;

		FloatType tt = f * e2.dot(q);

		if(tt < FLOATTYPE_EPSILON)
			continue;

		if(tt >= bestT)
			continue;

		// Compute face normal.
		Vector3 faceNormal = e1.cross(e2);
		if(faceNormal.isZero(FLOATTYPE_EPSILON)) continue;

		// Do backface culling.
		if(backfaceCull && faceNormal.dot(ray.dir) >= 0)
			continue;

		bestT = tt;
		normal = faceNormal;
		faceIndex = (face - faces().constBegin());
	}

	if(bestT != FLOATTYPE_MAX) {
		t = bestT;
		return true;
	}

	return false;
}

/******************************************************************************
* Exports the triangle mesh to a VTK file.
******************************************************************************/
void TriMesh::saveToVTK(CompressedTextWriter& stream)
{
	stream << "# vtk DataFile Version 3.0\n";
	stream << "# Triangle mesh\n";
	stream << "ASCII\n";
	stream << "DATASET UNSTRUCTURED_GRID\n";
	stream << "POINTS " << vertexCount() << " double\n";
	for(const Point3& p : vertices())
		stream << p.x() << " " << p.y() << " " << p.z() << "\n";
	stream << "\nCELLS " << faceCount() << " " << (faceCount() * 4) << "\n";
	for(const TriMeshFace& f : faces()) {
		stream << "3";
		for(size_t i = 0; i < 3; i++)
			stream << " " << f.vertex(i);
		stream << "\n";
	}
	stream << "\nCELL_TYPES " << faceCount() << "\n";
	for(size_t i = 0; i < faceCount(); i++)
		stream << "5\n";	// Triangle
}

/******************************************************************************
* Clips the mesh at the given plane.
******************************************************************************/
void TriMesh::clipAtPlane(const Plane3& plane)
{
	TriMesh clippedMesh;

	// Clip vertices.
	std::vector<int> existingVertexMapping(vertexCount(), -1);
	for(int vindex = 0; vindex < vertexCount(); vindex++) {
		if(plane.classifyPoint(vertex(vindex)) != +1) {
			existingVertexMapping[vindex] = clippedMesh.addVertex(vertex(vindex));
		}
	}

	// Clip edges.
	std::map<std::pair<int,int>, int> newVertexMapping;
	for(const TriMeshFace& face : faces()) {
		for(int v = 0; v < 3; v++) {
			auto vindices = std::make_pair(face.vertex(v), face.vertex((v+1)%3));
			if(vindices.first > vindices.second) std::swap(vindices.first, vindices.second);
			const Point3& v1 = vertex(vindices.first);
			const Point3& v2 = vertex(vindices.second);
			// Check if edge intersects plane.
			FloatType z1 = plane.pointDistance(v1);
			FloatType z2 = plane.pointDistance(v2);
			if((z1 < FLOATTYPE_EPSILON && z2 > FLOATTYPE_EPSILON) || (z2 < FLOATTYPE_EPSILON && z1 > FLOATTYPE_EPSILON)) {
				if(newVertexMapping.find(vindices) == newVertexMapping.end()) {
					Point3 intersection = v1 + (v1 - v2) * (z1 / (z2 - z1));
					newVertexMapping.insert(std::make_pair(vindices, clippedMesh.addVertex(intersection)));
				}
			}
		}
	}

	// Clip faces.
	for(const TriMeshFace& face : faces()) {
		for(int v0 = 0; v0 < 3; v0++) {
			Point3 current_pos = vertex(face.vertex(v0));
			int current_classification = plane.classifyPoint(current_pos);
			if(current_classification == -1) {
				int newface[4];
				int newface_vcount = 0;
				int next_classification;
				for(int v = v0; v < v0 + 3; v++, current_classification = next_classification) {
					next_classification = plane.classifyPoint(vertex(face.vertex((v+1)%3)));
					if((next_classification <= 0 && current_classification <= 0) || (next_classification == 1 && current_classification == 0)) {
						OVITO_ASSERT(existingVertexMapping[face.vertex(v%3)] >= 0);
						OVITO_ASSERT(newface_vcount <= 3);
						newface[newface_vcount++] = existingVertexMapping[face.vertex(v%3)];
					}
					else if((current_classification == +1 && next_classification == -1) || (current_classification == -1 && next_classification == +1)) {
						auto vindices = std::make_pair(face.vertex(v%3), face.vertex((v+1)%3));
						if(vindices.first > vindices.second) std::swap(vindices.first, vindices.second);
						auto ve = newVertexMapping.find(vindices);
						OVITO_ASSERT(ve != newVertexMapping.end());
						if(current_classification == -1) {
							OVITO_ASSERT(newface_vcount <= 3);
							newface[newface_vcount++] = existingVertexMapping[face.vertex(v%3)];
						}
						OVITO_ASSERT(newface_vcount <= 3);
						newface[newface_vcount++] = ve->second;
					}
				}
				if(newface_vcount >= 3) {
					OVITO_ASSERT(newface[0] >= 0 && newface[0] < clippedMesh.vertexCount());
					OVITO_ASSERT(newface[1] >= 0 && newface[1] < clippedMesh.vertexCount());
					OVITO_ASSERT(newface[2] >= 0 && newface[2] < clippedMesh.vertexCount());
					TriMeshFace& face1 = clippedMesh.addFace();
					face1.setVertices(newface[0], newface[1], newface[2]);
					face1.setSmoothingGroups(face.smoothingGroups());
					face1.setMaterialIndex(face.materialIndex());
					if(newface_vcount == 4) {
						OVITO_ASSERT(newface[3] >= 0 && newface[3] < clippedMesh.vertexCount());
						OVITO_ASSERT(newface[3] != newface[0]);
						TriMeshFace& face2 = clippedMesh.addFace();
						face2.setVertices(newface[0], newface[2], newface[3]);
						face2.setSmoothingGroups(face.smoothingGroups());
						face2.setMaterialIndex(face.materialIndex());
					}
				}
				break;
			}
		}
	}

	this->swap(clippedMesh);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
