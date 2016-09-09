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
#include <plugins/particles/modifier/ParticleModifier.h>
#include <plugins/particles/modifier/AsynchronousParticleModifier.h>
#include <plugins/particles/modifier/coloring/AssignColorModifier.h>
#include <plugins/particles/modifier/coloring/ColorCodingModifier.h>
#include <plugins/particles/modifier/coloring/AmbientOcclusionModifier.h>
#include <plugins/particles/modifier/modify/DeleteParticlesModifier.h>
#include <plugins/particles/modifier/modify/ShowPeriodicImagesModifier.h>
#include <plugins/particles/modifier/modify/WrapPeriodicImagesModifier.h>
#include <plugins/particles/modifier/modify/SliceModifier.h>
#include <plugins/particles/modifier/modify/AffineTransformationModifier.h>
#include <plugins/particles/modifier/modify/CreateBondsModifier.h>
#include <plugins/particles/modifier/modify/LoadTrajectoryModifier.h>
#include <plugins/particles/modifier/properties/ComputePropertyModifier.h>
#include <plugins/particles/modifier/properties/FreezePropertyModifier.h>
#include <plugins/particles/modifier/properties/ComputeBondLengthsModifier.h>
#include <plugins/particles/modifier/selection/ClearSelectionModifier.h>
#include <plugins/particles/modifier/selection/InvertSelectionModifier.h>
#include <plugins/particles/modifier/selection/ManualSelectionModifier.h>
#include <plugins/particles/modifier/selection/ExpandSelectionModifier.h>
#include <plugins/particles/modifier/selection/SelectExpressionModifier.h>
#include <plugins/particles/modifier/selection/SelectParticleTypeModifier.h>
#include <plugins/particles/modifier/analysis/StructureIdentificationModifier.h>
#include <plugins/particles/modifier/analysis/binandreduce/BinAndReduceModifier.h>
#include <plugins/particles/modifier/analysis/bondangle/BondAngleAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/cna/CommonNeighborAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/centrosymmetry/CentroSymmetryModifier.h>
#include <plugins/particles/modifier/analysis/cluster/ClusterAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/coordination/CoordinationNumberModifier.h>
#include <plugins/particles/modifier/analysis/displacements/CalculateDisplacementsModifier.h>
#include <plugins/particles/modifier/analysis/histogram/HistogramModifier.h>
#include <plugins/particles/modifier/analysis/scatterplot/ScatterPlotModifier.h>
#include <plugins/particles/modifier/analysis/strain/AtomicStrainModifier.h>
#include <plugins/particles/modifier/analysis/wignerseitz/WignerSeitzAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/ptm/PolyhedralTemplateMatchingModifier.h>
#include <plugins/particles/modifier/analysis/voronoi/VoronoiAnalysisModifier.h>
#include <plugins/particles/modifier/analysis/diamond/IdentifyDiamondModifier.h>
#include <core/scene/pipeline/ModifierApplication.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace boost::python;
using namespace PyScript;

