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


#include <core/Core.h>
#include <core/utilities/concurrent/Promise.h>
#include "SceneNode.h"
#include "objects/DataObject.h"
#include "objects/DisplayObject.h"
#include "pipeline/PipelineEvalRequest.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A node in the scene that represents an object.
 */
class OVITO_CORE_EXPORT ObjectNode : public SceneNode
{
public:

	/// \brief Constructs an object node.
	Q_INVOKABLE ObjectNode(DataSet* dataset);

	/// \brief Destructor.
	virtual ~ObjectNode();

	/// \brief Returns the data source of this node's pipeline, i.e., the object that provides the
	///        input data entering the pipeline.
	DataObject* sourceObject() const;

	/// \brief Sets the data source of this node's pipeline, i.e., the object that provides the
	///        input data entering the pipeline.
	void setSourceObject(DataObject* sourceObject);

	/// \brief Evaluates the data pipeline of this node.
	///        If the pipeline results are not immediately available, the method can react by returning an incomplete state (pending status).
	/// \param request An object that describes when and how the pipeline should be evaluated.
	/// \return The results of the pipeline (may be status pending).
	const PipelineFlowState& evaluatePipelineImmediately(const PipelineEvalRequest& request);

	/// \brief Asks the object for the result of the data pipeline.
	/// \param request An object that describes when and how the pipeline should be evaluated.
	Future<PipelineFlowState> evaluatePipelineAsync(const PipelineEvalRequest& request);

	/// \brief Applies a modifier by appending it to the end of the node's data pipeline.
	/// \param mod The modifier to be inserted into the data flow pipeline.
	/// \undoable
	Q_INVOKABLE void applyModifier(Modifier* mod);

	/// \brief Returns the bounding box of the node's object in local coordinates.
	/// \param time The time at which the bounding box should be computed.
	/// \return An axis-aligned box in the node's local coordinate system that fully contains
	///         the node's object.
	virtual Box3 localBoundingBox(TimePoint time) override;

	/// \brief Renders the node's object.
	/// \param time Specifies the animation frame to render.
	/// \param renderer The renderer that should be used to render graphics primitives.
	void render(TimePoint time, SceneRenderer* renderer);

	/// \brief Returns the title of this object.
	virtual QString objectTitle() override;

protected:

	/// This method is called when a referenced object has changed.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Is called when the value of a reference field of this object changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

	/// Saves the class' contents to the given stream.
	virtual void saveToStream(ObjectSaveStream& stream) override;

	/// Loads the class' contents from the given stream.
	virtual void loadFromStream(ObjectLoadStream& stream) override;

private:

	/// Checks if the data pipeline evaluation is completed.
	void serveEvaluationRequests();

	/// The object that generates the data to be displayed by this ObjectNode.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(DataObject, dataProvider, setDataProvider);

	/// The list of display objects that are responsible for displaying
	/// the node's data in the viewports. This is for internal caching purposes only.
	DECLARE_VECTOR_REFERENCE_FIELD(DisplayObject, displayObjects);

	/// The cached results from the last data pipeline evaluation.
	PipelineFlowState _pipelineCache;

	/// The cached results from the display preparation stage.
	PipelineFlowState _displayCache;

	/// This method invalidates the data pipeline cache of the object node.
	void invalidatePipelineCache() {
		// Reset data caches.
		_pipelineCache.clear();
		_displayCache.clear();
		// Also mark the cached bounding box of this scene node as invalid.
		invalidateBoundingBox();
	}

	/// List active asynchronous pipeline evaluation requests.
	std::vector<std::pair<PipelineEvalRequest, PromisePtr<PipelineFlowState>>> _evaluationRequests;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


