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

#include <plugins/particles/Particles.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <core/utilities/concurrent/Task.h>
#include "POSCARExporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(POSCARExporter, ParticleExporter);

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool POSCARExporter::exportObject(SceneNode* sceneNode, int frameNumber, TimePoint time, const QString& filePath, TaskManager& taskManager)
{
	// Get particle data to be exported.
	PipelineFlowState state;
	if(!getParticleData(sceneNode, time, state, taskManager))
		return false;

	SynchronousTask exportTask(taskManager);

	// Get particle positions and velocities.
	ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(state, ParticleProperty::PositionProperty);
	ParticlePropertyObject* velocityProperty = ParticlePropertyObject::findInState(state, ParticleProperty::VelocityProperty);

	// Get simulation cell info.
	SimulationCellObject* simulationCell = state.findObject<SimulationCellObject>();
	if(!simulationCell)
		throwException(tr("No simulation cell available. Cannot write POSCAR file."));

	// Write POSCAR header including the simulation cell geometry.
	textStream() << "POSCAR file written by OVITO\n";
	textStream() << "1\n";
	AffineTransformation cell = simulationCell->cellMatrix();
	for(size_t i = 0; i < 3; i++)
		textStream() << cell(0, i) << ' ' << cell(1, i) << ' ' << cell(2, i) << '\n';
	Vector3 origin = cell.translation();

	// Count number of particles per particle type.
	QMap<int,int> particleCounts;
	ParticleTypeProperty* particleTypeProperty = dynamic_object_cast<ParticleTypeProperty>(ParticlePropertyObject::findInState(state, ParticleProperty::ParticleTypeProperty));
	if(particleTypeProperty) {
		const int* ptype = particleTypeProperty->constDataInt();
		const int* ptype_end = ptype + particleTypeProperty->size();
		for(; ptype != ptype_end; ++ptype) {
			particleCounts[*ptype]++;
		}

		// Write line with particle type names.
		for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
			ParticleType* particleType = particleTypeProperty->particleType(c.key());
			if(particleType) {
				QString typeName = particleType->name();
				typeName.replace(' ', '_');
				textStream() << typeName << ' ';
			}
			else textStream() << "Type" << c.key() << ' ';
		}
		textStream() << '\n';

		// Write line with particle counts per type.
		for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
			textStream() << c.value() << ' ';
		}
		textStream() << '\n';
	}
	else {
		// Write line with particle type name.
		textStream() << "A\n";
		// Write line with particle count.
		textStream() << posProperty->size() << '\n';
		particleCounts[0] = posProperty->size();
	}

	size_t totalProgressCount = posProperty->size();
	if(velocityProperty) totalProgressCount += posProperty->size();
	size_t currentProgress = 0;
	exportTask.setProgressMaximum(100);

	// Write atomic positions.
	textStream() << "Cartesian\n";
	for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
		int ptype = c.key();
		const Point3* p = posProperty->constDataPoint3();
		for(size_t i = 0; i < posProperty->size(); i++, ++p) {
			if(particleTypeProperty && particleTypeProperty->getInt(i) != ptype)
				continue;
			textStream() << (p->x() - origin.x()) << ' ' << (p->y() - origin.y()) << ' ' << (p->z() - origin.z()) << '\n';
			currentProgress++;

			if((currentProgress % 1000) == 0) {
				exportTask.setProgressValue(currentProgress * 100 / totalProgressCount);
				if(exportTask.isCanceled())
					return false;
			}
		}
	}

	// Write atomic velocities.
	if(velocityProperty) {
		textStream() << "Cartesian\n";
		for(auto c = particleCounts.begin(); c != particleCounts.end(); ++c) {
			int ptype = c.key();
			const Vector3* v = velocityProperty->constDataVector3();
			for(size_t i = 0; i < velocityProperty->size(); i++, ++v) {
				if(particleTypeProperty && particleTypeProperty->getInt(i) != ptype)
					continue;
				textStream() << v->x() << ' ' << v->y() << ' ' << v->z() << '\n';
				currentProgress++;

				if((currentProgress % 1000) == 0) {
					exportTask.setProgressValue(currentProgress * 100 / totalProgressCount);
					if(exportTask.isCanceled())
						return false;
				}
			}
		}
	}

	return !exportTask.isCanceled();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
