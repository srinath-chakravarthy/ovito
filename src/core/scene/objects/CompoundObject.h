///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2014) Alexander Stukowski
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


#include <core/Core.h>
#include <core/scene/objects/DataObject.h>
#include <core/scene/pipeline/PipelineFlowState.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(ObjectSystem) OVITO_BEGIN_INLINE_NAMESPACE(Scene)

/**
 * \brief A DataObject that stores a collection of other \ref DataObject "DataObjects".
 */
class OVITO_CORE_EXPORT CompoundObject : public DataObject
{
public:

	/// Constructs an empty compound data object.
	Q_INVOKABLE CompoundObject(DataSet* dataset);

	/// Asks the object for the result of the data pipeline.
	virtual PipelineFlowState evaluateImmediately(const PipelineEvalRequest& request) override;

	/// \brief Inserts a new object into the list of data objects held by this container object.
	void addDataObject(DataObject* obj) {
		if(!_dataObjects.contains(obj))
			_dataObjects.push_back(obj);
	}

	/// \brief Inserts a new object into the list of data objects held by this container object.
	void insertDataObject(int index, DataObject* obj) {
		OVITO_ASSERT(!_dataObjects.contains(obj));
		if(!_dataObjects.contains(obj))
			_dataObjects.insert(index, obj);
	}

	/// \brief Removes a data object from the compound.
	void removeDataObjectByIndex(int index) {
		_dataObjects.remove(index);
	}

	/// \brief Removes a data object from the compound.
	void removeDataObject(DataObject* obj) {
		int index = _dataObjects.indexOf(obj);
		OVITO_ASSERT(index >= 0);
		if(index >= 0)
			removeDataObjectByIndex(index);
	}

	/// \brief Replaces a data object in the compound.
	void replaceDataObject(DataObject* oldObj, DataObject* newObj) {
		OVITO_ASSERT(newObj != nullptr);
		OVITO_ASSERT(oldObj != nullptr);
		int index = _dataObjects.indexOf(oldObj);
		if(index >= 0) {
			_dataObjects.remove(index);
			_dataObjects.insert(index, newObj);
		}
	}

	/// Replaces all data objects stored in this compound object with the data objects
	/// stored in the pipeline flow state.
	void setDataObjects(const PipelineFlowState& state);

	/// \brief Looks for an object of the given type in the list of data objects and returns it.
	template<class T>
	T* findDataObject() const {
		for(DataObject* obj : dataObjects()) {
			T* castObj = dynamic_object_cast<T>(obj);
			if(castObj) return castObj;
		}
		return nullptr;
	}

	/// \brief Removes all data objects owned by this CompoundObject that are not
	///        listed in the given set of active objects.
	void removeInactiveObjects(const QSet<DataObject*>& activeObjects) {
		for(int index = _dataObjects.size() - 1; index >= 0; index--)
			if(!activeObjects.contains(_dataObjects[index]))
				_dataObjects.remove(index);
	}

	/// Returns the attributes set or loaded by the file importer which are fed into the modification pipeline
	/// along with the data objects.
	const QVariantMap& attributes() const { return _attributes; }

	/// Sets the attributes that will be fed into the modification pipeline
	/// along with the data objects.
	void setAttributes(const QVariantMap& attributes) { _attributes = attributes; }

	/// Resets the attributes that will be fed into the modification pipeline
	/// along with the data objects.
	void clearAttributes() { _attributes.clear(); }

	/// Returns the number of sub-objects that should be displayed in the modifier stack.
	virtual int editableSubObjectCount() override;

	/// Returns a sub-object that should be listed in the modifier stack.
	virtual RefTarget* editableSubObject(int index) override;

protected:

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceInserted(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

	/// Is called when a RefTarget has been added to a VectorReferenceField of this RefMaker.
	virtual void referenceRemoved(const PropertyFieldDescriptor& field, RefTarget* newTarget, int listIndex) override;

private:

	/// Stores the data objects in the compound.
	DECLARE_VECTOR_REFERENCE_FIELD(DataObject, dataObjects);

	/// Attributes set or loaded by the file importer which will be fed into the modification pipeline
	/// along with the data objects.
	QVariantMap _attributes;

private:

	Q_CLASSINFO("DisplayName", "Compound data object");

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


