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
#include <plugins/particles/export/lammps/LAMMPSDataExporter.h>
#include <plugins/particles/import/lammps/LAMMPSDataImporter.h>
#include <gui/properties/VariantComboBoxParameterUI.h>
#include "LAMMPSDataExporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(LAMMPSDataExporterEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSDataExporter, LAMMPSDataExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSDataExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("LAMMPS Data File"), rolloutParams);

    // Create the rollout contents.
	QGridLayout* layout = new QGridLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);
	layout->setColumnStretch(1,1);
	layout->addWidget(new QLabel(tr("LAMMPS atom style:")), 0, 0);

	VariantComboBoxParameterUI* atomStyleUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(LAMMPSDataExporter::_atomStyle));
	atomStyleUI->comboBox()->addItem("angle", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Angle));
	atomStyleUI->comboBox()->addItem("atomic", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Atomic));
	atomStyleUI->comboBox()->addItem("bond", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Bond));
	atomStyleUI->comboBox()->addItem("charge", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Charge));
	atomStyleUI->comboBox()->addItem("dipole", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Dipole));
	atomStyleUI->comboBox()->addItem("full", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Full));
	atomStyleUI->comboBox()->addItem("molecular", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Molecular));
	atomStyleUI->comboBox()->addItem("sphere", QVariant::fromValue(LAMMPSDataImporter::AtomStyle_Sphere));
	layout->addWidget(atomStyleUI->comboBox(), 0, 1);
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
