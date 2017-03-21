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

#include <plugins/particles/Particles.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/particles/data/ParticleProperty.h>
#include <plugins/particles/objects/ParticlePropertyObject.h>
#include <plugins/particles/objects/ParticleTypeProperty.h>
#include <plugins/particles/objects/ParticleDisplay.h>
#include <plugins/particles/objects/VectorDisplay.h>
#include <plugins/particles/objects/SimulationCellDisplay.h>
#include <plugins/particles/objects/SurfaceMesh.h>
#include <plugins/particles/objects/SurfaceMeshDisplay.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/particles/objects/BondsDisplay.h>
#include <plugins/particles/objects/BondPropertyObject.h>
#include <plugins/particles/objects/BondTypeProperty.h>
#include <plugins/particles/objects/FieldQuantityObject.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/objects/TrajectoryObject.h>
#include <plugins/particles/objects/TrajectoryGeneratorObject.h>
#include <plugins/particles/objects/TrajectoryDisplay.h>
#include <plugins/particles/util/CutoffNeighborFinder.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <core/utilities/io/CompressedTextWriter.h>
#include <core/animation/AnimationSettings.h>
#include <core/plugins/PluginManager.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineModifiersSubmodule(py::module parentModule);	// Defined in ModifierBinding.cpp
void defineImportersSubmodule(py::module parentModule);	// Defined in ImporterBinding.cpp
void defineExportersSubmodule(py::module parentModule);	// Defined in ExporterBinding.cpp

template<class PropertyClass, bool ReadOnly>
py::dict PropertyObject__array_interface__(PropertyClass& p)
{
	py::dict ai;
	if(p.componentCount() == 1) {
		ai["shape"] = py::make_tuple(p.size());
		if(p.stride() != p.dataTypeSize())
			ai["strides"] = py::make_tuple(p.stride());
	}
	else if(p.componentCount() > 1) {
		ai["shape"] = py::make_tuple(p.size(), p.componentCount());
		ai["strides"] = py::make_tuple(p.stride(), p.dataTypeSize());
	}
	else throw Exception("Cannot access empty property from Python.");
	if(p.dataType() == qMetaTypeId<int>()) {
		OVITO_STATIC_ASSERT(sizeof(int) == 4);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<i4");
#else
		ai["typestr"] = py::bytes(">i4");
#endif
	}
	else if(p.dataType() == qMetaTypeId<FloatType>()) {
#ifdef FLOATTYPE_FLOAT		
		OVITO_STATIC_ASSERT(sizeof(FloatType) == 4);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<f4");
#else
		ai["typestr"] = py::bytes(">f4");
#endif
#else
		OVITO_STATIC_ASSERT(sizeof(FloatType) == 8);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
		ai["typestr"] = py::bytes("<f8");
#else
		ai["typestr"] = py::bytes(">f8");
#endif
#endif
	}
	else throw Exception("Cannot access property of this data type from Python.");
	if(ReadOnly) {
		ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(p.constData()), true);
	}
	else {
		ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(p.data()), false);
	}
	ai["version"] = py::cast(3);
	return ai;
}

py::dict BondsObject__array_interface__(const BondsObject& p)
{
	py::dict ai;
	ai["shape"] = py::make_tuple(p.storage()->size(), 2);
	OVITO_STATIC_ASSERT(sizeof(unsigned int) == 4);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	ai["typestr"] = py::bytes("<u4");
#else
	ai["typestr"] = py::bytes(">u4");
#endif
	const unsigned int* data;
	if(!p.storage()->empty()) {
		data = &p.storage()->front().index1;
	}
	else {
		static const unsigned int null_data = 0;
		data = &null_data;
	}
	ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(data), true);
	ai["strides"] = py::make_tuple(sizeof(Bond), sizeof(unsigned int));
	ai["version"] = py::cast(3);
	return ai;
}

py::dict BondsObject__pbc_vectors(const BondsObject& p)
{
	py::dict ai;
	ai["shape"] = py::make_tuple(p.storage()->size(), 3);
	OVITO_STATIC_ASSERT(sizeof(int8_t) == 1);
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
	ai["typestr"] = py::bytes("<i1");
#else
	ai["typestr"] = py::bytes(">i1");
#endif
	const int8_t* data;
	if(!p.storage()->empty()) {
		data = &p.storage()->front().pbcShift.x();
	}
	else {
		static const int8_t null_data = 0;
		data = &null_data;
	}
	ai["data"] = py::make_tuple(reinterpret_cast<std::intptr_t>(data), true);
	ai["strides"] = py::make_tuple(sizeof(Bond), sizeof(int8_t));
	ai["version"] = py::cast(3);
	return ai;
}

