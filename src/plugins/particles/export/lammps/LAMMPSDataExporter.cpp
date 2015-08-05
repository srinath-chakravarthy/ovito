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
#include "LAMMPSDataExporter.h"
#include "../ParticleExporterSettingsDialog.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(Particles, LAMMPSDataExporter, ParticleExporter);

/******************************************************************************
* Opens the export settings dialog for this exporter service.
******************************************************************************/
bool LAMMPSDataExporter::showSettingsDialog(const PipelineFlowState& state, QWidget* parent)
{
	ParticleExporterSettingsDialog dialog(parent, this, state);
	return (dialog.exec() == QDialog::Accepted);
}

/******************************************************************************
* Writes the particles of one animation frame to the current output file.
******************************************************************************/
bool LAMMPSDataExporter::exportParticles(const PipelineFlowState& state, int frameNumber, TimePoint time, const QString& filePath, ProgressInterface& progress)
{
	// Get particle positions.
	ParticlePropertyObject* posProperty = ParticlePropertyObject::findInState(state, ParticleProperty::PositionProperty);
	if(!posProperty)
		throw Exception(tr("No particle positions available. Cannot write LAMMPS file."));
	ParticlePropertyObject* velocityProperty = ParticlePropertyObject::findInState(state, ParticleProperty::VelocityProperty);
	ParticlePropertyObject* identifierProperty = ParticlePropertyObject::findInState(state, ParticleProperty::IdentifierProperty);
	ParticlePropertyObject* periodicImageProperty = ParticlePropertyObject::findInState(state, ParticleProperty::PeriodicImageProperty);
	ParticleTypeProperty* particleTypeProperty = dynamic_object_cast<ParticleTypeProperty>(ParticlePropertyObject::findInState(state, ParticleProperty::ParticleTypeProperty));

	// Get simulation cell info.
	SimulationCellObject* simulationCell = state.findObject<SimulationCellObject>();
	if(!simulationCell)
		throw Exception(tr("No simulation cell defined. Cannot write LAMMPS file."));

	AffineTransformation simCell = simulationCell->cellMatrix();

	// Transform triclinic cell to LAMMPS canonical format.
	Vector3 a,b,c;
	AffineTransformation transformation;
	bool transformCoordinates;
	if(simCell.column(0).y() != 0 || simCell.column(0).z() != 0 || simCell.column(1).z() != 0) {
		a.x() = simCell.column(0).length();
		a.y() = a.z() = 0;
		b.x() = simCell.column(1).dot(simCell.column(0)) / a.x();
		b.y() = sqrt(simCell.column(1).squaredLength() - b.x()*b.x());
		b.z() = 0;
		c.x() = simCell.column(2).dot(simCell.column(0)) / a.x();
		c.y() = (simCell.column(1).dot(simCell.column(2)) - b.x()*c.x()) / b.y();
		c.z() = sqrt(simCell.column(2).squaredLength() - c.x()*c.x() - c.y()*c.y());
		transformCoordinates = true;
		transformation = AffineTransformation(a,b,c,simCell.translation()) * simCell.inverse();
	}
	else {
		a = simCell.column(0);
		b = simCell.column(1);
		c = simCell.column(2);
		transformCoordinates = false;
	}

	FloatType xlo = simCell.translation().x();
	FloatType ylo = simCell.translation().y();
	FloatType zlo = simCell.translation().z();
	FloatType xhi = a.x() + xlo;
	FloatType yhi = b.y() + ylo;
	FloatType zhi = c.z() + zlo;
	FloatType xy = b.x();
	FloatType xz = c.x();
	FloatType yz = c.y();

	textStream() << "# LAMMPS data file written by OVITO\n";
	textStream() << posProperty->size() << " atoms\n";

	if(particleTypeProperty && particleTypeProperty->size() > 0) {
		int numParticleTypes = std::max(
				particleTypeProperty->particleTypes().size(),
				*std::max_element(particleTypeProperty->constDataInt(), particleTypeProperty->constDataInt() + particleTypeProperty->size()));
		textStream() << numParticleTypes << " atom types\n";
	}
	else textStream() << "1 atom types\n";

	textStream() << xlo << ' ' << xhi << " xlo xhi\n";
	textStream() << ylo << ' ' << yhi << " ylo yhi\n";
	textStream() << zlo << ' ' << zhi << " zlo zhi\n";
	if(xy != 0 || xz != 0 || yz != 0) {
		textStream() << xy << ' ' << xz << ' ' << yz << " xy xz yz\n";
	}
	textStream() << '\n';

	size_t totalProgressCount = posProperty->size();
	if(velocityProperty) totalProgressCount += posProperty->size();
	size_t currentProgress = 0;

	// Write atomic positions.
	textStream() << "Atoms\n\n";

	const Point3* p = posProperty->constDataPoint3();
	for(size_t i = 0; i < posProperty->size(); i++, ++p) {
		textStream() << (identifierProperty ? identifierProperty->getInt(i) : (i+1));
		textStream() << ' ';
		textStream() << (particleTypeProperty ? particleTypeProperty->getInt(i) : 1);
		if(!transformCoordinates) {
			for(size_t k = 0; k < 3; k++)
				textStream() << ' ' << (*p)[k];
		}
		else {
			for(size_t k = 0; k < 3; k++)
				textStream() << ' ' << transformation.prodrow(*p, k);
		}
		if(periodicImageProperty) {
			const Point3I& pbc = periodicImageProperty->getPoint3I(i);
			for(size_t k = 0; k < 3; k++) {
				textStream() << ' ' << pbc[k];
			}
		}
		textStream() << '\n';

		currentProgress++;
		if((currentProgress % 4096) == 0) {
			progress.setPercentage(currentProgress * 100 / totalProgressCount);
			if(progress.wasCanceled())
				return false;
		}
	}

	// Write atomic velocities.
	if(velocityProperty) {
		textStream() << "\nVelocities\n\n";
		const Vector3* v = velocityProperty->constDataVector3();
		for(size_t i = 0; i < velocityProperty->size(); i++, ++v) {
			textStream() << (identifierProperty ? identifierProperty->getInt(i) : (i+1));
			if(!transformCoordinates) {
				for(size_t k = 0; k < 3; k++)
					textStream() << ' ' << (*v)[k];
			}
			else {
				for(size_t k = 0; k < 3; k++)
					textStream() << ' ' << transformation.prodrow(*v, k);
			}
			textStream() << '\n';

			currentProgress++;
			if((currentProgress % 4096) == 0) {
				progress.setPercentage(currentProgress * 100 / totalProgressCount);
				if(progress.wasCanceled())
					return false;
			}
		}
	}

	return true;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
