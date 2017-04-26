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
#include <core/dataset/importexport/FileSource.h>
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/objects/SimulationCellDisplay.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/data/BondProperty.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/ParticleDisplay.h>
#include <plugins/particles/objects/ParticleType.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include <plugins/particles/objects/BondTypeProperty.h>
#include <plugins/particles/objects/BondType.h>
#include <plugins/particles/objects/FieldQuantityObject.h>
#include "ParticleFrameLoader.h"
#include "ParticleImporter.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Import)

/******************************************************************************
* Reads the data from the input file(s).
******************************************************************************/
void ParticleFrameLoader::perform()
{
	setProgressText(ParticleImporter::tr("Reading file %1").arg(frame().sourceFile.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

	// Fetch file.
	Future<QString> fetchFileFuture = Application::instance()->fileManager()->fetchUrl(datasetContainer(), frame().sourceFile);
	if(!waitForSubTask(fetchFileFuture))
		return;
	OVITO_ASSERT(fetchFileFuture.isCanceled() == false);

	// Open file.
	QFile file(fetchFileFuture.result());
	CompressedTextReader stream(file, frame().sourceFile.path());

	// Seek to byte offset of requested frame.
	if(frame().byteOffset != 0)
		stream.seek(frame().byteOffset);

	// Parse file.
	parseFile(stream);
}

/******************************************************************************
* Sorts the particle types w.r.t. their name. Reassigns the per-particle type IDs.
* This method is used by file parsers that create particle types on the go while the read the particle data.
* In such a case, the assignment of IDs to types depends on the storage order of particles in the file, which is not desirable.
******************************************************************************/
void ParticleFrameLoader::ParticleTypeList::sortParticleTypesByName(ParticleProperty* typeProperty)
{
	// Check if type IDs form a consecutive sequence starting at 1.
	for(size_t index = 0; index < _particleTypes.size(); index++) {
		if(_particleTypes[index].id != index + 1)
			return;
	}

	// Check if types are already in the correct order.
	auto compare = [](const ParticleTypeDefinition& a, const ParticleTypeDefinition& b) -> bool { return a.name.compare(b.name) < 0; };
	if(std::is_sorted(_particleTypes.begin(), _particleTypes.end(), compare))
		return;

	// Reorder types.
	std::sort(_particleTypes.begin(), _particleTypes.end(), compare);

	// Build map of IDs.
	std::vector<int> mapping(_particleTypes.size() + 1);
	for(size_t index = 0; index < _particleTypes.size(); index++) {
		mapping[_particleTypes[index].id] = index + 1;
		_particleTypes[index].id = index + 1;
	}

	// Remap particle type IDs.
	if(typeProperty) {
		for(int& t : typeProperty->intRange()) {
			OVITO_ASSERT(t >= 1 && t < mapping.size());
			t = mapping[t];
		}
	}
}

/******************************************************************************
* Sorts particle types with ascending identifier.
******************************************************************************/
void ParticleFrameLoader::ParticleTypeList::sortParticleTypesById()
{
	auto compare = [](const ParticleTypeDefinition& a, const ParticleTypeDefinition& b) -> bool { return a.id < b.id; };
	std::sort(_particleTypes.begin(), _particleTypes.end(), compare);
}

/******************************************************************************
* Inserts the data loaded by perform() into the provided container object.
* This function is called by the system from the main thread after the
* asynchronous loading task has finished.
******************************************************************************/
void ParticleFrameLoader::handOver(CompoundObject* container)
{
	QSet<DataObject*> activeObjects;

	// Transfer simulation cell.
	OORef<SimulationCellObject> cell = container->findDataObject<SimulationCellObject>();
	if(!cell) {
		cell = new SimulationCellObject(container->dataset(), simulationCell());

		// Set up display object for the simulation cell.
		if(!cell->displayObjects().empty()) {
			if(SimulationCellDisplay* cellDisplay = dynamic_object_cast<SimulationCellDisplay>(cell->displayObjects().front())) {
				cellDisplay->loadUserDefaults();

				// Choose an appropriate line width for the cell size.
				FloatType cellDiameter = (
						simulationCell().matrix().column(0) +
						simulationCell().matrix().column(1) +
						simulationCell().matrix().column(2)).length();
				cellDisplay->setCellLineWidth(cellDiameter * FloatType(1.4e-3));
			}
		}

		container->addDataObject(cell);
	}
	else {
		// Adopt pbc flags from input file only if it is a new file.
		// This gives the user the option to change the pbc flags without them
		// being overwritten when a new frame from a simulation sequence is loaded.
		cell->setData(simulationCell(), _isNewFile);
	}
	activeObjects.insert(cell);

	// Transfer particle properties.
	for(auto& property : _particleProperties) {
		OORef<ParticlePropertyObject> propertyObj;
		for(const auto& dataObj : container->dataObjects()) {
			ParticlePropertyObject* po = dynamic_object_cast<ParticlePropertyObject>(dataObj);
			if(po != nullptr && po->type() == property->type() && po->name() == property->name()) {
				propertyObj = po;
				break;
			}
		}

		if(propertyObj) {
			propertyObj->setStorage(QSharedDataPointer<ParticleProperty>(property.release()));
		}
		else {
			propertyObj = ParticlePropertyObject::createFromStorage(container->dataset(), QSharedDataPointer<ParticleProperty>(property.release()));
			container->addDataObject(propertyObj);
		}

		// Transfer particle types.
		if(ParticleTypeList* typeList = getTypeListOfParticleProperty(propertyObj->storage())) {
			insertParticleTypes(propertyObj, typeList);
		}

		activeObjects.insert(propertyObj);
	}

	// Transfer bonds.
	if(bonds()) {
		OORef<BondsObject> bondsObj = container->findDataObject<BondsObject>();
		QExplicitlySharedDataPointer<BondsStorage> bondsPtr(_bonds.release());
		if(!bondsObj) {
			bondsObj = new BondsObject(container->dataset(), bondsPtr.data());

			// Set up display object for the bonds.
			if(!bondsObj->displayObjects().empty()) {
				if(BondsDisplay* bondsDisplay = dynamic_object_cast<BondsDisplay>(bondsObj->displayObjects().front())) {
					bondsDisplay->loadUserDefaults();
				}
			}

			container->addDataObject(bondsObj);
		}
		else {
			bondsObj->setStorage(bondsPtr.data());
		}
		activeObjects.insert(bondsObj);

		// Transfer bond properties.
		for(auto& property : _bondProperties) {
			OORef<BondPropertyObject> propertyObj;
			for(const auto& dataObj : container->dataObjects()) {
				BondPropertyObject* po = dynamic_object_cast<BondPropertyObject>(dataObj);
				if(po != nullptr && po->type() == property->type() && po->name() == property->name()) {
					propertyObj = po;
					break;
				}
			}

			if(propertyObj) {
				propertyObj->setStorage(QSharedDataPointer<BondProperty>(property.release()));
			}
			else {
				propertyObj = BondPropertyObject::createFromStorage(container->dataset(), QSharedDataPointer<BondProperty>(property.release()));
				container->addDataObject(propertyObj);
			}

			// Transfer bond types.
			if(BondTypeList* typeList = getTypeListOfBondProperty(propertyObj->storage())) {
				insertBondTypes(propertyObj, typeList);
			}

			activeObjects.insert(propertyObj);
		}
	}

	// Transfer field quantities.
	for(auto& fq : _fieldQuantities) {
		OORef<FieldQuantityObject> fqObj;
		for(const auto& dataObj : container->dataObjects()) {
			FieldQuantityObject* po = dynamic_object_cast<FieldQuantityObject>(dataObj);
			if(po != nullptr && po->name() == fq->name()) {
				fqObj = po;
				break;
			}
		}

		if(fqObj) {
			fqObj->setStorage(QSharedDataPointer<FieldQuantity>(fq.release()));
		}
		else {
			fqObj = FieldQuantityObject::createFromStorage(container->dataset(), QSharedDataPointer<FieldQuantity>(fq.release()));
			container->addDataObject(fqObj);
		}

		activeObjects.insert(fqObj);
	}

	// Pass timestep information and other metadata to modification pipeline.
	container->setAttributes(attributes());

	container->removeInactiveObjects(activeObjects);
}

/******************************************************************************
* Inserts the stores particle types into the given destination object.
******************************************************************************/
void ParticleFrameLoader::insertParticleTypes(ParticlePropertyObject* propertyObj, ParticleTypeList* typeList)
{
	ParticleTypeProperty* typeProperty = dynamic_object_cast<ParticleTypeProperty>(propertyObj);
	if(!typeProperty)
		return;

	QSet<ParticleType*> activeTypes;
	if(typeList) {
		for(const auto& item : typeList->particleTypes()) {
			QString name = item.name;
			if(name.isEmpty())
				name = ParticleImporter::tr("Type %1").arg(item.id);
			OORef<ParticleType> ptype = typeProperty->particleType(name);
			if(ptype) {
				ptype->setId(item.id);
			}
			else {
				ptype = typeProperty->particleType(item.id);
				if(ptype) {
					if(item.name.isEmpty() == false)
						ptype->setName(item.name);
				}
				else {
					ptype = new ParticleType(typeProperty->dataset());
					ptype->setId(item.id);
					ptype->setName(name);

					// Assign initial standard color to new particle types.
					if(item.color != Color(0,0,0))
						ptype->setColor(item.color);
					else
						ptype->setColor(ParticleTypeProperty::getDefaultParticleColor(propertyObj->type(), name, ptype->id()));

					if(item.radius == 0)
						ptype->setRadius(ParticleTypeProperty::getDefaultParticleRadius(propertyObj->type(), name, ptype->id()));

					typeProperty->addParticleType(ptype);
				}
			}
			activeTypes.insert(ptype);

			if(item.color != Color(0,0,0))
				ptype->setColor(item.color);

			if(item.radius != 0)
				ptype->setRadius(item.radius);
		}
	}

	if(_isNewFile) {
		// Remove unused particle types.
		for(int index = typeProperty->particleTypes().size() - 1; index >= 0; index--) {
			if(!activeTypes.contains(typeProperty->particleTypes()[index]))
				typeProperty->removeParticleType(index);
		}
	}
}

/******************************************************************************
* Inserts the stores bond types into the given destination object.
******************************************************************************/
void ParticleFrameLoader::insertBondTypes(BondPropertyObject* propertyObj, BondTypeList* typeList)
{
	BondTypeProperty* typeProperty = dynamic_object_cast<BondTypeProperty>(propertyObj);
	if(!typeProperty)
		return;

	QSet<BondType*> activeTypes;
	if(typeList) {
		for(const auto& item : typeList->bondTypes()) {
			OORef<BondType> type = typeProperty->bondType(item.id);
			QString name = item.name;
			if(name.isEmpty())
				name = ParticleImporter::tr("Type %1").arg(item.id);

			if(type == nullptr) {
				type = new BondType(typeProperty->dataset());
				type->setId(item.id);

				// Assign initial standard color to new bond type.
				if(item.color != Color(0,0,0))
					type->setColor(item.color);
				else
					type->setColor(BondTypeProperty::getDefaultBondColor(propertyObj->type(), name, type->id()));

				if(item.radius == 0)
					type->setRadius(BondTypeProperty::getDefaultBondRadius(propertyObj->type(), name, type->id()));

				typeProperty->addBondType(type);
			}
			activeTypes.insert(type);

			if(type->name().isEmpty())
				type->setName(name);

			if(item.color != Color(0,0,0))
				type->setColor(item.color);

			if(item.radius != 0)
				type->setRadius(item.radius);
		}
	}

	// Remove unused bond types.
	for(int index = typeProperty->bondTypes().size() - 1; index >= 0; index--) {
		if(!activeTypes.contains(typeProperty->bondTypes()[index]))
			typeProperty->removeBondType(index);
	}
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