PYBIND11_PLUGIN(Particles)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("Particles");

	ovito_abstract_class<DataObjectWithSharedStorage<ParticleProperty>, DataObject>(m, nullptr, "DataObjectWithSharedParticlePropertyStorage");
	auto ParticlePropertyObject_py = ovito_abstract_class<ParticlePropertyObject, DataObjectWithSharedStorage<ParticleProperty>>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"A data object that stores the per-particle values of a particle property. "
			"\n\n"
			"The list of properties associated with a particle dataset can be access via the "
			":py:attr:`DataCollection.particle_properties` dictionary. The :py:attr:`.size` of a particle "
			"property is always equal to the number of particles in the dataset. The per-particle data "
			"of a property can be accessed as a NumPy array through the :py:attr:`.array` attribute. "
			"\n\n"
			"If you want to modify the property values, you have to use the :py:attr:`.marray` (*modifiable array*) "
			"attribute instead, which provides read/write access to the underlying per-particle data. "
			"After you are done modifying the property values, you should call :py:meth:`.changed` to inform "
			"the system that it needs to update any state that depends on the data. "
			,
			// Python class name:
			"ParticleProperty")
		.def_static("createUserProperty", &ParticlePropertyObject::createUserProperty)
		.def_static("createStandardProperty", &ParticlePropertyObject::createStandardProperty)
		.def_static("findInState", (ParticlePropertyObject* (*)(const PipelineFlowState&, ParticleProperty::Type))&ParticlePropertyObject::findInState)
		.def_static("findInState", (ParticlePropertyObject* (*)(const PipelineFlowState&, const QString&))&ParticlePropertyObject::findInState)
		.def("changed", &ParticlePropertyObject::changed,
				"Informs the particle property object that its internal data has changed. "
				"This function must be called after each direct modification of the per-particle data "
				"through the :py:attr:`.marray` attribute.\n\n"
				"Calling this method on an input particle property is necessary to invalidate data caches down the modification "
				"pipeline. Forgetting to call this method may result in an incomplete re-evaluation of the modification pipeline. "
				"See :py:attr:`.marray` for more information.")
		.def("nameWithComponent", &ParticlePropertyObject::nameWithComponent)
		.def_property("name", &ParticlePropertyObject::name, &ParticlePropertyObject::setName,
				"The human-readable name of this particle property.")
		.def_property_readonly("__len__", &ParticlePropertyObject::size)
		.def_property("size", &ParticlePropertyObject::size, &ParticlePropertyObject::resize,
				"The number of particles.")
		.def_property("type", &ParticlePropertyObject::type, &ParticlePropertyObject::setType,
				".. _particle-types-list:"
				"\n\n"
				"The type of the particle property (user-defined or one of the standard types).\n"
				"One of the following constants:"
				"\n\n"
				"======================================================= =================================================== ========== ==================================\n"
				"Type constant                                           Property name                                       Data type  Component names\n"
				"======================================================= =================================================== ========== ==================================\n"
				"``ParticleProperty.Type.User``                          (a user-defined property with a non-standard name)  int/float  \n"
				"``ParticleProperty.Type.ParticleType``                  :guilabel:`Particle Type`                           int        \n"
				"``ParticleProperty.Type.Position``                      :guilabel:`Position`                                float      X, Y, Z\n"
				"``ParticleProperty.Type.Selection``                     :guilabel:`Selection`                               int        \n"
				"``ParticleProperty.Type.Color``                         :guilabel:`Color`                                   float      R, G, B\n"
				"``ParticleProperty.Type.Displacement``                  :guilabel:`Displacement`                            float      X, Y, Z\n"
				"``ParticleProperty.Type.DisplacementMagnitude``         :guilabel:`Displacement Magnitude`                  float      \n"
				"``ParticleProperty.Type.PotentialEnergy``               :guilabel:`Potential Energy`                        float      \n"
				"``ParticleProperty.Type.KineticEnergy``                 :guilabel:`Kinetic Energy`                          float      \n"
				"``ParticleProperty.Type.TotalEnergy``                   :guilabel:`Total Energy`                            float      \n"
				"``ParticleProperty.Type.Velocity``                      :guilabel:`Velocity`                                float      X, Y, Z\n"
				"``ParticleProperty.Type.Radius``                        :guilabel:`Radius`                                  float      \n"
				"``ParticleProperty.Type.Cluster``                       :guilabel:`Cluster`                                 int        \n"
				"``ParticleProperty.Type.Coordination``                  :guilabel:`Coordination`                            int        \n"
				"``ParticleProperty.Type.StructureType``                 :guilabel:`Structure Type`                          int        \n"
				"``ParticleProperty.Type.Identifier``                    :guilabel:`Particle Identifier`                     int        \n"
				"``ParticleProperty.Type.StressTensor``                  :guilabel:`Stress Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
				"``ParticleProperty.Type.StrainTensor``                  :guilabel:`Strain Tensor`                           float      XX, YY, ZZ, XY, XZ, YZ\n"
				"``ParticleProperty.Type.DeformationGradient``           :guilabel:`Deformation Gradient`                    float      11, 21, 31, 12, 22, 32, 13, 23, 33\n"
				"``ParticleProperty.Type.Orientation``                   :guilabel:`Orientation`                             float      X, Y, Z, W\n"
				"``ParticleProperty.Type.Force``                         :guilabel:`Force`                                   float      X, Y, Z\n"
				"``ParticleProperty.Type.Mass``                          :guilabel:`Mass`                                    float      \n"
				"``ParticleProperty.Type.Charge``                        :guilabel:`Charge`                                  float      \n"
				"``ParticleProperty.Type.PeriodicImage``                 :guilabel:`Periodic Image`                          int        X, Y, Z\n"
				"``ParticleProperty.Type.Transparency``                  :guilabel:`Transparency`                            float      \n"
				"``ParticleProperty.Type.DipoleOrientation``             :guilabel:`Dipole Orientation`                      float      X, Y, Z\n"
				"``ParticleProperty.Type.DipoleMagnitude``               :guilabel:`Dipole Magnitude`                        float      \n"
				"``ParticleProperty.Type.AngularVelocity``               :guilabel:`Angular Velocity`                        float      X, Y, Z\n"
				"``ParticleProperty.Type.AngularMomentum``               :guilabel:`Angular Momentum`                        float      X, Y, Z\n"
				"``ParticleProperty.Type.Torque``                        :guilabel:`Torque`                                  float      X, Y, Z\n"
				"``ParticleProperty.Type.Spin``                          :guilabel:`Spin`                                    float      \n"
				"``ParticleProperty.Type.CentroSymmetry``                :guilabel:`Centrosymmetry`                          float      \n"
				"``ParticleProperty.Type.VelocityMagnitude``             :guilabel:`Velocity Magnitude`                      float      \n"
				"``ParticleProperty.Type.Molecule``                      :guilabel:`Molecule Identifier`                     int        \n"
				"``ParticleProperty.Type.AsphericalShape``               :guilabel:`Aspherical Shape`                        float      X, Y, Z\n"
				"``ParticleProperty.Type.VectorColor``                   :guilabel:`Vector Color`                            float      R, G, B\n"
				"``ParticleProperty.Type.ElasticStrainTensor``           :guilabel:`Elastic Strain`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
				"``ParticleProperty.Type.ElasticDeformationGradient``    :guilabel:`Elastic Deformation Gradient`            float      XX, YX, ZX, XY, YY, ZY, XZ, YZ, ZZ\n"
				"``ParticleProperty.Type.Rotation``                      :guilabel:`Rotation`                                float      X, Y, Z, W\n"
				"``ParticleProperty.Type.StretchTensor``                 :guilabel:`Stretch Tensor`                          float      XX, YY, ZZ, XY, XZ, YZ\n"
				"``ParticleProperty.Type.MoleculeType``                  :guilabel:`Molecule Type`                           int        \n"
				"======================================================= =================================================== ========== ==================================\n"
				)
		.def_property_readonly("data_type", &ParticlePropertyObject::dataType)
		.def_property_readonly("data_type_size", &ParticlePropertyObject::dataTypeSize)
		.def_property_readonly("stride", &ParticlePropertyObject::stride)
		.def_property_readonly("components", &ParticlePropertyObject::componentCount,
				"The number of vector components (if this is a vector particle property); otherwise 1 (= scalar property).")
		.def_property_readonly("__array_interface__", &PropertyObject__array_interface__<ParticlePropertyObject,true>)
		.def_property_readonly("__mutable_array_interface__", &PropertyObject__array_interface__<ParticlePropertyObject,false>)
	;

	py::enum_<ParticleProperty::Type>(ParticlePropertyObject_py, "Type")
		.value("User", ParticleProperty::UserProperty)
		.value("ParticleType", ParticleProperty::ParticleTypeProperty)
		.value("Position", ParticleProperty::PositionProperty)
		.value("Selection", ParticleProperty::SelectionProperty)
		.value("Color", ParticleProperty::ColorProperty)
		.value("Displacement", ParticleProperty::DisplacementProperty)
		.value("DisplacementMagnitude", ParticleProperty::DisplacementMagnitudeProperty)
		.value("PotentialEnergy", ParticleProperty::PotentialEnergyProperty)
		.value("KineticEnergy", ParticleProperty::KineticEnergyProperty)
		.value("TotalEnergy", ParticleProperty::TotalEnergyProperty)
		.value("Velocity", ParticleProperty::VelocityProperty)
		.value("Radius", ParticleProperty::RadiusProperty)
		.value("Cluster", ParticleProperty::ClusterProperty)
		.value("Coordination", ParticleProperty::CoordinationProperty)
		.value("StructureType", ParticleProperty::StructureTypeProperty)
		.value("Identifier", ParticleProperty::IdentifierProperty)
		.value("StressTensor", ParticleProperty::StressTensorProperty)
		.value("StrainTensor", ParticleProperty::StrainTensorProperty)
		.value("DeformationGradient", ParticleProperty::DeformationGradientProperty)
		.value("Orientation", ParticleProperty::OrientationProperty)
		.value("Force", ParticleProperty::ForceProperty)
		.value("Mass", ParticleProperty::MassProperty)
		.value("Charge", ParticleProperty::ChargeProperty)
		.value("PeriodicImage", ParticleProperty::PeriodicImageProperty)
		.value("Transparency", ParticleProperty::TransparencyProperty)
		.value("DipoleOrientation", ParticleProperty::DipoleOrientationProperty)
		.value("DipoleMagnitude", ParticleProperty::DipoleMagnitudeProperty)
		.value("AngularVelocity", ParticleProperty::AngularVelocityProperty)
		.value("AngularMomentum", ParticleProperty::AngularMomentumProperty)
		.value("Torque", ParticleProperty::TorqueProperty)
		.value("Spin", ParticleProperty::SpinProperty)
		.value("CentroSymmetry", ParticleProperty::CentroSymmetryProperty)
		.value("VelocityMagnitude", ParticleProperty::VelocityMagnitudeProperty)
		.value("Molecule", ParticleProperty::MoleculeProperty)
		.value("AsphericalShape", ParticleProperty::AsphericalShapeProperty)
		.value("VectorColor", ParticleProperty::VectorColorProperty)
		.value("ElasticStrainTensor", ParticleProperty::ElasticStrainTensorProperty)
		.value("ElasticDeformationGradient", ParticleProperty::ElasticDeformationGradientProperty)
		.value("Rotation", ParticleProperty::RotationProperty)
		.value("StretchTensor", ParticleProperty::StretchTensorProperty)
		.value("MoleculeType", ParticleProperty::MoleculeTypeProperty)
	;

	auto ParticleTypeProperty_py = ovito_abstract_class<ParticleTypeProperty, ParticlePropertyObject>(m,
			":Base class: :py:class:`ovito.data.ParticleProperty`\n\n"
			"This is a specialization of the :py:class:`ParticleProperty` class, which holds a list of :py:class:`ParticleType` instances in addition "
			"to the per-particle type values. "
			"\n\n"
			"OVITO encodes the types of particles (chemical and also others) as integer values starting at 1. "
			"Like for any other particle property, the numeric type of each particle can be accessed as a NumPy array through the :py:attr:`~ParticleProperty.array` attribute "
			"of the base class, or modified through the mutable :py:attr:`~ParticleProperty.marray` NumPy interface:: "
			"\n\n"
		    "    >>> type_property = node.source.particle_properties.particle_type\n"
			"    >>> print(type_property.array)\n"
			"    [1 3 2 ..., 2 1 2]\n"
			"\n\n"
			"In addition to these per-particle type values, the :py:class:`!ParticleTypeProperty` class keeps the :py:attr:`.type_list`, which "
			"contains all defined particle types including their names, IDs, display color and radius. "
			"Each defined type is represented by an :py:attr:`ParticleType` instance and has a unique integer ID, a human-readable name (e.g. the chemical symbol) "
			"and a display color and radius:: "
			"\n\n"
			"    >>> for t in type_property.type_list:\n"
			"    ...     print(t.id, t.name, t.color, t.radius)\n"
			"    ... \n"
			"    1 N (0.188 0.313 0.972) 0.74\n"
			"    2 C (0.564 0.564 0.564) 0.77\n"
			"    3 O (1 0.050 0.050) 0.74\n"
			"    4 S (0.97 0.97 0.97) 0.0\n"
			"\n\n"
			"Each particle type has a unique numeric ID (typically starting at 1). Note that, in this particular example, types were stored in order of ascending ID in the "
			":py:attr:`.type_list`. This may not always be the case. To quickly look up the :py:class:`ParticleType` and its name for a given ID, "
			"the :py:meth:`.get_type_by_id` method is available:: "
			"\n\n"
			"    >>> for t in type_property.array:\n"
			"    ...     print(type_property.get_type_by_id(t).name)\n"
			"    ... \n"
			"    N\n"
			"    O\n"
			"    C\n"
			"\n\n"
			"Conversely, the :py:attr:`ParticleType` and its numeric ID can be looked by name using the :py:meth:`.get_type_by_name` method. "
			"For example, to count the number of oxygen atoms in a system:"
			"\n\n"
			"    >>> O_type_id = type_property.get_type_by_name('O').id\n"
			"    >>> numpy.count_nonzero(type_property.array == O_type_id)\n"
			"    957\n"
			"\n\n"
			"Note that particles may be associated with multiple kinds of types in OVITO. This includes, for example, the chemical type and the structural type. "
			"Thus, several type classifications of particles can co-exist, each being represented by a separate instance of the :py:class:`!ParticleTypeProperty` class and a separate :py:attr:`.type_list`. "
			"For example, while the ``'Particle Type'`` property stores the chemical type of "
			"atoms (e.g. C, H, Fe, ...), the ``'Structure Type'`` property stores the structural type computed for each atom (e.g. FCC, BCC, ...). ")
		.def("_get_type_by_id", (ParticleType* (ParticleTypeProperty::*)(int) const)&ParticleTypeProperty::particleType)
		.def("_get_type_by_name", (ParticleType* (ParticleTypeProperty::*)(const QString&) const)&ParticleTypeProperty::particleType)
		//.def_static("getDefaultParticleColorFromId", &ParticleTypeProperty::getDefaultParticleColorFromId)
		//.def_static("getDefaultParticleColor", &ParticleTypeProperty::getDefaultParticleColor)
		//.def_static("setDefaultParticleColor", &ParticleTypeProperty::setDefaultParticleColor)
		//.def_static("getDefaultParticleRadius", &ParticleTypeProperty::getDefaultParticleRadius)
		//.def_static("setDefaultParticleRadius", &ParticleTypeProperty::setDefaultParticleRadius)
	;
	expose_mutable_subobject_list<ParticleTypeProperty, 
								  ParticleType, 
								  ParticleTypeProperty,
								  &ParticleTypeProperty::particleTypes, 
								  &ParticleTypeProperty::insertParticleType, 
								  &ParticleTypeProperty::removeParticleType>(
									  ParticleTypeProperty_py, "type_list", "ParticleTypeList",
										"A (mutable) list of :py:class:`ParticleType` instances. "
										"\n\n"
										"Note that the particle types may be stored in arbitrary order in this type list. "
        								"Each type has a unique integer ID (given by the :py:attr:`ParticleType.id` attribute). "
        								"The numbers stored in the particle type property :py:attr:`~ParticleProperty.array` refer to these type IDs.");

	ovito_class<SimulationCellObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"Stores the shape and the boundary conditions of the simulation cell."
			"\n\n"
			"Each instance of this class is associated with a corresponding :py:class:`~ovito.vis.SimulationCellDisplay` "
			"that controls the visual appearance of the simulation cell. It can be accessed through "
			"the :py:attr:`~DataObject.display` attribute of the :py:class:`!SimulationCell` object, which is defined by the :py:class:`~DataObject` base class."
			"\n\n"
			"The simulation cell of a particle dataset can be accessed via the :py:attr:`DataCollection.cell` property."
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell.py\n",
			// Python class name:
			"SimulationCell")
		.def_property("pbc_x", &SimulationCellObject::pbcX, &SimulationCellObject::setPbcX)
		.def_property("pbc_y", &SimulationCellObject::pbcY, &SimulationCellObject::setPbcY)
		.def_property("pbc_z", &SimulationCellObject::pbcZ, &SimulationCellObject::setPbcZ)
		.def_property("is2D", &SimulationCellObject::is2D, &SimulationCellObject::setIs2D,
				"Specifies whether the system is two-dimensional (true) or three-dimensional (false). "
				"For two-dimensional systems the PBC flag in the third direction (z) and the third cell vector are ignored. "
				"\n\n"
				":Default: ``false``\n")
		.def_property("matrix", MatrixGetterCopy<SimulationCellObject, AffineTransformation, &SimulationCellObject::cellMatrix>(),
								MatrixSetter<SimulationCellObject, AffineTransformation, &SimulationCellObject::setCellMatrix>(),
				"A 3x4 matrix containing the three edge vectors of the cell (matrix columns 0 to 2) "
        		"and the cell origin (matrix column 3).")
		.def_property("vector1", &SimulationCellObject::cellVector1, &SimulationCellObject::setCellVector1)
		.def_property("vector2", &SimulationCellObject::cellVector2, &SimulationCellObject::setCellVector2)
		.def_property("vector3", &SimulationCellObject::cellVector3, &SimulationCellObject::setCellVector3)
		.def_property("origin", &SimulationCellObject::cellOrigin, &SimulationCellObject::setCellOrigin)
		.def_property_readonly("volume", &SimulationCellObject::volume3D,
				"Returns the volume of the three-dimensional simulation cell.\n"
				"It is the absolute value of the determinant of the cell matrix.")
		.def_property_readonly("volume2D", &SimulationCellObject::volume2D,
				"Returns the volume of the two-dimensional simulation cell (see :py:attr:`.is2D`).\n")
	;

	ovito_abstract_class<DataObjectWithSharedStorage<BondsStorage>, DataObject>(m, nullptr, "DataObjectWithSharedBondsStorage");
	auto BondsObject_py = ovito_class<BondsObject, DataObjectWithSharedStorage<BondsStorage>>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"This data object stores a list of bonds between pairs of particles. "
			"Typically, bonds are loaded from a simulation file or created by inserting the :py:class:`~.ovito.modifiers.CreateBondsModifier` into the modification pipeline."
			"The following example demonstrates how to access the bonds list create by a :py:class:`~.ovito.modifiers.CreateBondsModifier`:\n"
			"\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 1-15\n"
			"\n"
			"OVITO represents each bond by two half-bonds, one pointing from a particle *A* to a particle *B*, and "
			"the other half-bond pointing back from *B* to *A*. Thus, you will typically find twice as many half-bonds in the :py:class:`!Bonds` object as there are bonds. \n"
			"The :py:attr:`.array` attribute of the :py:class:`!Bonds` class returns a (read-only) NumPy array that contains the list of half-bonds, each being "
			"defined as a pair of particle indices."
			"\n\n"
			"Note that half-bonds are not stored in any particular order in the :py:attr:`.array`. In particular, the half-bond (*a*, *b*) may not always be immediately succeeded by the corresponding "
			"reverse half-bond (*b*, *a*). Also, the half-bonds leaving a particle might not be not stored as a contiguous sequence. "
			"If you need to iterate over all half-bonds of a particle, you can use the :py:class:`.Enumerator` utility class described below. "
			"\n\n"
			"**Bond display settings**\n"
			"\n"
			"Every :py:class:`!Bonds` object is associated with a :py:class:`~ovito.vis.BondsDisplay` instance, "
			"which controls the visual appearance of the bonds in the viewports. It can be accessed through the :py:attr:`~DataObject.display` attribute:\n"
			"\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 17-20\n"
			"\n\n"
			"**Computing bond vectors**"
			"\n\n"
			"Note that the :py:class:`!Bonds` class only stores the indices of the particles connected by bonds (the *topology*). "
			"Sometimes it might be necessary to determine the corresponding spatial bond vectors. They can be computed "
			"from the current positions of the particles:\n"
			"\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 23-25\n"
			"\n\n"
			"Here, the first and the second column of the bonds array were each used to index the particle positions array. "
			"The subtraction of the two indexed arrays yields the list of bond vectors. Each vector in this list points "
			"from the first particle to the second particle of the corresponding half-bond."
			"\n\n"
			"Finally, we need to correct for the effect of periodic boundary conditions when bonds "
			"cross the box boundaries. This is achieved by multiplying the cell matrix and the stored PBC "
			"shift vector of each bond and adding the product to the bond vectors:\n"
			"\n"
			".. literalinclude:: ../example_snippets/bonds_data_object.py\n"
			"   :lines: 26-\n"
			"\n\n"
			"Note that it was necessary to transpose the PBC vectors array first to facilitate the transformation "
			"of the entire array of vectors with a single 3x3 cell matrix. In the above code snippets we have performed "
			"the following calculation for every half-bond (*a*, *b*) in parallel:"
			"\n\n"
			"   v = x(b) - x(a) + dot(H, pbc)\n"
			"\n\n"
			"where *H* is the cell matrix and *pbc* is the bond's PBC shift vector of the form (n\\ :sub:`x`, n\\ :sub:`y`, n\\ :sub:`z`). "
			"See the :py:attr:`.pbc_vectors` array for its meaning.\n"
			,
			// Python class name:
			"Bonds")
		.def_property_readonly("__array_interface__", &BondsObject__array_interface__)
		.def_property_readonly("_pbc_vectors", &BondsObject__pbc_vectors)
		.def("clear", &BondsObject::clear,
				"Removes all stored bonds.")
		// This is used by the Bonds.add() and Bonds.add_full() implementations:
		.def("addBond", &BondsObject::addBond)
		.def_property_readonly("size", &BondsObject::size)
	;

	py::class_<ParticleBondMap>(BondsObject_py, "ParticleBondMap")
		.def("__init__", [](ParticleBondMap& instance, BondsObject* bonds) {
            	new (&instance) ParticleBondMap(*bonds->storage());
        	}, py::arg("bonds"))
		.def("firstBondOfParticle", &ParticleBondMap::firstBondOfParticle)
		.def("nextBondOfParticle", &ParticleBondMap::nextBondOfParticle)
		.def_property_readonly("endOfListValue", &ParticleBondMap::endOfListValue)
	;

	ovito_class<ParticleType, RefTarget>(m,
			"Stores the properties of a particle type or atom type."
			"\n\n"
			"The list of particle types is stored in the :py:class:`~ovito.data.ParticleTypeProperty` class.")
		.def_property("id", &ParticleType::id, &ParticleType::setId,
				"The identifier of the particle type.")
		.def_property("color", &ParticleType::color, &ParticleType::setColor,
				"The display color to use for particles of this type.")
		.def_property("radius", &ParticleType::radius, &ParticleType::setRadius,
				"The display radius to use for particles of this type.")
		.def_property("name", &ParticleType::name, &ParticleType::setName,
				"The display name of this particle type.")
	;

	auto ParticleDisplay_py = ovito_class<ParticleDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"This object controls the visual appearance of particles."
			"\n\n"
			"An instance of this class is attached to the ``Position`` :py:class:`~ovito.data.ParticleProperty` "
			"and can be accessed via its :py:attr:`~ovito.data.DataObject.display` property. "
			"\n\n"
			"For example, the following script demonstrates how to change the display shape of particles to a square:"
			"\n\n"
			".. literalinclude:: ../example_snippets/particle_display.py\n")
		.def_property("radius", &ParticleDisplay::defaultParticleRadius, &ParticleDisplay::setDefaultParticleRadius,
				"The standard display radius of particles. "
				"This value is only used if no per-particle or per-type radii have been set. "
				"A per-type radius can be set via :py:attr:`ovito.data.ParticleType.radius`. "
				"An individual display radius can be assigned to particles by creating a ``Radius`` "
				":py:class:`~ovito.data.ParticleProperty`, e.g. using the :py:class:`~ovito.modifiers.ComputePropertyModifier`. "
				"\n\n"
				":Default: 1.2\n")
		.def_property_readonly("default_color", &ParticleDisplay::defaultParticleColor)
		.def_property_readonly("selection_color", &ParticleDisplay::selectionParticleColor)
		.def_property("rendering_quality", &ParticleDisplay::renderingQuality, &ParticleDisplay::setRenderingQuality)
		.def_property("shape", &ParticleDisplay::particleShape, &ParticleDisplay::setParticleShape,
				"The display shape of particles.\n"
				"Possible values are:"
				"\n\n"
				"   * ``ParticleDisplay.Shape.Sphere`` (default) \n"
				"   * ``ParticleDisplay.Shape.Box``\n"
				"   * ``ParticleDisplay.Shape.Circle``\n"
				"   * ``ParticleDisplay.Shape.Square``\n"
				"   * ``ParticleDisplay.Shape.Cylinder``\n"
				"   * ``ParticleDisplay.Shape.Spherocylinder``\n"
				"\n")
		;

	py::enum_<ParticleDisplay::ParticleShape>(ParticleDisplay_py, "Shape")
		.value("Sphere", ParticleDisplay::Sphere)
		.value("Box", ParticleDisplay::Box)
		.value("Circle", ParticleDisplay::Circle)
		.value("Square", ParticleDisplay::Square)
		.value("Cylinder", ParticleDisplay::Cylinder)
		.value("Spherocylinder", ParticleDisplay::Spherocylinder)
	;

	auto VectorDisplay_py = ovito_class<VectorDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of vectors (arrows)."
			"\n\n"
			"An instance of this class is attached to particle properties "
			"like for example the ``Displacement`` property, which represent vector quantities. "
			"It can be accessed via the :py:attr:`~ovito.data.DataObject.display` property of the :py:class:`~ovito.data.ParticleProperty` class. "
			"\n\n"
			"For example, the following script demonstrates how to change the display color of force vectors loaded from an input file:"
			"\n\n"
			".. literalinclude:: ../example_snippets/vector_display.py\n")
		.def_property("shading", &VectorDisplay::shadingMode, &VectorDisplay::setShadingMode,
				"The shading style used for the arrows.\n"
				"Possible values:"
				"\n\n"
				"   * ``VectorDisplay.Shading.Normal`` (default) \n"
				"   * ``VectorDisplay.Shading.Flat``\n"
				"\n")
		.def_property("rendering_quality", &VectorDisplay::renderingQuality, &VectorDisplay::setRenderingQuality)
		.def_property("reverse", &VectorDisplay::reverseArrowDirection, &VectorDisplay::setReverseArrowDirection,
				"Boolean flag controlling the reversal of arrow directions."
				"\n\n"
				":Default: ``False``\n")
		.def_property("alignment", &VectorDisplay::arrowPosition, &VectorDisplay::setArrowPosition,
				"Controls the positioning of arrows with respect to the particles.\n"
				"Possible values:"
				"\n\n"
				"   * ``VectorDisplay.Alignment.Base`` (default) \n"
				"   * ``VectorDisplay.Alignment.Center``\n"
				"   * ``VectorDisplay.Alignment.Head``\n"
				"\n")
		.def_property("color", &VectorDisplay::arrowColor, &VectorDisplay::setArrowColor,
				"The display color of arrows."
				"\n\n"
				":Default: ``(1.0, 1.0, 0.0)``\n")
		.def_property("width", &VectorDisplay::arrowWidth, &VectorDisplay::setArrowWidth,
				"Controls the width of arrows (in natural length units)."
				"\n\n"
				":Default: 0.5\n")
		.def_property("scaling", &VectorDisplay::scalingFactor, &VectorDisplay::setScalingFactor,
				"The uniform scaling factor applied to vectors."
				"\n\n"
				":Default: 1.0\n")
	;

	py::enum_<VectorDisplay::ArrowPosition>(VectorDisplay_py, "Alignment")
		.value("Base", VectorDisplay::Base)
		.value("Center", VectorDisplay::Center)
		.value("Head", VectorDisplay::Head)
	;

	ovito_class<SimulationCellDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of :py:class:`~ovito.data.SimulationCell` objects."
			"The following script demonstrates how to change the line width of the simulation cell:"
			"\n\n"
			".. literalinclude:: ../example_snippets/simulation_cell_display.py\n")
		.def_property("line_width", &SimulationCellDisplay::cellLineWidth, &SimulationCellDisplay::setCellLineWidth,
				"The width of the simulation cell line (in simulation units of length)."
				"\n\n"
				":Default: 0.14% of the simulation box diameter\n")
		.def_property("render_cell", &SimulationCellDisplay::renderCellEnabled, &SimulationCellDisplay::setRenderCellEnabled,
				"Boolean flag controlling the cell's visibility in rendered images. "
				"If ``False``, the cell will only be visible in the interactive viewports. "
				"\n\n"
				":Default: ``True``\n")
		.def_property("rendering_color", &SimulationCellDisplay::cellColor, &SimulationCellDisplay::setCellColor,
				"The line color used when rendering the cell."
				"\n\n"
				":Default: ``(0, 0, 0)``\n")
	;

	ovito_class<SurfaceMeshDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of a surface mesh computed by the :py:class:`~ovito.modifiers.ConstructSurfaceModifier`.")
		.def_property("surface_color", &SurfaceMeshDisplay::surfaceColor, &SurfaceMeshDisplay::setSurfaceColor,
				"The display color of the surface mesh."
				"\n\n"
				":Default: ``(1.0, 1.0, 1.0)``\n")
		.def_property("cap_color", &SurfaceMeshDisplay::capColor, &SurfaceMeshDisplay::setCapColor,
				"The display color of the cap polygons at periodic boundaries."
				"\n\n"
				":Default: ``(0.8, 0.8, 1.0)``\n")
		.def_property("show_cap", &SurfaceMeshDisplay::showCap, &SurfaceMeshDisplay::setShowCap,
				"Controls the visibility of cap polygons, which are created at the intersection of the surface mesh with periodic box boundaries."
				"\n\n"
				":Default: ``True``\n")
		.def_property("surface_transparency", &SurfaceMeshDisplay::surfaceTransparency, &SurfaceMeshDisplay::setSurfaceTransparency,
				"The level of transparency of the displayed surface. Valid range is 0.0 -- 1.0."
				"\n\n"
				":Default: 0.0\n")
		.def_property("cap_transparency", &SurfaceMeshDisplay::capTransparency, &SurfaceMeshDisplay::setCapTransparency,
				"The level of transparency of the displayed cap polygons. Valid range is 0.0 -- 1.0."
				"\n\n"
				":Default: 0.0\n")
		.def_property("smooth_shading", &SurfaceMeshDisplay::smoothShading, &SurfaceMeshDisplay::setSmoothShading,
				"Enables smooth shading of the triangulated surface mesh."
				"\n\n"
				":Default: ``True``\n")
		.def_property("reverse_orientation", &SurfaceMeshDisplay::reverseOrientation, &SurfaceMeshDisplay::setReverseOrientation,
				"Flips the orientation of the surface. This affects the generation of cap polygons."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<BondsDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of particle bonds. An instance of this class is attached to every :py:class:`~ovito.data.Bonds` data object.")
		.def_property("width", &BondsDisplay::bondWidth, &BondsDisplay::setBondWidth,
				"The display width of bonds (in natural length units)."
				"\n\n"
				":Default: 0.4\n")
		.def_property("color", &BondsDisplay::bondColor, &BondsDisplay::setBondColor,
				"The display color of bonds. Used only if :py:attr:`.use_particle_colors` == False."
				"\n\n"
				":Default: ``(0.6, 0.6, 0.6)``\n")
		.def_property("shading", &BondsDisplay::shadingMode, &BondsDisplay::setShadingMode,
				"The shading style used for bonds.\n"
				"Possible values:"
				"\n\n"
				"   * ``BondsDisplay.Shading.Normal`` (default) \n"
				"   * ``BondsDisplay.Shading.Flat``\n"
				"\n")
		.def_property("rendering_quality", &BondsDisplay::renderingQuality, &BondsDisplay::setRenderingQuality)
		.def_property("use_particle_colors", &BondsDisplay::useParticleColors, &BondsDisplay::setUseParticleColors,
				"If ``True``, bonds are assigned the same color as the particles they are adjacent to."
				"\n\n"
				":Default: ``True``\n")
	;

	ovito_abstract_class<DataObjectWithSharedStorage<HalfEdgeMesh<>>, DataObject>(m, nullptr, "DataObjectWithSharedHalfEdgeMeshStorage");
	ovito_class<SurfaceMesh, DataObjectWithSharedStorage<HalfEdgeMesh<>>>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"This data object stores the surface mesh computed by a :py:class:`~ovito.modifiers.ConstructSurfaceModifier`. "
			"\n\n"
			"Currently, no direct script access to the vertices and faces of the mesh is possible. But you can export the mesh to a VTK text file, "
			"which can be further processed by external tools such as ParaView. "
			"\n\n"
			"The visual appearance of the surface mesh within Ovito is controlled by its attached :py:class:`~ovito.vis.SurfaceMeshDisplay` instance, which is "
			"accessible through the :py:attr:`~DataObject.display` attribute of the :py:class:`DataObject` base class or through the :py:attr:`~ovito.modifiers.ConstructSurfaceModifier.mesh_display` attribute "
			"of the :py:class:`~ovito.modifiers.ConstructSurfaceModifier` that created the surface mesh."
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/surface_mesh.py\n"
			"   :lines: 4-\n"
		)
		.def_property("is_completely_solid", &SurfaceMesh::isCompletelySolid, &SurfaceMesh::setIsCompletelySolid)
		.def("export_vtk", [](SurfaceMesh& mesh, const QString& filename, SimulationCellObject* simCellObj) {
				if(!simCellObj)
					throw Exception("A simulation cell is required to generate non-periodic mesh for export.");
				TriMesh output;
				if(!SurfaceMeshDisplay::buildSurfaceMesh(*mesh.storage(), simCellObj->data(), false, mesh.cuttingPlanes(), output))
					throw Exception("Failed to generate non-periodic mesh for export. Simulation cell might be too small.");
				QFile file(filename);
				CompressedTextWriter writer(file, mesh.dataset());
				output.saveToVTK(writer);
			},
			"export_vtk(filename, cell)"
			"\n\n"
			"Writes the surface mesh to a VTK file, which is a simple text-based format and which can be opened with the software ParaView. "
			"The method takes the output filename and a :py:class:`~ovito.data.SimulationCell` object as input. The simulation cell information "
			"is needed by the method to generate a non-periodic version of the mesh, which is truncated at the periodic boundaries "
			"of the simulation cell (if it has any).")
		.def("export_cap_vtk", [](SurfaceMesh& mesh, const QString& filename, SimulationCellObject* simCellObj) {
				if(!simCellObj)
					throw Exception("A simulation cell is required to generate cap mesh for export.");
				TriMesh output;
				SurfaceMeshDisplay::buildCapMesh(*mesh.storage(), simCellObj->data(), mesh.isCompletelySolid(), false, mesh.cuttingPlanes(), output);
				QFile file(filename);
				CompressedTextWriter writer(file, mesh.dataset());
				output.saveToVTK(writer);
			},
			"export_cap_vtk(filename, cell)"
			"\n\n"
			"If the surface mesh has been generated from a :py:class:`~ovito.data.SimulationCell` with periodic boundary conditions, then this "
			"method computes the cap polygons from the intersection of the surface mesh with the periodic cell boundaries. "
			"The cap polygons are written to a VTK file, which is a simple text-based format and which can be opened with the software ParaView.")
	;

	auto CutoffNeighborFinder_py = py::class_<CutoffNeighborFinder>(m, "CutoffNeighborFinder")
		.def(py::init<>())
		.def("prepare", [](CutoffNeighborFinder& finder, FloatType cutoff, ParticlePropertyObject& positions, SimulationCellObject& cell) {
				SynchronousTask task(ScriptEngine::activeTaskManager());
				return finder.prepare(cutoff, positions.storage(), cell.data(), nullptr, task.promise());
			})
	;

	py::class_<CutoffNeighborFinder::Query>(CutoffNeighborFinder_py, "Query")
		.def(py::init<const CutoffNeighborFinder&, size_t>())
		.def("next", &CutoffNeighborFinder::Query::next)
		.def_property_readonly("at_end", &CutoffNeighborFinder::Query::atEnd)
		.def_property_readonly("index", &CutoffNeighborFinder::Query::current)
		.def_property_readonly("distance_squared", &CutoffNeighborFinder::Query::distanceSquared)
		.def_property_readonly("distance", [](const CutoffNeighborFinder::Query& query) -> FloatType { return sqrt(query.distanceSquared()); })
		.def_property_readonly("delta", &CutoffNeighborFinder::Query::delta)
		.def_property_readonly("pbc_shift", &CutoffNeighborFinder::Query::pbcShift)
	;

	auto NearestNeighborFinder_py = py::class_<NearestNeighborFinder>(m, "NearestNeighborFinder")
		.def(py::init<size_t>())
		.def("prepare", [](NearestNeighborFinder& finder, ParticlePropertyObject& positions, SimulationCellObject& cell) {
			SynchronousTask task(ScriptEngine::activeTaskManager());
			return finder.prepare(positions.storage(), cell.data(), nullptr, task.promise());
		})
	;

	typedef NearestNeighborFinder::Query<30> NearestNeighborQuery;

	py::class_<NearestNeighborFinder::Neighbor>(NearestNeighborFinder_py, "Neighbor")
		.def_readonly("index", &NearestNeighborFinder::Neighbor::index)
		.def_readonly("distance_squared", &NearestNeighborFinder::Neighbor::distanceSq)
		.def_property_readonly("distance", [](const NearestNeighborFinder::Neighbor& n) -> FloatType { return sqrt(n.distanceSq); })
		.def_readonly("delta", &NearestNeighborFinder::Neighbor::delta)
	;

	py::class_<NearestNeighborQuery>(NearestNeighborFinder_py, "Query")
		.def(py::init<const NearestNeighborFinder&>())
		.def("findNeighbors", static_cast<void (NearestNeighborQuery::*)(size_t)>(&NearestNeighborQuery::findNeighbors))
		.def("findNeighborsAtLocation", static_cast<void (NearestNeighborQuery::*)(const Point3&, bool)>(&NearestNeighborQuery::findNeighbors))
		.def_property_readonly("count", [](const NearestNeighborQuery& q) -> int { return q.results().size(); })
		.def("__getitem__", [](const NearestNeighborQuery& q, int index) -> const NearestNeighborFinder::Neighbor& { return q.results()[index]; },
			py::return_value_policy::reference_internal)
	;

	ovito_abstract_class<DataObjectWithSharedStorage<BondProperty>, DataObject>(m, nullptr, "DataObjectWithSharedBondPropertyStorage");
	auto BondPropertyObject_py = ovito_abstract_class<BondPropertyObject, DataObjectWithSharedStorage<BondProperty>>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"This data object stores the values of a certain bond property. A bond property is a quantity associated with every bond in a system. "
			"Bond properties work similar to particle properties (see :py:class:`ParticleProperty` class)."
			"\n\n"
			"All bond properties associated with the bonds in a system are stored in the :py:attr:`DataCollection.bond_properties` dictionary of the :py:class:`DataCollection` container. "
			"Bond properties are either read from the external simulation file or can be newly generated by OVITO's modifiers, the "
			":py:class:`~ovito.modifiers.ComputeBondLengthsModifier` being one example. "
			"\n\n"
			"The topological definition of bonds, i.e. the connectivity of particles, is stored separately from the bond properties in the :py:class:`Bonds` data object. "
			"The :py:class:`Bonds` can be accessed through the :py:attr:`DataCollection.bonds` field. "
			"\n\n"
			"Note that OVITO internally works with half-bonds, i.e., every full bond is represented as two half-bonds, one pointing "
			"from particle A to particle B and the other from B to A. Each half-bond is associated with its own property value, "
			"and the :py:attr:`.size` of a bond property array is always twice as large as the number of full bonds "
			"(see :py:attr:`DataCollection.number_of_half_bonds` and :py:attr:`DataCollection.number_of_full_bonds`). "
			"Typically, however, the property values of a half-bond and its reverse bond are identical. "
			"\n\n"
			"Similar to particle properties, it is possible to associate user-defined properties with bonds. OVITO also knows a set of standard "
			"bond properties (see the :py:attr:`.type` attribute below), which control the visual appearance of bonds. For example, "
			"it is possible to assign the ``Color`` property to bonds, giving one control over the rendering color of each individual (half-)bond. "
			"The color values stored in this property array will be used by OVITO to render the bonds. If not present, OVITO will fall back to the "
			"default behavior, which is determined by the :py:class:`ovito.vis.BondsDisplay` associated with the :py:class:`Bonds` object. ",
			// Python class name:
			"BondProperty")
		.def_static("createUserProperty", &BondPropertyObject::createUserProperty)
		.def_static("createStandardProperty", &BondPropertyObject::createStandardProperty)
		.def_static("findInState", (BondPropertyObject* (*)(const PipelineFlowState&, BondProperty::Type))&BondPropertyObject::findInState)
		.def_static("findInState", (BondPropertyObject* (*)(const PipelineFlowState&, const QString&))&BondPropertyObject::findInState)
		.def("changed", &BondPropertyObject::changed,
				"Informs the bond property object that its stored data has changed. "
				"This function must be called after each direct modification of the per-bond data "
				"through the :py:attr:`.marray` attribute.\n\n"
				"Calling this method on an input bond property is necessary to invalidate data caches down the modification "
				"pipeline. Forgetting to call this method may result in an incomplete re-evaluation of the modification pipeline. "
				"See :py:attr:`.marray` for more information.")
		.def("nameWithComponent", &BondPropertyObject::nameWithComponent)
		.def_property("name", &BondPropertyObject::name, &BondPropertyObject::setName,
				"The human-readable name of the bond property.")
		.def_property_readonly("__len__", &BondPropertyObject::size)
		.def_property("size", &BondPropertyObject::size, &BondPropertyObject::resize,
				"The number of stored property values, which is always equal to the number of half-bonds.")
		.def_property("type", &BondPropertyObject::type, &BondPropertyObject::setType,
				".. _bond-types-list:"
				"\n\n"
				"The type of the bond property (user-defined or one of the standard types).\n"
				"One of the following constants:"
				"\n\n"
				"======================================================= =================================================== ==========\n"
				"Type constant                                           Property name                                       Data type \n"
				"======================================================= =================================================== ==========\n"
				"``BondProperty.Type.User``                              (a user-defined property with a non-standard name)  int/float \n"
				"``BondProperty.Type.BondType``                          :guilabel:`Bond Type`                               int       \n"
				"``BondProperty.Type.Selection``                         :guilabel:`Selection`                               int       \n"
				"``BondProperty.Type.Color``                             :guilabel:`Color`                                   float     \n"
				"``BondProperty.Type.Length``                            :guilabel:`Length`                                  float     \n"
				"======================================================= =================================================== ==========\n"
				)
		.def_property_readonly("dataType", &BondPropertyObject::dataType)
		.def_property_readonly("dataTypeSize", &BondPropertyObject::dataTypeSize)
		.def_property_readonly("stride", &BondPropertyObject::stride)
		.def_property_readonly("components", &BondPropertyObject::componentCount,
				"The number of vector components (if this is a vector bond property); otherwise 1 (= scalar property).")
		.def_property_readonly("__array_interface__", &PropertyObject__array_interface__<BondPropertyObject,true>)
		.def_property_readonly("__mutable_array_interface__", &PropertyObject__array_interface__<BondPropertyObject,false>)
	;

	py::enum_<BondProperty::Type>(BondPropertyObject_py, "Type")
		.value("User", BondProperty::UserProperty)
		.value("BondType", BondProperty::BondTypeProperty)
		.value("Selection", BondProperty::SelectionProperty)
		.value("Color", BondProperty::ColorProperty)
		.value("Length", BondProperty::LengthProperty)
	;

	auto BondTypeProperty_py = ovito_abstract_class<BondTypeProperty, BondPropertyObject>(m,
			":Base class: :py:class:`ovito.data.BondProperty`\n\n"
			"A special :py:class:`BondProperty` that stores a list of :py:class:`BondType` instances in addition "
			"to the per-bond values. "
			"\n\n"
			"The bond property ``Bond Type`` is represented by an instance of this class. In addition to the regular per-bond "
			"data (consisting of an integer per half-bond, indicating its type ID), this class holds the list of defined bond types. These are "
			":py:class:`BondType` instances, which store the ID, name, and color of each bond type.")
		.def("_get_type_by_id", (BondType* (BondTypeProperty::*)(int) const)&BondTypeProperty::bondType)
		.def("_get_type_by_name", (BondType* (BondTypeProperty::*)(const QString&) const)&BondTypeProperty::bondType)
	;
	expose_mutable_subobject_list<BondTypeProperty, 
								  BondType, 
								  BondTypeProperty,
								  &BondTypeProperty::bondTypes, 
								  &BondTypeProperty::insertBondType, 
								  &BondTypeProperty::removeBondType>(
									  BondTypeProperty_py, "type_list", "BondTypeList",
									  	"A (mutable) list of :py:class:`BondType` instances. "
										"\n\n"
										"Note that the bond types may be stored in arbitrary order in this type list. "
        								"Each type has a unique integer ID (given by the :py:attr:`BondType.id` attribute). "
        								"The numbers stored in the bond type property :py:attr:`~BondProperty.array` refer to these type IDs.");

	ovito_class<BondType, RefTarget>(m,
			"Stores the properties of a bond type."
			"\n\n"
			"The list of bond types is stored in the :py:class:`~ovito.data.BondTypeProperty` class.")
		.def_property("id", &BondType::id, &BondType::setId,
				"The identifier of the bond type.")
		.def_property("color", &BondType::color, &BondType::setColor,
				"The display color to use for bonds of this type.")
		.def_property("name", &BondType::name, &BondType::setName,
				"The display name of this bond type.")
	;

	ovito_abstract_class<DataObjectWithSharedStorage<FieldQuantity>, DataObject>(m, nullptr, "DataObjectWithSharedFieldQuantityStorage");
	auto FieldQuantityObject_py = ovito_abstract_class<FieldQuantityObject, DataObjectWithSharedStorage<FieldQuantity>>(m)
		.def("changed", &FieldQuantityObject::changed,
				"Informs the object that its stored data has changed. "
				"This function must be called after each direct modification of the field data "
				"through the :py:attr:`.marray` attribute.\n\n"
				"Calling this method on an input field quantity is necessary to invalidate data caches down the data "
				"pipeline. Forgetting to call this method may result in an incomplete re-evaluation of the data pipeline. "
				"See :py:attr:`.marray` for more information.")
		.def_property("name", &FieldQuantityObject::name, &FieldQuantityObject::setName,
				"The human-readable name of the field quantitz.")
		.def_property_readonly("components", &FieldQuantityObject::componentCount,
				"The number of vector components (if this is a vector quantity); otherwise 1 (= scalar quantity).")
	;
	
	ovito_class<TrajectoryObject, DataObject>{m};

	ovito_class<TrajectoryGeneratorObject, TrajectoryObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"Data object that generates and stores the trajectory lines from a set of moving particles. "
			"\n\n"
			"The visual appearance of the trajectory lines is controlled by the attached :py:class:`~ovito.vis.TrajectoryLineDisplay` instance, which is "
			"accessible through the :py:attr:`~DataObject.display` attribute."
			"\n\n"
			"**Usage example:**"
			"\n\n"
			".. literalinclude:: ../example_snippets/trajectory_lines.py",
			"TrajectoryLineGenerator")
		.def_property("source_node", &TrajectoryGeneratorObject::source, &TrajectoryGeneratorObject::setSource,
				"The :py:class:`~ovito.ObjectNode` that serves as source for particle trajectory data. ") 
		.def_property("only_selected", &TrajectoryGeneratorObject::onlySelectedParticles, &TrajectoryGeneratorObject::setOnlySelectedParticles,
				"Controls whether trajectory lines should only by generated for currently selected particles."
				"\n\n"
				":Default: ``True``\n")
		.def_property("unwrap_trajectories", &TrajectoryGeneratorObject::unwrapTrajectories, &TrajectoryGeneratorObject::setUnwrapTrajectories,
				"Controls whether trajectory lines should be automatically unwrapped at the box boundaries when the particles cross a periodic boundary."
				"\n\n"
				":Default: ``True``\n")
		.def_property("sampling_frequency", &TrajectoryGeneratorObject::everyNthFrame, &TrajectoryGeneratorObject::setEveryNthFrame,
				"Length of animation frame interval at which the particle positions should be sampled when generating the trajectory lines."
				"\n\n"
				":Default: 1\n")
		.def_property("frame_interval", [](TrajectoryGeneratorObject& tgo) -> py::object {
					if(tgo.useCustomInterval()) return py::make_tuple(
						tgo.dataset()->animationSettings()->timeToFrame(tgo.customIntervalStart()),
						tgo.dataset()->animationSettings()->timeToFrame(tgo.customIntervalEnd()));
					else
						return py::none();
				},
				[](TrajectoryGeneratorObject& tgo, py::object arg) {
					if(py::isinstance<py::none>(arg)) {
						tgo.setUseCustomInterval(false);
						return;
					}
					else if(py::isinstance<py::tuple>(arg)) {
						py::tuple tup = py::reinterpret_borrow<py::tuple>(arg);
						if(tup.size() == 2) {
							int a  = tup[0].cast<int>();
							int b  = tup[1].cast<int>();
							tgo.setCustomIntervalStart(tgo.dataset()->animationSettings()->frameToTime(a));
							tgo.setCustomIntervalEnd(tgo.dataset()->animationSettings()->frameToTime(b));
							tgo.setUseCustomInterval(true);
							return;
						}
					}
					throw py::value_error("Tuple of two integers or None expected.");
				},
				"The animation frame interval over which the particle positions are sampled to generate the trajectory lines. "
				"Set this to a tuple of two integers to specify the first and the last animation frame; or use ``None`` to generate trajectory lines "
				"over the entire input sequence."
				"\n\n"
				":Default: ``None``\n")
		.def("generate", [](TrajectoryGeneratorObject& obj) {
				return obj.generateTrajectories(ScriptEngine::activeTaskManager());
			},
			"Generates the trajectory lines by sampling the positions of the particles in the :py:attr:`.source_node` at regular time intervals. "
			"The trajectory line data is cached by the :py:class:`!TrajectoryLineGenerator`.")
	;	

	ovito_class<TrajectoryDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of particle trajectory lines. An instance of this class is attached to every :py:class:`~ovito.data.TrajectoryLineGenerator` data object.",
			"TrajectoryLineDisplay")
		.def_property("width", &TrajectoryDisplay::lineWidth, &TrajectoryDisplay::setLineWidth,
				"The display width of trajectory lines."
				"\n\n"
				":Default: 0.2\n")
		.def_property("color", &TrajectoryDisplay::lineColor, &TrajectoryDisplay::setLineColor,
				"The display color of trajectory lines."
				"\n\n"
				":Default: ``(0.6, 0.6, 0.6)``\n")
		.def_property("shading", &TrajectoryDisplay::shadingMode, &TrajectoryDisplay::setShadingMode,
				"The shading style used for trajectory lines.\n"
				"Possible values:"
				"\n\n"
				"   * ``TrajectoryLineDisplay.Shading.Normal`` \n"
				"   * ``TrajectoryLineDisplay.Shading.Flat`` (default)\n"
				"\n")
		.def_property("upto_current_time", &TrajectoryDisplay::showUpToCurrentTime, &TrajectoryDisplay::setShowUpToCurrentTime,
				"If ``True``, trajectory lines are only rendered up to the particle positions at the current animation time. "
				"Otherwise, the complete trajectory lines are displayed."
				"\n\n"
				":Default: ``False``\n")
	;

	// Register submodules.
	defineModifiersSubmodule(m);	// Defined in ModifierBinding.cpp
	defineImportersSubmodule(m);	// Defined in ImporterBinding.cpp
	defineExportersSubmodule(m);	// Defined in ExporterBinding.cpp

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(Particles);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
