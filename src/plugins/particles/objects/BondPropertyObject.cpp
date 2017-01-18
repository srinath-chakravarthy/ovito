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
#include "BondPropertyObject.h"
#include "BondTypeProperty.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(BondPropertyObject, DataObject);

/******************************************************************************
* Constructor.
******************************************************************************/
BondPropertyObject::BondPropertyObject(DataSet* dataset, BondProperty* storage)
	: DataObjectWithSharedStorage(dataset, storage ? storage : new BondProperty())
{
}

/******************************************************************************
* Factory function that creates a user-defined property object.
******************************************************************************/
OORef<BondPropertyObject> BondPropertyObject::createUserProperty(DataSet* dataset, size_t bondCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory)
{
	return createFromStorage(dataset, new BondProperty(bondCount, dataType, componentCount, stride, name, initializeMemory));
}

/******************************************************************************
* Factory function that creates a standard property object.
******************************************************************************/
OORef<BondPropertyObject> BondPropertyObject::createStandardProperty(DataSet* dataset, size_t bondCount, BondProperty::Type which, size_t componentCount, bool initializeMemory)
{
	return createFromStorage(dataset, new BondProperty(bondCount, which, componentCount, initializeMemory));
}

/******************************************************************************
* Factory function that creates a property object based on an existing storage.
******************************************************************************/
OORef<BondPropertyObject> BondPropertyObject::createFromStorage(DataSet* dataset, BondProperty* storage)
{
	OORef<BondPropertyObject> propertyObj;

	switch(storage->type()) {
	case BondProperty::BondTypeProperty:
		propertyObj = new BondTypeProperty(dataset, storage);
		break;
	default:
		propertyObj = new BondPropertyObject(dataset, storage);
	}

	return propertyObj;
}

/******************************************************************************
* Resizes the property storage.
******************************************************************************/
void BondPropertyObject::resize(size_t newSize, bool preserveData)
{
	if(preserveData) {
		// If existing data should be preserved, resize existing storage.
		modifiableStorage()->resize(newSize, true);
		changed();
	}
	else {
		// If data should not be preserved, allocate new storage to replace old one.
		// This avoids calling the BondProperty copy constructor unnecessarily.
		if(type() != BondProperty::UserProperty)
			setStorage(new BondProperty(newSize, type(), componentCount(), false));
		else
			setStorage(new BondProperty(newSize, dataType(), componentCount(), stride(), name(), false));
	}
}

/******************************************************************************
* Sets the property's name.
******************************************************************************/
void BondPropertyObject::setName(const QString& newName)
{
	if(newName == name())
		return;

	// Make the property change undoable.
	dataset()->undoStack().pushIfRecording<SimplePropertyChangeOperation>(this, "name");

	modifiableStorage()->setName(newName);
	changed();
	notifyDependents(ReferenceEvent::TitleChanged);
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void BondPropertyObject::saveToStream(ObjectSaveStream& stream)
{
	DataObject::saveToStream(stream);

	stream.beginChunk(0x01);
	storage()->saveToStream(stream, !saveWithScene());
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void BondPropertyObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObject::loadFromStream(stream);

	stream.expectChunk(0x01);
	modifiableStorage()->loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
* This helper method returns a standard bond property (if present) from the
* given pipeline state.
******************************************************************************/
BondPropertyObject* BondPropertyObject::findInState(const PipelineFlowState& state, BondProperty::Type type)
{
	for(DataObject* o : state.objects()) {
		BondPropertyObject* prop = dynamic_object_cast<BondPropertyObject>(o);
		if(prop && prop->type() == type)
			return prop;
	}
	return nullptr;
}

/******************************************************************************
* This helper method returns a specific user-defined bond property (if present) from the
* given pipeline state.
******************************************************************************/
BondPropertyObject* BondPropertyObject::findInState(const PipelineFlowState& state, const QString& name)
{
	for(DataObject* o : state.objects()) {
		BondPropertyObject* prop = dynamic_object_cast<BondPropertyObject>(o);
		if(prop && prop->type() == BondProperty::UserProperty && prop->name() == name)
			return prop;
	}
	return nullptr;
}

/******************************************************************************
* This helper method finds the bond property referenced by this BondPropertyReference
* in the given pipeline state.
******************************************************************************/
BondPropertyObject* BondPropertyReference::findInState(const PipelineFlowState& state) const
{
	if(isNull())
		return nullptr;
	for(DataObject* o : state.objects()) {
		BondPropertyObject* prop = dynamic_object_cast<BondPropertyObject>(o);
		if(prop) {
			if((this->type() == BondProperty::UserProperty && prop->name() == this->name()) ||
					(this->type() != BondProperty::UserProperty && prop->type() == this->type())) {
				return prop;
			}
		}
	}
	return nullptr;
}

}	// End of namespace
}	// End of namespace
