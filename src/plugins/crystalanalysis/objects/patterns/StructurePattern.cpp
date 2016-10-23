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

#include <core/Core.h>
#include "StructurePattern.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, StructurePattern, ParticleType);
DEFINE_PROPERTY_FIELD(StructurePattern, _shortName, "ShortName");
DEFINE_PROPERTY_FIELD(StructurePattern, _structureType, "StructureType");
DEFINE_PROPERTY_FIELD(StructurePattern, _symmetryType, "SymmetryType");
DEFINE_VECTOR_REFERENCE_FIELD(StructurePattern, _burgersVectorFamilies, "BurgersVectorFamilies", BurgersVectorFamily);
SET_PROPERTY_FIELD_LABEL(StructurePattern, _shortName, "Short name");
SET_PROPERTY_FIELD_LABEL(StructurePattern, _structureType, "Structure type");
SET_PROPERTY_FIELD_LABEL(StructurePattern, _symmetryType, "Symmetry type");
SET_PROPERTY_FIELD_LABEL(StructurePattern, _burgersVectorFamilies, "Burgers vector families");

/******************************************************************************
* Constructs the StructurePattern object.
******************************************************************************/
StructurePattern::StructurePattern(DataSet* dataset) : ParticleType(dataset),
		_structureType(OtherStructure), _symmetryType(OtherSymmetry)
{
	INIT_PROPERTY_FIELD(StructurePattern::_shortName);
	INIT_PROPERTY_FIELD(StructurePattern::_structureType);
	INIT_PROPERTY_FIELD(StructurePattern::_symmetryType);
	INIT_PROPERTY_FIELD(StructurePattern::_burgersVectorFamilies);

	// Create "unknown" Burgers vector family.
	BurgersVectorFamily* family = new BurgersVectorFamily(dataset);
	family->setColor(Color(0.9f, 0.2f, 0.2f));
	family->setName(tr("Other"));
	family->setBurgersVector(Vector3::Zero());
	addBurgersVectorFamily(family);
}

/******************************************************************************
* Returns the display color to be used for a given Burgers vector.
******************************************************************************/
Color StructurePattern::getBurgersVectorColor(const QString& latticeName, const Vector3& b)
{
	static const Color predefinedLineColors[] = {
			Color(0.4f,1.0f,0.4f),
			Color(1.0f,0.2f,0.2f),
			Color(0.4f,0.4f,1.0f),
			Color(0.9f,0.5f,0.0f),
			Color(1.0f,1.0f,0.0f),
			Color(1.0f,0.4f,1.0f),
			Color(0.7f,0.0f,1.0f),
			Color(0.2f,1.0f,1.0f)
	};

	if(latticeName == "bcc") {
		static const Vector3 bccBurgersVectors[] = {
				{ 0.5, 0.5, 0.5 },
				{ -0.5, 0.5, 0.5 },
				{ 0.5, -0.5, 0.5 },
				{ 0.5, 0.5, -0.5 },
				{ 1.0, 0.0, 0.0 },
				{ 0.0, 1.0, 0.0 },
				{ 0.0, 0.0, 1.0 },
		};
		OVITO_ASSERT(sizeof(bccBurgersVectors)/sizeof(bccBurgersVectors[0]) < sizeof(predefinedLineColors)/sizeof(predefinedLineColors[0]));
		for(size_t i = 0; i < sizeof(bccBurgersVectors)/sizeof(bccBurgersVectors[0]); i++) {
			if(b.equals(bccBurgersVectors[i]) || b.equals(-bccBurgersVectors[i]))
				return predefinedLineColors[i];
		}
	}
	return Color(0.9f, 0.9f, 0.9f);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
