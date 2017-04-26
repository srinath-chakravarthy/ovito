///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2015) Alexander Stukowski
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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <core/utilities/concurrent/Task.h>
#include "FHIAimsExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FHIAimsExporter, ParticleExporter);

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool FHIAimsExporter::exportObject(SceneNode* sceneNode, int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager)
{
	// Get particle data to be exported.
	PipelineFlowState state;
	if(!getParticleData(sceneNode, time, state, taskManager))
		return false;

	SynchronousTask exportTask(taskManager);

	// Get particle positions and types.
	ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(state, ParticleProperty::PositionProperty);
	ParticleTypeProperty* particleTypeProperty = dynamic_object_cast<ParticleTypeProperty>(ParticlePropertyObject::findInState(state, ParticleProperty::ParticleTypeProperty));

	textStream() << "# FHI-aims file written by OVITO\n";

	// Output simulation cell.
	Point3 origin = Point3::Origin();
	SimulationCellObject* simulationCell = state.findObject<SimulationCellObject>();
	if(simulationCell) {
		origin = simulationCell->cellOrigin();
		if(simulationCell->pbcX() || simulationCell->pbcY() || simulationCell->pbcZ()) {
			AffineTransformation cell = simulationCell->cellMatrix();
			for(size_t i = 0; i < 3; i++)
				textStream() << "lattice_vector " << cell(0, i) << ' ' << cell(1, i) << ' ' << cell(2, i) << '\n';
		}
	}

	// Output atoms.
	exportTask.setProgressMaximum(100);
	for(size_t i = 0; i < posProperty->size(); i++) {
		const Point3& p = posProperty->getPoint3(i);
		const ParticleType* type = particleTypeProperty->particleType(particleTypeProperty->getInt(i));

		textStream() << "atom " << (p.x() - origin.x()) << ' ' << (p.y() - origin.y()) << ' ' << (p.z() - origin.z());
		if(type && !type->name().isEmpty()) {
			QString s = type->name();
			textStream() << ' ' << s.replace(QChar(' '), QChar('_')) << '\n';
		}
		else {
			textStream() << ' ' << particleTypeProperty->getInt(i) << '\n';
		}

		if((i % 1000) == 0) {
			exportTask.setProgressValue((qint64)i * 100 / posProperty->size());
			if(exportTask.isCanceled())
				return false;
		}
	}

	return !exportTask.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
