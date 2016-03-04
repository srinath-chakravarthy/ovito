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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/modifier/ConstructSurfaceModifier.h>
#include <plugins/crystalanalysis/modifier/SmoothDislocationsModifier.h>
#include <plugins/crystalanalysis/modifier/SmoothSurfaceModifier.h>
#include <plugins/crystalanalysis/modifier/dxa/DislocationAnalysisModifier.h>
#include <plugins/crystalanalysis/modifier/dxa/StructureAnalysis.h>
#include <plugins/crystalanalysis/modifier/elasticstrain/ElasticStrainModifier.h>
#include <plugins/crystalanalysis/modifier/grains/GrainSegmentationModifier.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/BurgersVectorFamily.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include <plugins/crystalanalysis/importer/CAImporter.h>
#include <plugins/crystalanalysis/exporter/CAExporter.h>
#include <plugins/pyscript/binding/PythonBinding.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

using namespace boost::python;
using namespace PyScript;

BOOST_PYTHON_MODULE(CrystalAnalysis)
{
	docstring_options docoptions(true, false);

	ovito_class<ConstructSurfaceModifier, AsynchronousParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Constructs the geometric surface of a solid made of point-like particles. The modifier generates "
			"a :py:class:`~ovito.data.SurfaceMesh`, which is a closed manifold consisting of triangles. It also computes the total "
			"surface area and the volume of the region enclosed by the surface mesh."
			"\n\n"
			"The :py:attr:`.radius` parameter controls how many details of the solid shape are resolved during surface construction. "
			"A larger radius leads to a surface with fewer details, reflecting only coarse features of the surface topology. "
			"A small radius, on the other hand, will resolve finer surface features and small pores inside a solid, for example. "
			"\n\n"
			"See `this article <http://dx.doi.org/10.1007/s11837-013-0827-5>`_ for a description of the surface construction algorithm."
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/construct_surface_modifier.py"
			)
		.add_property("radius", &ConstructSurfaceModifier::probeSphereRadius, &ConstructSurfaceModifier::setProbeSphereRadius,
				"The radius of the probe sphere used in the surface construction algorithm."
				"\n\n"
				"A rule of thumb is that the radius parameter should be slightly larger than the typical distance between "
				"nearest neighbor particles."
				"\n\n"
				":Default: 4.0\n")
		.add_property("smoothing_level", &ConstructSurfaceModifier::smoothingLevel, &ConstructSurfaceModifier::setSmoothingLevel,
				"The number of iterations of the smoothing algorithm applied to the computed surface mesh."
				"\n\n"
				":Default: 8\n")
		.add_property("only_selected", &ConstructSurfaceModifier::onlySelectedParticles, &ConstructSurfaceModifier::setOnlySelectedParticles,
				"If ``True``, the modifier acts only on selected particles and ignores other particles; "
				"if ``False``, the modifier constructs the surface around all particles."
				"\n\n"
				":Default: ``False``\n")
		.add_property("solid_volume", &ConstructSurfaceModifier::solidVolume,
				"After the modifier has computed the surface, this output field contains the volume of the solid region enclosed "
				"by the surface."
				"\n\n"
				"Note that this value is only available after the modifier has computed its results. "
				"Thus, you have to call :py:meth:`ovito.ObjectNode.compute` first to ensure that this information is up to date. ")
		.add_property("total_volume", &ConstructSurfaceModifier::totalVolume,
				"This output field reports the volume of the input simulation cell, which can be used "
				"to calculate the solid volume fraction or porosity of a system (in conjunction with the "
				":py:attr:`.solid_volume` computed by the modifier). ")
		.add_property("surface_area", &ConstructSurfaceModifier::surfaceArea,
				"After the modifier has computed the surface, this output field contains the area of the surface."
				"\n\n"
				"Note that this value is only available after the modifier has computed its results. "
				"Thus, you have to call :py:meth:`ovito.ObjectNode.compute` first to ensure that this information is up to date. ")
		.add_property("mesh_display", make_function(&ConstructSurfaceModifier::surfaceMeshDisplay, return_value_policy<ovito_object_reference>()),
				"The :py:class:`~ovito.vis.SurfaceMeshDisplay` controlling the visual representation of the computed surface.\n")
	;

	{
		scope s = ovito_class<DislocationAnalysisModifier, StructureIdentificationModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"This analysis modifier extracts all dislocations in a crystal and converts them to continuous line segments. "
				"The method behind this is called *Dislocation Extraction Algorithm* (DXA). "
				"\n\n"
				"The extracted dislocation lines are returned as a :py:class:`~ovito.data.DislocationNetwork` object, which "
				"will be present in the output :py:class:`~ovito.data.DataCollection` of the node after the modification pipeline has been evaluated. "
				"\n\n"
				"Usage example:\n\n"
				".. literalinclude:: ../example_snippets/dislocation_analysis_modifier.py"
				)
			.add_property("trial_circuit_length", &DislocationAnalysisModifier::maxTrialCircuitSize, &DislocationAnalysisModifier::setMaxTrialCircuitSize,
					"The maximum length of trial Burgers circuits constructed by the DXA to discover dislocations. "
					"The length is specified in terms of the number of atom-to-atom steps."
					"\n\n"
					":Default: 14\n")
			.add_property("circuit_stretchability", &DislocationAnalysisModifier::circuitStretchability, &DislocationAnalysisModifier::setCircuitStretchability,
					"The number of steps by which a Burgers circuit can stretch while it is being advanced along a dislocation line."
					"\n\n"
					":Default: 9\n")
			.add_property("input_crystal_structure", &DislocationAnalysisModifier::inputCrystalStructure, &DislocationAnalysisModifier::setInputCrystalStructure,
					"The type of crystal to analyze. Must be one of: "
					"\n\n"
					"  * ``DislocationAnalysisModifier.Lattice.FCC``\n"
					"  * ``DislocationAnalysisModifier.Lattice.HCP``\n"
					"  * ``DislocationAnalysisModifier.Lattice.BCC``\n"
					"  * ``DislocationAnalysisModifier.Lattice.CubicDiamond``\n"
					"  * ``DislocationAnalysisModifier.Lattice.HexagonalDiamond``\n"
					"\n\n"
					":Default: ``DislocationAnalysisModifier.Lattice.FCC``\n")
