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

#ifndef __OVITO_PARTITION_MESH_DISPLAY_H
#define __OVITO_PARTITION_MESH_DISPLAY_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/scene/objects/AsynchronousDisplayObject.h>
#include <core/scene/objects/WeakVersionedObjectReference.h>
#include <core/utilities/mesh/TriMesh.h>
#include <core/utilities/mesh/HalfEdgeMesh.h>
#include <core/rendering/MeshPrimitive.h>
#include <core/gui/properties/PropertiesEditor.h>
#include <core/animation/controller/Controller.h>
#include <plugins/particles/data/SimulationCell.h>
#include "PartitionMesh.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A display object for the PartitionMesh data object class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT PartitionMeshDisplay : public AsynchronousDisplayObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE PartitionMeshDisplay(DataSet* dataset);

	/// \brief Lets the display object render the data object.
	virtual void render(TimePoint time, DataObject* dataObject, const PipelineFlowState& flowState, SceneRenderer* renderer, ObjectNode* contextNode) override;

	/// \brief Computes the bounding box of the object.
	virtual Box3 boundingBox(TimePoint time, DataObject* dataObject, ObjectNode* contextNode, const PipelineFlowState& flowState) override;

	/// Returns the color of the outer surface.
	const Color& surfaceColor() const { return _surfaceColor; }

	/// Sets the color of the outer surface.
	void setSurfaceColor(const Color& color) { _surfaceColor = color; }

	/// Returns whether the cap polygons are rendered.
	bool showCap() const { return _showCap; }

	/// Sets whether the cap polygons are rendered.
	void setShowCap(bool show) { _showCap = show; }

	/// Returns whether the mesh is rendered using smooth shading.
	bool smoothShading() const { return _smoothShading; }

	/// Sets whether the mesh is rendered using smooth shading.
	void setSmoothShading(bool smoothShading) { _smoothShading = smoothShading; }

	/// Returns the transparency of the surface mesh.
	FloatType surfaceTransparency() const { return _surfaceTransparency ? _surfaceTransparency->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the surface mesh.
	void setSurfaceTransparency(FloatType transparency) { if(_surfaceTransparency) _surfaceTransparency->setCurrentFloatValue(transparency); }

	/// Returns the transparency of the cap polygons.
	FloatType capTransparency() const { return _capTransparency ? _capTransparency->currentFloatValue() : 0.0f; }

	/// Sets the transparency of the cap polygons.
	void setCapTransparency(FloatType transparency) { if(_capTransparency) _capTransparency->setCurrentFloatValue(transparency); }

	/// Generates the final triangle mesh, which will be rendered.
	static bool buildMesh(const PartitionMeshData& input, const SimulationCell& cell, const QVector<Plane3>& cuttingPlanes, TriMesh& output, FutureInterfaceBase* progress = nullptr);

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
		PrepareMeshEngine(PartitionMeshData* mesh, const SimulationCell& simCell, int spaceFillingRegion, const QVector<Plane3>& cuttingPlanes) :
			_inputMesh(mesh), _simCell(simCell), _spaceFillingRegion(spaceFillingRegion), _cuttingPlanes(cuttingPlanes) {}

		/// Computes the results and stores them in this object for later retrieval.
		virtual void perform() override;

		TriMesh& surfaceMesh() { return _surfaceMesh; }
		TriMesh& capPolygonsMesh() { return _capPolygonsMesh; }

	private:

		QExplicitlySharedDataPointer<PartitionMeshData> _inputMesh;
		SimulationCell _simCell;
		int _spaceFillingRegion;
		QVector<Plane3> _cuttingPlanes;
		TriMesh _surfaceMesh;
		TriMesh _capPolygonsMesh;
	};

protected:

	/// Controls the display color of the outer surface mesh.
	PropertyField<Color, QColor> _surfaceColor;

	/// Controls whether the cap polygons are rendered.
	PropertyField<bool> _showCap;

	/// Controls whether the mesh is rendered using smooth shading.
	PropertyField<bool> _smoothShading;

	/// Controls the transparency of the surface mesh.
	ReferenceField<Controller> _surfaceTransparency;

	/// Controls the transparency of the surface cap mesh.
	ReferenceField<Controller> _capTransparency;

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

private:

	Q_OBJECT
	OVITO_OBJECT

	Q_CLASSINFO("DisplayName", "Microstructure");

	DECLARE_PROPERTY_FIELD(_surfaceColor);
	DECLARE_PROPERTY_FIELD(_showCap);
	DECLARE_PROPERTY_FIELD(_smoothShading);
	DECLARE_REFERENCE_FIELD(_surfaceTransparency);
	DECLARE_REFERENCE_FIELD(_capTransparency);
};

/**
 * \brief A properties editor for the PartitionMesh class.
 */
class OVITO_CRYSTALANALYSIS_EXPORT PartitionMeshDisplayEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE PartitionMeshDisplayEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_PARTITION_MESH_DISPLAY_H
