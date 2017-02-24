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


#include <plugins/particles/Particles.h>
#include <core/scene/objects/AsynchronousDisplayObject.h>
#include <core/scene/objects/WeakVersionedObjectReference.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include <core/rendering/MeshPrimitive.h>
#include <core/animation/controller/Controller.h>
#include <plugins/particles/data/SimulationCell.h>

namespace Ovito { namespace Particles {

/**
 * \brief A display object for the SurfaceMesh data object class.
 */
class OVITO_PARTICLES_EXPORT SurfaceMeshDisplay : public AsynchronousDisplayObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE SurfaceMeshDisplay(DataSet* dataset);

	/// \brief Lets the display object render the data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// Returns the transparency of the surface mesh.
	FloatType surfaceTransparency() const { return surfaceTransparencyController() ? surfaceTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface mesh.
	void setSurfaceTransparency(FloatType transparency) { if(surfaceTransparencyController()) surfaceTransparencyController()->setCurrentFloatValue(transparency); }

	/// Returns the transparency of the surface cap mesh.
	FloatType capTransparency() const { return capTransparencyController() ? capTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface cap mesh.
	void setCapTransparency(FloatType transparency) { if(capTransparencyController()) capTransparencyController()->setCurrentFloatValue(transparency); }

	/// Generates the final triangle mesh, which will be rendered.
	static bool buildSurfaceMesh(const HalfEdgeMesh<>& input, const SimulationCell& cell, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseBase* progress = nullptr);

	/// Generates the triangle mesh for the PBC cap.
	static void buildCapMesh(const HalfEdgeMesh<>& input, const SimulationCell& cell, bool isCompletelySolid, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes, TriMesh& output, PromiseBase* progress = nullptr);

protected:

	/// Creates a computation engine that will prepare the data to be displayed.
	virtual std::shared_ptr<AsynchronousTask> createEngine(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState) override;

	/// Unpacks the results of the computation engine and stores them in the display object.
	virtual void transferComputationResults(AsynchronousTask* engine) override;

	/// Computation engine that builds the render mesh.
	class PrepareSurfaceEngine : public AsynchronousTask
	{
	public:

		/// Constructor.
		PrepareSurfaceEngine(HalfEdgeMesh<>* mesh, const SimulationCell& simCell, bool isCompletelySolid, bool reverseOrientation, const QVector<Plane3>& cuttingPlanes) :
			_inputMesh(mesh), _simCell(simCell), _isCompletelySolid(isCompletelySolid), _reverseOrientation(reverseOrientation), _cuttingPlanes(cuttingPlanes) {}

		/// Computes the results and stores them in this object for later retrieval.
		virtual void perform() override;

		TriMesh& surfaceMesh() { return _surfaceMesh; }
		TriMesh& capPolygonsMesh() { return _capPolygonsMesh; }

	private:

		QExplicitlySharedDataPointer<HalfEdgeMesh<>> _inputMesh;
		SimulationCell _simCell;
		bool _isCompletelySolid;
		bool _reverseOrientation;
		QVector<Plane3> _cuttingPlanes;
		TriMesh _surfaceMesh;
		TriMesh _capPolygonsMesh;
	};

protected:

	/// Splits a triangle face at a periodic boundary.
	static bool splitFace(TriMesh& output, TriMeshFace& face, int oldVertexCount, std::vector<Point3>& newVertices, std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim);

	/// Traces the closed contour of the surface-boundary intersection.
	static std::vector<Point2> traceContour(HalfEdgeMesh<>::Edge* firstEdge, const std::vector<Point3>& reducedPos, const SimulationCell& cell, size_t dim);

	/// Clips a 2d contour at a periodic boundary.
	static void clipContour(std::vector<Point2>& input, std::array<bool,2> periodic, std::vector<std::vector<Point2>>& openContours, std::vector<std::vector<Point2>>& closedContours);

	/// Computes the intersection point of a 2d contour segment crossing a periodic boundary.
	static void computeContourIntersection(size_t dim, FloatType t, Point2& base, Vector2& delta, int crossDir, std::vector<std::vector<Point2>>& contours);

	/// Determines if the 2D box corner (0,0) is inside the closed region described by the 2d polygon.
	static bool isCornerInside2DRegion(const std::vector<std::vector<Point2>>& contours);

	/// Determines if the 3D box corner (0,0,0) is inside the region described by the half-edge polyhedron.
	static bool isCornerInside3DRegion(const HalfEdgeMesh<>& mesh, const std::vector<Point3>& reducedPos, const std::array<bool,3> pbcFlags, bool isCompletelySolid);

	/// Controls the display color of the surface mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, surfaceColor, setSurfaceColor);

	/// Controls the display color of the cap mesh.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(Color, capColor, setCapColor);

	/// Controls whether the cap mesh is rendered.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, showCap, setShowCap);

	/// Controls whether the surface mesh is rendered using smooth shading.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, smoothShading, setSmoothShading);

	/// Controls whether the mesh' orientation is flipped.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, reverseOrientation, setReverseOrientation);

	/// Controls the transparency of the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, surfaceTransparencyController, setSurfaceTransparencyController);

	/// Controls the transparency of the surface cap mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, capTransparencyController, setCapTransparencyController);

	/// The buffered geometry used to render the surface mesh.
	std::shared_ptr<MeshPrimitive> _surfaceBuffer;

	/// The buffered geometry used to render the surface cap.
	std::shared_ptr<MeshPrimitive> _capBuffer;

	/// The non-periodic triangle mesh generated from the surface mesh for rendering.
	TriMesh _surfaceMesh;

	/// The cap polygons generated from the surface mesh for rendering.
	TriMesh _capPolygonsMesh;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	SceneObjectCacheHelper<
		ColorA,								// Surface color
		ColorA,								// Cap color
		bool								// Smooth shading
		> _geometryCacheHelper;

	/// This helper structure is used to detect any changes in the input data
	/// that require recomputing the cached triangle mesh for rendering.
	SceneObjectCacheHelper<
		WeakVersionedOORef<DataObject>,		// Source object + revision number
		SimulationCell,						// Simulation cell geometry
		bool								// Reverse orientation flag
		> _preparationCacheHelper;

	/// Indicates that the triangle mesh representation of the surface has recently been updated.
	bool _trimeshUpdate;

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Surface mesh");
};

}	// End of namespace
}	// End of namespace


