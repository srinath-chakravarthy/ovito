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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/export/xyz/XYZExporter.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include "XYZExporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, XYZExporterEditor, FileColumnParticleExporterEditor);
SET_OVITO_OBJECT_EDITOR(XYZExporter, XYZExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void XYZExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("XYZ File"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1,1);
	layout->addWidget(new QLabel(tr("Format style:")), 0, 0);

	VariantComboBoxParameterUI* subFormatUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(XYZExporter::_subFormat));
	subFormatUI->comboBox()->addItem("Extended (default)", QVariant::fromValue(XYZExporter::ExtendedFormat));
	subFormatUI->comboBox()->addItem("Parcas", QVariant::fromValue(XYZExporter::ParcasFormat));
	layout->addWidget(subFormatUI->comboBox(), 0, 1);

	FileColumnParticleExporterEditor::createUI(rolloutParams.before(rollout));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
