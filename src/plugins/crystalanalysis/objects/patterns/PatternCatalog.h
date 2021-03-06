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

#ifndef __OVITO_CA_PATTERN_CATALOG_H
#define __OVITO_CA_PATTERN_CATALOG_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/scene/objects/DataObject.h>
#include "StructurePattern.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A catalog of structure patterns.
 */
class OVITO_CRYSTALANALYSIS_EXPORT PatternCatalog : public DataObject
{
public:

	/// \brief Constructor.
	Q_INVOKABLE PatternCatalog(DataSet* dataset);

	/// Returns the list of structure patterns in this catalog.
	const QVector<StructurePattern*>& patterns() const { return _patterns; }

	/// Adds a new patterns to the catalog.
	void addPattern(StructurePattern* pattern) { _patterns.push_back(pattern); }

	/// Removes a pattern from the catalog.
	void removePattern(int index) { _patterns.remove(index); }

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("Pattern catalog"); }

	/// Returns the structure pattern with the given ID, or NULL if no such structure exists.
	StructurePattern* structureById(int id) const {
		for(StructurePattern* stype : patterns())
			if(stype->id() == id)
				return stype;
		return nullptr;
	}

private:

	/// List of structure patterns.
	VectorReferenceField<StructurePattern> _patterns;

	Q_OBJECT
	OVITO_OBJECT

	DECLARE_VECTOR_REFERENCE_FIELD(_patterns);
};

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // __OVITO_CA_PATTERN_CATALOG_H
