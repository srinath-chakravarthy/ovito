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
//#include <plugins/crystalanalysis/modifier/grains/GrainSegmentationModifier.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationDisplay.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/BurgersVectorFamily.h>
#include <plugins/crystalanalysis/objects/patterns/PatternCatalog.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMesh.h>
#include <plugins/crystalanalysis/objects/partition_mesh/PartitionMeshDisplay.h>
#include <plugins/crystalanalysis/importer/CAImporter.h>
#include <plugins/crystalanalysis/exporter/CAExporter.h>
#include <plugins/pyscript/binding/PythonBinding.h>
#include <plugins/particles/scripting/PythonBinding.h>
#include <core/plugins/PluginManager.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

using namespace PyScript;

PYBIND11_PLUGIN(CrystalAnalysis)
{
	// Register the classes of this plugin with the global PluginManager.
	PluginManager::instance().registerLoadedPluginClasses();
	
	py::options options;
	options.disable_function_signatures();

	py::module m("CrystalAnalysis");

	ovito_class<ConstructSurfaceModifier, AsynchronousParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Constructs the geometric surface of a solid made of point-like particles. The modifier generates "
			"a :py:class:`~ovito.data.SurfaceMesh`, which is a closed manifold consisting of triangles. It also computes the total "
			"surface area and the volume of the region enclosed by the surface mesh."
			"\n\n"
			"The :py:attr:`.radius` parameter controls how many details of the solid shape are resolved during surface construction. "
			"A larger radius leads to a surface with fewer details, reflecting only coarse features of the surface topology. "
			"A small radius, on the other hand, will resolve finer surface features and small pores in the interior of a solid, for example. "
			"\n\n"
			"See `[A. Stukowski, JOM 66 (2014), 399-407] <http://dx.doi.org/10.1007/s11837-013-0827-5>`_ for a description of the surface construction algorithm."
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * :py:attr:`DataCollection.surface <ovito.data.DataCollection.surface>` (:py:class:`~ovito.data.SurfaceMesh`):\n"
			"   This property of the output data collection provides access to the surface mesh computed by the modifier.\n"
			"   See the example script below.\n"
			" * ``ConstructSurfaceMesh.surface_area`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The area of the surface mesh.\n"
			" * ``ConstructSurfaceMesh.solid_volume`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The volume of the solid region bounded by the surface mesh.\n"
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/construct_surface_modifier.py\n"
			"   :lines: 4-\n"
			)
		.def_property("radius", &ConstructSurfaceModifier::probeSphereRadius, &ConstructSurfaceModifier::setProbeSphereRadius,
				"The radius of the probe sphere used in the surface construction algorithm."
				"\n\n"
				"A rule of thumb is that the radius parameter should be slightly larger than the typical distance between "
				"nearest neighbor particles."
				"\n\n"
				":Default: 4.0\n")
		.def_property("smoothing_level", &ConstructSurfaceModifier::smoothingLevel, &ConstructSurfaceModifier::setSmoothingLevel,
				"The number of iterations of the smoothing algorithm applied to the computed surface mesh."
				"\n\n"
				"Note that the smoothing level does only affect the computed surface area but not the solid volume. "
				"That is because the solid volume is computed before smoothing the mesh. (Smoothing is supposed to be "
				"volume preserving.)"
				"\n\n"
				":Default: 8\n")
		.def_property("only_selected", &ConstructSurfaceModifier::onlySelectedParticles, &ConstructSurfaceModifier::setOnlySelectedParticles,
				"If ``True``, the modifier acts only on selected particles and ignores other particles; "
				"if ``False``, the modifier constructs the surface around all particles."
				"\n\n"
				":Default: ``False``\n")
		.def_property_readonly("mesh_display", &ConstructSurfaceModifier::surfaceMeshDisplay,
				"The :py:class:`~ovito.vis.SurfaceMeshDisplay` controlling the visual representation of the computed surface.\n")
	;

	auto DislocationAnalysisModifier_py = ovito_class<DislocationAnalysisModifier, StructureIdentificationModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This analysis modifier extracts all dislocations in a crystal and converts them to continuous line segments. "
			"The computational method behind this is called *Dislocation Extraction Algorithm* (DXA) and is described "
			"in the paper `[MSMSE 20 (2012), 085007] <http://stacks.iop.org/0965-0393/20/085007>`_."
			"\n\n"
			"The extracted dislocation lines are output as a :py:class:`~ovito.data.DislocationNetwork` object by the modifier "
			"and can be accessed through the :py:attr:`DataCollection.dislocations <ovito.data.DataCollection.dislocations>` field "
			"after the modification pipeline has been evaluated. This is demonstrated in the example script below. "
			"\n\n"
			"Furthermore, you can use the :py:func:`~ovito.io.export_file` function to write the dislocation lines "
			"to a so-called CA file. The CA file format is described in the documentation section of the OVITO user manual for the "
			"Dislocation Analysis modifier."
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * :py:attr:`DataCollection.dislocations <ovito.data.DataCollection.dislocations>` (:py:class:`~ovito.data.DislocationNetwork`):\n"
			"   This property of the output data collection provides access to the dislocation lines found by the modifier.\n"
			"   See the example script below.\n"
			" * ``DislocationAnalysis.total_line_length`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The total length of all dislocation lines found by the DXA.\n"
			" * ``DislocationAnalysis.length.1/n<ijk>`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The modifier outputs a set of attributes that indicate the length of dislocations broken down by Burgers vector type. "
			"   For example, the attribute ``DislocationAnalysis.length.1/6<112>`` specifies the total amount of Shockley partials found by the DXA.\n"
			" * ``DislocationAnalysis.length.other`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The length of dislocation lines with an unusual Burgers vector that do not belong to any of the predefined standard dislocation types.\n"
			" * ``DislocationAnalysis.cell_volume`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The volume of the simulation cell. This is output for convenience to enable the calculation of dislocation densities from the line length.\n"
			" * ``DislocationAnalysis.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles not matching any of the known structure types.\n"
			" * ``DislocationAnalysis.counts.FCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles with local FCC structure.\n"
			" * ``DislocationAnalysis.counts.HCP`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles with local HCP structure.\n"
			" * ``DislocationAnalysis.counts.BCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles with local BCC structure.\n"
			" * ``DislocationAnalysis.counts.CubicDiamond`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles with local cubic diamond structure.\n"
			" * ``DislocationAnalysis.counts.HexagonalDiamond`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles with local hexagonal diamond structure.\n"
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/dislocation_analysis_modifier.py\n"
			"   :lines: 4-\n"
			)
		.def_property("trial_circuit_length", &DislocationAnalysisModifier::maxTrialCircuitSize, &DislocationAnalysisModifier::setMaxTrialCircuitSize,
				"The maximum length of trial Burgers circuits constructed by the DXA to discover dislocations. "
				"The length is specified in terms of the number of atom-to-atom steps."
				"\n\n"
				":Default: 14\n")
		.def_property("circuit_stretchability", &DislocationAnalysisModifier::circuitStretchability, &DislocationAnalysisModifier::setCircuitStretchability,
				"The number of steps by which a Burgers circuit can stretch while it is being advanced along a dislocation line."
				"\n\n"
				":Default: 9\n")
		.def_property("input_crystal_structure", &DislocationAnalysisModifier::inputCrystalStructure, &DislocationAnalysisModifier::setInputCrystalStructure,
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
		.def_property("reconstruct_edge_vectors", &DislocationAnalysisModifier::reconstructEdgeVectors, &DislocationAnalysisModifier::setReconstructEdgeVectors,
				"Flag that enables the reconstruction of ideal lattice vectors in highly distorted crystal regions. This algorithm step is supposed to improve "
				"the identification of dislocations in some situations, but it may have undesirable side effect. Use with care, only for experts!"
				"\n\n"
				":Default: False\n")
#endif
		.def_property("line_smoothing_enabled", &DislocationAnalysisModifier::lineSmoothingEnabled, &DislocationAnalysisModifier::setLineSmoothingEnabled,
				"Flag that enables the smoothing of extracted dislocation lines after they have been coarsened."
				"\n\n"
				":Default: True\n")
		.def_property("line_coarsening_enabled", &DislocationAnalysisModifier::lineCoarseningEnabled, &DislocationAnalysisModifier::setLineCoarseningEnabled,
				"Flag that enables the coarsening of extracted dislocation lines, which reduces the number of sample points along the lines."
				"\n\n"
				":Default: True\n")
		.def_property("line_smoothing_level", &DislocationAnalysisModifier::lineSmoothingLevel, &DislocationAnalysisModifier::setLineSmoothingLevel,
				"The number of iterations of the line smoothing algorithm to perform."
				"\n\n"
				":Default: 1\n")
		.def_property("line_point_separation", &DislocationAnalysisModifier::linePointInterval, &DislocationAnalysisModifier::setLinePointInterval,
				"Sets the desired distance between successive sample points along the dislocation lines, measured in multiples of the interatomic spacing. "
				"This parameter controls the amount of coarsening performed during post-processing of dislocation lines."
				"\n\n"
				":Default: 2.5\n")
		.def_property("defect_mesh_smoothing_level", &DislocationAnalysisModifier::defectMeshSmoothingLevel, &DislocationAnalysisModifier::setDefectMeshSmoothingLevel,
				"Specifies the number of iterations of the surface smoothing algorithm to perform when post-processing the extracted defect mesh."
				"\n\n"
				":Default: 8\n")
		.def_property("only_perfect_dislocations", &DislocationAnalysisModifier::onlyPerfectDislocations, &DislocationAnalysisModifier::setOnlyPerfectDislocations,
				"This flag controls whether the algorithm should extract only perfect dislocations (and no partial dislocations, which is normally done for FCC/HCP and diamond lattices). "
				"Make sure you set the :py:attr:`.circuit_stretchability` parameter to a high value when activating this option, because "
				"large Burgers circuits are needed to identify dissociated dislocations with a wide core. "
				"\n\n"
				":Default: False\n")
		.def_property("output_interface_mesh", &DislocationAnalysisModifier::outputInterfaceMesh, &DislocationAnalysisModifier::setOutputInterfaceMesh)
	;

	py::enum_<StructureAnalysis::LatticeStructureType>(DislocationAnalysisModifier_py, "Lattice")
		.value("Other", StructureAnalysis::LATTICE_OTHER)
		.value("FCC", StructureAnalysis::LATTICE_FCC)
		.value("HCP", StructureAnalysis::LATTICE_HCP)
		.value("BCC", StructureAnalysis::LATTICE_BCC)
		.value("CubicDiamond", StructureAnalysis::LATTICE_CUBIC_DIAMOND)
		.value("HexagonalDiamond", StructureAnalysis::LATTICE_HEX_DIAMOND)
	;

	ovito_class<ElasticStrainModifier, StructureIdentificationModifier>(m,
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
		.def_property("input_crystal_structure", &ElasticStrainModifier::inputCrystalStructure, &ElasticStrainModifier::setInputCrystalStructure,
				"The type of crystal to analyze. Must be one of: "
				"\n\n"
				"  * ``ElasticStrainModifier.Lattice.FCC``\n"
				"  * ``ElasticStrainModifier.Lattice.HCP``\n"
				"  * ``ElasticStrainModifier.Lattice.BCC``\n"
				"  * ``ElasticStrainModifier.Lattice.CubicDiamond``\n"
				"  * ``ElasticStrainModifier.Lattice.HexagonalDiamond``\n"
				"\n\n"
				":Default: ``ElasticStrainModifier.Lattice.FCC``\n")
		.def_property("calculate_deformation_gradients", &ElasticStrainModifier::calculateDeformationGradients, &ElasticStrainModifier::setCalculateDeformationGradients,
				"Flag that enables the output of the calculated elastic deformation gradient tensors. The per-particle tensors will be stored in a new "
				"particle property named ``Elastic Deformation Gradient`` with nine components (stored in column-major order). "
				"Particles for which the local elastic deformation cannot be calculated, are assigned the null tensor. "
				"\n\n"
				":Default: False\n")
		.def_property("calculate_strain_tensors", &ElasticStrainModifier::calculateStrainTensors, &ElasticStrainModifier::setCalculateStrainTensors,
				"Flag that enables the calculation and out of the elastic strain tensors. The symmetric strain tensors will be stored in a new "
				"particle property named ``Elastic Strain`` with six components (XX, YY, ZZ, XY, XZ, YZ). "
				"\n\n"
				":Default: True\n")
		.def_property("push_strain_tensors_forward", &ElasticStrainModifier::pushStrainTensorsForward, &ElasticStrainModifier::setPushStrainTensorsForward,
				"Selects the frame in which the elastic strain tensors are calculated. "
				"\n\n"
				"If true, the *Eulerian-Almansi* finite strain tensor is computed, which measures the elastic strain in the global coordinate system (spatial frame). "
				"\n\n"
				"If false, the *Green-Lagrangian* strain tensor is computed, which measures the elastic strain in the local lattice coordinate system (material frame). "
				"\n\n"
				":Default: True\n")
		.def_property("lattice_constant", &ElasticStrainModifier::latticeConstant, &ElasticStrainModifier::setLatticeConstant,
				"Lattice constant (*a*:sub:`0`) of the ideal unit cell."
				"\n\n"
				":Default: 1.0\n")
		.def_property("axial_ratio", &ElasticStrainModifier::axialRatio, &ElasticStrainModifier::setAxialRatio,
				"The *c/a* ratio of the ideal unit cell for crystals with hexagonal symmetry."
				"\n\n"
				":Default: sqrt(8/3)\n")
	;

#if 0
	ovito_class<GrainSegmentationModifier, StructureIdentificationModifier>(m)
		.def_property("input_crystal_structure", &GrainSegmentationModifier::inputCrystalStructure, &GrainSegmentationModifier::setInputCrystalStructure)
		.def_property("misorientation_threshold", &GrainSegmentationModifier::misorientationThreshold, &GrainSegmentationModifier::setMisorientationThreshold)
		.def_property("fluctuation_tolerance", &GrainSegmentationModifier::fluctuationTolerance, &GrainSegmentationModifier::setFluctuationTolerance)
		.def_property("min_atom_count", &GrainSegmentationModifier::minGrainAtomCount, &GrainSegmentationModifier::setMinGrainAtomCount)
		.def_property("probe_sphere_radius", &GrainSegmentationModifier::probeSphereRadius, &GrainSegmentationModifier::setProbeSphereRadius,
				"The radius of the probe sphere used in the free surface construction algorithm."
				"\n\n"
				"A rule of thumb is that the probe sphere radius should be slightly larger than the typical distance between "
				"nearest neighbor atoms."
				"\n\n"
				"This parameter is ignored if :py:attr:`.output_mesh` is false."
				"\n\n"
				":Default: 4.0\n")
		.def_property("smoothing_level", &GrainSegmentationModifier::smoothingLevel, &GrainSegmentationModifier::setSmoothingLevel,
				"The number of iterations of the smoothing algorithm applied to the computed grain boundary mesh. "
				"\n\n"
				"This parameter is ignored if :py:attr:`.output_mesh` is false."
				"\n\n"
				":Default: 8\n")
		.def_property("only_selected", &GrainSegmentationModifier::onlySelectedParticles, &GrainSegmentationModifier::setOnlySelectedParticles,
				"This flag tells the modifier to ignore unselected particles."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_mesh", &GrainSegmentationModifier::outputPartitionMesh, &GrainSegmentationModifier::setOutputPartitionMesh,
				"This flag controls the generation of the grain boundary mesh. "
				"If true, the modifier will generate a geometric representation of the grain boundary network "
				"in addition to assigning each input atom to a grain. "
				"\n\n"
				":Default: ``False``\n")
	;
#endif

	ovito_class<SmoothDislocationsModifier, Modifier>(m)
		.def_property("smoothingEnabled", &SmoothDislocationsModifier::smoothingEnabled, &SmoothDislocationsModifier::setSmoothingEnabled)
		.def_property("smoothingLevel", &SmoothDislocationsModifier::smoothingLevel, &SmoothDislocationsModifier::setSmoothingLevel)
		.def_property("coarseningEnabled", &SmoothDislocationsModifier::coarseningEnabled, &SmoothDislocationsModifier::setCoarseningEnabled)
		.def_property("linePointInterval", &SmoothDislocationsModifier::linePointInterval, &SmoothDislocationsModifier::setLinePointInterval)
	;

	ovito_class<SmoothSurfaceModifier, Modifier>(m)
		.def_property("smoothingLevel", &SmoothSurfaceModifier::smoothingLevel, &SmoothSurfaceModifier::setSmoothingLevel)
	;

	ovito_class<CAImporter, FileSourceImporter>(m)
		.def_property("loadParticles", &CAImporter::loadParticles, &CAImporter::setLoadParticles)
	;

	ovito_class<CAExporter, FileExporter>(m)
		.def_property("export_mesh", &CAExporter::meshExportEnabled, &CAExporter::setMeshExportEnabled)
	;

	auto DislocationDisplay_py = ovito_class<DislocationDisplay, DisplayObject>(m,
			":Base class: :py:class:`ovito.vis.Display`\n\n"
			"Controls the visual appearance of dislocation lines extracted by a :py:class:`~ovito.modifier.DislocationAnalysisModifier`. "
			"An instance of this class is attached to every :py:class:`~ovito.data.DislocationNetwork` data object. ")
		.def_property("shading", &DislocationDisplay::shadingMode, &DislocationDisplay::setShadingMode,
				"The shading style used for the lines.\n"
				"Possible values:"
				"\n\n"
				"   * ``DislocationDisplay.Shading.Normal`` (default) \n"
				"   * ``DislocationDisplay.Shading.Flat``\n"
				"\n")
		.def_property("burgers_vector_width", &DislocationDisplay::burgersVectorWidth, &DislocationDisplay::setBurgersVectorWidth,
				"Specifies the width of Burgers vector arrows (in length units)."
				"\n\n"
				":Default: 0.6\n")
		.def_property("burgers_vector_width", &DislocationDisplay::burgersVectorScaling, &DislocationDisplay::setBurgersVectorScaling,
				"The scaling factor applied to displayed Burgers vectors. This can be used to exaggerate the arrow size."
				"\n\n"
				":Default: 1.0\n")
		.def_property("burgers_vector_color", &DislocationDisplay::burgersVectorColor, &DislocationDisplay::setBurgersVectorColor,
				"The color of Burgers vector arrows."
				"\n\n"
				":Default: ``(0.7, 0.7, 0.7)``\n")
		.def_property("show_burgers_vectors", &DislocationDisplay::showBurgersVectors, &DislocationDisplay::setShowBurgersVectors,
				"Boolean flag that enables the display of Burgers vector arrows."
				"\n\n"
				":Default: ``False``\n")
		.def_property("show_line_directions", &DislocationDisplay::showLineDirections, &DislocationDisplay::setShowLineDirections,
				"Boolean flag that enables the visualization of line directions."
				"\n\n"
				":Default: ``False``\n")
		.def_property("indicate_character", &DislocationDisplay::lineColoringMode, &DislocationDisplay::setLineColoringMode,
				"Controls how the display color of dislocation lines is chosen."
				"Possible values:"
				"\n\n"
				"   * ``DislocationDisplay.ColoringMode.ByDislocationType`` (default) \n"
				"   * ``DislocationDisplay.ColoringMode.ByBurgersVector``\n"
				"   * ``DislocationDisplay.ColoringMode.ByCharacter``\n"
				"\n")
	;

	py::enum_<DislocationDisplay::LineColoringMode>(DislocationDisplay_py, "ColoringMode")
		.value("ByDislocationType", DislocationDisplay::ColorByDislocationType)
		.value("ByBurgersVector", DislocationDisplay::ColorByBurgersVector)
		.value("ByCharacter", DislocationDisplay::ColorByCharacter)
	;

	ovito_class<DislocationNetworkObject, DataObject>(m,
			":Base class: :py:class:`ovito.data.DataObject`\n\n"
			"This data object types stores the network of dislocation lines extracted by a :py:class:`~ovito.modifiers.DislocationAnalysisModifier`."
			"\n\n"
			"Instances of this class are associated with a :py:class:`~ovito.vis.DislocationDisplay` "
			"that controls the visual appearance of the dislocation lines. It can be accessed through "
			"the :py:attr:`~DataObject.display` attribute of the :py:class:`~DataObject` base class."
			"\n\n"
			"Example:\n\n"
			".. literalinclude:: ../example_snippets/dislocation_analysis_modifier.py\n"
			"   :lines: 4-\n",
			// Python class name:
			"DislocationNetwork")

		.def_property_readonly("segments", py::cpp_function(&DislocationNetworkObject::segments, py::return_value_policy::reference_internal),
				"The list of dislocation segments in this dislocation network. "
				"This list-like object is read-only and contains :py:class:`~ovito.data.DislocationSegment` objects.")
	;

	py::class_<DislocationSegment>(m, "DislocationSegment",
			"A single dislocation line from a :py:class:`DislocationNetwork`. "
			"\n\n"
			"The list of dislocation segments is returned by the :py:attr:`DislocationNetwork.segments` attribute.")
		.def_readonly("id", &DislocationSegment::id,
				"The unique identifier of this dislocation segment.")
		.def_property_readonly("is_loop", &DislocationSegment::isClosedLoop,
				"This property indicates whether this segment forms a closed dislocation loop. "
				"Note that an infinite dislocation line passing through a periodic boundary is also considered a loop. "
				"\n\n"
				"See also the :py:attr:`.is_infinite_line` property. ")
		.def_property_readonly("is_infinite_line", &DislocationSegment::isInfiniteLine,
				"This property indicates whether this segment is an infinite line passing through a periodic simulation box boundary. "
				"A segment is considered infinite if it is a closed loop and its start and end points do not coincide. "
				"\n\n"
				"See also the :py:attr:`.is_loop` property. ")
		.def_property_readonly("length", &DislocationSegment::calculateLength,
				"Returns the length of this dislocation segment.")
		.def_property_readonly("true_burgers_vector", py::cpp_function([](const DislocationSegment& segment) -> const Vector3& {
					return segment.burgersVector.localVec();
				}, py::return_value_policy::reference_internal),
				"The Burgers vector of the segment, expressed in the local coordinate system of the crystal. Also known as the True Burgers vector.")
		.def_property_readonly("spatial_burgers_vector", [](const DislocationSegment& segment) -> Vector3 {
					return segment.burgersVector.toSpatialVector();
				},
				"The Burgers vector of the segment, expressed in the global coordinate system of the simulation. This vector is calculated "
				"by transforming the true Burgers vector from the local lattice coordinate system to the global simulation coordinate system "
				"using the average orientation matrix of the crystal cluster the dislocation segment is embedded in.")
		.def_property_readonly("points", [](const DislocationSegment& segment) {
					py::array_t<FloatType> array({ segment.line.size(), 3 });
					for(size_t i = 0; i < segment.line.size(); i++) {
						for(size_t j = 0; j < 3; j++) {
							array.mutable_at(i, j) = segment.line[i][j];
						}
					}
					return array;
				},
				"The list of space points that define the shape of this dislocation segment. " 
        		"This is a *N* x 3 Numpy array, where *N* is the number of points along the "
        		"segment. For closed loops, the first and the last point coincide.")
		.def_property_readonly("cluster_id", [](const DislocationSegment& segment) { 
					return segment.burgersVector.cluster()->id; 
				},
				"The numeric identifier of the crystal cluster of atoms containing this dislocation segment. "
				"\n\n"
				"The true Burgers vector of the segment is expressed in the local coordinate system of this crystal cluster.")
	;

	ovito_class<ClusterGraphObject, DataObject>(m, "ClusterGraph")
		//.def_property("clusters", make_function(&ClusterGraphObject::clusters, return_internal_reference<>()))
		//.def("find_cluster", make_function(&ClusterGraphObject::findCluster, return_internal_reference<>()))
	;

	py::class_<Cluster>(m, "Cluster")
		.def_readonly("id", &Cluster::id,
				"The unique identifier of this atomic cluster.")
		.def_readonly("atom_count", &Cluster::atomCount)
		.def_readonly("orientation", &Cluster::orientation)
	;

	ovito_class<PartitionMesh, DataObject>{m}
	;

	ovito_class<PartitionMeshDisplay, DisplayObject>(m)
		.def_property("surface_color", &PartitionMeshDisplay::surfaceColor, &PartitionMeshDisplay::setSurfaceColor,
				"The display color of the outer free surface."
				"\n\n"
				":Default: ``(1.0, 1.0, 1.0)``\n")
		.def_property("show_cap", &PartitionMeshDisplay::showCap, &PartitionMeshDisplay::setShowCap,
				"Controls the visibility of cap polygons, which are created at the intersection of the mesh with periodic box boundaries."
				"\n\n"
				":Default: ``True``\n")
		.def_property("surface_transparency", &PartitionMeshDisplay::surfaceTransparency, &PartitionMeshDisplay::setSurfaceTransparency,
				"The level of transparency of the displayed surface. Valid range is 0.0 -- 1.0."
				"\n\n"
				":Default: 0.0\n")
		.def_property("cap_transparency", &PartitionMeshDisplay::capTransparency, &PartitionMeshDisplay::setCapTransparency,
				"The level of transparency of the displayed cap polygons. Valid range is 0.0 -- 1.0."
				"\n\n"
				":Default: 0.0\n")
		.def_property("smooth_shading", &PartitionMeshDisplay::smoothShading, &PartitionMeshDisplay::setSmoothShading,
				"Enables smooth shading of the triangulated surface mesh."
				"\n\n"
				":Default: ``True``\n")
	;

	return m.ptr();
}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(CrystalAnalysis);

}	// End of namespace
}	// End of namespace
}	// End of namespace
