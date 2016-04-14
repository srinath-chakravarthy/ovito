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
#include <plugins/particles/import/lammps/LAMMPSDataImporter.h>
#include "LAMMPSDataImporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

IMPLEMENT_OVITO_OBJECT(ParticlesGui, LAMMPSDataImporterEditor, FileImporterEditor);
SET_OVITO_OBJECT_EDITOR(LAMMPSDataImporter, LAMMPSDataImporterEditor);

/******************************************************************************
* This method is called by the FileSource each time a new source
* file has been selected by the user.
******************************************************************************/
bool LAMMPSDataImporterEditor::inspectNewFile(FileImporter* importer, const QUrl& sourceFile, QWidget* parent)
{
	LAMMPSDataImporter* dataImporter = static_object_cast<LAMMPSDataImporter>(importer);

	// Inspect the data file and try to detect the LAMMPS atom style.
	LAMMPSDataImporter::LAMMPSAtomStyle detectedAtomStyle;
	bool successful;
	std::tie(detectedAtomStyle, successful) = dataImporter->inspectFileHeader(FileSourceImporter::Frame(sourceFile));
	if(!successful) return false;

	// Show dialog to ask user for the right LAMMPS atom style if it could not be detected.
	if(detectedAtomStyle == LAMMPSDataImporter::AtomStyle_Unknown) {

		QMap<QString, LAMMPSDataImporter::LAMMPSAtomStyle> styleList = {
				{ QStringLiteral("atomic"), LAMMPSDataImporter::AtomStyle_Atomic },
				{ QStringLiteral("bond"), LAMMPSDataImporter::AtomStyle_Bond },
				{ QStringLiteral("charge"), LAMMPSDataImporter::AtomStyle_Charge },
				{ QStringLiteral("dipole"), LAMMPSDataImporter::AtomStyle_Dipole },
				{ QStringLiteral("molecular"), LAMMPSDataImporter::AtomStyle_Molecular },
				{ QStringLiteral("full"), LAMMPSDataImporter::AtomStyle_Full },
				{ QStringLiteral("angle"), LAMMPSDataImporter::AtomStyle_Angle }
		};
		QStringList itemList = styleList.keys();

		QSettings settings;
		settings.beginGroup(LAMMPSDataImporter::OOType.plugin()->pluginId());
		settings.beginGroup(LAMMPSDataImporter::OOType.name());

		int currentIndex = -1;
		for(int i = 0; i < itemList.size(); i++)
			if(dataImporter->atomStyle() == styleList[itemList[i]])
				currentIndex = i;
		if(currentIndex == -1)
			currentIndex = itemList.indexOf(settings.value("DefaultAtomStyle").toString());
		if(currentIndex == -1)
			currentIndex = itemList.indexOf("atomic");

		bool ok;
		QString selectedItem = QInputDialog::getItem(parent, tr("LAMMPS data file"), tr("Please select the LAMMPS atom style used by the data file:"), itemList, currentIndex, false, &ok);
		if(!ok) return false;

		settings.setValue("DefaultAtomStyle", selectedItem);
		dataImporter->setAtomStyle(styleList[selectedItem]);
	}
	else {
		dataImporter->setAtomStyle(detectedAtomStyle);
	}

	return true;
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void LAMMPSDataImporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// This editor class provides to UI.
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