BOOST_PYTHON_MODULE(ParticlesModify)
{
	docstring_options docoptions(true, false, false);

	ovito_abstract_class<ParticleModifier, Modifier>()
	;

	ovito_abstract_class<AsynchronousParticleModifier, ParticleModifier>()
	;

	ovito_class<AssignColorModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Assigns a uniform color to all selected particles. "
			"If no particle selection is defined (i.e. the ``\"Selection\"`` particle property does not exist), "
			"the modifier assigns the color to all particles. ")
		.add_property("color", &AssignColorModifier::color, &AssignColorModifier::setColor,
				"The color that will be assigned to particles."
				"\n\n"
				":Default: ``(0.3,0.3,1.0)``\n")
		.add_property("colorController", make_function(&AssignColorModifier::colorController, return_value_policy<ovito_object_reference>()), &AssignColorModifier::setColorController)
		.add_property("keepSelection", &AssignColorModifier::keepSelection, &AssignColorModifier::setKeepSelection)
	;

	{
		scope s = ovito_class<ColorCodingModifier, ParticleModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"Colors particles or bonds based on the value of a property."
				"\n\n"
				"Usage example::"
				"\n\n"
				"    from ovito.modifiers import *\n"
				"    \n"
				"    modifier = ColorCodingModifier(\n"
				"        particle_property = \"Potential Energy\",\n"
				"        gradient = ColorCodingModifier.Hot()\n"
				"    )\n"
				"    node.modifiers.append(modifier)\n"
				"\n"
				"If, as in the example above, the :py:attr:`.start_value` and :py:attr:`.end_value` parameters are not explicitly set, "
				"then the modifier automatically adjusts them to the minimum and maximum values of the property when the modifier "
				"is inserted into the modification pipeline.")
			.add_property("property", make_function(&ColorCodingModifier::sourceParticleProperty, return_value_policy<copy_const_reference>()), &ColorCodingModifier::setSourceParticleProperty)
			.add_property("particle_property", make_function(&ColorCodingModifier::sourceParticleProperty, return_value_policy<copy_const_reference>()), &ColorCodingModifier::setSourceParticleProperty,
					"The name of the input particle property that should be used to color particles. "
					"This can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. "
					"When using vector properties the component must be included in the name, e.g. ``\"Velocity.X\"``. "
					"\n\n"
					"This field is only used if :py:attr:`.bond_mode` is set to false. ")
			.add_property("bond_property", make_function(&ColorCodingModifier::sourceBondProperty, return_value_policy<copy_const_reference>()), &ColorCodingModifier::setSourceBondProperty,
					"The name of the input bond property that should be used to color bonds. "
					"This can be one of the :ref:`standard bond properties <bond-types-list>` or a custom bond property. "
					"\n\n"
					"This field is only used if :py:attr:`.bond_mode` is set to true. ")
			.add_property("start_value", &ColorCodingModifier::startValue, &ColorCodingModifier::setStartValue,
					"This parameter defines the value range when mapping the input property to a color.")
			.add_property("startValueController", make_function(&ColorCodingModifier::startValueController, return_value_policy<ovito_object_reference>()), &ColorCodingModifier::setStartValueController)
			.add_property("end_value", &ColorCodingModifier::endValue, &ColorCodingModifier::setEndValue,
					"This parameter defines the value range when mapping the input property to a color.")
			.add_property("endValueController", make_function(&ColorCodingModifier::endValueController, return_value_policy<ovito_object_reference>()), &ColorCodingModifier::setEndValueController)
			.add_property("gradient", make_function(&ColorCodingModifier::colorGradient, return_value_policy<ovito_object_reference>()), &ColorCodingModifier::setColorGradient,
					"The color gradient object, which is responsible for mapping normalized property values to colors. "
					"Available gradient types are:\n"
					" * ``ColorCodingModifier.Rainbow()`` [default]\n"
					" * ``ColorCodingModifier.Grayscale()``\n"
					" * ``ColorCodingModifier.Hot()``\n"
					" * ``ColorCodingModifier.Jet()``\n"
					" * ``ColorCodingModifier.BlueWhiteRed()``\n"
					" * ``ColorCodingModifier.Custom(\"<image file>\")``\n"
					"\n"
					"The last color map constructor expects the path to an image file on disk, "
					"which will be used to create a custom color gradient from a row of pixels in the image.")
			.add_property("only_selected", &ColorCodingModifier::colorOnlySelected, &ColorCodingModifier::setColorOnlySelected,
					"If ``True``, only selected particles or bonds will be affected by the modifier and the existing colors "
					"of unselected particles or bonds will be preserved; if ``False``, all particles/bonds will be colored."
					"\n\n"
					":Default: ``False``\n")
			.add_property("keepSelection", &ColorCodingModifier::keepSelection, &ColorCodingModifier::setKeepSelection)
			.add_property("bond_mode", &ColorCodingModifier::operateOnBonds, &ColorCodingModifier::setOperateOnBonds,
					"Controls whether the modifier assigns colors to particles or bonds.\n"
					"\n\n"
					"If this is set to true, then the bond property selected by :py:attr:`.bond_property` is used to color bonds. "
					"Otherwise the particle property selected by :py:attr:`.particle_property` is used to color particles."
					"\n\n"
					":Default: ``False``\n")
		;

		ovito_abstract_class<ColorCodingGradient, RefTarget>()
			.def("valueToColor", pure_virtual(&ColorCodingGradient::valueToColor))
		;

		ovito_class<ColorCodingHSVGradient, ColorCodingGradient>(nullptr, "Rainbow")
		;
		ovito_class<ColorCodingGrayscaleGradient, ColorCodingGradient>(nullptr, "Grayscale")
		;
		ovito_class<ColorCodingHotGradient, ColorCodingGradient>(nullptr, "Hot")
		;
		ovito_class<ColorCodingJetGradient, ColorCodingGradient>(nullptr, "Jet")
		;
		ovito_class<ColorCodingBlueWhiteRedGradient, ColorCodingGradient>(nullptr, "BlueWhiteRed")
		;
		ovito_class<ColorCodingImageGradient, ColorCodingGradient>(nullptr, "Image")
			.def("loadImage", &ColorCodingImageGradient::loadImage)
		;
	}

	ovito_class<AmbientOcclusionModifier, AsynchronousParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Performs a quick lighting calculation to shade particles according to the degree of occlusion by other particles. ")
		.add_property("intensity", &AmbientOcclusionModifier::intensity, &AmbientOcclusionModifier::setIntensity,
				"A number controlling the strength of the applied shading effect. "
				"\n\n"
				":Valid range: [0.0, 1.0]\n"
				":Default: 0.7")
		.add_property("sample_count", &AmbientOcclusionModifier::samplingCount, &AmbientOcclusionModifier::setSamplingCount,
				"The number of light exposure samples to compute. More samples give a more even light distribution "
				"but take longer to compute."
				"\n\n"
				":Default: 40\n")
		.add_property("buffer_resolution", &AmbientOcclusionModifier::bufferResolution, &AmbientOcclusionModifier::setBufferResolution,
				"A positive integer controlling the resolution of the internal render buffer, which is used to compute how much "
				"light each particle receives. When the number of particles is large, a larger buffer resolution should be used."
				"\n\n"
				":Valid range: [1, 4]\n"
				":Default: 3\n")
	;

	ovito_class<DeleteParticlesModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier deletes the selected particles. It has no parameters.",
			// Python class name:
			"DeleteSelectedParticlesModifier")
	;

	ovito_class<ShowPeriodicImagesModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier replicates all particles to display periodic images of the system.")
		.add_property("replicate_x", &ShowPeriodicImagesModifier::showImageX, &ShowPeriodicImagesModifier::setShowImageX,
				"Enables replication of particles along *x*."
				"\n\n"
				":Default: ``False``\n")
		.add_property("replicate_y", &ShowPeriodicImagesModifier::showImageY, &ShowPeriodicImagesModifier::setShowImageY,
				"Enables replication of particles along *y*."
				"\n\n"
				":Default: ``False``\n")
		.add_property("replicate_z", &ShowPeriodicImagesModifier::showImageZ, &ShowPeriodicImagesModifier::setShowImageZ,
				"Enables replication of particles along *z*."
				"\n\n"
				":Default: ``False``\n")
		.add_property("num_x", &ShowPeriodicImagesModifier::numImagesX, &ShowPeriodicImagesModifier::setNumImagesX,
				"A positive integer specifying the number of copies to generate in the *x* direction (including the existing primary image)."
				"\n\n"
				":Default: 3\n")
		.add_property("num_y", &ShowPeriodicImagesModifier::numImagesY, &ShowPeriodicImagesModifier::setNumImagesY,
				"A positive integer specifying the number of copies to generate in the *y* direction (including the existing primary image)."
				"\n\n"
				":Default: 3\n")
		.add_property("num_z", &ShowPeriodicImagesModifier::numImagesZ, &ShowPeriodicImagesModifier::setNumImagesZ,
				"A positive integer specifying the number of copies to generate in the *z* direction (including the existing primary image)."
				"\n\n"
				":Default: 3\n")
		.add_property("adjust_box", &ShowPeriodicImagesModifier::adjustBoxSize, &ShowPeriodicImagesModifier::setAdjustBoxSize,
				"A boolean flag controlling the modification of the simulation cell geometry. "
				"If ``True``, the simulation cell is extended to fit the multiplied system. "
				"If ``False``, the original simulation cell (containing only the primary image of the system) is kept. "
				"\n\n"
				":Default: ``False``\n")
		.add_property("unique_ids", &ShowPeriodicImagesModifier::uniqueIdentifiers, &ShowPeriodicImagesModifier::setUniqueIdentifiers,
				"If ``True``, the modifier automatically generates a new unique ID for each copy of a particle. "
				"This option has no effect if the input system does not contain particle IDs. "
				"\n\n"
				":Default: ``True``\n")
	;

	ovito_class<WrapPeriodicImagesModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier maps particles located outside the simulation cell back into the box by \"wrapping\" their coordinates "
			"around at the periodic boundaries of the simulation cell. This modifier has no parameters.")
	;

	ovito_class<ComputeBondLengthsModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Computes the length of every bond in the system and outputs the values as "
			"a new bond property named ``Length``. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Length`` (:py:class:`~ovito.data.BondProperty`):\n"
			"   The output bond property containing the length of each bond.\n"
			"\n")
	;

	ovito_class<ComputePropertyModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Evaluates a user-defined math expression to compute the values of a particle property."
			"\n\n"
			"Example::"
			"\n\n"
			"    from ovito.modifiers import *\n"
			"    \n"
			"    modifier = ComputePropertyModifier()\n"
			"    modifier.output_property = \"Color\"\n"
			"    modifier.expressions = [\"Position.X / CellSize.X\", \"0.0\", \"0.5\"]\n"
			"\n")
		.add_property("expressions", make_function(&ComputePropertyModifier::expressions, return_value_policy<copy_const_reference>()), &ComputePropertyModifier::setExpressions,
				"A list of strings containing the math expressions to compute, one for each vector component of the output property. "
				"If the output property is a scalar property, the list should comprise exactly one string. "
				"\n\n"
				":Default: ``[\"0\"]``\n")
		.add_property("neighbor_expressions", make_function(&ComputePropertyModifier::neighborExpressions, return_value_policy<copy_const_reference>()), &ComputePropertyModifier::setNeighborExpressions,
				"A list of strings containing the math expressions for the per-neighbor terms, one for each vector component of the output property. "
				"If the output property is a scalar property, the list should comprise exactly one string. "
				"\n\n"
				"The neighbor expressions are only evaluated if :py:attr:`.neighbor_mode` is enabled."
				"\n\n"
				":Default: ``[\"0\"]``\n")
		.add_property("output_property", make_function(&ComputePropertyModifier::outputProperty, return_value_policy<copy_const_reference>()), &ComputePropertyModifier::setOutputProperty,
				"The output particle property in which the modifier should store the computed values. "
				"\n\n"
				":Default: ``\"Custom property\"``\n")
		.add_property("propertyComponentCount", &ComputePropertyModifier::propertyComponentCount, &ComputePropertyModifier::setPropertyComponentCount)
		.add_property("only_selected", &ComputePropertyModifier::onlySelectedParticles, &ComputePropertyModifier::setOnlySelectedParticles,
				"If ``True``, the property is only computed for selected particles and existing property values "
				"are preserved for unselected particles."
				"\n\n"
				":Default: ``False``\n")
		.add_property("neighbor_mode", &ComputePropertyModifier::neighborModeEnabled, &ComputePropertyModifier::setNeighborModeEnabled,
				"Boolean flag that enabled the neighbor computation mode, where contributions from neighbor particles within the "
				"cutoff radius are taken into account. "
				"\n\n"
				":Default: ``False``\n")
		.add_property("cutoff_radius", &ComputePropertyModifier::cutoff, &ComputePropertyModifier::setCutoff,
				"The cutoff radius up to which neighboring particles are visited. This parameter is only used if :py:attr:`.neighbor_mode` is enabled. "
				"\n\n"
				":Default: 3.0\n")
	;

	ovito_class<FreezePropertyModifier, ParticleModifier>()
		.add_property("source_property", make_function(&FreezePropertyModifier::sourceProperty, return_value_policy<copy_const_reference>()), &FreezePropertyModifier::setSourceProperty)
		.add_property("destination_property", make_function(&FreezePropertyModifier::destinationProperty, return_value_policy<copy_const_reference>()), &FreezePropertyModifier::setDestinationProperty)
		.def("take_snapshot", &FreezePropertyModifier::takePropertySnapshot)
	;

	ovito_class<ClearSelectionModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier clears the particle selection by deleting the ``\"Selection\"`` particle property. "
			"The modifier has no input parameters.")
	;

	ovito_class<InvertSelectionModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier inverts the particle selection. It has no input parameters.")
	;

	ovito_class<ManualSelectionModifier, ParticleModifier>()
		.def("resetSelection", &ManualSelectionModifier::resetSelection)
		.def("selectAll", &ManualSelectionModifier::selectAll)
		.def("clearSelection", &ManualSelectionModifier::clearSelection)
		.def("toggleParticleSelection", &ManualSelectionModifier::toggleParticleSelection)
	;

	{
		scope s = ovito_class<ExpandSelectionModifier, ParticleModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"Expands the current particle selection by selecting particles that are neighbors of already selected particles.")
			.add_property("mode", &ExpandSelectionModifier::mode, &ExpandSelectionModifier::setMode,
					"Selects the mode of operation, i.e., how the modifier extends the selection around already selected particles. "
					"Valid values are:"
					"\n\n"
					"  * ``ExpandSelectionModifier.ExpansionMode.Cutoff``\n"
					"  * ``ExpandSelectionModifier.ExpansionMode.Nearest``\n"
					"  * ``ExpandSelectionModifier.ExpansionMode.Bonded``\n"
					"\n\n"
					":Default: ``ExpandSelectionModifier.ExpansionMode.Cutoff``\n")
			.add_property("cutoff", &ExpandSelectionModifier::cutoffRange, &ExpandSelectionModifier::setCutoffRange,
					"The maximum distance up to which particles are selected around already selected particles. "
					"This parameter is only used if :py:attr:`.mode` is set to ``ExpansionMode.Cutoff``."
					"\n\n"
					":Default: 3.2\n")
			.add_property("num_neighbors", &ExpandSelectionModifier::numNearestNeighbors, &ExpandSelectionModifier::setNumNearestNeighbors,
					"The number of nearest neighbors to select around each already selected particle. "
					"This parameter is only used if :py:attr:`.mode` is set to ``ExpansionMode.Nearest``."
					"\n\n"
					":Default: 1\n")
			.add_property("iterations", &ExpandSelectionModifier::numberOfIterations, &ExpandSelectionModifier::setNumberOfIterations,
					"Controls how many iterations of the modifier are executed. This can be used to select "
					"neighbors of neighbors up to a certain recursive depth."
					"\n\n"
					":Default: 1\n")
		;

		enum_<ExpandSelectionModifier::ExpansionMode>("ExpansionMode")
			.value("Cutoff", ExpandSelectionModifier::CutoffRange)
			.value("Nearest", ExpandSelectionModifier::NearestNeighbors)
			.value("Bonded", ExpandSelectionModifier::BondedNeighbors)
		;
	}

	ovito_class<SelectExpressionModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier selects particles based on a user-defined Boolean expression. "
			"Those particles will be selected for which the expression yields a non-zero value. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Selection`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property is set to 1 for selected particles and 0 for others.\n"
			" * ``SelectExpression.num_selected`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles selected by the modifier.\n"
			"\n\n"
			"**Example:**"
			"\n\n"
			".. literalinclude:: ../example_snippets/select_expression_modifier.py\n"
			"   :lines: 6-\n"
			"\n")
		.add_property("expression", make_function(&SelectExpressionModifier::expression, return_value_policy<copy_const_reference>()), &SelectExpressionModifier::setExpression,
				"A string containing the Boolean expression to be evaluated for every particle. "
				"The expression syntax is documented in `OVITO's user manual <../../particles.modifiers.expression_select.html>`_.")
	;

	ovito_class<SelectParticleTypeModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Selects all particles of a certain type (or multiple types). "
			"\n\n"
			"Note that OVITO knows several classes of particle types, e.g. chemical types and "
			"structural types. Each of which are encoded as integer values by a different particle property. "
			"The :py:attr:`.property` field of this modifier selects the class of types considered "
			"by the modifier, and the :py:attr:`.types` field determines which of the defined types get selected. "
			"\n\n"
			"Example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/select_particle_type_modifier.py\n"
			"   :lines: 8-\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Selection`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This particle property is set to 1 for selected particles and 0 for others.\n"
			" * ``SelectParticleType.num_selected`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles selected by the modifier.\n"
			"\n")
		.add_property("property", make_function(&SelectParticleTypeModifier::sourceProperty, return_value_policy<copy_const_reference>()), &SelectParticleTypeModifier::setSourceProperty,
				"The name of the particle property storing the input particle types. "
				"This can be a :ref:`standard particle property <particle-types-list>` such as ``\"Particle Type\"`` or ``\"Structure Type\"``, or "
				"a custom integer particle property."
				"\n\n"
				":Default: ``\"Particle Type\"``\n")
		.add_property("types", make_function(&SelectParticleTypeModifier::selectedParticleTypes, return_value_policy<copy_const_reference>()), &SelectParticleTypeModifier::setSelectedParticleTypes,
				"A Python ``set`` of integers, which specifies the particle types to select. "
				"\n\n"
				":Default: ``set([])``\n")
	;

	ovito_class<SliceModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Deletes or selects particles in a region bounded by one or two parallel infinite planes in three-dimensional space.")
		.add_property("distance", &SliceModifier::distance, &SliceModifier::setDistance,
				"The distance of the slicing plane from the origin (along its normal vector)."
				"\n\n"
				":Default: 0.0\n")
		.add_property("distanceController", make_function(&SliceModifier::distanceController, return_value_policy<ovito_object_reference>()), &SliceModifier::setDistanceController)
		.add_property("normal", &SliceModifier::normal, &SliceModifier::setNormal,
				"The normal vector of the slicing plane. Does not have to be a unit vector."
				"\n\n"
				":Default: ``(1,0,0)``\n")
		.add_property("normalController", make_function(&SliceModifier::normalController, return_value_policy<ovito_object_reference>()), &SliceModifier::setNormalController)
		.add_property("slice_width", &SliceModifier::sliceWidth, &SliceModifier::setSliceWidth,
				"The width of the slab to cut. If zero, the modifier cuts all particles on one "
				"side of the slicing plane."
				"\n\n"
				":Default: 0.0\n")
		.add_property("sliceWidthController", make_function(&SliceModifier::sliceWidthController, return_value_policy<ovito_object_reference>()), &SliceModifier::setSliceWidthController)
		.add_property("inverse", &SliceModifier::inverse, &SliceModifier::setInverse,
				"Reverses the sense of the slicing plane."
				"\n\n"
				":Default: ``False``\n")
		.add_property("select", &SliceModifier::createSelection, &SliceModifier::setCreateSelection,
				"If ``True``, the modifier selects particles instead of deleting them."
				"\n\n"
				":Default: ``False``\n")
		.add_property("only_selected", &SliceModifier::applyToSelection, &SliceModifier::setApplyToSelection,
				"If ``True``, the modifier acts only on selected particles; if ``False``, the modifier acts on all particles."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<AffineTransformationModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Applies an affine transformation to particles and/or the simulation cell."
			"\n\n"
			"Example::"
			"\n\n"
			"    from ovito.modifiers import *\n"
			"    \n"
			"    xy_shear = 0.05\n"
			"    mod = AffineTransformationModifier(\n"
			"              transform_particles = True,\n"
			"              transform_box = True,\n"
			"              transformation = [[1,xy_shear,0,0],\n"
			"                                [0,       1,0,0],\n"
			"                                [0,       0,1,0]])\n"
			"\n")
		.add_property("transformation", make_function(&AffineTransformationModifier::transformation, return_value_policy<copy_const_reference>()), &AffineTransformationModifier::setTransformation,
				"The 3x4 transformation matrix being applied to particle positions and/or the simulation cell. "
				"The first three matrix columns define the linear part of the transformation, while the fourth "
				"column specifies the translation vector. "
				"\n\n"
				"This matrix describes a relative transformation and is used only if :py:attr:`.relative_mode` == ``True``."
				"\n\n"
				":Default: ``[[ 1.  0.  0.  0.] [ 0.  1.  0.  0.] [ 0.  0.  1.  0.]]``\n")
		.add_property("target_cell", make_function(&AffineTransformationModifier::targetCell, return_value_policy<copy_const_reference>()), &AffineTransformationModifier::setTargetCell,
				"This 3x4 matrix specifies the target cell shape. It is used when :py:attr:`.relative_mode` == ``False``. "
				"\n\n"
				"The first three columns of the matrix specify the three edge vectors of the target cell. "
				"The fourth column defines the origin vector of the target cell.")
		.add_property("relative_mode", &AffineTransformationModifier::relativeMode, &AffineTransformationModifier::setRelativeMode,
				"Selects the operation mode of the modifier."
				"\n\n"
				"If ``relative_mode==True``, the modifier transforms the particles and/or the simulation cell "
				"by applying the matrix given by the :py:attr:`.transformation` parameter."
				"\n\n"
				"If ``relative_mode==False``, the modifier transforms the particles and/or the simulation cell "
				"such that the old simulation cell will have the shape given by the the :py:attr:`.target_cell` parameter after the transformation."
				"\n\n"
				":Default: ``True``\n")
		.add_property("transform_particles", &AffineTransformationModifier::applyToParticles, &AffineTransformationModifier::setApplyToParticles,
				"If ``True``, the modifier transforms the particle positions."
				"\n\n"
				":Default: ``True``\n")
		.add_property("only_selected", &AffineTransformationModifier::selectionOnly, &AffineTransformationModifier::setSelectionOnly,
				"If ``True``, the modifier acts only on selected particles; if ``False``, the modifier acts on all particles."
				"\n\n"
				":Default: ``False``\n")
		.add_property("transform_box", &AffineTransformationModifier::applyToSimulationBox, &AffineTransformationModifier::setApplyToSimulationBox,
				"If ``True``, the modifier transforms the simulation cell."
				"\n\n"
				":Default: ``False``\n")
		.add_property("transform_surface", &AffineTransformationModifier::applyToSurfaceMesh, &AffineTransformationModifier::setApplyToSurfaceMesh,
				"If ``True``, the modifier transforms the surface mesh (if any) that has previously been generated by a :py:class:`ConstructSurfaceModifier`."
				"\n\n"
				":Default: ``True``\n")
	;

	{
		scope s = ovito_class<BinAndReduceModifier, ParticleModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"This modifier applies a reduction operation to a property of the particles within a spatial bin. "
				"The output of the modifier is a one or two-dimensional grid of bin values. ")
			.add_property("property", make_function(&BinAndReduceModifier::sourceProperty, return_value_policy<copy_const_reference>()), &BinAndReduceModifier::setSourceProperty,
					"The name of the input particle property to which the reduction operation should be applied. "
					"This can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. "
					"For vector properties the selected component must be appended to the name, e.g. ``\"Velocity.X\"``. ")
			.add_property("reduction_operation", &BinAndReduceModifier::reductionOperation, &BinAndReduceModifier::setReductionOperation,
					"Selects the reduction operation to be carried out. Possible values are:"
					"\n\n"
					"   * ``BinAndReduceModifier.Operation.Mean``\n"
					"   * ``BinAndReduceModifier.Operation.Sum``\n"
					"   * ``BinAndReduceModifier.Operation.SumVol``\n"
					"   * ``BinAndReduceModifier.Operation.Min``\n"
					"   * ``BinAndReduceModifier.Operation.Max``\n"
					"\n"
					"The operation ``SumVol`` first computes the sum and then divides the result by the volume of the respective bin. "
					"It is intended to compute pressure (or stress) within each bin from the per-atom virial."
					"\n\n"
					":Default: ``BinAndReduceModifier.Operation.Mean``\n")
			.add_property("first_derivative", &BinAndReduceModifier::firstDerivative, &BinAndReduceModifier::setFirstDerivative,
					"If true, the modifier numerically computes the first derivative of the binned data using a finite differences approximation. "
					"This works only for one-dimensional bin grids. "
					"\n\n"
					":Default: ``False``\n")
			.add_property("direction", &BinAndReduceModifier::binDirection, &BinAndReduceModifier::setBinDirection,
					"Selects the alignment of the bins. Possible values:"
					"\n\n"
					"   * ``BinAndReduceModifier.Direction.Vector_1``\n"
					"   * ``BinAndReduceModifier.Direction.Vector_2``\n"
					"   * ``BinAndReduceModifier.Direction.Vector_3``\n"
					"   * ``BinAndReduceModifier.Direction.Vectors_1_2``\n"
					"   * ``BinAndReduceModifier.Direction.Vectors_1_3``\n"
					"   * ``BinAndReduceModifier.Direction.Vectors_2_3``\n"
					"\n"
					"In the first three cases the modifier generates a one-dimensional grid with bins aligned perpendicular to the selected simulation cell vector. "
					"In the last three cases the modifier generates a two-dimensional grid with bins aligned perpendicular to both selected simulation cell vectors (i.e. parallel to the third vector). "
					"\n\n"
					":Default: ``BinAndReduceModifier.Direction.Vector_3``\n")
			.add_property("bin_count_x", &BinAndReduceModifier::numberOfBinsX, &BinAndReduceModifier::setNumberOfBinsX,
					"This attribute sets the number of bins to generate along the first binning axis."
					"\n\n"
					":Default: 200\n")
			.add_property("bin_count_y", &BinAndReduceModifier::numberOfBinsY, &BinAndReduceModifier::setNumberOfBinsY,
					"This attribute sets the number of bins to generate along the second binning axis (only used when working with a two-dimensional grid)."
					"\n\n"
					":Default: 200\n")
			.add_property("only_selected", &BinAndReduceModifier::onlySelected, &BinAndReduceModifier::setOnlySelected,
					"If ``True``, the computation takes into account only the currently selected particles. "
					"You can use this to restrict the calculation to a subset of particles. "
					"\n\n"
					":Default: ``False``\n")
			.add_property("_binData", make_function(&BinAndReduceModifier::binData, return_internal_reference<>()))
			.add_property("_is1D", &BinAndReduceModifier::is1D)
			.add_property("axis_range_x", lambda_address([](BinAndReduceModifier& modifier) {
					return make_tuple(modifier.xAxisRangeStart(), modifier.xAxisRangeEnd());
				}),
				"A 2-tuple containing the range of the generated bin grid along the first binning axis. "
				"Note that this is an output attribute which is only valid after the modifier has performed the bin and reduce operation. "
				"That means you have to call :py:meth:`ovito.ObjectNode.compute` first to evaluate the data pipeline.")
			.add_property("axis_range_y", lambda_address([](BinAndReduceModifier& modifier) {
					return make_tuple(modifier.yAxisRangeStart(), modifier.yAxisRangeEnd());
				}),
				"A 2-tuple containing the range of the generated bin grid along the second binning axis. "
				"Note that this is an output attribute which is only valid after the modifier has performed the bin and reduce operation. "
				"That means you have to call :py:meth:`ovito.ObjectNode.compute` first to evaluate the data pipeline.")
		;

		enum_<BinAndReduceModifier::ReductionOperationType>("Operation")
			.value("Mean", BinAndReduceModifier::RED_MEAN)
			.value("Sum", BinAndReduceModifier::RED_SUM)
			.value("SumVol", BinAndReduceModifier::RED_SUM_VOL)
			.value("Min", BinAndReduceModifier::RED_MIN)
			.value("Max", BinAndReduceModifier::RED_MAX)
		;

		enum_<BinAndReduceModifier::BinDirectionType>("Direction")
			.value("Vector_1", BinAndReduceModifier::CELL_VECTOR_1)
			.value("Vector_2", BinAndReduceModifier::CELL_VECTOR_2)
			.value("Vector_3", BinAndReduceModifier::CELL_VECTOR_3)
			.value("Vectors_1_2", BinAndReduceModifier::CELL_VECTORS_1_2)
			.value("Vectors_1_3", BinAndReduceModifier::CELL_VECTORS_1_3)
			.value("Vectors_2_3", BinAndReduceModifier::CELL_VECTORS_2_3)
		;
	}

	ovito_abstract_class<StructureIdentificationModifier, AsynchronousParticleModifier>()
	;

	{
		scope s = ovito_class<BondAngleAnalysisModifier, StructureIdentificationModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"Performs the bond-angle analysis described by Ackland & Jones to classify the local "
				"crystal structure around each particle. "
				"\n\n"
				"The modifier stores the results as integer values in the ``\"Structure Type\"`` particle property. "
				"The following structure type constants are defined: "
				"\n\n"
				"   * ``BondAngleAnalysisModifier.Type.OTHER`` (0)\n"
				"   * ``BondAngleAnalysisModifier.Type.FCC`` (1)\n"
				"   * ``BondAngleAnalysisModifier.Type.HCP`` (2)\n"
				"   * ``BondAngleAnalysisModifier.Type.BCC`` (3)\n"
				"   * ``BondAngleAnalysisModifier.Type.ICO`` (4)\n"
				"\n\n"
				"**Modifier outputs:**"
				"\n\n"
				" * ``BondAngleAnalysis.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of particles not matching any of the known structure types.\n"
				" * ``BondAngleAnalysis.counts.FCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of FCC particles found.\n"
				" * ``BondAngleAnalysis.counts.HCP`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of HCP particles found.\n"
				" * ``BondAngleAnalysis.counts.BCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of BCC particles found.\n"
				" * ``BondAngleAnalysis.counts.ICO`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of icosahedral found.\n"
				" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This particle property will contain the per-particle structure type assigned by the modifier.\n"
				" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   The modifier assigns a color to each particle based on its identified structure type. "
				"   You can change the color representing a structural type as follows::"
				"\n\n"
				"      modifier = BondAngleAnalysisModifier()\n"
				"      # Give all FCC atoms a blue color:\n"
				"      modifier.structures[BondAngleAnalysisModifier.Type.FCC].color = (0.0, 0.0, 1.0)\n"
				"\n")
			.add_property("structures", make_function(&BondAngleAnalysisModifier::structureTypes, return_internal_reference<>()),
					"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
					"You can adjust the color of structural types as shown in the code example above.")
			.add_property("counts", make_function(&BondAngleAnalysisModifier::structureCounts, return_value_policy<copy_const_reference>()))
		;

		enum_<BondAngleAnalysisModifier::StructureType>("Type")
			.value("OTHER", BondAngleAnalysisModifier::OTHER)
			.value("FCC", BondAngleAnalysisModifier::FCC)
			.value("HCP", BondAngleAnalysisModifier::HCP)
			.value("BCC", BondAngleAnalysisModifier::BCC)
			.value("ICO", BondAngleAnalysisModifier::ICO)
		;
	}

	{
		scope s = ovito_class<CommonNeighborAnalysisModifier, StructureIdentificationModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"Performs the common neighbor analysis (CNA) to classify the structure of the local neighborhood "
				"of each particle. "
				"\n\n"
				"The modifier stores its results as integer values in the ``\"Structure Type\"`` particle property. "
				"The following constants are defined: "
				"\n\n"
				"   * ``CommonNeighborAnalysisModifier.Type.OTHER`` (0)\n"
				"   * ``CommonNeighborAnalysisModifier.Type.FCC`` (1)\n"
				"   * ``CommonNeighborAnalysisModifier.Type.HCP`` (2)\n"
				"   * ``CommonNeighborAnalysisModifier.Type.BCC`` (3)\n"
				"   * ``CommonNeighborAnalysisModifier.Type.ICO`` (4)\n"
				"\n\n"
				"**Modifier outputs:**"
				"\n\n"
				" * ``CommonNeighborAnalysis.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of particles not matching any of the known structure types.\n"
				" * ``CommonNeighborAnalysis.counts.FCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of FCC particles found.\n"
				" * ``CommonNeighborAnalysis.counts.HCP`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of HCP particles found.\n"
				" * ``CommonNeighborAnalysis.counts.BCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of BCC particles found.\n"
				" * ``CommonNeighborAnalysis.counts.ICO`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of icosahedral particles found.\n"
				" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This output particle property contains the per-particle structure types assigned by the modifier.\n"
				" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   The modifier assigns a color to each particle based on its identified structure type. "
				"   You can change the color representing a structural type as follows::"
				"\n\n"
				"      modifier = CommonNeighborAnalysisModifier()\n"
				"      # Give all FCC atoms a blue color:\n"
				"      modifier.structures[CommonNeighborAnalysisModifier.Type.FCC].color = (0.0, 0.0, 1.0)\n"
				"\n")
			.add_property("structures", make_function(&CommonNeighborAnalysisModifier::structureTypes, return_internal_reference<>()),
					"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
					"You can adjust the color of structural types here as shown in the code example above.")
			.add_property("counts", make_function(&CommonNeighborAnalysisModifier::structureCounts, return_value_policy<copy_const_reference>()))
			.add_property("cutoff", &CommonNeighborAnalysisModifier::cutoff, &CommonNeighborAnalysisModifier::setCutoff,
					"The cutoff radius used for the conventional common neighbor analysis. "
					"This parameter is only used if :py:attr:`.mode` == ``CommonNeighborAnalysisModifier.Mode.FixedCutoff``."
					"\n\n"
					":Default: 3.2\n")
			.add_property("mode", &CommonNeighborAnalysisModifier::mode, &CommonNeighborAnalysisModifier::setMode,
					"Selects the mode of operation. "
					"Valid values are:"
					"\n\n"
					"  * ``CommonNeighborAnalysisModifier.Mode.FixedCutoff``\n"
					"  * ``CommonNeighborAnalysisModifier.Mode.AdaptiveCutoff``\n"
					"  * ``CommonNeighborAnalysisModifier.Mode.BondBased``\n"
					"\n\n"
					":Default: ``CommonNeighborAnalysisModifier.Mode.AdaptiveCutoff``\n")
			.add_property("only_selected", &CommonNeighborAnalysisModifier::onlySelectedParticles, &CommonNeighborAnalysisModifier::setOnlySelectedParticles,
					"Lets the modifier perform the analysis only for selected particles. Particles that are not selected will be treated as if they did not exist."
					"\n\n"
					":Default: ``False``\n")

			// For backward compatibility with OVITO 2.5.1.
			.add_property("adaptive_mode",
					lambda_address([](CommonNeighborAnalysisModifier& mod) -> bool { return (mod.mode() == CommonNeighborAnalysisModifier::AdaptiveCutoffMode); }),
					lambda_address([](CommonNeighborAnalysisModifier& mod, bool adaptive) { mod.setMode(adaptive ? CommonNeighborAnalysisModifier::AdaptiveCutoffMode : CommonNeighborAnalysisModifier::FixedCutoffMode); })
					)
		;

		enum_<CommonNeighborAnalysisModifier::CNAMode>("Mode")
			.value("FixedCutoff", CommonNeighborAnalysisModifier::FixedCutoffMode)
			.value("AdaptiveCutoff", CommonNeighborAnalysisModifier::AdaptiveCutoffMode)
			.value("BondBased", CommonNeighborAnalysisModifier::BondMode)
		;

		enum_<CommonNeighborAnalysisModifier::StructureType>("Type")
			.value("OTHER", CommonNeighborAnalysisModifier::OTHER)
			.value("FCC", CommonNeighborAnalysisModifier::FCC)
			.value("HCP", CommonNeighborAnalysisModifier::HCP)
			.value("BCC", CommonNeighborAnalysisModifier::BCC)
			.value("ICO", CommonNeighborAnalysisModifier::ICO)
		;
	}

	{
		scope s = ovito_class<IdentifyDiamondModifier, StructureIdentificationModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"This analysis modifier finds atoms that are arranged in a cubic or hexagonal diamond lattice."
				"\n\n"
				"The modifier stores its results as integer values in the ``\"Structure Type\"`` particle property. "
				"The following structure type constants are defined: "
				"\n\n"
				"   * ``IdentifyDiamondModifier.Type.OTHER`` (0)\n"
				"   * ``IdentifyDiamondModifier.Type.CUBIC_DIAMOND`` (1)\n"
				"   * ``IdentifyDiamondModifier.Type.CUBIC_DIAMOND_FIRST_NEIGHBOR`` (2)\n"
				"   * ``IdentifyDiamondModifier.Type.CUBIC_DIAMOND_SECOND_NEIGHBOR`` (3)\n"
				"   * ``IdentifyDiamondModifier.Type.HEX_DIAMOND`` (4)\n"
				"   * ``IdentifyDiamondModifier.Type.HEX_DIAMOND_FIRST_NEIGHBOR`` (5)\n"
				"   * ``IdentifyDiamondModifier.Type.HEX_DIAMOND_SECOND_NEIGHBOR`` (6)\n"
				"\n\n"
				"**Modifier outputs:**"
				"\n\n"
				" * ``IdentifyDiamond.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of atoms not matching any of the known structure types.\n"
				" * ``IdentifyDiamond.counts.CUBIC_DIAMOND`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of cubic diamond atoms found.\n"
				" * ``IdentifyDiamond.counts.CUBIC_DIAMOND_FIRST_NEIGHBOR`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of atoms found that are first neighbors of a cubic diamond atom.\n"
				" * ``IdentifyDiamond.counts.CUBIC_DIAMOND_SECOND_NEIGHBOR`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of atoms found that are second neighbors of a cubic diamond atom.\n"
				" * ``IdentifyDiamond.counts.HEX_DIAMOND`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of hexagonal diamond atoms found.\n"
				" * ``IdentifyDiamond.counts.HEX_DIAMOND_FIRST_NEIGHBOR`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of atoms found that are first neighbors of a hexagonal diamond atom.\n"
				" * ``IdentifyDiamond.counts.HEX_DIAMOND_SECOND_NEIGHBOR`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of atoms found that are second neighbors of a hexagonal diamond atom.\n"
				" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This particle property will contain the per-particle structure type assigned by the modifier.\n"
				" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   The modifier assigns a color to each atom based on its identified structure type. "
				"   You can change the color representing a structural type as follows::"
				"\n\n"
				"      modifier = BondAngleAnalysisModifier()\n"
				"      # Give all hexagonal diamond atoms a blue color:\n"
				"      modifier.structures[IdentifyDiamondModifier.Type.HEX_DIAMOND].color = (0.0, 0.0, 1.0)\n"
				"\n")
			.add_property("structures", make_function(&IdentifyDiamondModifier::structureTypes, return_internal_reference<>()),
					"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
					"This lets you adjust the colors assigned to structural types.")
			.add_property("only_selected", &IdentifyDiamondModifier::onlySelectedParticles, &IdentifyDiamondModifier::setOnlySelectedParticles,
					"Lets the modifier perform the analysis only for selected particles. Particles that are not selected will be treated as if they did not exist."
					"\n\n"
					":Default: ``False``\n")
			.add_property("counts", make_function(&IdentifyDiamondModifier::structureCounts, return_value_policy<copy_const_reference>()))
		;

		enum_<IdentifyDiamondModifier::StructureType>("Type")
			.value("OTHER", IdentifyDiamondModifier::OTHER)
			.value("CUBIC_DIAMOND", IdentifyDiamondModifier::CUBIC_DIAMOND)
			.value("CUBIC_DIAMOND_FIRST_NEIGHBOR", IdentifyDiamondModifier::CUBIC_DIAMOND_FIRST_NEIGH)
			.value("CUBIC_DIAMOND_SECOND_NEIGHBOR", IdentifyDiamondModifier::CUBIC_DIAMOND_SECOND_NEIGH)
			.value("HEX_DIAMOND", IdentifyDiamondModifier::HEX_DIAMOND)
			.value("HEX_DIAMOND_FIRST_NEIGHBOR", IdentifyDiamondModifier::HEX_DIAMOND_FIRST_NEIGH)
			.value("HEX_DIAMOND_SECOND_NEIGHBOR", IdentifyDiamondModifier::HEX_DIAMOND_SECOND_NEIGH)
		;
	}

	{
		scope s = ovito_class<CreateBondsModifier, AsynchronousParticleModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"Creates bonds between nearby particles. The modifier outputs its results as a :py:class:`~ovito.data.Bonds` data object, which "
				"can be accessed through the :py:attr:`DataCollection.bonds <ovito.data.DataCollection.bonds>` attribute of the pipeline output data collection."
				"\n\n"
				"**Modifier outputs:**"
				"\n\n"
				" * ``CreateBonds.num_bonds`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of full bonds created by the modifier.\n"
				" * :py:attr:`~ovito.data.Bonds` object (:py:attr:`DataCollection.bonds <ovito.data.DataCollection.bonds>`):\n"
				"   Contains the list of bonds created by the modifier.\n")
			.add_property("mode", &CreateBondsModifier::cutoffMode, &CreateBondsModifier::setCutoffMode,
					"Selects the mode of operation. Valid modes are:"
					"\n\n"
					"  * ``CreateBondsModifier.Mode.Uniform``\n"
					"  * ``CreateBondsModifier.Mode.Pairwise``\n"
					"\n\n"
					"In ``Uniform`` mode one global :py:attr:`.cutoff` is used irrespective of the atom types. "
					"In ``Pairwise`` mode a separate cutoff distance must be specified for all pairs of atom types between which bonds are to be created. "
					"\n\n"
					":Default: ``CreateBondsModifier.Mode.Uniform``\n")
			.add_property("cutoff", &CreateBondsModifier::uniformCutoff, &CreateBondsModifier::setUniformCutoff,
					"The maximum cutoff distance for the creation of bonds between particles. This parameter is only used if :py:attr:`.mode` is ``Uniform``. "
					"\n\n"
					":Default: 3.2\n")
			.add_property("intra_molecule_only", &CreateBondsModifier::onlyIntraMoleculeBonds, &CreateBondsModifier::setOnlyIntraMoleculeBonds,
					"If this option is set to true, the modifier will create bonds only between atoms that belong to the same molecule (i.e. which have the same molecule ID assigned to them)."
					"\n\n"
					":Default: ``False``\n")
			.add_property("bonds_display", make_function(&CreateBondsModifier::bondsDisplay, return_value_policy<ovito_object_reference>()),
					"The :py:class:`~ovito.vis.BondsDisplay` object controlling the visual appearance of the bonds created by this modifier.")
			.add_property("lower_cutoff", &CreateBondsModifier::minimumCutoff, &CreateBondsModifier::setMinimumCutoff,
					"The minimum bond length. No bonds will be created between atoms whose distance is below this threshold."
					"\n\n"
					":Default: 0.0\n")
			.def("set_pairwise_cutoff", &CreateBondsModifier::setPairCutoff,
					"SIGNATURE: (type_a, type_b, cutoff)\n"
					"Sets the pair-wise cutoff distance for a pair of atom types. This information is only used if :py:attr:`.mode` is ``Pairwise``."
					"\n\n"
			        ":param str type_a: The :py:attr:`~ovito.data.ParticleType.name` of the first atom type\n"
			        ":param str type_b: The :py:attr:`~ovito.data.ParticleType.name` of the second atom type (order doesn't matter)\n"
			        ":param float cutoff: The cutoff distance to be set for the type pair.\n"
					"\n\n"
					"If you do not want to create any bonds between a pair of types, set the corresponding cutoff radius to zero (which is the default).")
			.def("get_pairwise_cutoff", &CreateBondsModifier::getPairCutoff,
					"SIGNATURE: (type_a, type_b)\n"
					"Returns the pair-wise cutoff distance set for a pair of atom types."
					"\n\n"
			        ":param str type_a: The :py:attr:`~ovito.data.ParticleType.name` of the first atom type\n"
			        ":param str type_b: The :py:attr:`~ovito.data.ParticleType.name` of the second atom type (order doesn't matter)\n"
			        ":return: The cutoff distance set for the type pair. Returns zero if no cutoff has been set for the pair.\n")
		;

		enum_<CreateBondsModifier::CutoffMode>("Mode")
			.value("Uniform", CreateBondsModifier::UniformCutoff)
			.value("Pairwise", CreateBondsModifier::PairCutoff)
		;
	}

	ovito_class<CentroSymmetryModifier, AsynchronousParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Computes the centro-symmetry parameter (CSP) of each particle."
			"\n\n"
			"The modifier outputs the computed values in the ``\"Centrosymmetry\"`` particle property.")
		.add_property("num_neighbors", &CentroSymmetryModifier::numNeighbors, &CentroSymmetryModifier::setNumNeighbors,
				"The number of neighbors to take into account (12 for FCC crystals, 8 for BCC crystals)."
				"\n\n"
				":Default: 12\n")
	;

	ovito_class<ClusterAnalysisModifier, AsynchronousParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Groups particles into clusters using a distance cutoff criterion. "
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Cluster`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   This output particle property stores the IDs of the clusters the particles have been assigned to.\n"
			" * ``ClusterAnalysis.cluster_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The total number of clusters produced by the modifier. Cluster IDs range from 1 to this number.\n"
			" * ``ClusterAnalysis.largest_size`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles belonging to the largest cluster (cluster ID 1). This attribute is only computed by the modifier when :py:attr:`.sort_by_size` is set.\n"
			"\n"
			"**Example:**"
			"\n\n"
			"The following script demonstrates how to apply the `numpy.bincount() <http://docs.scipy.org/doc/numpy/reference/generated/numpy.bincount.html>`_ "
			"function to the generated ``Cluster`` particle property to determine the size (=number of particles) of each cluster "
			"found by the modifier. "
			"\n\n"
			".. literalinclude:: ../example_snippets/cluster_analysis_modifier.py\n"
			"\n"
			)
		.add_property("cutoff", &ClusterAnalysisModifier::cutoff, &ClusterAnalysisModifier::setCutoff,
				"The cutoff distance used by the algorithm to form clusters of connected particles."
				"\n\n"
				":Default: 3.2\n")
		.add_property("only_selected", &ClusterAnalysisModifier::onlySelectedParticles, &ClusterAnalysisModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only for selected particles. "
				"Particles that are not selected will be assigned cluster ID 0 and treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.add_property("sort_by_size", &ClusterAnalysisModifier::sortBySize, &ClusterAnalysisModifier::setSortBySize,
				"Enables the sorting of clusters by size (in descending order). Cluster 1 will be the largest cluster, cluster 2 the second largest, and so on."
				"\n\n"
				":Default: ``False``\n")
		.add_property("count", &ClusterAnalysisModifier::clusterCount)
	;

	ovito_class<CoordinationNumberModifier, AsynchronousParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Computes coordination numbers of particles and the radial distribution function (RDF) of the system."
			"\n\n"
			"The modifier stores the computed coordination numbers in the ``\"Coordination\"`` particle property."
			"\n\n"
			"Example showing how to export the RDF data to a text file:\n\n"
			".. literalinclude:: ../example_snippets/coordination_analysis_modifier.py")
		.add_property("cutoff", &CoordinationNumberModifier::cutoff, &CoordinationNumberModifier::setCutoff,
				"The neighbor cutoff distance."
				"\n\n"
				":Default: 3.2\n")
		.add_property("number_of_bins", &CoordinationNumberModifier::numberOfBins, &CoordinationNumberModifier::setNumberOfBins,
				"The number of histogram bins to use when computing the RDF."
				"\n\n"
				":Default: 200\n")
		.add_property("rdf_x", make_function(&CoordinationNumberModifier::rdfX, return_internal_reference<>()))
		.add_property("rdf_y", make_function(&CoordinationNumberModifier::rdfY, return_internal_reference<>()))
	;

	ovito_class<CalculateDisplacementsModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Computes the displacement vectors of particles based on a separate reference configuration. "
			"The modifier requires you to load a reference configuration from an external file::"
			"\n\n"
			"    from ovito.modifiers import *\n"
			"    \n"
			"    modifier = CalculateDisplacementsModifier()\n"
			"    modifier.reference.load(\"frame0000.dump\")\n"
			"\n\n"
			"The modifier stores the computed displacement vectors in the ``\"Displacement\"`` particle property. "
			"The displacement magnitudes are stored in the ``\"Displacement Magnitude\"`` property. ")
		.add_property("reference", make_function(&CalculateDisplacementsModifier::referenceConfiguration, return_value_policy<ovito_object_reference>()), &CalculateDisplacementsModifier::setReferenceConfiguration,
				"A :py:class:`~ovito.io.FileSource` that provides the reference positions of particles. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a reference simulation file "
				"as shown in the code example above.")
		.add_property("eliminate_cell_deformation", &CalculateDisplacementsModifier::eliminateCellDeformation, &CalculateDisplacementsModifier::setEliminateCellDeformation,
				"Boolean flag that controls the elimination of the affine cell deformation prior to calculating the "
				"displacement vectors."
				"\n\n"
				":Default: ``False``\n")
		.add_property("assume_unwrapped_coordinates", &CalculateDisplacementsModifier::assumeUnwrappedCoordinates, &CalculateDisplacementsModifier::setAssumeUnwrappedCoordinates,
				"If ``True``, the particle coordinates of the reference and of the current configuration are taken as is. "
				"If ``False``, the minimum image convention is used to deal with particles that have crossed a periodic boundary. "
				"\n\n"
				":Default: ``False``\n")
		.add_property("reference_frame", &CalculateDisplacementsModifier::referenceFrameNumber, &CalculateDisplacementsModifier::setReferenceFrameNumber,
				"The frame number to use as reference configuration if the reference data comprises multiple "
				"simulation frames. Only used if ``use_frame_offset==False``."
				"\n\n"
				":Default: 0\n")
		.add_property("use_frame_offset", &CalculateDisplacementsModifier::useReferenceFrameOffset, &CalculateDisplacementsModifier::setUseReferenceFrameOffset,
				"Determines whether a sliding reference configuration is taken at a constant time offset (specified by :py:attr:`.frame_offset`) "
				"relative to the current frame. If ``False``, a constant reference configuration is used (set by the :py:attr:`.reference_frame` parameter) "
				"irrespective of the current frame."
				"\n\n"
				":Default: ``False``\n")
		.add_property("frame_offset", &CalculateDisplacementsModifier::referenceFrameOffset, &CalculateDisplacementsModifier::setReferenceFrameOffset,
				"The relative frame offset when using a sliding reference configuration (``use_frame_offset==True``)."
				"\n\n"
				":Default: -1\n")
		.add_property("vector_display", make_function(&CalculateDisplacementsModifier::vectorDisplay, return_value_policy<ovito_object_reference>()),
				"A :py:class:`~ovito.vis.VectorDisplay` instance controlling the visual representation of the computed "
				"displacement vectors. \n"
				"Note that the computed displacement vectors are not shown by default. You can enable "
				"the arrow display as follows::"
				"\n\n"
				"   modifier = CalculateDisplacementsModifier()\n"
				"   modifier.vector_display.enabled = True\n"
				"   modifier.vector_display.color = (0,0,0)\n"
				"\n")
	;

	ovito_class<HistogramModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Generates a histogram from the values of a particle property. "
			"\n\n"
			"The value range of the histogram is determined automatically from the minimum and maximum values of the selected property "
			"unless :py:attr:`.fix_xrange` is set to ``True``. In this case the range of the histogram is controlled by the "
			":py:attr:`.xrange_start` and :py:attr:`.xrange_end` parameters."
			"\n\n"
			"Example::"
			"\n\n"
			"    from ovito.modifiers import *\n"
			"    modifier = HistogramModifier(bin_count=100, property=\"Potential Energy\")\n"
			"    node.modifiers.append(modifier)\n"
			"    node.compute()\n"
			"    \n"
			"    import numpy\n"
			"    numpy.savetxt(\"histogram.txt\", modifier.histogram)\n"
			"\n")
		.add_property("property", make_function(&HistogramModifier::sourceProperty, return_value_policy<copy_const_reference>()), &HistogramModifier::setSourceProperty,
				"The name of the input particle property for which to compute the histogram. "
				"This can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. "
				"For vector properties a specific component name must be included in the string, e.g. ``\"Velocity.X\"``. ")
		.add_property("bin_count", &HistogramModifier::numberOfBins, &HistogramModifier::setNumberOfBins,
				"The number of histogram bins."
				"\n\n"
				":Default: 200\n")
		.add_property("fix_xrange", &HistogramModifier::fixXAxisRange, &HistogramModifier::setFixXAxisRange,
				"Controls how the value range of the histogram is determined. If false, the range is chosen automatically by the modifier to include "
				"all particle property values. If true, the range is specified manually using the :py:attr:`.xrange_start` and :py:attr:`.xrange_end` attributes."
				"\n\n"
				":Default: ``False``\n")
		.add_property("xrange_start", &HistogramModifier::xAxisRangeStart, &HistogramModifier::setXAxisRangeStart,
				"If :py:attr:`.fix_xrange` is true, then this specifies the lower end of the value range covered by the histogram."
				"\n\n"
				":Default: 0.0\n")
		.add_property("xrange_end", &HistogramModifier::xAxisRangeEnd, &HistogramModifier::setXAxisRangeEnd,
				"If :py:attr:`.fix_xrange` is true, then this specifies the upper end of the value range covered by the histogram."
				"\n\n"
				":Default: 0.0\n")
		.add_property("only_selected", &HistogramModifier::onlySelected, &HistogramModifier::setOnlySelected,
				"If ``True``, the histogram is computed only on the basis of currently selected particles. "
				"You can use this to restrict histogram calculation to a subset of particles. "
				"\n\n"
				":Default: ``False``\n")
		.add_property("histogramData", make_function(&HistogramModifier::histogramData, return_internal_reference<>()))
	;

	ovito_class<ScatterPlotModifier, ParticleModifier>()
		.add_property("xAxisProperty", make_function(&ScatterPlotModifier::xAxisProperty, return_value_policy<copy_const_reference>()), &ScatterPlotModifier::setXAxisProperty)
		.add_property("yAxisProperty", make_function(&ScatterPlotModifier::yAxisProperty, return_value_policy<copy_const_reference>()), &ScatterPlotModifier::setYAxisProperty)
	;

	ovito_class<AtomicStrainModifier, AsynchronousParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Computes the atomic-level deformation with respect to a reference configuration. "
			"The reference configuration required for the calculation must be explicitly loaded from an external simulation file::"
			"\n\n"
			"    from ovito.modifiers import *\n"
			"    \n"
			"    modifier = AtomicStrainModifier()\n"
			"    modifier.reference.load(\"initial_config.dump\")\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Shear Strain`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The *von Mises* shear strain invariant of the atomic Green-Lagrangian strain tensor.\n"
			" * ``Volumetric Strain`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   One third of the trace of the atomic Green-Lagrangian strain tensor.\n"
			" * ``Strain Tensor`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The six components of the symmetric Green-Lagrangian strain tensor.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_strain_tensors` flag.\n"
			" * ``Deformation Gradient`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The nine components of the atomic deformation gradient tensor.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_deformation_gradients` flag.\n"
			" * ``Stretch Tensor`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The six components of the symmetric right stretch tensor U in the polar decomposition F=RU.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_stretch_tensors` flag.\n"
			" * ``Rotation`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The atomic microrotation obtained from the polar decomposition F=RU as a quaternion.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_rotations` flag.\n"
			" * ``Nonaffine Squared Displacement`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The D\\ :sup:`2`\\ :sub:`min` measure of Falk & Langer, which describes the non-affine part of the local deformation.\n"
			"   Output of this property must be explicitly enabled with the :py:attr:`.output_nonaffine_squared_displacements` flag.\n"
			" * ``Selection`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The modifier can select those particles for which a local deformation could not be computed because there were not\n"
			"   enough neighbors within the :py:attr:`.cutoff` range. Those particles with invalid deformation values can subsequently be removed using the\n"
			"   :py:class:`DeleteSelectedParticlesModifier`, for example. Selection of invalid particles is controlled by the :py:attr:`.select_invalid_particles` flag.\n"
			" * ``AtomicStrain.invalid_particle_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The number of particles for which the local strain calculation failed because they had not enough neighbors within the :py:attr:`.cutoff` range.\n"
			)
		.add_property("reference", make_function(&AtomicStrainModifier::referenceConfiguration, return_value_policy<ovito_object_reference>()), &AtomicStrainModifier::setReferenceConfiguration,
				"A :py:class:`~ovito.io.FileSource` that provides the reference positions of particles. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a reference simulation file "
				"as shown in the code example above.")
		.add_property("eliminate_cell_deformation", &AtomicStrainModifier::eliminateCellDeformation, &AtomicStrainModifier::setEliminateCellDeformation,
				"Boolean flag that controls the elimination of the affine cell deformation prior to calculating the "
				"local strain."
				"\n\n"
				":Default: ``False``\n")
		.add_property("assume_unwrapped_coordinates", &AtomicStrainModifier::assumeUnwrappedCoordinates, &AtomicStrainModifier::setAssumeUnwrappedCoordinates,
				"If ``True``, the particle coordinates of the reference and of the current configuration are taken as is. "
				"If ``False``, the minimum image convention is used to deal with particles that have crossed a periodic boundary. "
				"\n\n"
				":Default: ``False``\n")
		.add_property("use_frame_offset", &AtomicStrainModifier::useReferenceFrameOffset, &AtomicStrainModifier::setUseReferenceFrameOffset,
				"Determines whether a sliding reference configuration is taken at a constant time offset (specified by :py:attr:`.frame_offset`) "
				"relative to the current frame. If ``False``, a constant reference configuration is used (set by the :py:attr:`.reference_frame` parameter) "
				"irrespective of the current frame."
				"\n\n"
				":Default: ``False``\n")
		.add_property("reference_frame", &AtomicStrainModifier::referenceFrameNumber, &AtomicStrainModifier::setReferenceFrameNumber,
				"The frame number to use as reference configuration if the reference data comprises multiple "
				"simulation frames. Only used if ``use_frame_offset==False``."
				"\n\n"
				":Default: 0\n")
		.add_property("frame_offset", &AtomicStrainModifier::referenceFrameOffset, &AtomicStrainModifier::setReferenceFrameOffset,
				"The relative frame offset when using a sliding reference configuration (``use_frame_offset==True``)."
				"\n\n"
				":Default: -1\n")
		.add_property("cutoff", &AtomicStrainModifier::cutoff, &AtomicStrainModifier::setCutoff,
				"Sets the distance up to which neighbor atoms are taken into account in the local strain calculation."
				"\n\n"
				":Default: 3.0\n")
		.add_property("output_deformation_gradients", &AtomicStrainModifier::calculateDeformationGradients, &AtomicStrainModifier::setCalculateDeformationGradients,
				"Controls the output of the per-particle deformation gradient tensors. If ``False``, the computed tensors are not output as a particle property to save memory."
				"\n\n"
				":Default: ``False``\n")
		.add_property("output_strain_tensors", &AtomicStrainModifier::calculateStrainTensors, &AtomicStrainModifier::setCalculateStrainTensors,
				"Controls the output of the per-particle strain tensors. If ``False``, the computed strain tensors are not output as a particle property to save memory."
				"\n\n"
				":Default: ``False``\n")
		.add_property("output_stretch_tensors", &AtomicStrainModifier::calculateStretchTensors, &AtomicStrainModifier::setCalculateStretchTensors,
				"Flag that controls the calculation of the per-particle stretch tensors."
				"\n\n"
				":Default: ``False``\n")
		.add_property("output_rotations", &AtomicStrainModifier::calculateRotations, &AtomicStrainModifier::setCalculateRotations,
				"Flag that controls the calculation of the per-particle rotations."
				"\n\n"
				":Default: ``False``\n")
		.add_property("output_nonaffine_squared_displacements", &AtomicStrainModifier::calculateNonaffineSquaredDisplacements, &AtomicStrainModifier::setCalculateNonaffineSquaredDisplacements,
				"Enables the computation of the squared magnitude of the non-affine part of the atomic displacements. The computed values are output in the ``\"Nonaffine Squared Displacement\"`` particle property."
				"\n\n"
				":Default: ``False``\n")
		.add_property("select_invalid_particles", &AtomicStrainModifier::selectInvalidParticles, &AtomicStrainModifier::setSelectInvalidParticles,
				"If ``True``, the modifier selects the particle for which the local strain tensor could not be computed (because of an insufficient number of neighbors within the cutoff)."
				"\n\n"
				":Default: ``True``\n")
		.add_property("invalid_particle_count", &AtomicStrainModifier::invalidParticleCount)
	;

	ovito_class<WignerSeitzAnalysisModifier, AsynchronousParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Performs the Wigner-Seitz cell analysis to identify point defects in crystals. "
			"The modifier requires loading a reference configuration from an external file::"
			"\n\n"
			"    from ovito.modifiers import *\n"
			"    \n"
			"    mod = WignerSeitzAnalysisModifier()\n"
			"    mod.reference.load(\"frame0000.dump\")\n"
			"    node.modifiers.append(mod)\n"
			"    node.compute()\n"
			"    print(\"Number of vacant sites: %i\" % node.output.attributes['WignerSeitz.vacancy_count'])\n"
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Occupancy`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   The computed site occupation numbers, one for each particle in the reference configuration.\n"
			" * ``WignerSeitz.vacancy_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The total number of vacant sites (having ``Occupancy`` == 0). \n"
			" * ``WignerSeitz.interstitial_count`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   The total number of of interstitial atoms. This is equal to the sum of occupancy numbers of all non-empty sites minus the number of non-empty sites.\n"
			"\n")
		.add_property("reference", make_function(&WignerSeitzAnalysisModifier::referenceConfiguration, return_value_policy<ovito_object_reference>()), &WignerSeitzAnalysisModifier::setReferenceConfiguration,
				"A :py:class:`~ovito.io.FileSource` that provides the reference positions of particles. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a reference simulation file "
				"as shown in the code example above.")
		.add_property("eliminate_cell_deformation", &WignerSeitzAnalysisModifier::eliminateCellDeformation, &WignerSeitzAnalysisModifier::setEliminateCellDeformation,
				"Boolean flag that controls the elimination of the affine cell deformation prior to performing the analysis."
				"\n\n"
				":Default: ``False``\n")
		.add_property("use_frame_offset", &WignerSeitzAnalysisModifier::useReferenceFrameOffset, &WignerSeitzAnalysisModifier::setUseReferenceFrameOffset,
				"Determines whether a sliding reference configuration is taken at a constant time offset (specified by :py:attr:`.frame_offset`) "
				"relative to the current frame. If ``False``, a constant reference configuration is used (set by the :py:attr:`.reference_frame` parameter) "
				"irrespective of the current frame."
				"\n\n"
				":Default: ``False``\n")
		.add_property("reference_frame", &WignerSeitzAnalysisModifier::referenceFrameNumber, &WignerSeitzAnalysisModifier::setReferenceFrameNumber,
				"The frame number to use as reference configuration if the reference data comprises multiple "
				"simulation frames. Only used if ``use_frame_offset==False``."
				"\n\n"
				":Default: 0\n")
		.add_property("frame_offset", &WignerSeitzAnalysisModifier::referenceFrameOffset, &WignerSeitzAnalysisModifier::setReferenceFrameOffset,
				"The relative frame offset when using a sliding reference configuration (``use_frame_offset==True``)."
				"\n\n"
				":Default: -1\n")
		.add_property("per_type_occupancies", &WignerSeitzAnalysisModifier::perTypeOccupancy, &WignerSeitzAnalysisModifier::setPerTypeOccupancy,
				"A parameter flag that controls whether occupancy numbers are determined per particle type. "
				"\n\n"
				"If false, only the total occupancy number is computed for each reference site, which counts the number "
				"of particles that occupy the site irrespective of their types. If true, then the ``Occupancy`` property "
				"computed by the modifier becomes a vector property with one component per particle type. "
				"Each property component counts the number of particles of the corresponding type that occupy a site. For example, "
				"the property component ``Occupancy.1`` contains the number of particles of type 1 that occupy a site. "
				"\n\n"
				":Default: ``False``\n")
		.add_property("vacancy_count", &WignerSeitzAnalysisModifier::vacancyCount)
		.add_property("interstitial_count", &WignerSeitzAnalysisModifier::interstitialCount)
	;

	ovito_class<VoronoiAnalysisModifier, AsynchronousParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Computes the atomic volumes and coordination numbers using a Voronoi tessellation of the particle system."
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * ``Atomic Volume`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the computed Voronoi cell volume of each particle.\n"
			" * ``Coordination`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the number of faces of each particle's Voronoi cell.\n"
			" * ``Voronoi Index`` (:py:class:`~ovito.data.ParticleProperty`):\n"
			"   Stores the Voronoi indices computed from each particle's Voronoi cell. This property is only generated when :py:attr:`.compute_indices` is set.\n"
			" * ``Bonds`` (:py:class:`~ovito.data.Bonds`):\n"
			"   The list of nearest neighbor bonds, one for each Voronoi face. Bonds are only generated when :py:attr:`.generate_bonds` is set.\n"
			" * ``Voronoi.max_face_order`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
			"   This output attribute reports the maximum number of edges of any face in the computed Voronoi tessellation "
			"   (ignoring edges and faces that are below the area and length thresholds)."
			"   Note that, if calculation of Voronoi indices is enabled (:py:attr:`.compute_indices` == true), and :py:attr:`.edge_count` < ``max_face_order``, then "
			"   the computed Voronoi index vectors will be truncated because there exists at least one Voronoi face having more edges than "
			"   the maximum Voronoi vector length specified by :py:attr:`.edge_count`. In such a case you should consider increasing "
			"   :py:attr:`.edge_count` (to at least ``max_face_order``) to not lose information because of truncated index vectors."
			"\n")
		.add_property("only_selected", &VoronoiAnalysisModifier::onlySelected, &VoronoiAnalysisModifier::setOnlySelected,
				"Lets the modifier perform the analysis only for selected particles. Particles that are currently not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.add_property("use_radii", &VoronoiAnalysisModifier::useRadii, &VoronoiAnalysisModifier::setUseRadii,
				"If ``True``, the modifier computes the poly-disperse Voronoi tessellation, which takes into account the radii of particles. "
				"Otherwise a mono-disperse Voronoi tessellation is computed, which is independent of the particle sizes. "
				"\n\n"
				":Default: ``False``\n")
		.add_property("face_threshold", &VoronoiAnalysisModifier::faceThreshold, &VoronoiAnalysisModifier::setFaceThreshold,
				"Specifies a minimum area for faces of a Voronoi cell. The modifier will ignore any Voronoi cell faces with an area smaller than this "
				"threshold when computing the coordination number and the Voronoi index of particles."
				"\n\n"
				":Default: 0.0\n")
		.add_property("edge_threshold", &VoronoiAnalysisModifier::edgeThreshold, &VoronoiAnalysisModifier::setEdgeThreshold,
				"Specifies the minimum length an edge must have to be considered in the Voronoi index calculation. Edges that are shorter "
				"than this threshold will be ignored when counting the number of edges of a Voronoi face."
				"\n\n"
				":Default: 0.0\n")
		.add_property("compute_indices", &VoronoiAnalysisModifier::computeIndices, &VoronoiAnalysisModifier::setComputeIndices,
				"If ``True``, the modifier calculates the Voronoi indices of particles. The modifier stores the computed indices in a vector particle property "
				"named ``Voronoi Index``. The *i*-th component of this property will contain the number of faces of the "
				"Voronoi cell that have *i* edges. Thus, the first two components of the per-particle vector will always be zero, because the minimum "
				"number of edges a polygon can have is three. "
				"\n\n"
				":Default: ``False``\n")
		.add_property("generate_bonds", &VoronoiAnalysisModifier::computeBonds, &VoronoiAnalysisModifier::setComputeBonds,
				"Controls whether the modifier outputs the nearest neighbor bonds. The modifier will generate a bond "
				"for every pair of adjacent atoms that share a face of the Voronoi tessellation. "
				"No bond will be created if the face's area is below the :py:attr:`.face_threshold` or if "
				"the face has less than three edges that are longer than the :py:attr:`.edge_threshold`."
				"\n\n"
				":Default: ``False``\n")
		.add_property("edge_count", &VoronoiAnalysisModifier::edgeCount, &VoronoiAnalysisModifier::setEdgeCount,
				"Integer parameter controlling the order up to which Voronoi indices are computed by the modifier. "
				"Any Voronoi face with more edges than this maximum value will not be counted! Computed Voronoi index vectors are truncated at the index specified by :py:attr:`.edge_count`. "
				"\n\n"
				"See the ``Voronoi.max_face_order`` output attributes described above on how to avoid truncated Voronoi index vectors."
				"\n\n"
				"This parameter is ignored if :py:attr:`.compute_indices` is false."
				"\n\n"
				":Minimum: 3\n"
				":Default: 6\n")
		.add_property("max_face_order", &VoronoiAnalysisModifier::maxFaceOrder)
	;

	ovito_class<LoadTrajectoryModifier, ParticleModifier>(
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier loads trajectories of particles from a separate simulation file. "
			"\n\n"
			"A typical usage scenario for this modifier is when the topology of a molecular system (i.e. the definition of atom types, bonds, etc.) is "
			"stored separately from the trajectories of atoms. In this case you should load the topology file first using :py:func:`~ovito.io.import_file`. "
			"Then create and apply the :py:class:`!LoadTrajectoryModifier` to the topology dataset, which loads the trajectory file. "
			"The modifier will replace the static atom positions from the topology dataset with the time-dependent positions from the trajectory file. "
			"\n\n"
			"Example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/load_trajectory_modifier.py")
		.add_property("source", make_function(&LoadTrajectoryModifier::trajectorySource, return_value_policy<ovito_object_reference>()), &LoadTrajectoryModifier::setTrajectorySource,
				"A :py:class:`~ovito.io.FileSource` that provides the trajectories of particles. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a simulation trajectory file "
				"as shown in the code example above.")
	;

	{
		scope s = ovito_class<PolyhedralTemplateMatchingModifier, StructureIdentificationModifier>(
				":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
				"Uses the Polyhedral Template Matching (PTM) method to classify the local structural neighborhood "
				"of each particle. "
				"\n\n"
				"The modifier stores its results as integer values in the ``\"Structure Type\"`` particle property. "
				"The following constants are defined: "
				"\n\n"
				"   * ``PolyhedralTemplateMatchingModifier.Type.OTHER`` (0)\n"
				"   * ``PolyhedralTemplateMatchingModifier.Type.FCC`` (1)\n"
				"   * ``PolyhedralTemplateMatchingModifier.Type.HCP`` (2)\n"
				"   * ``PolyhedralTemplateMatchingModifier.Type.BCC`` (3)\n"
				"   * ``PolyhedralTemplateMatchingModifier.Type.ICO`` (4)\n"
				"   * ``PolyhedralTemplateMatchingModifier.Type.SC`` (5)\n"
				"\n"
				"**Modifier outputs:**"
				"\n\n"
				" * ``PolyhedralTemplateMatching.counts.OTHER`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of particles not matching any of the known structure types.\n"
				" * ``PolyhedralTemplateMatching.counts.FCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of FCC particles found.\n"
				" * ``PolyhedralTemplateMatching.counts.HCP`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of HCP particles found.\n"
				" * ``PolyhedralTemplateMatching.counts.BCC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of BCC particles found.\n"
				" * ``PolyhedralTemplateMatching.counts.ICO`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of icosahedral particles found.\n"
				" * ``PolyhedralTemplateMatching.counts.SC`` (:py:attr:`attribute <ovito.data.DataCollection.attributes>`):\n"
				"   The number of simple cubic particles found.\n"
				" * ``Structure Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This output particle property will contain the per-particle structure types assigned by the modifier.\n"
				" * ``RMSD`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This particle property will contain the per-particle RMSD values computed by the PTM method.\n"
				"   The modifier will output this property only if the :py:attr:`.output_rmsd` flag is set.\n"
				" * ``Interatomic Distance`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This particle property will contain the local interatomic distances computed by the PTM method.\n"
				"   The modifier will output this property only if the :py:attr:`.output_interatomic_distance` flag is set.\n"
				" * ``Orientation`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This particle property will contain the local lattice orientations computed by the PTM method\n"
				"   encoded as quaternions.\n"
				"   The modifier will generate this property only if the :py:attr:`.output_orientation` flag is set.\n"
				" * ``Elastic Deformation Gradient`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This particle property will contain the local elastic deformation gradient tensors computed by the PTM method.\n"
				"   The modifier will output this property only if the :py:attr:`.output_deformation_gradient` flag is set.\n"
				" * ``Alloy Type`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   This output particle property contains the alloy type assigned to particles by the modifier.\n"
				"   (only if the :py:attr:`.output_alloy_types` flag is set).\n"
				"   The alloy types get stored as integer values in the ``\"Alloy Type\"`` particle property. "
				"   The following alloy type constants are defined: "
				"\n\n"
				"      * ``PolyhedralTemplateMatchingModifier.AlloyType.NONE`` (0)\n"
				"      * ``PolyhedralTemplateMatchingModifier.AlloyType.PURE`` (1)\n"
				"      * ``PolyhedralTemplateMatchingModifier.AlloyType.L10`` (2)\n"
				"      * ``PolyhedralTemplateMatchingModifier.AlloyType.L12_CU`` (3)\n"
				"      * ``PolyhedralTemplateMatchingModifier.AlloyType.L12_AU`` (4)\n"
				"      * ``PolyhedralTemplateMatchingModifier.AlloyType.B2`` (5)\n"
				" * ``Color`` (:py:class:`~ovito.data.ParticleProperty`):\n"
				"   The modifier assigns a color to each particle based on its identified structure type. "
				"   You can change the color representing a structural type as follows::"
				"\n\n"
				"      modifier = PolyhedralTemplateMatchingModifier()\n"
				"      # Give all FCC atoms a blue color:\n"
				"      modifier.structures[PolyhedralTemplateMatchingModifier.Type.FCC].color = (0.0, 0.0, 1.0)\n"
				"\n")
			.add_property("structures", make_function(&PolyhedralTemplateMatchingModifier::structureTypes, return_internal_reference<>()),
					"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
					"You can adjust the color of structural types here as shown in the code example above.")
			.add_property("rmsd_cutoff", &PolyhedralTemplateMatchingModifier::rmsdCutoff, &PolyhedralTemplateMatchingModifier::setRmsdCutoff,
					"The maximum allowed root mean square deviation for positive structure matches. "
					"If the cutoff is non-zero, template matches that yield a RMSD value above the cutoff are classified as \"Other\". "
					"This can be used to filter out spurious template matches (false positives). "
					"\n\n"
					"If this parameter is zero, no cutoff is applied."
					"\n\n"
					":Default: 0.0\n")
			.add_property("only_selected", &PolyhedralTemplateMatchingModifier::onlySelectedParticles, &PolyhedralTemplateMatchingModifier::setOnlySelectedParticles,
					"Lets the modifier perform the analysis only on the basis of currently selected particles. Unselected particles will be treated as if they did not exist."
					"\n\n"
					":Default: ``False``\n")
			.add_property("output_rmsd", &PolyhedralTemplateMatchingModifier::outputRmsd, &PolyhedralTemplateMatchingModifier::setOutputRmsd,
					"Boolean flag that controls whether the modifier outputs the computed per-particle RMSD values to the pipeline."
					"\n\n"
					":Default: ``False``\n")
			.add_property("output_interatomic_distance", &PolyhedralTemplateMatchingModifier::outputInteratomicDistance, &PolyhedralTemplateMatchingModifier::setOutputInteratomicDistance,
					"Boolean flag that controls whether the modifier outputs the computed per-particle interatomic distance to the pipeline."
					"\n\n"
					":Default: ``False``\n")
			.add_property("output_orientation", &PolyhedralTemplateMatchingModifier::outputOrientation, &PolyhedralTemplateMatchingModifier::setOutputOrientation,
					"Boolean flag that controls whether the modifier outputs the computed per-particle lattice orientation to the pipeline."
					"\n\n"
					":Default: ``False``\n")
			.add_property("output_deformation_gradient", &PolyhedralTemplateMatchingModifier::outputDeformationGradient, &PolyhedralTemplateMatchingModifier::setOutputDeformationGradient,
					"Boolean flag that controls whether the modifier outputs the computed per-particle elastic deformation gradients to the pipeline."
					"\n\n"
					":Default: ``False``\n")
			.add_property("output_alloy_types", &PolyhedralTemplateMatchingModifier::outputAlloyTypes, &PolyhedralTemplateMatchingModifier::setOutputAlloyTypes,
					"Boolean flag that controls whether the modifier identifies localalloy types and outputs them to the pipeline."
					"\n\n"
					":Default: ``False``\n")
		;

		enum_<PolyhedralTemplateMatchingModifier::StructureType>("Type")
			.value("OTHER", PolyhedralTemplateMatchingModifier::OTHER)
			.value("FCC", PolyhedralTemplateMatchingModifier::FCC)
			.value("HCP", PolyhedralTemplateMatchingModifier::HCP)
			.value("BCC", PolyhedralTemplateMatchingModifier::BCC)
			.value("ICO", PolyhedralTemplateMatchingModifier::ICO)
			.value("SC", PolyhedralTemplateMatchingModifier::SC)
		;

		enum_<PolyhedralTemplateMatchingModifier::AlloyType>("AlloyType")
			.value("NONE", PolyhedralTemplateMatchingModifier::ALLOY_NONE)
			.value("PURE", PolyhedralTemplateMatchingModifier::ALLOY_PURE)
			.value("L10", PolyhedralTemplateMatchingModifier::ALLOY_L10)
			.value("L12_CU", PolyhedralTemplateMatchingModifier::ALLOY_L12_CU)
			.value("L12_AU", PolyhedralTemplateMatchingModifier::ALLOY_L12_AU)
			.value("B2", PolyhedralTemplateMatchingModifier::ALLOY_B2)
		;
	}

}

OVITO_REGISTER_PLUGIN_PYTHON_INTERFACE(ParticlesModify);

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
