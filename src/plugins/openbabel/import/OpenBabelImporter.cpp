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

#include <plugins/particles/Particles.h>
#include <core/utilities/concurrent/Future.h>
#include <core/dataset/importexport/FileSource.h>
#include "OpenBabelImporter.h"

#include <openbabel/obconversion.h>
#include <openbabel/mol.h>
#include <openbabel/obiter.h>
#include <openbabel/generic.h>
#include <openbabel/bond.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import) OVITO_BEGIN_INLINE_NAMESPACE(Formats)

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(OpenBabelImporter, ParticleImporter);

/******************************************************************************
* Parses the given input file and stores the data in this container object.
******************************************************************************/
void OpenBabelImporter::OpenBabelImportTask::parseFile(CompressedTextReader& stream)
{
	setProgressText(tr("Reading file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// First close text stream.
	QFileDevice& file = stream.device();
	file.close();

	OpenBabel::OBConversion obconversion;
	OpenBabel::OBMol mol;

	obconversion.SetInFormat(_obFormat);
	obconversion.ReadFile(&mol, file.fileName().toStdString());

	// Create the particle properties.
	ParticleProperty* posProperty = new ParticleProperty(mol.NumAtoms(), ParticleProperty::PositionProperty, 0, false);
	addParticleProperty(posProperty);
	ParticleProperty* typeProperty = new ParticleProperty(mol.NumAtoms(), ParticleProperty::ParticleTypeProperty, 0, false);
	ParticleFrameLoader::ParticleTypeList* typeList = new ParticleFrameLoader::ParticleTypeList();
	addParticleProperty(typeProperty, typeList);
	ParticleProperty* identifierProperty = new ParticleProperty(mol.NumAtoms(), ParticleProperty::IdentifierProperty, 0, false);
	addParticleProperty(identifierProperty);

	// Transfer atoms.
	Point3* pos = posProperty->dataPoint3();
	int* type = typeProperty->dataInt();
	int* id = identifierProperty->dataInt();
	FOR_ATOMS_OF_MOL(obatom, mol) {
		pos->x() = (FloatType)obatom->x();
		pos->y() = (FloatType)obatom->y();
		pos->z() = (FloatType)obatom->z();

		// Remove digits at the end of the type name.
		const char* typeName = obatom->GetType();
		const char* typeNameEnd = typeName;
		while(*typeNameEnd != '\0' && !std::isdigit(*typeNameEnd))
			++typeNameEnd;

		*type = typeList->addParticleTypeName(typeName, typeNameEnd);
		*id = obatom->GetId();

		++pos;
		++type;
		++id;
	}

	// Since we created particle types on the go while reading the particles, the assigned particle type IDs
	// depend on the storage order of particles in the file. We rather want a well-defined particle type ordering, that's
	// why we sort them now.
	typeList->sortParticleTypesByName(typeProperty);

	// Transfer bonds.
	if(mol.NumBonds() > 0) {
		setBonds(new BondsStorage());
		FOR_BONDS_OF_MOL(obbond, mol) {
			unsigned int atomIndex1 = obbond->GetBeginAtomIdx() - 1;
			unsigned int atomIndex2 = obbond->GetEndAtomIdx() - 1;
			bonds()->push_back({ Vector_3<int8_t>::Zero(), atomIndex1, atomIndex2 });
			bonds()->push_back({ Vector_3<int8_t>::Zero(), atomIndex2, atomIndex1 });
		}
	}

	// Transfer simulation cell.
	if(OpenBabel::OBUnitCell* obcell = static_cast<OpenBabel::OBUnitCell*>(mol.GetData(OpenBabel::OBGenericDataType::UnitCell))) {
		std::vector<OpenBabel::vector3> cellVectors = obcell->GetCellVectors();
		OpenBabel::vector3 cellOrigin = obcell->GetOffset();
		AffineTransformation cell;
		cell.column(0) = Vector3((FloatType)cellVectors[0].x(), (FloatType)cellVectors[0].y(), (FloatType)cellVectors[0].z());
		cell.column(1) = Vector3((FloatType)cellVectors[1].x(), (FloatType)cellVectors[1].y(), (FloatType)cellVectors[1].z());
		cell.column(2) = Vector3((FloatType)cellVectors[2].x(), (FloatType)cellVectors[2].y(), (FloatType)cellVectors[2].z());
		cell.column(3) = Vector3((FloatType)cellOrigin.x(), (FloatType)cellOrigin.y(), (FloatType)cellOrigin.z());
		simulationCell().setMatrix(cell);
		simulationCell().setPbcFlags(true, true, true);
	}
	else {
		// If the input file does not contain simulation cell info,
		// Use bounding box of particles as simulation cell.

		Box3 boundingBox;
		boundingBox.addPoints(posProperty->constDataPoint3(), posProperty->size());
		simulationCell().setMatrix(AffineTransformation(
				Vector3(boundingBox.sizeX(), 0, 0),
				Vector3(0, boundingBox.sizeY(), 0),
				Vector3(0, 0, boundingBox.sizeZ()),
				boundingBox.minc - Point3::Origin()));
		simulationCell().setPbcFlags(false, false, false);
	}
	setStatus(tr("%1 atoms and %2 bonds").arg(mol.NumAtoms()).arg(mol.NumBonds()));
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
