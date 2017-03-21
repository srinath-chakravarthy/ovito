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
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/pipeline/PipelineEvalRequest.h>
#include <core/animation/AnimationSettings.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Modifier, RefTarget);
DEFINE_PROPERTY_FIELD(Modifier, isEnabled, "IsEnabled");
SET_PROPERTY_FIELD_LABEL(Modifier, isEnabled, "Enabled");
SET_PROPERTY_FIELD_CHANGE_EVENT(Modifier, isEnabled, ReferenceEvent::TargetEnabledOrDisabled);
DEFINE_PROPERTY_FIELD(Modifier, title, "Name");
SET_PROPERTY_FIELD_LABEL(Modifier, title, "Name");
SET_PROPERTY_FIELD_CHANGE_EVENT(Modifier, title, ReferenceEvent::TitleChanged);

/******************************************************************************
* Constructor.
******************************************************************************/
Modifier::Modifier(DataSet* dataset) : RefTarget(dataset), _isEnabled(true)
{
	INIT_PROPERTY_FIELD(isEnabled);
	INIT_PROPERTY_FIELD(title);
}

/******************************************************************************
* Returns the list of applications associated with this modifier. 
******************************************************************************/
QVector<ModifierApplication*> Modifier::modifierApplications() const
{
	QVector<ModifierApplication*> apps;
	for(RefMaker* dependent : dependents()) {
        ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(dependent);
		if(modApp != nullptr && modApp->modifier() == this)
			apps.push_back(modApp);
	}
	return apps;	
}

/******************************************************************************
* Returns the input object of this modifier for each application of the modifier. 
* This method evaluates the modifier stack up this modifier.
* Note: This method might return empty result objects in some cases when the modifier stack
* cannot be evaluated because of an invalid modifier.
******************************************************************************/
QVector<QPair<ModifierApplication*, PipelineFlowState>> Modifier::getModifierInputs(TimePoint time) const
{
	QVector<QPair<ModifierApplication*, PipelineFlowState>> results;
	for(RefMaker* dependent : dependents()) {
        ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(dependent);
		if(modApp != nullptr && modApp->modifier() == this) {
			if(PipelineObject* pipelineObj = modApp->pipelineObject())
				results.push_back(qMakePair(modApp, pipelineObj->evaluateImmediately(PipelineEvalRequest(time, false, modApp, false))));
		}
	}

	return results;
}

/******************************************************************************
* Same function as above but using the current animation time as 
* evaluation time and only returning the input object for the first application
* of this modifier.
******************************************************************************/
PipelineFlowState Modifier::getModifierInput(ModifierApplication* modApp) const
{
	TimePoint time = dataset()->animationSettings()->time();
	if(modApp != nullptr && modApp->modifier() == this) {
		if(PipelineObject* pipelineObj = modApp->pipelineObject()) {
			return pipelineObj->evaluateImmediately(PipelineEvalRequest(time, false, modApp, false));
		}
	}
	else {
		for(RefMaker* dependent : dependents()) {
			ModifierApplication* modApp = dynamic_object_cast<ModifierApplication>(dependent);
			if(modApp != nullptr && modApp->modifier() == this) {
				if(PipelineObject* pipelineObj = modApp->pipelineObject()) {
					return pipelineObj->evaluateImmediately(PipelineEvalRequest(time, false, modApp, false));
				}
			}
		}
	}
	return PipelineFlowState();
}

/******************************************************************************
* Asks the modifier for its validity interval at the given time.
******************************************************************************/
TimeInterval Modifier::modifierValidity(TimePoint time)
{
	// Return an empty validity interval if the modifier is currently being edited
	// to let the system create a pipeline cache point just before the modifier.
	// This will speed up re-evaluation of the pipeline if the user adjusts this modifier's parameters interactively.
	if(isObjectBeingEdited())
		return TimeInterval::empty();

	return TimeInterval::infinite();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
