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
#include <core/scene/objects/AsynchronousDisplayObject.h>
#include <core/scene/objects/WeakVersionedObjectReference.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include <core/rendering/MeshPrimitive.h>
#include <core/animation/controller/Controller.h>
#include <plugins/particles/data/SimulationCell.h>
#include "SlipSurface.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A display object for the SlipSurface data object class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT SlipSurfaceDisplay : public AsynchronousDisplayObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE SlipSurfaceDisplay(DataSet* dataset);

	/// \brief Lets the display object render the data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// Returns the transparency of the surface mesh.
	FloatType surfaceTransparency() const { return surfaceTransparencyController() ? surfaceTransparencyController()->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface mesh.
	void setSurfaceTransparency(FloatType transparency) { if(surfaceTransparencyController()) surfaceTransparencyController()->setCurrentFloatValue(transparency); }

	/// Generates the final triangle mesh, which will be rendered.
	static bool buildMesh(const SlipSurfaceData& input, const SimulationCell& cell, const QVector<Plane3>& cuttingPlanes, const QStringList& structureNames, TriMesh& output, std::vector<ColorA>& materialColors, PromiseBase& promise);

protected:

	/// Creates a computation engine that will prepare the data to be displayed.
	virtual std::shared_ptr<AsynchronousTask> createEngine(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState) override;

	/// Unpacks the results of the computation engine and stores them in the display object.
	virtual void transferComputationResults(AsynchronousTask* engine) override;

	/// Computation engine that builds the render mesh.
	class PrepareMeshEngine : public AsynchronousTask
	{
	public:

		/// Constructor.
		PrepareMeshEngine(SlipSurfaceData* mesh, ClusterGraph* clusterGraph, const SimulationCell& simCell,
				QStringList structureNames, const QVector<Plane3>& cuttingPlanes) :
			_inputMesh(mesh), _clusterGraph(clusterGraph), _simCell(simCell),
			_structureNames(structureNames), _cuttingPlanes(cuttingPlanes) {}

		/// Computes the results and stores them in this object for later retrieval.
		virtual void perform() override;

		TriMesh& surfaceMesh() { return _surfaceMesh; }
		std::vector<ColorA>& materialColors() { return _materialColors; }

	private:

		QExplicitlySharedDataPointer<SlipSurfaceData> _inputMesh;
		QExplicitlySharedDataPointer<ClusterGraph> _clusterGraph;
		SimulationCell _simCell;
		QStringList _structureNames;
		QVector<Plane3> _cuttingPlanes;
		TriMesh _surfaceMesh;
		std::vector<ColorA> _materialColors;
	};

protected:

	/// Splits a triangle face at a periodic boundary.
	static bool splitFace(TriMesh& output, int faceIndex, int oldVertexCount, std::vector<Point3>& newVertices, std::map<std::pair<int,int>,std::pair<int,int>>& newVertexLookupMap, const SimulationCell& cell, size_t dim);

	/// Controls whether the mesh is rendered using smooth shading.
	DECLARE_MODIFIABLE_PROPERTY_FIELD(bool, smoothShading, setSmoothShading);

	/// Controls the transparency of the surface mesh.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(Controller, surfaceTransparencyController, setSurfaceTransparencyController);

	/// The buffered geometry used to render the surface mesh.
	std::shared_ptr<MeshPrimitive> _surfaceBuffer;

	/// The non-periodic triangle mesh generated from the surface mesh for rendering.
	TriMesh _surfaceMesh;

	/// The material colors used for mesh rendering.
	std::vector<ColorA> _materialColors;

	/// This helper structure is used to detect any changes in the input data
	/// that require updating the geometry buffer.
	SceneObjectCacheHelper<
		FloatType,							// Surface transparency
		bool								// Smooth shading
		> _geometryCacheHelper;

	/// This helper structure is used to detect any changes in the input data
	/// that require recomputing the cached triangle mesh for rendering.
	SceneObjectCacheHelper<
		WeakVersionedOORef<DataObject>,		// Source object + revision number
		SimulationCell						// Simulation cell geometry
		> _preparationCacheHelper;

	/// Indicates that the triangle mesh representation of the surface has recently been updated.
	bool _trimeshUpdate;

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Slip surface");
};

}	// End of namespace
}	// End of namespace
}	// End of namespace


