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
#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/AsyncPipelineEvaluationHelper.h>
#include <core/utilities/concurrent/Future.h>
#include <core/utilities/concurrent/Promise.h>
#include "ModifierApplication.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief This is a data flow pipeline. It has a source object, which provides the input data,
 *        and a list of modifiers that act on the data.
 */
class OVITO_CORE_EXPORT PipelineObject : public DataObject
{
public:

	/// \brief Constructor that creates an empty pipeline.
	Q_INVOKABLE PipelineObject(DataSet* dataset);

	/// \brief Inserts a modifier into the data flow pipeline.
	/// \param modifier The modifier to be inserted.
	/// \param index Specifies the position in the pipeline where the modifier should be inserted.
	///              It must be between zero and the number of existing modifier applications as returned by
	///              modifierApplications(). Modifiers are applied in ascending order, i.e., the modifier
	///              at index 0 is applied first to the input data.
	/// \return The ModifierApplication object that has been created for the usage of the modifier in this pipeline.
	/// \undoable
	ModifierApplication* insertModifier(int index, Modifier* modifier);

	/// \brief Inserts a modifier application into the pipeline.
	/// \param modApp The modifier application to be inserted.
	/// \param index Specifies the position in the pipeline where the modifier should be inserted.
	///              It must be between zero and the number of existing modifier applications as returned by
	///              modifierApplications(). Modifiers are applied in ascending order, i.e., the modifier
	///              at index 0 is applied first.
	/// \undoable
	void insertModifierApplication(int index, ModifierApplication* modApp);

	/// \brief Removes a modifier from the geometry pipeline.
	/// \param index The index of the ModifierApplication that should be removed.
	/// \undoable
	void removeModifierApplication(int index);

	/////////////////////////////////////// from DataObject /////////////////////////////////////////

	/// Asks the object for the result of the data pipeline.
	virtual PipelineFlowState evaluateImmediately(const PipelineEvalRequest& request) override;

	/// Asks the object for the result of the data pipeline.
	virtual Future<PipelineFlowState> evaluateAsync(const PipelineEvalRequest& request) override {
		return _evaluationRequestHelper.createRequest(this, request);
	}

	/// Sends an event to all dependents of this RefTarget.
	virtual void notifyDependents(ReferenceEvent& event) override;

	/// \brief Sends an event to all dependents of this RefTarget.
	/// \param eventType The event type passed to the ReferenceEvent constructor.
	inline void notifyDependents(ReferenceEvent::Type eventType) {
		DataObject::notifyDependents(eventType);
	}

protected:

	/// This method is called when a reference target changes.
	virtual bool referenceEvent(RefTarget* source, ReferenceEvent* event) override;

	/// Is called when a reference target has been added to a list reference field of this RefMaker.
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// Is called when a reference target has been removed from a list reference field of this RefMaker.
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* oldTarget, int listIndex) override;

	/// Is called when the value of a reference field of this RefMaker changes.
	virtual void referenceReplaced(const PropertyFieldDescriptor& field, RefTarget* oldTarget, RefTarget* newTarget) override;

private:

	/// This function is called when a part of the pipeline (or its source) have changed.
	/// Notifies all modifiers starting at the given index that their input has changed.
	void modifierChanged(int changedIndex);

	/// Asks the object for the result of the geometry pipeline at the given time
	/// up to a given point in the modifier stack. The result may be incomplete (i.e. status pending)
	/// if the results are not immediately available.
	PipelineFlowState evaluatePipeline(TimePoint time, int upToHereIndex);

private:

	/// The object providing the input data that is processed by the modifiers.
	DECLARE_MODIFIABLE_REFERENCE_FIELD(DataObject, sourceObject, setSourceObject);

	/// The ordered list of modifiers that are applied to the input object.
	/// The modifiers are applied to the input object in the reverse order of this list.
	DECLARE_VECTOR_REFERENCE_FIELD(ModifierApplication, modifierApplications);

	/// The state of the input object from the last evaluation of the pipeline.
	/// This is used to detect changes in the input that require recalculation of the pipeline.
	PipelineFlowState _lastInput;

	/// The cached results from the last pipeline evaluation.
	PipelineFlowState _cachedState;

	/// Indicates which pipeline stage has been stored in the cache.
	/// If the cache is empty, then this is set to -1.
	int _cachedIndex;

	/// Manages pending asynchronous pipeline requests.
	AsyncPipelineEvaluationHelper _evaluationRequestHelper;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


