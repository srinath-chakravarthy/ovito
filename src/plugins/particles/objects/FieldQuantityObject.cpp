///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include "FieldQuantityObject.h"

namespace Ovito { namespace Particles {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(FieldQuantityObject, DataObject);

/******************************************************************************
* Constructor.
******************************************************************************/
FieldQuantityObject::FieldQuantityObject(DataSet* dataset, FieldQuantity* storage)
	: DataObjectWithSharedStorage(dataset, storage ? storage : new FieldQuantity())
{
}

/******************************************************************************
* Factory function that creates a user-defined property object.
******************************************************************************/
OORef<FieldQuantityObject> FieldQuantityObject::createFieldQuantity(DataSet* dataset, std::vector<size_t> shape, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory)
{
	return createFromStorage(dataset, new FieldQuantity(std::move(shape), dataType, componentCount, stride, name, initializeMemory));
}

/******************************************************************************
* Factory function that creates a property object based on an existing storage.
******************************************************************************/
OORef<FieldQuantityObject> FieldQuantityObject::createFromStorage(DataSet* dataset, FieldQuantity* storage)
{
	OORef<FieldQuantityObject> obj = new FieldQuantityObject(dataset, storage);
	return obj;
}

/******************************************************************************
* Sets the quantity's name.
******************************************************************************/
void FieldQuantityObject::setName(const QString& newName)
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
void FieldQuantityObject::saveToStream(ObjectSaveStream& stream)
{
	DataObject::saveToStream(stream);

	stream.beginChunk(0x01);
	storage()->saveToStream(stream, !saveWithScene());
	stream.endChunk();
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void FieldQuantityObject::loadFromStream(ObjectLoadStream& stream)
{
	DataObject::loadFromStream(stream);

	stream.expectChunk(0x01);
	modifiableStorage()->loadFromStream(stream);
	stream.closeChunk();
}

/******************************************************************************
* This helper method returns a specific field quantity (if present) from the
* given pipeline state.
******************************************************************************/
FieldQuantityObject* FieldQuantityObject::findInState(const PipelineFlowState& state, const QString& name)
{
	for(DataObject* o : state.objects()) {
		FieldQuantityObject* fieldQuantity = dynamic_object_cast<FieldQuantityObject>(o);
		if(fieldQuantity && fieldQuantity->name() == name)
			return fieldQuantity;
	}
	return nullptr;
}

/******************************************************************************
* This helper method finds the field quantity referenced by this FieldQuantityReference
* in the given pipeline state.
******************************************************************************/
FieldQuantityObject* FieldQuantityReference::findInState(const PipelineFlowState& state) const
{
	if(isNull())
		return nullptr;
	for(DataObject* o : state.objects()) {
		if(FieldQuantityObject* fq = dynamic_object_cast<FieldQuantityObject>(o)) {
			if(fq->name() == this->name()) {
				return fq;
			}
		}
	}
	return nullptr;
}

}	// End of namespace
}	// End of namespace
