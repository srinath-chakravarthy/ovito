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
#include <plugins/particles/modifier/modify/CombineParticleSetsModifier.h>
#include <plugins/particles/modifier/modify/CoordinationPolyhedraModifier.h>
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
#include <plugins/particles/modifier/fields/CreateIsosurfaceModifier.h>
#include <core/scene/pipeline/ModifierApplication.h>
#include <core/utilities/concurrent/TaskManager.h>
#include "PythonBinding.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Internal)

using namespace PyScript;

void defineModifiersSubmodule(py::module parentModule)
{
	py::module m = parentModule.def_submodule("Modifiers");
	
	ovito_abstract_class<ParticleModifier, Modifier>{m}
	;

	ovito_abstract_class<AsynchronousParticleModifier, ParticleModifier>{m}
	;

	ovito_class<AssignColorModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Assigns a uniform color to all selected particles. "
			"If no particle selection is defined (i.e. the ``\"Selection\"`` particle property does not exist), "
			"the modifier assigns the color to all particles. ")
		.def_property("color", VectorGetter<AssignColorModifier, Color, &AssignColorModifier::color>(), 
							   VectorSetter<AssignColorModifier, Color, &AssignColorModifier::setColor>(),
				"The color that will be assigned to particles."
				"\n\n"
				":Default: ``(0.3,0.3,1.0)``\n")
		.def_property("color_ctrl", &AssignColorModifier::colorController, &AssignColorModifier::setColorController)
		.def_property("keep_selection", &AssignColorModifier::keepSelection, &AssignColorModifier::setKeepSelection)
	;

	auto ColorCodingModifier_py = ovito_class<ColorCodingModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Colors particles, bonds, or vectors based on the value of a property."
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
			"then the modifier automatically adjusts them to the minimum and maximum values of the input property at the time the modifier "
			"is inserted into the data pipeline.")
		.def_property("property", &ColorCodingModifier::sourceParticleProperty, &ColorCodingModifier::setSourceParticleProperty)
		.def_property("particle_property", &ColorCodingModifier::sourceParticleProperty, &ColorCodingModifier::setSourceParticleProperty,
				"The name of the input particle property that should be used to color particles. "
				"This can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. "
				"When using vector properties the component must be included in the name, e.g. ``\"Velocity.X\"``. "
				"\n\n"
				"This field is only used if :py:attr:`.assign_to` is not set to ``Bonds``. ")
		.def_property("bond_property", &ColorCodingModifier::sourceBondProperty, &ColorCodingModifier::setSourceBondProperty,
				"The name of the input bond property that should be used to color bonds. "
				"This can be one of the :ref:`standard bond properties <bond-types-list>` or a custom bond property. "
				"\n\n"
				"This field is only used if :py:attr:`.assign_to` is set to ``Bonds``. ")
		.def_property("start_value", &ColorCodingModifier::startValue, &ColorCodingModifier::setStartValue,
				"This parameter defines the value range when mapping the input property to a color.")
		.def_property("start_value_ctrl", &ColorCodingModifier::startValueController, &ColorCodingModifier::setStartValueController)
		.def_property("end_value", &ColorCodingModifier::endValue, &ColorCodingModifier::setEndValue,
				"This parameter defines the value range when mapping the input property to a color.")
		.def_property("end_value_ctrl", &ColorCodingModifier::endValueController, &ColorCodingModifier::setEndValueController)
		.def_property("gradient", &ColorCodingModifier::colorGradient, &ColorCodingModifier::setColorGradient,
				"The color gradient object, which is responsible for mapping normalized property values to colors. "
				"Available gradient types are:\n"
				" * ``ColorCodingModifier.BlueWhiteRed()``\n"
				" * ``ColorCodingModifier.Grayscale()``\n"
				" * ``ColorCodingModifier.Hot()``\n"
				" * ``ColorCodingModifier.Jet()``\n"
				" * ``ColorCodingModifier.Magma()``\n"
				" * ``ColorCodingModifier.Rainbow()`` [default]\n"
				" * ``ColorCodingModifier.Viridis()``\n"
				" * ``ColorCodingModifier.Custom(\"<image file>\")``\n"
				"\n"
				"The last color map constructor expects the path to an image file on disk, "
				"which will be used to create a custom color gradient from a row of pixels in the image.")
		.def_property("only_selected", &ColorCodingModifier::colorOnlySelected, &ColorCodingModifier::setColorOnlySelected,
				"If ``True``, only selected particles or bonds will be affected by the modifier and the existing colors "
				"of unselected particles or bonds will be preserved; if ``False``, all particles/bonds will be colored."
				"\n\n"
				":Default: ``False``\n")
		.def_property("keep_selection", &ColorCodingModifier::keepSelection, &ColorCodingModifier::setKeepSelection)
		.def_property("assign_to", &ColorCodingModifier::colorApplicationMode, &ColorCodingModifier::setColorApplicationMode,
				"Determines what the modifier assigns the colors to. "
				"This must be one of the following constants:\n"
				" * ``ColorCodingModifier.AssignmentMode.Particles``\n"
				" * ``ColorCodingModifier.AssignmentMode.Bonds``\n"
				" * ``ColorCodingModifier.AssignmentMode.Vectors``\n"
				"\n"
				"If this attribute is set to ``Bonds``, then the bond property selected by :py:attr:`.bond_property` is used to color bonds. "
				"Otherwise the particle property selected by :py:attr:`.particle_property` is used to color particles or vectors. "
				"\n\n"
				":Default: ``ColorCodingModifier.AssignmentMode.Particles``\n")
	;

	py::enum_<ColorCodingModifier::ColorApplicationMode>(ColorCodingModifier_py, "AssignmentMode")
		.value("Particles", ColorCodingModifier::Particles)
		.value("Bonds", ColorCodingModifier::Bonds)
		.value("Vectors", ColorCodingModifier::Vectors)
	;

	ovito_abstract_class<ColorCodingGradient, RefTarget>(ColorCodingModifier_py)
		.def("valueToColor", &ColorCodingGradient::valueToColor)
	;

	ovito_class<ColorCodingHSVGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Rainbow")
	;
	ovito_class<ColorCodingGrayscaleGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Grayscale")
	;
	ovito_class<ColorCodingHotGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Hot")
	;
	ovito_class<ColorCodingJetGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Jet")
	;
	ovito_class<ColorCodingBlueWhiteRedGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "BlueWhiteRed")
	;
	ovito_class<ColorCodingViridisGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Viridis")
	;
	ovito_class<ColorCodingMagmaGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Magma")
	;
	ovito_class<ColorCodingImageGradient, ColorCodingGradient>(ColorCodingModifier_py, nullptr, "Image")
		.def("load_image", &ColorCodingImageGradient::loadImage)
	;
	ColorCodingModifier_py.def_static("Custom", [](const QString& filename) {
	    OORef<ColorCodingImageGradient> gradient(new ColorCodingImageGradient(ScriptEngine::activeDataset()));
    	gradient->loadImage(filename);
    	return gradient;
	});

	ovito_class<AmbientOcclusionModifier, AsynchronousParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Performs a quick lighting calculation to shade particles according to the degree of occlusion by other particles. ")
		.def_property("intensity", &AmbientOcclusionModifier::intensity, &AmbientOcclusionModifier::setIntensity,
				"A number controlling the strength of the applied shading effect. "
				"\n\n"
				":Valid range: [0.0, 1.0]\n"
				":Default: 0.7")
		.def_property("sample_count", &AmbientOcclusionModifier::samplingCount, &AmbientOcclusionModifier::setSamplingCount,
				"The number of light exposure samples to compute. More samples give a more even light distribution "
				"but take longer to compute."
				"\n\n"
				":Default: 40\n")
		.def_property("buffer_resolution", &AmbientOcclusionModifier::bufferResolution, &AmbientOcclusionModifier::setBufferResolution,
				"A positive integer controlling the resolution of the internal render buffer, which is used to compute how much "
				"light each particle receives. When the number of particles is large, a larger buffer resolution should be used."
				"\n\n"
				":Valid range: [1, 4]\n"
				":Default: 3\n")
	;

	ovito_class<DeleteParticlesModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier deletes the selected particles. It has no parameters.",
			// Python class name:
			"DeleteSelectedParticlesModifier")
	;

	ovito_class<ShowPeriodicImagesModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier replicates all particles to display periodic images of the system.")
		.def_property("replicate_x", &ShowPeriodicImagesModifier::showImageX, &ShowPeriodicImagesModifier::setShowImageX,
				"Enables replication of particles along *x*."
				"\n\n"
				":Default: ``False``\n")
		.def_property("replicate_y", &ShowPeriodicImagesModifier::showImageY, &ShowPeriodicImagesModifier::setShowImageY,
				"Enables replication of particles along *y*."
				"\n\n"
				":Default: ``False``\n")
		.def_property("replicate_z", &ShowPeriodicImagesModifier::showImageZ, &ShowPeriodicImagesModifier::setShowImageZ,
				"Enables replication of particles along *z*."
				"\n\n"
				":Default: ``False``\n")
		.def_property("num_x", &ShowPeriodicImagesModifier::numImagesX, &ShowPeriodicImagesModifier::setNumImagesX,
				"A positive integer specifying the number of copies to generate in the *x* direction (including the existing primary image)."
				"\n\n"
				":Default: 3\n")
		.def_property("num_y", &ShowPeriodicImagesModifier::numImagesY, &ShowPeriodicImagesModifier::setNumImagesY,
				"A positive integer specifying the number of copies to generate in the *y* direction (including the existing primary image)."
				"\n\n"
				":Default: 3\n")
		.def_property("num_z", &ShowPeriodicImagesModifier::numImagesZ, &ShowPeriodicImagesModifier::setNumImagesZ,
				"A positive integer specifying the number of copies to generate in the *z* direction (including the existing primary image)."
				"\n\n"
				":Default: 3\n")
		.def_property("adjust_box", &ShowPeriodicImagesModifier::adjustBoxSize, &ShowPeriodicImagesModifier::setAdjustBoxSize,
				"A boolean flag controlling the modification of the simulation cell geometry. "
				"If ``True``, the simulation cell is extended to fit the multiplied system. "
				"If ``False``, the original simulation cell (containing only the primary image of the system) is kept. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("unique_ids", &ShowPeriodicImagesModifier::uniqueIdentifiers, &ShowPeriodicImagesModifier::setUniqueIdentifiers,
				"If ``True``, the modifier automatically generates a new unique ID for each copy of a particle. "
				"This option has no effect if the input system does not contain particle IDs. "
				"\n\n"
				":Default: ``True``\n")
	;

	ovito_class<WrapPeriodicImagesModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier maps particles located outside the simulation cell back into the box by \"wrapping\" their coordinates "
			"around at the periodic boundaries of the simulation cell. This modifier has no parameters.")
	;

	ovito_class<ComputeBondLengthsModifier, ParticleModifier>(m,
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

	ovito_class<ComputePropertyModifier, ParticleModifier>(m,
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
		.def_property("expressions", &ComputePropertyModifier::expressions, &ComputePropertyModifier::setExpressions,
				"A list of strings containing the math expressions to compute, one for each vector component of the output property. "
				"If the output property is a scalar property, the list should comprise exactly one string. "
				"\n\n"
				":Default: ``[\"0\"]``\n")
		.def_property("neighbor_expressions", &ComputePropertyModifier::neighborExpressions, &ComputePropertyModifier::setNeighborExpressions,
				"A list of strings containing the math expressions for the per-neighbor terms, one for each vector component of the output property. "
				"If the output property is a scalar property, the list should comprise exactly one string. "
				"\n\n"
				"The neighbor expressions are only evaluated if :py:attr:`.neighbor_mode` is enabled."
				"\n\n"
				":Default: ``[\"0\"]``\n")
		.def_property("output_property", &ComputePropertyModifier::outputProperty, &ComputePropertyModifier::setOutputProperty,
				"The output particle property in which the modifier should store the computed values. "
				"\n\n"
				":Default: ``\"Custom property\"``\n")
		.def_property("component_count", &ComputePropertyModifier::propertyComponentCount, &ComputePropertyModifier::setPropertyComponentCount)
		.def_property("only_selected", &ComputePropertyModifier::onlySelectedParticles, &ComputePropertyModifier::setOnlySelectedParticles,
				"If ``True``, the property is only computed for selected particles and existing property values "
				"are preserved for unselected particles."
				"\n\n"
				":Default: ``False``\n")
		.def_property("neighbor_mode", &ComputePropertyModifier::neighborModeEnabled, &ComputePropertyModifier::setNeighborModeEnabled,
				"Boolean flag that enabled the neighbor computation mode, where contributions from neighbor particles within the "
				"cutoff radius are taken into account. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("cutoff_radius", &ComputePropertyModifier::cutoff, &ComputePropertyModifier::setCutoff,
				"The cutoff radius up to which neighboring particles are visited. This parameter is only used if :py:attr:`.neighbor_mode` is enabled. "
				"\n\n"
				":Default: 3.0\n")
	;

	ovito_class<FreezePropertyModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier can store a static copy of a particle property and inject it back into the pipeline (optionally under a different name than the original property). "
			"Since the snapshot of the current particle property values is taken by the modifier at a particular animation time, "
			"the :py:class:`!FreezePropertyModifier` allows to *freeze* the property and overwrite any dynamically changing property values with the stored static copy. "
			"\n\n"
			"**Example:**"
			"\n\n"
			".. literalinclude:: ../example_snippets/freeze_property_modifier.py\n"
			"\n")
		.def_property("source_property", &FreezePropertyModifier::sourceProperty, &FreezePropertyModifier::setSourceProperty,
				"The name of the input particle property that should be copied by the modifier. "
				"It can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. ")
		.def_property("destination_property", &FreezePropertyModifier::destinationProperty, &FreezePropertyModifier::setDestinationProperty,
				"The name of the output particle property that should be written to by the modifier. "
				"It can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. ")
		.def("_take_snapshot", static_cast<bool (FreezePropertyModifier::*)(TimePoint,TaskManager&,bool)>(&FreezePropertyModifier::takePropertySnapshot))
	;

	ovito_class<ClearSelectionModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier clears the particle selection by deleting the ``\"Selection\"`` particle property. "
			"The modifier has no input parameters.")
	;

	ovito_class<InvertSelectionModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier inverts the particle selection. It has no input parameters.")
	;

	ovito_class<ManualSelectionModifier, ParticleModifier>(m)
		.def("reset_selection", &ManualSelectionModifier::resetSelection)
		.def("select_all", &ManualSelectionModifier::selectAll)
		.def("clear_selection", &ManualSelectionModifier::clearSelection)
		.def("toggle_particle_selection", &ManualSelectionModifier::toggleParticleSelection)
	;

	auto ExpandSelectionModifier_py = ovito_class<ExpandSelectionModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Expands the current particle selection by selecting particles that are neighbors of already selected particles.")
		.def_property("mode", &ExpandSelectionModifier::mode, &ExpandSelectionModifier::setMode,
				"Selects the mode of operation, i.e., how the modifier extends the selection around already selected particles. "
				"Valid values are:"
				"\n\n"
				"  * ``ExpandSelectionModifier.ExpansionMode.Cutoff``\n"
				"  * ``ExpandSelectionModifier.ExpansionMode.Nearest``\n"
				"  * ``ExpandSelectionModifier.ExpansionMode.Bonded``\n"
				"\n\n"
				":Default: ``ExpandSelectionModifier.ExpansionMode.Cutoff``\n")
		.def_property("cutoff", &ExpandSelectionModifier::cutoffRange, &ExpandSelectionModifier::setCutoffRange,
				"The maximum distance up to which particles are selected around already selected particles. "
				"This parameter is only used if :py:attr:`.mode` is set to ``ExpansionMode.Cutoff``."
				"\n\n"
				":Default: 3.2\n")
		.def_property("num_neighbors", &ExpandSelectionModifier::numNearestNeighbors, &ExpandSelectionModifier::setNumNearestNeighbors,
				"The number of nearest neighbors to select around each already selected particle. "
				"This parameter is only used if :py:attr:`.mode` is set to ``ExpansionMode.Nearest``."
				"\n\n"
				":Default: 1\n")
		.def_property("iterations", &ExpandSelectionModifier::numberOfIterations, &ExpandSelectionModifier::setNumberOfIterations,
				"Controls how many iterations of the modifier are executed. This can be used to select "
				"neighbors of neighbors up to a certain recursive depth."
				"\n\n"
				":Default: 1\n")
	;

	py::enum_<ExpandSelectionModifier::ExpansionMode>(ExpandSelectionModifier_py, "ExpansionMode")
		.value("Cutoff", ExpandSelectionModifier::CutoffRange)
		.value("Nearest", ExpandSelectionModifier::NearestNeighbors)
		.value("Bonded", ExpandSelectionModifier::BondedNeighbors)
	;

	ovito_class<SelectExpressionModifier, ParticleModifier>(m,
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
		.def_property("expression", &SelectExpressionModifier::expression, &SelectExpressionModifier::setExpression,
				"A string containing the Boolean expression to be evaluated for every particle. "
				"The expression syntax is documented in `OVITO's user manual <../../particles.modifiers.expression_select.html>`_.")
	;

	ovito_class<SelectParticleTypeModifier, ParticleModifier>(m,
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
		.def_property("property", &SelectParticleTypeModifier::sourceProperty, &SelectParticleTypeModifier::setSourceProperty,
				"The name of the particle property storing the input particle types. "
				"This can be a :ref:`standard particle property <particle-types-list>` such as ``\"Particle Type\"`` or ``\"Structure Type\"``, or "
				"a custom integer particle property."
				"\n\n"
				":Default: ``\"Particle Type\"``\n")
		.def_property("types", &SelectParticleTypeModifier::selectedParticleTypes, &SelectParticleTypeModifier::setSelectedParticleTypes,
				"A Python ``set`` of integers, which specifies the particle types to select. "
				"\n\n"
				":Default: ``set([])``\n")
	;

	ovito_class<SliceModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Deletes or selects particles in a region bounded by one or two parallel infinite planes in three-dimensional space.")
		.def_property("distance", &SliceModifier::distance, &SliceModifier::setDistance,
				"The distance of the slicing plane from the origin (along its normal vector)."
				"\n\n"
				":Default: 0.0\n")
		.def_property("normal", &SliceModifier::normal, &SliceModifier::setNormal,
				"The normal vector of the slicing plane. Does not have to be a unit vector."
				"\n\n"
				":Default: ``(1,0,0)``\n")
		.def_property("slice_width", &SliceModifier::sliceWidth, &SliceModifier::setSliceWidth,
				"The width of the slab to cut. If zero, the modifier cuts all particles on one "
				"side of the slicing plane."
				"\n\n"
				":Default: 0.0\n")
		.def_property("inverse", &SliceModifier::inverse, &SliceModifier::setInverse,
				"Reverses the sense of the slicing plane."
				"\n\n"
				":Default: ``False``\n")
		.def_property("select", &SliceModifier::createSelection, &SliceModifier::setCreateSelection,
				"If ``True``, the modifier selects particles instead of deleting them."
				"\n\n"
				":Default: ``False``\n")
		.def_property("only_selected", &SliceModifier::applyToSelection, &SliceModifier::setApplyToSelection,
				"If ``True``, the modifier acts only on selected particles; if ``False``, the modifier acts on all particles."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<AffineTransformationModifier, ParticleModifier>(m,
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
		.def_property("transformation", MatrixGetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::transformationTM>(), 
										MatrixSetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::setTransformationTM>(),
				"The 3x4 transformation matrix being applied to particle positions and/or the simulation cell. "
				"The first three matrix columns define the linear part of the transformation, while the fourth "
				"column specifies the translation vector. "
				"\n\n"
				"This matrix describes a relative transformation and is used only if :py:attr:`.relative_mode` == ``True``."
				"\n\n"
				":Default: ``[[ 1.  0.  0.  0.] [ 0.  1.  0.  0.] [ 0.  0.  1.  0.]]``\n")
		.def_property("target_cell", MatrixGetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::targetCell>(), 
									 MatrixSetter<AffineTransformationModifier, AffineTransformation, &AffineTransformationModifier::setTargetCell>(),
				"This 3x4 matrix specifies the target cell shape. It is used when :py:attr:`.relative_mode` == ``False``. "
				"\n\n"
				"The first three columns of the matrix specify the three edge vectors of the target cell. "
				"The fourth column defines the origin vector of the target cell.")
		.def_property("relative_mode", &AffineTransformationModifier::relativeMode, &AffineTransformationModifier::setRelativeMode,
				"Selects the operation mode of the modifier."
				"\n\n"
				"If ``relative_mode==True``, the modifier transforms the particles and/or the simulation cell "
				"by applying the matrix given by the :py:attr:`.transformation` parameter."
				"\n\n"
				"If ``relative_mode==False``, the modifier transforms the particles and/or the simulation cell "
				"such that the old simulation cell will have the shape given by the the :py:attr:`.target_cell` parameter after the transformation."
				"\n\n"
				":Default: ``True``\n")
		.def_property("transform_particles", &AffineTransformationModifier::applyToParticles, &AffineTransformationModifier::setApplyToParticles,
				"If ``True``, the modifier transforms the particle positions."
				"\n\n"
				":Default: ``True``\n")
		.def_property("only_selected", &AffineTransformationModifier::selectionOnly, &AffineTransformationModifier::setSelectionOnly,
				"If ``True``, the modifier acts only on selected particles; if ``False``, the modifier acts on all particles."
				"\n\n"
				":Default: ``False``\n")
		.def_property("transform_box", &AffineTransformationModifier::applyToSimulationBox, &AffineTransformationModifier::setApplyToSimulationBox,
				"If ``True``, the modifier transforms the simulation cell."
				"\n\n"
				":Default: ``False``\n")
		.def_property("transform_surface", &AffineTransformationModifier::applyToSurfaceMesh, &AffineTransformationModifier::setApplyToSurfaceMesh,
				"If ``True``, the modifier transforms the surface mesh (if any) that has previously been generated by a :py:class:`ConstructSurfaceModifier`."
				"\n\n"
				":Default: ``True``\n")
		.def_property("transform_vector_properties", &AffineTransformationModifier::applyToVectorProperties, &AffineTransformationModifier::setApplyToVectorProperties,
				"If ``True``, the modifier applies the transformation to certain standard particle and bond properties that represent vectorial quantities. "
				"This option is useful if you are using the AffineTransformationModifier to rotate a particle system and want also to rotate vectorial "
				"properties associated with the individual particles or bonds, like e.g. the velocity vectors. See the user manual of OVITO for the list of standard particle properties that are affected by this option. "
				"\n\n"
				":Default: ``False``\n")
	;

	auto BinAndReduceModifier_py = ovito_class<BinAndReduceModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier applies a reduction operation to a property of the particles within a spatial bin. "
			"The output of the modifier is a one or two-dimensional grid of bin values. ")
		.def_property("property", &BinAndReduceModifier::sourceProperty, &BinAndReduceModifier::setSourceProperty,
				"The name of the input particle property to which the reduction operation should be applied. "
				"This can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. "
				"For vector properties the selected component must be appended to the name, e.g. ``\"Velocity.X\"``. ")
		.def_property("reduction_operation", &BinAndReduceModifier::reductionOperation, &BinAndReduceModifier::setReductionOperation,
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
		.def_property("first_derivative", &BinAndReduceModifier::firstDerivative, &BinAndReduceModifier::setFirstDerivative,
				"If true, the modifier numerically computes the first derivative of the binned data using a finite differences approximation. "
				"This works only for one-dimensional bin grids. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("direction", &BinAndReduceModifier::binDirection, &BinAndReduceModifier::setBinDirection,
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
		.def_property("bin_count_x", &BinAndReduceModifier::numberOfBinsX, &BinAndReduceModifier::setNumberOfBinsX,
				"This attribute sets the number of bins to generate along the first binning axis."
				"\n\n"
				":Default: 200\n")
		.def_property("bin_count_y", &BinAndReduceModifier::numberOfBinsY, &BinAndReduceModifier::setNumberOfBinsY,
				"This attribute sets the number of bins to generate along the second binning axis (only used when working with a two-dimensional grid)."
				"\n\n"
				":Default: 200\n")
		.def_property("only_selected", &BinAndReduceModifier::onlySelected, &BinAndReduceModifier::setOnlySelected,
				"If ``True``, the computation takes into account only the currently selected particles. "
				"You can use this to restrict the calculation to a subset of particles. "
				"\n\n"
				":Default: ``False``\n")
		.def_property_readonly("bin_data", py::cpp_function([](BinAndReduceModifier& mod) {
					std::vector<size_t> shape;
					if(mod.is1D()) shape.push_back(mod.binData().size());
					else {
						shape.push_back(mod.numberOfBinsY());
						shape.push_back(mod.numberOfBinsX());
						OVITO_ASSERT(shape[0] * shape[1] == mod.binData().size());
					}
					py::array_t<double> array(shape, mod.binData().data(), py::cast(&mod));
					// Mark array as read-only.
					reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
					return array;
				}),
				"Returns a NumPy array containing the reduced bin values computed by the modifier. "
    			"Depending on the selected binning :py:attr:`.direction` the returned array is either "
    			"one or two-dimensional. In the two-dimensional case the outer index of the returned array "
    			"runs over the bins along the second binning axis. "
				"\n\n"    
    			"Note that accessing this array is only possible after the modifier has computed its results. " 
    			"Thus, you have to call :py:meth:`ovito.ObjectNode.compute` first to ensure that the binning and reduction operation was performed.")
		.def_property_readonly("axis_range_x", [](BinAndReduceModifier& modifier) {
				return py::make_tuple(modifier.xAxisRangeStart(), modifier.xAxisRangeEnd());
			},
			"A 2-tuple containing the range of the generated bin grid along the first binning axis. "
			"Note that this is an output attribute which is only valid after the modifier has performed the bin and reduce operation. "
			"That means you have to call :py:meth:`ovito.ObjectNode.compute` first to evaluate the data pipeline.")
		.def_property_readonly("axis_range_y", [](BinAndReduceModifier& modifier) {
				return py::make_tuple(modifier.yAxisRangeStart(), modifier.yAxisRangeEnd());
			},
			"A 2-tuple containing the range of the generated bin grid along the second binning axis. "
			"Note that this is an output attribute which is only valid after the modifier has performed the bin and reduce operation. "
			"That means you have to call :py:meth:`ovito.ObjectNode.compute` first to evaluate the data pipeline.")
	;

	py::enum_<BinAndReduceModifier::ReductionOperationType>(BinAndReduceModifier_py, "Operation")
		.value("Mean", BinAndReduceModifier::RED_MEAN)
		.value("Sum", BinAndReduceModifier::RED_SUM)
		.value("SumVol", BinAndReduceModifier::RED_SUM_VOL)
		.value("Min", BinAndReduceModifier::RED_MIN)
		.value("Max", BinAndReduceModifier::RED_MAX)
	;

	py::enum_<BinAndReduceModifier::BinDirectionType>(BinAndReduceModifier_py, "Direction")
		.value("Vector_1", BinAndReduceModifier::CELL_VECTOR_1)
		.value("Vector_2", BinAndReduceModifier::CELL_VECTOR_2)
		.value("Vector_3", BinAndReduceModifier::CELL_VECTOR_3)
		.value("Vectors_1_2", BinAndReduceModifier::CELL_VECTORS_1_2)
		.value("Vectors_1_3", BinAndReduceModifier::CELL_VECTORS_1_3)
		.value("Vectors_2_3", BinAndReduceModifier::CELL_VECTORS_2_3)
	;

	ovito_abstract_class<StructureIdentificationModifier, AsynchronousParticleModifier>{m}
	;

	auto BondAngleAnalysisModifier_py = ovito_class<BondAngleAnalysisModifier, StructureIdentificationModifier>(m,
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
	;
	expose_subobject_list<BondAngleAnalysisModifier, 
						  ParticleType, 
						  StructureIdentificationModifier,
						  &StructureIdentificationModifier::structureTypes>(
							  BondAngleAnalysisModifier_py, "structures", "BondAngleAnalysisStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
		"You can adjust the color of structural types as shown in the code example above.");

	py::enum_<BondAngleAnalysisModifier::StructureType>(BondAngleAnalysisModifier_py, "Type")
		.value("OTHER", BondAngleAnalysisModifier::OTHER)
		.value("FCC", BondAngleAnalysisModifier::FCC)
		.value("HCP", BondAngleAnalysisModifier::HCP)
		.value("BCC", BondAngleAnalysisModifier::BCC)
		.value("ICO", BondAngleAnalysisModifier::ICO)
	;

	auto CommonNeighborAnalysisModifier_py = ovito_class<CommonNeighborAnalysisModifier, StructureIdentificationModifier>(m,
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
		.def_property("cutoff", &CommonNeighborAnalysisModifier::cutoff, &CommonNeighborAnalysisModifier::setCutoff,
				"The cutoff radius used for the conventional common neighbor analysis. "
				"This parameter is only used if :py:attr:`.mode` == ``CommonNeighborAnalysisModifier.Mode.FixedCutoff``."
				"\n\n"
				":Default: 3.2\n")
		.def_property("mode", &CommonNeighborAnalysisModifier::mode, &CommonNeighborAnalysisModifier::setMode,
				"Selects the mode of operation. "
				"Valid values are:"
				"\n\n"
				"  * ``CommonNeighborAnalysisModifier.Mode.FixedCutoff``\n"
				"  * ``CommonNeighborAnalysisModifier.Mode.AdaptiveCutoff``\n"
				"  * ``CommonNeighborAnalysisModifier.Mode.BondBased``\n"
				"\n\n"
				":Default: ``CommonNeighborAnalysisModifier.Mode.AdaptiveCutoff``\n")
		.def_property("only_selected", &CommonNeighborAnalysisModifier::onlySelectedParticles, &CommonNeighborAnalysisModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only for selected particles. Particles that are not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
	;
	expose_subobject_list<CommonNeighborAnalysisModifier, 
						  ParticleType,
						  StructureIdentificationModifier, 
						  &StructureIdentificationModifier::structureTypes>(
							  CommonNeighborAnalysisModifier_py, "structures", "CommonNeighborAnalysisStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
		"You can adjust the color of structural types as shown in the code example above.");

	py::enum_<CommonNeighborAnalysisModifier::CNAMode>(CommonNeighborAnalysisModifier_py, "Mode")
		.value("FixedCutoff", CommonNeighborAnalysisModifier::FixedCutoffMode)
		.value("AdaptiveCutoff", CommonNeighborAnalysisModifier::AdaptiveCutoffMode)
		.value("BondBased", CommonNeighborAnalysisModifier::BondMode)
	;

	py::enum_<CommonNeighborAnalysisModifier::StructureType>(CommonNeighborAnalysisModifier_py, "Type")
		.value("OTHER", CommonNeighborAnalysisModifier::OTHER)
		.value("FCC", CommonNeighborAnalysisModifier::FCC)
		.value("HCP", CommonNeighborAnalysisModifier::HCP)
		.value("BCC", CommonNeighborAnalysisModifier::BCC)
		.value("ICO", CommonNeighborAnalysisModifier::ICO)
	;

	auto IdentifyDiamondModifier_py = ovito_class<IdentifyDiamondModifier, StructureIdentificationModifier>(m,
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
		.def_property("only_selected", &IdentifyDiamondModifier::onlySelectedParticles, &IdentifyDiamondModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only for selected particles. Particles that are not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
	;
	expose_subobject_list<IdentifyDiamondModifier, 
						  ParticleType,
						  StructureIdentificationModifier, 
						  &StructureIdentificationModifier::structureTypes>(
							  IdentifyDiamondModifier_py, "structures", "IdentifyDiamondStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
		"You can adjust the color of structural types as shown in the code example above.");

	py::enum_<IdentifyDiamondModifier::StructureType>(IdentifyDiamondModifier_py, "Type")
		.value("OTHER", IdentifyDiamondModifier::OTHER)
		.value("CUBIC_DIAMOND", IdentifyDiamondModifier::CUBIC_DIAMOND)
		.value("CUBIC_DIAMOND_FIRST_NEIGHBOR", IdentifyDiamondModifier::CUBIC_DIAMOND_FIRST_NEIGH)
		.value("CUBIC_DIAMOND_SECOND_NEIGHBOR", IdentifyDiamondModifier::CUBIC_DIAMOND_SECOND_NEIGH)
		.value("HEX_DIAMOND", IdentifyDiamondModifier::HEX_DIAMOND)
		.value("HEX_DIAMOND_FIRST_NEIGHBOR", IdentifyDiamondModifier::HEX_DIAMOND_FIRST_NEIGH)
		.value("HEX_DIAMOND_SECOND_NEIGHBOR", IdentifyDiamondModifier::HEX_DIAMOND_SECOND_NEIGH)
	;

	auto CreateBondsModifier_py = ovito_class<CreateBondsModifier, AsynchronousParticleModifier>(m,
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
		.def_property("mode", &CreateBondsModifier::cutoffMode, &CreateBondsModifier::setCutoffMode,
				"Selects the mode of operation. Valid modes are:"
				"\n\n"
				"  * ``CreateBondsModifier.Mode.Uniform``\n"
				"  * ``CreateBondsModifier.Mode.Pairwise``\n"
				"\n\n"
				"In ``Uniform`` mode one global :py:attr:`.cutoff` is used irrespective of the atom types. "
				"In ``Pairwise`` mode a separate cutoff distance must be specified for all pairs of atom types between which bonds are to be created. "
				"\n\n"
				":Default: ``CreateBondsModifier.Mode.Uniform``\n")
		.def_property("cutoff", &CreateBondsModifier::uniformCutoff, &CreateBondsModifier::setUniformCutoff,
				"The maximum cutoff distance for the creation of bonds between particles. This parameter is only used if :py:attr:`.mode` is ``Uniform``. "
				"\n\n"
				":Default: 3.2\n")
		.def_property("intra_molecule_only", &CreateBondsModifier::onlyIntraMoleculeBonds, &CreateBondsModifier::setOnlyIntraMoleculeBonds,
				"If this option is set to true, the modifier will create bonds only between atoms that belong to the same molecule (i.e. which have the same molecule ID assigned to them)."
				"\n\n"
				":Default: ``False``\n")
		.def_property_readonly("bonds_display", &CreateBondsModifier::bondsDisplay,
				"The :py:class:`~ovito.vis.BondsDisplay` object controlling the visual appearance of the bonds created by this modifier.")
		.def_property("lower_cutoff", &CreateBondsModifier::minimumCutoff, &CreateBondsModifier::setMinimumCutoff,
				"The minimum bond length. No bonds will be created between atoms whose distance is below this threshold."
				"\n\n"
				":Default: 0.0\n")
		.def("set_pairwise_cutoff", &CreateBondsModifier::setPairCutoff,
				"Sets the pair-wise cutoff distance for a pair of atom types. This information is only used if :py:attr:`.mode` is ``Pairwise``."
				"\n\n"
				":param str type_a: The :py:attr:`~ovito.data.ParticleType.name` of the first atom type\n"
				":param str type_b: The :py:attr:`~ovito.data.ParticleType.name` of the second atom type (order doesn't matter)\n"
				":param float cutoff: The cutoff distance to be set for the type pair.\n"
				"\n\n"
				"If you do not want to create any bonds between a pair of types, set the corresponding cutoff radius to zero (which is the default).",
				py::arg("type_a"), py::arg("type_b"), py::arg("cutoff"))
		.def("get_pairwise_cutoff", &CreateBondsModifier::getPairCutoff,
				"Returns the pair-wise cutoff distance set for a pair of atom types."
				"\n\n"
				":param str type_a: The :py:attr:`~ovito.data.ParticleType.name` of the first atom type\n"
				":param str type_b: The :py:attr:`~ovito.data.ParticleType.name` of the second atom type (order doesn't matter)\n"
				":return: The cutoff distance set for the type pair. Returns zero if no cutoff has been set for the pair.\n",
				py::arg("type_a"), py::arg("type_b"))
	;

	py::enum_<CreateBondsModifier::CutoffMode>(CreateBondsModifier_py, "Mode")
		.value("Uniform", CreateBondsModifier::UniformCutoff)
		.value("Pairwise", CreateBondsModifier::PairCutoff)
	;

	ovito_class<CentroSymmetryModifier, AsynchronousParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Computes the centro-symmetry parameter (CSP) of each particle."
			"\n\n"
			"The modifier outputs the computed values in the ``\"Centrosymmetry\"`` particle property.")
		.def_property("num_neighbors", &CentroSymmetryModifier::numNeighbors, &CentroSymmetryModifier::setNumNeighbors,
				"The number of neighbors to take into account (12 for FCC crystals, 8 for BCC crystals)."
				"\n\n"
				":Default: 12\n")
	;

	ovito_class<ClusterAnalysisModifier, AsynchronousParticleModifier>(m,
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
		.def_property("cutoff", &ClusterAnalysisModifier::cutoff, &ClusterAnalysisModifier::setCutoff,
				"The cutoff distance used by the algorithm to form clusters of connected particles."
				"\n\n"
				":Default: 3.2\n")
		.def_property("only_selected", &ClusterAnalysisModifier::onlySelectedParticles, &ClusterAnalysisModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only for selected particles. "
				"Particles that are not selected will be assigned cluster ID 0 and treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("sort_by_size", &ClusterAnalysisModifier::sortBySize, &ClusterAnalysisModifier::setSortBySize,
				"Enables the sorting of clusters by size (in descending order). Cluster 1 will be the largest cluster, cluster 2 the second largest, and so on."
				"\n\n"
				":Default: ``False``\n")
	;

	ovito_class<CoordinationNumberModifier, AsynchronousParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Computes coordination numbers of particles and the radial distribution function (RDF) of the system."
			"\n\n"
			"The modifier stores the computed coordination numbers in the ``\"Coordination\"`` particle property."
			"\n\n"
			"Example showing how to export the RDF data to a text file:\n\n"
			".. literalinclude:: ../example_snippets/coordination_analysis_modifier.py")
		.def_property("cutoff", &CoordinationNumberModifier::cutoff, &CoordinationNumberModifier::setCutoff,
				"The neighbor cutoff distance."
				"\n\n"
				":Default: 3.2\n")
		.def_property("number_of_bins", &CoordinationNumberModifier::numberOfBins, &CoordinationNumberModifier::setNumberOfBins,
				"The number of histogram bins to use when computing the RDF."
				"\n\n"
				":Default: 200\n")
		.def_property_readonly("rdf_x", py::cpp_function([](CoordinationNumberModifier& mod) {
					py::array_t<double> array((size_t)mod.rdfX().size(), mod.rdfX().data(), py::cast(&mod));
					reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
					return array;
				}))
		.def_property_readonly("rdf_y", py::cpp_function([](CoordinationNumberModifier& mod) {
					py::array_t<double> array((size_t)mod.rdfY().size(), mod.rdfY().data(), py::cast(&mod));
					reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
					return array;
				}))
	;

	ovito_class<CalculateDisplacementsModifier, ParticleModifier>(m,
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
		.def_property("reference", &CalculateDisplacementsModifier::referenceConfiguration, &CalculateDisplacementsModifier::setReferenceConfiguration,
				"A :py:class:`~ovito.io.FileSource` that provides the reference positions of particles. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a reference simulation file "
				"as shown in the code example above.")
		.def_property("eliminate_cell_deformation", &CalculateDisplacementsModifier::eliminateCellDeformation, &CalculateDisplacementsModifier::setEliminateCellDeformation,
				"Boolean flag that controls the elimination of the affine cell deformation prior to calculating the "
				"displacement vectors."
				"\n\n"
				":Default: ``False``\n")
		.def_property("assume_unwrapped_coordinates", &CalculateDisplacementsModifier::assumeUnwrappedCoordinates, &CalculateDisplacementsModifier::setAssumeUnwrappedCoordinates,
				"If ``True``, the particle coordinates of the reference and of the current configuration are taken as is. "
				"If ``False``, the minimum image convention is used to deal with particles that have crossed a periodic boundary. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("reference_frame", &CalculateDisplacementsModifier::referenceFrameNumber, &CalculateDisplacementsModifier::setReferenceFrameNumber,
				"The frame number to use as reference configuration if the reference data comprises multiple "
				"simulation frames. Only used if ``use_frame_offset==False``."
				"\n\n"
				":Default: 0\n")
		.def_property("use_frame_offset", &CalculateDisplacementsModifier::useReferenceFrameOffset, &CalculateDisplacementsModifier::setUseReferenceFrameOffset,
				"Determines whether a sliding reference configuration is taken at a constant time offset (specified by :py:attr:`.frame_offset`) "
				"relative to the current frame. If ``False``, a constant reference configuration is used (set by the :py:attr:`.reference_frame` parameter) "
				"irrespective of the current frame."
				"\n\n"
				":Default: ``False``\n")
		.def_property("frame_offset", &CalculateDisplacementsModifier::referenceFrameOffset, &CalculateDisplacementsModifier::setReferenceFrameOffset,
				"The relative frame offset when using a sliding reference configuration (``use_frame_offset==True``)."
				"\n\n"
				":Default: -1\n")
		.def_property_readonly("vector_display", &CalculateDisplacementsModifier::vectorDisplay,
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

	auto HistogramModifier_py = ovito_class<HistogramModifier, ParticleModifier>(m,
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
			"    modifier = HistogramModifier(bin_count=100, particle_property=\"Potential Energy\")\n"
			"    node.modifiers.append(modifier)\n"
			"    node.compute()\n"
			"    \n"
			"    import numpy\n"
			"    numpy.savetxt(\"histogram.txt\", modifier.histogram)\n"
			"\n")
		.def_property("particle_property", &HistogramModifier::sourceParticleProperty, &HistogramModifier::setSourceParticleProperty,
				"The name of the input particle property for which to compute the histogram. "
				"This can be one of the :ref:`standard particle properties <particle-types-list>` or a custom particle property. "
				"When using vector properties the component must be included in the name, e.g. ``\"Velocity.X\"``. "
				"\n\n"
				"This field is only used if :py:attr:`.source_mode` is set to ``Particles``. ")
		// For backward compatibility with OVITO 2.8.1:
		.def_property("property", &HistogramModifier::sourceParticleProperty, &HistogramModifier::setSourceParticleProperty)
		.def_property("bond_property", &HistogramModifier::sourceBondProperty, &HistogramModifier::setSourceBondProperty,
				"The name of the input bond property for which to compute the histogram. "
				"This can be one of the :ref:`standard bond properties <bond-types-list>` or a custom bond property. "
				"\n\n"
				"This field is only used if :py:attr:`.source_mode` is set to ``Bonds``. ")
		.def_property("bin_count", &HistogramModifier::numberOfBins, &HistogramModifier::setNumberOfBins,
				"The number of histogram bins."
				"\n\n"
				":Default: 200\n")
		.def_property("fix_xrange", &HistogramModifier::fixXAxisRange, &HistogramModifier::setFixXAxisRange,
				"Controls how the value range of the histogram is determined. If false, the range is chosen automatically by the modifier to include "
				"all input values. If true, the range is specified manually using the :py:attr:`.xrange_start` and :py:attr:`.xrange_end` attributes."
				"\n\n"
				":Default: ``False``\n")
		.def_property("xrange_start", &HistogramModifier::xAxisRangeStart, &HistogramModifier::setXAxisRangeStart,
				"If :py:attr:`.fix_xrange` is true, then this specifies the lower end of the value range covered by the histogram."
				"\n\n"
				":Default: 0.0\n")
		.def_property("xrange_end", &HistogramModifier::xAxisRangeEnd, &HistogramModifier::setXAxisRangeEnd,
				"If :py:attr:`.fix_xrange` is true, then this specifies the upper end of the value range covered by the histogram."
				"\n\n"
				":Default: 0.0\n")
		.def_property("only_selected", &HistogramModifier::onlySelected, &HistogramModifier::setOnlySelected,
				"If ``True``, the histogram is computed only on the basis of currently selected particles or bonds. "
				"You can use this to restrict histogram calculation to a subset of particles/bonds. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("source_mode", &HistogramModifier::dataSourceType, &HistogramModifier::setDataSourceType,
				"Determines where this modifier takes its input values from. "
				"This must be one of the following constants:\n"
				" * ``HistogramModifier.SourceMode.Particles``\n"
				" * ``HistogramModifier.SourceMode.Bonds``\n"
				"\n"
				"If this is set to ``Bonds``, then the histogram is computed from the bond property selected by :py:attr:`.bond_property`. "
				"Otherwise it is computed from the particle property selected by :py:attr:`.particle_property`. "
				"\n\n"
				":Default: ``HistogramModifier.SourceMode.Particles``\n")
		.def_property_readonly("_histogram_data", py::cpp_function([](HistogramModifier& mod) {
					py::array_t<int> array((size_t)mod.histogramData().size(), mod.histogramData().data(), py::cast(&mod));
					// Mark array as read-only.
					reinterpret_cast<py::detail::PyArray_Proxy*>(array.ptr())->flags &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
					return array;
				}))
	;

	py::enum_<HistogramModifier::DataSourceType>(HistogramModifier_py, "SourceMode")
		.value("Particles", HistogramModifier::Particles)
		.value("Bonds", HistogramModifier::Bonds)
	;
	

	ovito_class<ScatterPlotModifier, ParticleModifier>{m}
	;

	ovito_class<AtomicStrainModifier, AsynchronousParticleModifier>(m,
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
		.def_property("reference", &AtomicStrainModifier::referenceConfiguration, &AtomicStrainModifier::setReferenceConfiguration,
				"A :py:class:`~ovito.io.FileSource` that provides the reference positions of particles. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a reference simulation file "
				"as shown in the code example above.")
		.def_property("eliminate_cell_deformation", &AtomicStrainModifier::eliminateCellDeformation, &AtomicStrainModifier::setEliminateCellDeformation,
				"Boolean flag that controls the elimination of the affine cell deformation prior to calculating the "
				"local strain."
				"\n\n"
				":Default: ``False``\n")
		.def_property("assume_unwrapped_coordinates", &AtomicStrainModifier::assumeUnwrappedCoordinates, &AtomicStrainModifier::setAssumeUnwrappedCoordinates,
				"If ``True``, the particle coordinates of the reference and of the current configuration are taken as is. "
				"If ``False``, the minimum image convention is used to deal with particles that have crossed a periodic boundary. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("use_frame_offset", &AtomicStrainModifier::useReferenceFrameOffset, &AtomicStrainModifier::setUseReferenceFrameOffset,
				"Determines whether a sliding reference configuration is taken at a constant time offset (specified by :py:attr:`.frame_offset`) "
				"relative to the current frame. If ``False``, a constant reference configuration is used (set by the :py:attr:`.reference_frame` parameter) "
				"irrespective of the current frame."
				"\n\n"
				":Default: ``False``\n")
		.def_property("reference_frame", &AtomicStrainModifier::referenceFrameNumber, &AtomicStrainModifier::setReferenceFrameNumber,
				"The frame number to use as reference configuration if the reference data comprises multiple "
				"simulation frames. Only used if ``use_frame_offset==False``."
				"\n\n"
				":Default: 0\n")
		.def_property("frame_offset", &AtomicStrainModifier::referenceFrameOffset, &AtomicStrainModifier::setReferenceFrameOffset,
				"The relative frame offset when using a sliding reference configuration (``use_frame_offset==True``)."
				"\n\n"
				":Default: -1\n")
		.def_property("cutoff", &AtomicStrainModifier::cutoff, &AtomicStrainModifier::setCutoff,
				"Sets the distance up to which neighbor atoms are taken into account in the local strain calculation."
				"\n\n"
				":Default: 3.0\n")
		.def_property("output_deformation_gradients", &AtomicStrainModifier::calculateDeformationGradients, &AtomicStrainModifier::setCalculateDeformationGradients,
				"Controls the output of the per-particle deformation gradient tensors. If ``False``, the computed tensors are not output as a particle property to save memory."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_strain_tensors", &AtomicStrainModifier::calculateStrainTensors, &AtomicStrainModifier::setCalculateStrainTensors,
				"Controls the output of the per-particle strain tensors. If ``False``, the computed strain tensors are not output as a particle property to save memory."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_stretch_tensors", &AtomicStrainModifier::calculateStretchTensors, &AtomicStrainModifier::setCalculateStretchTensors,
				"Flag that controls the calculation of the per-particle stretch tensors."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_rotations", &AtomicStrainModifier::calculateRotations, &AtomicStrainModifier::setCalculateRotations,
				"Flag that controls the calculation of the per-particle rotations."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_nonaffine_squared_displacements", &AtomicStrainModifier::calculateNonaffineSquaredDisplacements, &AtomicStrainModifier::setCalculateNonaffineSquaredDisplacements,
				"Enables the computation of the squared magnitude of the non-affine part of the atomic displacements. The computed values are output in the ``\"Nonaffine Squared Displacement\"`` particle property."
				"\n\n"
				":Default: ``False``\n")
		.def_property("select_invalid_particles", &AtomicStrainModifier::selectInvalidParticles, &AtomicStrainModifier::setSelectInvalidParticles,
				"If ``True``, the modifier selects the particle for which the local strain tensor could not be computed (because of an insufficient number of neighbors within the cutoff)."
				"\n\n"
				":Default: ``True``\n")
		.def_property_readonly("invalid_particle_count", &AtomicStrainModifier::invalidParticleCount)
	;

	ovito_class<WignerSeitzAnalysisModifier, AsynchronousParticleModifier>(m,
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
			"\n\n"
			"**Usage example:**"
			"\n\n"
			"The ``Occupancy`` particle property generated by the Wigner-Seitz algorithm allows to select specific types of point defects, e.g. "
			"antisites, using OVITO's selection tools. One option is to use the :py:class:`SelectExpressionModifier` to pick "
			"sites with a certain occupancy. Here we exemplarily demonstrate the alternative use of a custom :py:class:`PythonScriptModifier` to "
			"select and count A-sites occupied by B-atoms in a binary system with two atom types (A=1 and B=2). "
			"\n\n"
			".. literalinclude:: ../example_snippets/wigner_seitz_example.py\n"
			)
		.def_property("reference", &WignerSeitzAnalysisModifier::referenceConfiguration, &WignerSeitzAnalysisModifier::setReferenceConfiguration,
				"A :py:class:`~ovito.io.FileSource` that provides the reference positions of particles. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a reference simulation file "
				"as shown in the code example above.")
		.def_property("eliminate_cell_deformation", &WignerSeitzAnalysisModifier::eliminateCellDeformation, &WignerSeitzAnalysisModifier::setEliminateCellDeformation,
				"Boolean flag that controls the elimination of the affine cell deformation prior to performing the analysis."
				"\n\n"
				":Default: ``False``\n")
		.def_property("use_frame_offset", &WignerSeitzAnalysisModifier::useReferenceFrameOffset, &WignerSeitzAnalysisModifier::setUseReferenceFrameOffset,
				"Determines whether a sliding reference configuration is taken at a constant time offset (specified by :py:attr:`.frame_offset`) "
				"relative to the current frame. If ``False``, a constant reference configuration is used (set by the :py:attr:`.reference_frame` parameter) "
				"irrespective of the current frame."
				"\n\n"
				":Default: ``False``\n")
		.def_property("reference_frame", &WignerSeitzAnalysisModifier::referenceFrameNumber, &WignerSeitzAnalysisModifier::setReferenceFrameNumber,
				"The frame number to use as reference configuration if the reference data comprises multiple "
				"simulation frames. Only used if ``use_frame_offset==False``."
				"\n\n"
				":Default: 0\n")
		.def_property("frame_offset", &WignerSeitzAnalysisModifier::referenceFrameOffset, &WignerSeitzAnalysisModifier::setReferenceFrameOffset,
				"The relative frame offset when using a sliding reference configuration (``use_frame_offset==True``)."
				"\n\n"
				":Default: -1\n")
		.def_property("per_type_occupancies", &WignerSeitzAnalysisModifier::perTypeOccupancy, &WignerSeitzAnalysisModifier::setPerTypeOccupancy,
				"A parameter flag that controls whether occupancy numbers are determined per particle type. "
				"\n\n"
				"If false, only the total occupancy number is computed for each reference site, which counts the number "
				"of particles that occupy the site irrespective of their types. If true, then the ``Occupancy`` property "
				"computed by the modifier becomes a vector property with one component per particle type. "
				"Each property component counts the number of particles of the corresponding type that occupy a site. For example, "
				"the property component ``Occupancy.1`` contains the number of particles of type 1 that occupy a site. "
				"\n\n"
				":Default: ``False``\n")
		.def_property_readonly("vacancy_count", &WignerSeitzAnalysisModifier::vacancyCount)
		.def_property_readonly("interstitial_count", &WignerSeitzAnalysisModifier::interstitialCount)
	;

	ovito_class<VoronoiAnalysisModifier, AsynchronousParticleModifier>(m,
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
		.def_property("only_selected", &VoronoiAnalysisModifier::onlySelected, &VoronoiAnalysisModifier::setOnlySelected,
				"Lets the modifier perform the analysis only for selected particles. Particles that are currently not selected will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("use_radii", &VoronoiAnalysisModifier::useRadii, &VoronoiAnalysisModifier::setUseRadii,
				"If ``True``, the modifier computes the poly-disperse Voronoi tessellation, which takes into account the radii of particles. "
				"Otherwise a mono-disperse Voronoi tessellation is computed, which is independent of the particle sizes. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("face_threshold", &VoronoiAnalysisModifier::faceThreshold, &VoronoiAnalysisModifier::setFaceThreshold,
				"Specifies a minimum area for faces of a Voronoi cell. The modifier will ignore any Voronoi cell faces with an area smaller than this "
				"threshold when computing the coordination number and the Voronoi index of particles."
				"\n\n"
				":Default: 0.0\n")
		.def_property("edge_threshold", &VoronoiAnalysisModifier::edgeThreshold, &VoronoiAnalysisModifier::setEdgeThreshold,
				"Specifies the minimum length an edge must have to be considered in the Voronoi index calculation. Edges that are shorter "
				"than this threshold will be ignored when counting the number of edges of a Voronoi face."
				"\n\n"
				":Default: 0.0\n")
		.def_property("compute_indices", &VoronoiAnalysisModifier::computeIndices, &VoronoiAnalysisModifier::setComputeIndices,
				"If ``True``, the modifier calculates the Voronoi indices of particles. The modifier stores the computed indices in a vector particle property "
				"named ``Voronoi Index``. The *i*-th component of this property will contain the number of faces of the "
				"Voronoi cell that have *i* edges. Thus, the first two components of the per-particle vector will always be zero, because the minimum "
				"number of edges a polygon can have is three. "
				"\n\n"
				":Default: ``False``\n")
		.def_property("generate_bonds", &VoronoiAnalysisModifier::computeBonds, &VoronoiAnalysisModifier::setComputeBonds,
				"Controls whether the modifier outputs the nearest neighbor bonds. The modifier will generate a bond "
				"for every pair of adjacent atoms that share a face of the Voronoi tessellation. "
				"No bond will be created if the face's area is below the :py:attr:`.face_threshold` or if "
				"the face has less than three edges that are longer than the :py:attr:`.edge_threshold`."
				"\n\n"
				":Default: ``False``\n")
		.def_property("edge_count", &VoronoiAnalysisModifier::edgeCount, &VoronoiAnalysisModifier::setEdgeCount,
				"Integer parameter controlling the order up to which Voronoi indices are computed by the modifier. "
				"Any Voronoi face with more edges than this maximum value will not be counted! Computed Voronoi index vectors are truncated at the index specified by :py:attr:`.edge_count`. "
				"\n\n"
				"See the ``Voronoi.max_face_order`` output attributes described above on how to avoid truncated Voronoi index vectors."
				"\n\n"
				"This parameter is ignored if :py:attr:`.compute_indices` is false."
				"\n\n"
				":Minimum: 3\n"
				":Default: 6\n")
		.def_property_readonly("max_face_order", &VoronoiAnalysisModifier::maxFaceOrder)
	;

	ovito_class<LoadTrajectoryModifier, ParticleModifier>(m,
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
		.def_property("source", &LoadTrajectoryModifier::trajectorySource, &LoadTrajectoryModifier::setTrajectorySource,
				"A :py:class:`~ovito.io.FileSource` that provides the trajectories of particles. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a simulation trajectory file "
				"as shown in the code example above.")
	;

	ovito_class<CombineParticleSetsModifier, ParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"This modifier loads a set of particles from a separate simulation file and merges them into the current dataset. "
			"\n\n"
			"Example:"
			"\n\n"
			".. literalinclude:: ../example_snippets/combine_particle_sets_modifier.py")
		.def_property("source", &CombineParticleSetsModifier::secondaryDataSource, &CombineParticleSetsModifier::setSecondaryDataSource,
				"A :py:class:`~ovito.io.FileSource` that provides the set of particles to be merged. "
				"You can call its :py:meth:`~ovito.io.FileSource.load` function to load a data file "
				"as shown in the code example above.")
	;

	auto PolyhedralTemplateMatchingModifier_py = ovito_class<PolyhedralTemplateMatchingModifier, StructureIdentificationModifier>(m,
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
		.def_property("rmsd_cutoff", &PolyhedralTemplateMatchingModifier::rmsdCutoff, &PolyhedralTemplateMatchingModifier::setRmsdCutoff,
				"The maximum allowed root mean square deviation for positive structure matches. "
				"If the cutoff is non-zero, template matches that yield a RMSD value above the cutoff are classified as \"Other\". "
				"This can be used to filter out spurious template matches (false positives). "
				"\n\n"
				"If this parameter is zero, no cutoff is applied."
				"\n\n"
				":Default: 0.0\n")
		.def_property("only_selected", &PolyhedralTemplateMatchingModifier::onlySelectedParticles, &PolyhedralTemplateMatchingModifier::setOnlySelectedParticles,
				"Lets the modifier perform the analysis only on the basis of currently selected particles. Unselected particles will be treated as if they did not exist."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_rmsd", &PolyhedralTemplateMatchingModifier::outputRmsd, &PolyhedralTemplateMatchingModifier::setOutputRmsd,
				"Boolean flag that controls whether the modifier outputs the computed per-particle RMSD values to the pipeline."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_interatomic_distance", &PolyhedralTemplateMatchingModifier::outputInteratomicDistance, &PolyhedralTemplateMatchingModifier::setOutputInteratomicDistance,
				"Boolean flag that controls whether the modifier outputs the computed per-particle interatomic distance to the pipeline."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_orientation", &PolyhedralTemplateMatchingModifier::outputOrientation, &PolyhedralTemplateMatchingModifier::setOutputOrientation,
				"Boolean flag that controls whether the modifier outputs the computed per-particle lattice orientation to the pipeline."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_deformation_gradient", &PolyhedralTemplateMatchingModifier::outputDeformationGradient, &PolyhedralTemplateMatchingModifier::setOutputDeformationGradient,
				"Boolean flag that controls whether the modifier outputs the computed per-particle elastic deformation gradients to the pipeline."
				"\n\n"
				":Default: ``False``\n")
		.def_property("output_alloy_types", &PolyhedralTemplateMatchingModifier::outputAlloyTypes, &PolyhedralTemplateMatchingModifier::setOutputAlloyTypes,
				"Boolean flag that controls whether the modifier identifies localalloy types and outputs them to the pipeline."
				"\n\n"
				":Default: ``False``\n")
	;
	expose_subobject_list<PolyhedralTemplateMatchingModifier, 
						  ParticleType,
						  StructureIdentificationModifier, 
						  &PolyhedralTemplateMatchingModifier::structureTypes>(
							  PolyhedralTemplateMatchingModifier_py, "structures", "PolyhedralTemplateMatchingStructureTypeList",
		"A list of :py:class:`~ovito.data.ParticleType` instances managed by this modifier, one for each structural type. "
		"You can adjust the color of structural types as shown in the code example above.");

	py::enum_<PolyhedralTemplateMatchingModifier::StructureType>(PolyhedralTemplateMatchingModifier_py, "Type")
		.value("OTHER", PolyhedralTemplateMatchingModifier::OTHER)
		.value("FCC", PolyhedralTemplateMatchingModifier::FCC)
		.value("HCP", PolyhedralTemplateMatchingModifier::HCP)
		.value("BCC", PolyhedralTemplateMatchingModifier::BCC)
		.value("ICO", PolyhedralTemplateMatchingModifier::ICO)
		.value("SC", PolyhedralTemplateMatchingModifier::SC)
	;

	py::enum_<PolyhedralTemplateMatchingModifier::AlloyType>(PolyhedralTemplateMatchingModifier_py, "AlloyType")
		.value("NONE", PolyhedralTemplateMatchingModifier::ALLOY_NONE)
		.value("PURE", PolyhedralTemplateMatchingModifier::ALLOY_PURE)
		.value("L10", PolyhedralTemplateMatchingModifier::ALLOY_L10)
		.value("L12_CU", PolyhedralTemplateMatchingModifier::ALLOY_L12_CU)
		.value("L12_AU", PolyhedralTemplateMatchingModifier::ALLOY_L12_AU)
		.value("B2", PolyhedralTemplateMatchingModifier::ALLOY_B2)
	;

	ovito_class<CreateIsosurfaceModifier, AsynchronousParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Generates an isosurface from a scalar field defined on a structured data grid."
			"\n\n"
			"**Modifier outputs:**"
			"\n\n"
			" * :py:attr:`DataCollection.surface <ovito.data.DataCollection.surface>` (:py:class:`~ovito.data.SurfaceMesh`):\n"
			"   The isosurface mesh generted by the modifier.\n"
			)
		.def_property("isolevel", &CreateIsosurfaceModifier::isolevel, &CreateIsosurfaceModifier::setIsolevel,
				"The value at which to create the isosurface."
				"\n\n"
				":Default: 0.0\n")
		.def_property("field_quantity", &CreateIsosurfaceModifier::sourceQuantity, &CreateIsosurfaceModifier::setSourceQuantity,
				"The name of the field quantity for which the isosurface should be constructed.")
		.def_property_readonly("mesh_display", &CreateIsosurfaceModifier::surfaceMeshDisplay,
				"The :py:class:`~ovito.vis.SurfaceMeshDisplay` controlling the visual representation of the generated isosurface.\n")
	;

	ovito_class<CoordinationPolyhedraModifier, AsynchronousParticleModifier>(m,
			":Base class: :py:class:`ovito.modifiers.Modifier`\n\n"
			"Constructs coordination polyhedra around currently selected particles. "
			"A coordination polyhedron is the convex hull spanned by the bonded neighbors of a particle. ")
		.def_property_readonly("polyhedra_display", &CoordinationPolyhedraModifier::surfaceMeshDisplay,
				"A :py:class:`~ovito.vis.SurfaceMeshDisplay` instance controlling the visual representation of the generated polyhedra.\n")
	;
	
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