#if 0
			.add_property("reconstruct_edge_vectors", &DislocationAnalysisModifier::reconstructEdgeVectors, &DislocationAnalysisModifier::setReconstructEdgeVectors,
					"Flag that enables the reconstruction of ideal lattice vectors in highly distorted crystal regions. This algorithm step is supposed to improve "
					"the identification of dislocations in some situations, but it may have undesirable side effect. Use with care, only for experts!"
					"\n\n"
					":Default: False\n")
#endif
			.add_property("line_smoothing_enabled", &DislocationAnalysisModifier::lineSmoothingEnabled, &DislocationAnalysisModifier::setLineSmoothingEnabled,
					"Flag that enables the smoothing of extracted dislocation lines after they have been coarsened."
					"\n\n"
					":Default: True\n")
			.add_property("line_coarsening_enabled", &DislocationAnalysisModifier::lineCoarseningEnabled, &DislocationAnalysisModifier::setLineCoarseningEnabled,
					"Flag that enables the coarsening of extracted dislocation lines, which reduces the number of sample points along the lines."
					"\n\n"
					":Default: True\n")
			.add_property("line_smoothing_level", &DislocationAnalysisModifier::lineSmoothingLevel, &DislocationAnalysisModifier::setLineSmoothingLevel,
					"The number of iterations of the line smoothing algorithm to perform."
					"\n\n"
					":Default: 1\n")
			.add_property("line_point_separation", &DislocationAnalysisModifier::linePointInterval, &DislocationAnalysisModifier::setLinePointInterval,
					"Sets the desired distance between successive sample points along the dislocation lines, measured in multiples of the interatomic spacing. "
					"This parameter controls the amount of coarsening performed during post-processing of dislocation lines."
					"\n\n"
					":Default: 2.5\n")
			.add_property("defect_mesh_smoothing_level", &DislocationAnalysisModifier::defectMeshSmoothingLevel, &DislocationAnalysisModifier::setDefectMeshSmoothingLevel,
					"Specifies the number of iterations of the surface smoothing algorithm to perform when post-processing the extracted defect mesh."
					"\n\n"
					":Default: 8\n")
		;

		enum_<StructureAnalysis::LatticeStructureType>("Lattice")
			.value("Other", StructureAnalysis::LATTICE_OTHER)
			.value("FCC", StructureAnalysis::LATTICE_FCC)
			.value("HCP", StructureAnalysis::LATTICE_HCP)
			.value("BCC", StructureAnalysis::LATTICE_BCC)
			.value("CubicDiamond", StructureAnalysis::LATTICE_CUBIC_DIAMOND)
			.value("HexagonalDiamond", StructureAnalysis::LATTICE_HEX_DIAMOND)
		;
	}

	ovito_class<ElasticStrainModifier, StructureIdentificationModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier computes the atomic-level elastic strain and deformation gradient tensors in crystalline systems. "
			"\n\n"
			"The modifier first performs an identification of the local crystal structure and stores the results in the ``Structure Type`` particle "
			"property. Possible structure type values are listed under the :py:attr:`.input_crystal_structure` property. "
			"Atoms that do not form a crystalline structure or which are part of defects are assigned the special type ``OTHER`` (=0). "
			"For these atoms the local elastic deformation cannot be computed. "
			"\n\n"
			"If :py:attr:`.calculate_deformation_gradients` is set to true, the modifier outputs a new particle property named ``Elastic Deformation Gradient``, "
			"which contains the per-atom elastic deformation gradient tensors. Each tensor has nine components stored in column-major order. "
			"Atoms for which the elastic deformation gradient could not be determined (i.e. which are classified as ``OTHER``) will be assigned the null tensor. "
			"\n\n"
			"If :py:attr:`.calculate_strain_tensors` is set to true, the modifier outputs a new particle property named ``Elastic Strain``, "
			"which contains the per-atom elastic strain tensors. Each symmetric strain tensor has six components stored in the order XX, YY, ZZ, XY, XZ, YZ. "
			"Atoms for which the elastic strain tensor could not be determined (i.e. which are classified as ``OTHER``) will be assigned the null tensor. "
			"\n\n"
			"Furthermore, the modifier generates a particle property ``Volumetric Strain``, which stores the trace divided by three of the local elastic strain tensor. "
			"Atoms for which the elastic strain tensor could not be determined (i.e. which are classified as ``OTHER``) will be assigned a value of zero. "
			"\n\n"
			)
		.add_property("input_crystal_structure", &ElasticStrainModifier::inputCrystalStructure, &ElasticStrainModifier::setInputCrystalStructure,
				"The type of crystal to analyze. Must be one of: "
				"\n\n"
				"  * ``ElasticStrainModifier.Lattice.FCC``\n"
				"  * ``ElasticStrainModifier.Lattice.HCP``\n"
				"  * ``ElasticStrainModifier.Lattice.BCC``\n"
				"  * ``ElasticStrainModifier.Lattice.CubicDiamond``\n"
				"  * ``ElasticStrainModifier.Lattice.HexagonalDiamond``\n"
				"\n\n"
				":Default: ``ElasticStrainModifier.Lattice.FCC``\n")
		.add_property("calculate_deformation_gradients", &ElasticStrainModifier::calculateDeformationGradients, &ElasticStrainModifier::setCalculateDeformationGradients,
				"Flag that enables the output of the calculated elastic deformation gradient tensors. The per-particle tensors will be stored in a new "
				"particle property named ``Elastic Deformation Gradient`` with nine components (stored in column-major order). "
				"Particles for which the local elastic deformation cannot be calculated, are assigned the null tensor. "
				"\n\n"
				":Default: False\n")
		.add_property("calculate_strain_tensors", &ElasticStrainModifier::calculateStrainTensors, &ElasticStrainModifier::setCalculateStrainTensors,
				"Flag that enables the calculation and out of the elastic strain tensors. The symmetric strain tensors will be stored in a new "
				"particle property named ``Elastic Strain`` with six components (XX, YY, ZZ, XY, XZ, YZ). "
				"\n\n"
				":Default: True\n")
		.add_property("push_strain_tensors_forward", &ElasticStrainModifier::pushStrainTensorsForward, &ElasticStrainModifier::setPushStrainTensorsForward,
				"Selects the frame in which the elastic strain tensors are calculated. "
				"\n\n"
				"If true, the *Eulerian-Almansi* finite strain tensor is computed, which measures the elastic strain in the global coordinate system (spatial frame). "
				"\n\n"
				"If false, the *Green-Lagrangian* strain tensor is computed, which measures the elastic strain in the local lattice coordinate system (material frame). "
				"\n\n"
				":Default: True\n")
		.add_property("lattice_constant", &ElasticStrainModifier::latticeConstant, &ElasticStrainModifier::setLatticeConstant,
				"Lattice constant (*a*:sub:`0`) of the ideal unit cell."
				"\n\n"
				":Default: 1.0\n")
		.add_property("axial_ratio", &ElasticStrainModifier::axialRatio, &ElasticStrainModifier::setAxialRatio,
				"The *c/a* ratio of the ideal unit cell for crystals with hexagonal symmetry."
				"\n\n"
				":Default: sqrt(8/3)\n")
	;

	ovito_class<GrainSegmentationModifier, StructureIdentificationModifier>()
		.add_property("input_crystal_structure", &GrainSegmentationModifier::inputCrystalStructure, &GrainSegmentationModifier::setInputCrystalStructure)
		.add_property("misorientation_threshold", &GrainSegmentationModifier::misorientationThreshold, &GrainSegmentationModifier::setMisorientationThreshold)
		.add_property("fluctuation_tolerance", &GrainSegmentationModifier::fluctuationTolerance, &GrainSegmentationModifier::setFluctuationTolerance)
		.add_property("min_atom_count", &GrainSegmentationModifier::minGrainAtomCount, &GrainSegmentationModifier::setMinGrainAtomCount)
	;

	ovito_class<SmoothDislocationsModifier, Modifier>()
		.add_property("smoothingEnabled", &SmoothDislocationsModifier::smoothingEnabled, &SmoothDislocationsModifier::setSmoothingEnabled)
		.add_property("smoothingLevel", &SmoothDislocationsModifier::smoothingLevel, &SmoothDislocationsModifier::setSmoothingLevel)
		.add_property("coarseningEnabled", &SmoothDislocationsModifier::coarseningEnabled, &SmoothDislocationsModifier::setCoarseningEnabled)
		.add_property("linePointInterval", &SmoothDislocationsModifier::linePointInterval, &SmoothDislocationsModifier::setLinePointInterval)
	;

	ovito_class<SmoothSurfaceModifier, Modifier>()
		.add_property("smoothingLevel", &SmoothSurfaceModifier::smoothingLevel, &SmoothSurfaceModifier::setSmoothingLevel)
	;

	ovito_class<CAImporter, FileSourceImporter>()
		.add_property("loadParticles", &CAImporter::loadParticles, &CAImporter::setLoadParticles)
	;

	ovito_class<CAExporter, ParticleExporter>()
		.add_property("export_mesh", &CAExporter::meshExportEnabled, &CAExporter::setMeshExportEnabled)
	;

	ovito_class<DislocationDisplay, DisplayObject>(
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of dislocation lines extracted by a :py:class:`~ovito.modifier.DislocationAnalysisModifier`. "
			"An instance of this class is attached to every :py:class:`~ovito.data.DislocationNetwork` data object. ")
		.add_property("shading", &DislocationDisplay::shadingMode, &DislocationDisplay::setShadingMode,
				"The shading style used for the lines.\n"
				"Possible values:"
				"\n\n"
				"   * ``DislocationDisplay.Shading.Normal`` (default) \n"
				"   * ``DislocationDisplay.Shading.Flat``\n"
				"\n")
		.add_property("burgers_vector_width", &DislocationDisplay::burgersVectorWidth, &DislocationDisplay::setBurgersVectorWidth,
				"Specifies the width of Burgers vector arrows (in length units)."
				"\n\n"
				":Default: 0.6\n")
		.add_property("burgers_vector_width", &DislocationDisplay::burgersVectorScaling, &DislocationDisplay::setBurgersVectorScaling,
				"The scaling factor applied to displayed Burgers vectors. This can be used to exaggerate the arrow size."
				"\n\n"
				":Default: 1.0\n")
		.add_property("burgers_vector_color", make_function(&DislocationDisplay::burgersVectorColor, return_value_policy<copy_const_reference>()), &DislocationDisplay::setBurgersVectorColor,
				"The color of Burgers vector arrows."
				"\n\n"
				":Default: ``(0.7, 0.7, 0.7)``\n")
		.add_property("show_burgers_vectors", &DislocationDisplay::showBurgersVectors, &DislocationDisplay::setShowBurgersVectors,
				"Boolean flag that enables the display of Burgers vector arrows."
				"\n\n"
				":Default: ``False``\n")
		.add_property("show_line_directions", &DislocationDisplay::showLineDirections, &DislocationDisplay::setShowLineDirections,
				"Boolean flag that enables the visualization of line directions."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<DislocationNetworkObject, DataObject>(
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"This data object types stores the network of dislocation lines extracted by a :py:class:`~ovito.modifiers.DislocationAnalysisModifier`."
			"\n\n"
			"Instances of this class are associated with a :py:class:`~ovito.vis.DislocationDisplay` "
			"that controls the visual appearance of the dislocation lines. It can be accessed through "
			"the :py:attr:`~DataObject.display` attribute of the :py:class:`~DataObject` base class."
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/dislocation_analysis_modifier.py",
			// Python class name:
			"DislocationNetwork")

		.add_property("segments", make_function(&DislocationNetworkObject::segments, return_internal_reference<>()),
				"The list of dislocation segments in this dislocation network. "
				"This list-like object is read-only and contains :py:class:`~ovito.data.DislocationSegment` objects.")

	;

	class_<DislocationSegment, DislocationSegment*, boost::noncopyable>("DislocationSegment",
		"A single dislocation line from a :py:class:`DislocationNetwork`. "
		"\n\n"
		"The list of dislocation segments is returned by the :py:attr:`DislocationNetwork.segments` attribute.",
		no_init)
		.add_property("id", &DislocationSegment::id,
				"The unique identifier of this dislocation segment.")
		.add_property("is_loop", &DislocationSegment::isClosedLoop,
				"This property indicates whether this segment forms a closed dislocation loop. "
				"Note that an infinite dislocation line passing through a periodic boundary is also considered a loop. "
				"\n\n"
				"See also the :py:attr:`.is_infinite_line` property. ")
		.add_property("is_infinite_line", &DislocationSegment::isInfiniteLine,
				"This property indicates whether this segment is an infinite line passing through a periodic simulation box boundary. "
				"A segment is considered infinite if it is a closed loop and its start and end points do not coincide. "
				"\n\n"
				"See also the :py:attr:`.is_loop` property. ")
		.add_property("length", &DislocationSegment::calculateLength,
				"Returns the length of this dislocation segment.")
		.add_property("true_burgers_vector", make_function(static_cast<const Vector3& (*)(const DislocationSegment&)>(
				[](const DislocationSegment& segment) -> const Vector3& { return segment.burgersVector.localVec(); }), return_internal_reference<>()),
				"The Burgers vector of the segment, expressed in the local coordinate system of the crystal. Also known as the True Burgers vector.")
		.def_readonly("_line", &DislocationSegment::line)
		.add_property("cluster_id", static_cast<int (*)(const DislocationSegment&)>(
				[](const DislocationSegment& segment) { return segment.burgersVector.cluster()->id; }),
				"The unique identifier of the cluster of crystal atoms that contains this dislocation segment. "
				"\n\n"
				"The Burgers vector of the segment is expressed in the local coordinate system of this atomic cluster.")
	;

	class_<std::vector<DislocationSegment*>, boost::noncopyable>("DislocationSegmentList", no_init)
		.def(array_indexing_suite<std::vector<DislocationSegment*>, return_internal_reference<>>())
	;

	class_<std::deque<Point3>>("deque<Point3>")
		.def(array_indexing_suite<std::deque<Point3>>())
	;

}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(CrystalAnalysis);

}	// End of namespace
}	// End of namespace
}	// End of namespace
