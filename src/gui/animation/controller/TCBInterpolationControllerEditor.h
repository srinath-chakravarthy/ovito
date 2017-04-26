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


#include <gui/GUI.h>
#include <gui/properties/PropertiesEditor.h>
#include <gui/properties/FloatParameterUI.h>
#include <core/animation/controller/TCBInterpolationControllers.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Anim) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * A properties editor template for the TCBAnimationKey class template.
 */
template<class TCBAnimationKeyType>
class TCBAnimationKeyEditor : public PropertiesEditor
{
protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override {
		// Create the rollout.
		QWidget* rollout = createRollout(tr("TCB Animation Key"), rolloutParams);

		QVBoxLayout* layout = new QVBoxLayout(rollout);
		layout->setContentsMargins(4,4,4,4);
		layout->setSpacing(2);

		QGridLayout* sublayout = new QGridLayout();
		sublayout->setContentsMargins(0,0,0,0);
		sublayout->setColumnStretch(2, 1);
		layout->addLayout(sublayout);

		// Ease to parameter.
		FloatParameterUI* easeInPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::easeTo));
		sublayout->addWidget(easeInPUI->label(), 0, 0);
		sublayout->addLayout(easeInPUI->createFieldLayout(), 0, 1);

		// Ease from parameter.
		FloatParameterUI* easeFromPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::easeFrom));
		sublayout->addWidget(easeFromPUI->label(), 1, 0);
		sublayout->addLayout(easeFromPUI->createFieldLayout(), 1, 1);

		// Tension parameter.
		FloatParameterUI* tensionPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::tension));
		sublayout->addWidget(tensionPUI->label(), 2, 0);
		sublayout->addLayout(tensionPUI->createFieldLayout(), 2, 1);

		// Continuity parameter.
		FloatParameterUI* continuityPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::continuity));
		sublayout->addWidget(continuityPUI->label(), 3, 0);
		sublayout->addLayout(continuityPUI->createFieldLayout(), 3, 1);

		// Continuity parameter.
		FloatParameterUI* biasPUI = new FloatParameterUI(this, PROPERTY_FIELD(TCBAnimationKeyType::bias));
		sublayout->addWidget(biasPUI->label(), 4, 0);
		sublayout->addLayout(biasPUI->createFieldLayout(), 4, 1);
	}
};

/**
 * A properties editor for the PositionTCBAnimationKey class.
 */
class PositionTCBAnimationKeyEditor : public TCBAnimationKeyEditor<PositionTCBAnimationKey>
{
public:

	/// Constructor.
	Q_INVOKABLE PositionTCBAnimationKeyEditor() {}

private:

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


