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
#include "ParticleProperty.h"

namespace Ovito { namespace Particles {

/******************************************************************************
* Constructor for user-defined properties.
******************************************************************************/
ParticleProperty::ParticleProperty(size_t particleCount, int dataType, size_t componentCount, size_t stride, const QString& name, bool initializeMemory)
	: PropertyBase(particleCount, dataType, componentCount, stride, name, initializeMemory), _type(UserProperty)
{
}

/******************************************************************************
* Constructor for standard properties.
******************************************************************************/
ParticleProperty::ParticleProperty(size_t particleCount, Type type, size_t componentCount, bool initializeMemory)
	: PropertyBase(), _type(type)
{
	switch(type) {
	case ParticleTypeProperty:
	case StructureTypeProperty:
	case SelectionProperty:
	case ClusterProperty:
	case CoordinationProperty:
	case IdentifierProperty:
	case MoleculeProperty:
		_dataType = qMetaTypeId<int>();
		_componentCount = 1;
		_stride = sizeof(int);
		break;
	case PositionProperty:
	case DisplacementProperty:
	case VelocityProperty:
	case ForceProperty:
	case DipoleOrientationProperty:
	case AngularVelocityProperty:
	case AngularMomentumProperty:
	case TorqueProperty:
		_dataType = qMetaTypeId<FloatType>();
		_componentCount = 3;
		_stride = sizeof(Vector3);
		OVITO_ASSERT(_stride == sizeof(Point3));
		break;
	case ColorProperty:
	case VectorColorProperty:
		_dataType = qMetaTypeId<FloatType>();
		_componentCount = 3;
		_stride = _componentCount * sizeof(FloatType);
		OVITO_ASSERT(_stride == sizeof(Color));
		break;
	case PotentialEnergyProperty:
	case KineticEnergyProperty:
	case TotalEnergyProperty:
	case RadiusProperty:
	case MassProperty:
	case ChargeProperty:
	case TransparencyProperty:
	case SpinProperty:
	case DipoleMagnitudeProperty:
	case CentroSymmetryProperty:
	case DisplacementMagnitudeProperty:
	case VelocityMagnitudeProperty:
		_dataType = qMetaTypeId<FloatType>();
		_componentCount = 1;
		_stride = sizeof(FloatType);
		break;
	case StressTensorProperty:
	case StrainTensorProperty:
		_dataType = qMetaTypeId<FloatType>();
		_componentCount = 6;
		_stride = _componentCount * sizeof(FloatType);
		OVITO_ASSERT(_stride == sizeof(SymmetricTensor2));
		break;
	case DeformationGradientProperty:
		_dataType = qMetaTypeId<FloatType>();
		_componentCount = 9;
		_stride = _componentCount * sizeof(FloatType);
		break;
	case OrientationProperty:
		_dataType = qMetaTypeId<FloatType>();
		_componentCount = 4;
		_stride = _componentCount * sizeof(FloatType);
		OVITO_ASSERT(_stride == sizeof(Quaternion));
		break;
	case PeriodicImageProperty:
		_dataType = qMetaTypeId<int>();
		_componentCount = 3;
		_stride = _componentCount * sizeof(FloatType);
		break;
	default:
		OVITO_ASSERT_MSG(false, "ParticleProperty constructor", "Invalid standard property type");
		throw Exception(ParticlePropertyObject::tr("This is not a valid standard property type: %1").arg(type));
	}
	_dataTypeSize = QMetaType::sizeOf(_dataType);
	OVITO_ASSERT(_dataTypeSize > 0);
	OVITO_ASSERT_MSG(componentCount == 0 || componentCount == _componentCount, "ParticleProperty::ParticleProperty(type)", "Cannot specify component count for a standard property with a fixed component count.");
	OVITO_ASSERT(_stride >= _dataTypeSize * _componentCount);
	OVITO_ASSERT((_stride % _dataTypeSize) == 0);

	_componentNames = standardPropertyComponentNames(type, _componentCount);
	_name = standardPropertyName(type);
	resize(particleCount, initializeMemory);
}

/******************************************************************************
* Copy constructor.
******************************************************************************/
ParticleProperty::ParticleProperty(const ParticleProperty& other)
	: PropertyBase(other), _type(other._type)
{
}

/******************************************************************************
* Saves the class' contents to the given stream.
******************************************************************************/
void ParticleProperty::saveToStream(SaveStream& stream, bool onlyMetadata) const
{
	PropertyBase::saveToStream(stream, onlyMetadata, type());
}

/******************************************************************************
* Loads the class' contents from the given stream.
******************************************************************************/
void ParticleProperty::loadFromStream(LoadStream& stream)
{
	_type = (Type)PropertyBase::loadFromStream(stream);
}

/******************************************************************************
* Returns the name of a standard property.
******************************************************************************/
QString ParticleProperty::standardPropertyName(Type which)
{
	switch(which) {
	case ParticleTypeProperty: return ParticlePropertyObject::tr("Particle Type");
	case SelectionProperty: return ParticlePropertyObject::tr("Selection");
	case ClusterProperty: return ParticlePropertyObject::tr("Cluster");
	case CoordinationProperty: return ParticlePropertyObject::tr("Coordination");
	case PositionProperty: return ParticlePropertyObject::tr("Position");
	case ColorProperty: return ParticlePropertyObject::tr("Color");
	case DisplacementProperty: return ParticlePropertyObject::tr("Displacement");
	case DisplacementMagnitudeProperty: return ParticlePropertyObject::tr("Displacement Magnitude");
	case VelocityProperty: return ParticlePropertyObject::tr("Velocity");
	case PotentialEnergyProperty: return ParticlePropertyObject::tr("Potential Energy");
	case KineticEnergyProperty: return ParticlePropertyObject::tr("Kinetic Energy");
	case TotalEnergyProperty: return ParticlePropertyObject::tr("Total Energy");
	case RadiusProperty: return ParticlePropertyObject::tr("Radius");
	case StructureTypeProperty: return ParticlePropertyObject::tr("Structure Type");
	case IdentifierProperty: return ParticlePropertyObject::tr("Particle Identifier");
	case StressTensorProperty: return ParticlePropertyObject::tr("Stress Tensor");
	case StrainTensorProperty: return ParticlePropertyObject::tr("Strain Tensor");
	case DeformationGradientProperty: return ParticlePropertyObject::tr("Deformation Gradient");
	case OrientationProperty: return ParticlePropertyObject::tr("Orientation");
	case ForceProperty: return ParticlePropertyObject::tr("Force");
	case MassProperty: return ParticlePropertyObject::tr("Mass");
	case ChargeProperty: return ParticlePropertyObject::tr("Charge");
	case PeriodicImageProperty: return ParticlePropertyObject::tr("Periodic Image");
	case TransparencyProperty: return ParticlePropertyObject::tr("Transparency");
	case DipoleOrientationProperty: return ParticlePropertyObject::tr("Dipole Orientation");
	case DipoleMagnitudeProperty: return ParticlePropertyObject::tr("Dipole Magnitude");
	case AngularVelocityProperty: return ParticlePropertyObject::tr("Angular Velocity");
	case AngularMomentumProperty: return ParticlePropertyObject::tr("Angular Momentum");
	case TorqueProperty: return ParticlePropertyObject::tr("Torque");
	case SpinProperty: return ParticlePropertyObject::tr("Spin");
	case CentroSymmetryProperty: return ParticlePropertyObject::tr("Centrosymmetry");
	case VelocityMagnitudeProperty: return ParticlePropertyObject::tr("Velocity Magnitude");
	case MoleculeProperty: return ParticlePropertyObject::tr("Molecule Identifier");
	case AsphericalShapeProperty: return ParticlePropertyObject::tr("Aspherical Shape");
	case VectorColorProperty: return ParticlePropertyObject::tr("Vector Color");
	default:
		OVITO_ASSERT_MSG(false, "ParticleProperty::standardPropertyName", "Invalid standard particle property type");
		throw Exception(ParticlePropertyObject::tr("This is not a valid standard particle property type: %1").arg(which));
	}
}

/******************************************************************************
* Returns the display title used for a standard property object.
******************************************************************************/
QString ParticleProperty::standardPropertyTitle(Type which)
{
	switch(which) {
	case ParticleTypeProperty: return ParticlePropertyObject::tr("Particle types");
	case PositionProperty: return ParticlePropertyObject::tr("Particle positions");
	case ColorProperty: return ParticlePropertyObject::tr("Particle colors");
	case DisplacementProperty: return ParticlePropertyObject::tr("Displacements");
	case VelocityProperty: return ParticlePropertyObject::tr("Velocities");
	case RadiusProperty: return ParticlePropertyObject::tr("Radii");
	case StructureTypeProperty: return ParticlePropertyObject::tr("Structure types");
	case IdentifierProperty: return ParticlePropertyObject::tr("Particle identifiers");
	case VectorColorProperty: return ParticlePropertyObject::tr("Vector colors");
	default:
		return standardPropertyName(which);
	}
}

/******************************************************************************
* Returns the data type used by the given standard property.
******************************************************************************/
int ParticleProperty::standardPropertyDataType(Type which)
{
	switch(which) {
	case ParticleTypeProperty:
	case StructureTypeProperty:
	case SelectionProperty:
	case ClusterProperty:
	case CoordinationProperty:
	case IdentifierProperty:
	case PeriodicImageProperty:
	case MoleculeProperty:
		return qMetaTypeId<int>();
	case PositionProperty:
	case ColorProperty:
	case DisplacementProperty:
	case DisplacementMagnitudeProperty:
	case VelocityProperty:
	case PotentialEnergyProperty:
	case KineticEnergyProperty:
	case TotalEnergyProperty:
	case RadiusProperty:
	case StressTensorProperty:
	case StrainTensorProperty:
	case DeformationGradientProperty:
	case OrientationProperty:
	case ForceProperty:
	case MassProperty:
	case ChargeProperty:
	case TransparencyProperty:
	case DipoleMagnitudeProperty:
	case SpinProperty:
	case DipoleOrientationProperty:
	case AngularVelocityProperty:
	case AngularMomentumProperty:
	case TorqueProperty:
	case CentroSymmetryProperty:
	case VelocityMagnitudeProperty:
	case AsphericalShapeProperty:
	case VectorColorProperty:
		return qMetaTypeId<FloatType>();
	default:
		OVITO_ASSERT_MSG(false, "ParticleProperty::standardPropertyDataType", "Invalid standard particle property type");
		throw Exception(ParticlePropertyObject::tr("This is not a valid standard particle property type: %1").arg(which));
	}
}

/******************************************************************************
* Returns a list with the names and identifiers of all defined standard properties.
******************************************************************************/
QMap<QString, ParticleProperty::Type> ParticleProperty::standardPropertyList()
{
	static QMap<QString, Type> table;
	if(table.empty()) {
		table.insert(standardPropertyName(ParticleTypeProperty), ParticleTypeProperty);
		table.insert(standardPropertyName(SelectionProperty), SelectionProperty);
		table.insert(standardPropertyName(ClusterProperty), ClusterProperty);
		table.insert(standardPropertyName(CoordinationProperty), CoordinationProperty);
		table.insert(standardPropertyName(PositionProperty), PositionProperty);
		table.insert(standardPropertyName(ColorProperty), ColorProperty);
		table.insert(standardPropertyName(DisplacementProperty), DisplacementProperty);
		table.insert(standardPropertyName(DisplacementMagnitudeProperty), DisplacementMagnitudeProperty);
		table.insert(standardPropertyName(VelocityProperty), VelocityProperty);
		table.insert(standardPropertyName(PotentialEnergyProperty), PotentialEnergyProperty);
		table.insert(standardPropertyName(KineticEnergyProperty), KineticEnergyProperty);
		table.insert(standardPropertyName(TotalEnergyProperty), TotalEnergyProperty);
		table.insert(standardPropertyName(RadiusProperty), RadiusProperty);
		table.insert(standardPropertyName(StructureTypeProperty), StructureTypeProperty);
		table.insert(standardPropertyName(IdentifierProperty), IdentifierProperty);
		table.insert(standardPropertyName(StressTensorProperty), StressTensorProperty);
		table.insert(standardPropertyName(StrainTensorProperty), StrainTensorProperty);
		table.insert(standardPropertyName(DeformationGradientProperty), DeformationGradientProperty);
		table.insert(standardPropertyName(OrientationProperty), OrientationProperty);
		table.insert(standardPropertyName(ForceProperty), ForceProperty);
		table.insert(standardPropertyName(MassProperty), MassProperty);
		table.insert(standardPropertyName(ChargeProperty), ChargeProperty);
		table.insert(standardPropertyName(PeriodicImageProperty), PeriodicImageProperty);
		table.insert(standardPropertyName(TransparencyProperty), TransparencyProperty);
		table.insert(standardPropertyName(DipoleOrientationProperty), DipoleOrientationProperty);
		table.insert(standardPropertyName(DipoleMagnitudeProperty), DipoleMagnitudeProperty);
		table.insert(standardPropertyName(AngularVelocityProperty), AngularVelocityProperty);
		table.insert(standardPropertyName(AngularMomentumProperty), AngularMomentumProperty);
		table.insert(standardPropertyName(TorqueProperty), TorqueProperty);
		table.insert(standardPropertyName(SpinProperty), SpinProperty);
		table.insert(standardPropertyName(CentroSymmetryProperty), CentroSymmetryProperty);
		table.insert(standardPropertyName(VelocityMagnitudeProperty), VelocityMagnitudeProperty);
		table.insert(standardPropertyName(MoleculeProperty), MoleculeProperty);
		table.insert(standardPropertyName(AsphericalShapeProperty), AsphericalShapeProperty);
		table.insert(standardPropertyName(VectorColorProperty), VectorColorProperty);
	}
	return table;
}

/******************************************************************************
* Returns the number of vector components per atom used by the given standard data channel.
******************************************************************************/
size_t ParticleProperty::standardPropertyComponentCount(Type which)
{
	switch(which) {
	case ParticleTypeProperty:
	case StructureTypeProperty:
	case SelectionProperty:
	case ClusterProperty:
	case CoordinationProperty:
	case IdentifierProperty:
	case PotentialEnergyProperty:
	case KineticEnergyProperty:
	case TotalEnergyProperty:
	case RadiusProperty:
	case MassProperty:
	case ChargeProperty:
	case TransparencyProperty:
	case DipoleMagnitudeProperty:
	case SpinProperty:
	case CentroSymmetryProperty:
	case DisplacementMagnitudeProperty:
	case VelocityMagnitudeProperty:
    case MoleculeProperty:
		return 1;
	case PositionProperty:
	case ColorProperty:
	case DisplacementProperty:
	case VelocityProperty:
	case ForceProperty:
	case PeriodicImageProperty:
	case DipoleOrientationProperty:
	case AngularVelocityProperty:
	case AngularMomentumProperty:
	case TorqueProperty:
	case AsphericalShapeProperty:
	case VectorColorProperty:
		return 3;
	case StressTensorProperty:
	case StrainTensorProperty:
		return 6;
	case DeformationGradientProperty:
		return 9;
	case OrientationProperty:
		return 4;
	default:
		OVITO_ASSERT_MSG(false, "ParticleProperty::standardPropertyComponentCount", "Invalid standard particle property type");
		throw Exception(ParticlePropertyObject::tr("This is not a valid standard particle property type: %1").arg(which));
	}
}

/******************************************************************************
* Returns the list of component names for the given standard property.
******************************************************************************/
QStringList ParticleProperty::standardPropertyComponentNames(Type which, size_t componentCount)
{
	const static QStringList emptyList;
	const static QStringList xyzList = QStringList() << "X" << "Y" << "Z";
	const static QStringList rgbList = QStringList() << "R" << "G" << "B";
	const static QStringList symmetricTensorList = QStringList() << "XX" << "YY" << "ZZ" << "XY" << "XZ" << "YZ";
	const static QStringList matrix3List = QStringList() << "11" << "21" << "31" << "12" << "22" << "32" << "13" << "23" << "33";
	const static QStringList quaternionList = QStringList() << "X" << "Y" << "Z" << "W";

	switch(which) {
	case ParticleTypeProperty:
	case StructureTypeProperty:
	case SelectionProperty:
	case ClusterProperty:
	case CoordinationProperty:
	case IdentifierProperty:
	case PotentialEnergyProperty:
	case KineticEnergyProperty:
	case TotalEnergyProperty:
	case RadiusProperty:
	case MassProperty:
	case ChargeProperty:
	case TransparencyProperty:
	case DipoleMagnitudeProperty:
	case SpinProperty:
	case CentroSymmetryProperty:
	case DisplacementMagnitudeProperty:
	case VelocityMagnitudeProperty:
    case MoleculeProperty:
		return emptyList;
	case PositionProperty:
	case DisplacementProperty:
	case VelocityProperty:
	case ForceProperty:
	case PeriodicImageProperty:
	case DipoleOrientationProperty:
	case AngularVelocityProperty:
	case AngularMomentumProperty:
	case TorqueProperty:
	case AsphericalShapeProperty:
		return xyzList;
	case ColorProperty:
	case VectorColorProperty:
		return rgbList;
	case StressTensorProperty:
	case StrainTensorProperty:
		return symmetricTensorList;
	case DeformationGradientProperty:
		return matrix3List;
	case OrientationProperty:
		return quaternionList;
	default:
		OVITO_ASSERT_MSG(false, "ParticleProperty::standardPropertyComponentNames", "Invalid standard particle property type");
		throw Exception(ParticlePropertyObject::tr("This is not a valid standard particle property type: %1").arg(which));
	}
}

}	// End of namespace
}	// End of namespace
