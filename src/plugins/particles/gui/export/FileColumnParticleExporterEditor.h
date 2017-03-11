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


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/export/OutputColumnMapping.h>
#include <plugins/particles/export/FileColumnParticleExporter.h>
#include <gui/properties/PropertiesEditor.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

/**
 * \brief User interface component for the FileColumnParticleExporter class.
 */
class FileColumnParticleExporterEditor : public PropertiesEditor
{
public:

	/// Constructor.
	Q_INVOKABLE FileColumnParticleExporterEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

private Q_SLOTS:

	/// Is called when the exporter is associated with the editor.
	void onContentsReplaced(Ovito::RefTarget* newEditObject);

	/// Is called when the user checked/unchecked an item.
	void onListChanged();

private:

	/// Populates the column mapping list box with an entry.
	void insertPropertyItem(ParticlePropertyReference propRef, const QString& displayName, const OutputColumnMapping& columnMapping);

	/// This writes the settings made in the UI back to the exporter.
	void saveChanges(FileColumnParticleExporter* particleExporter);

	QListWidget* _columnMappingWidget;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


