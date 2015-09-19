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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/gui/properties/BooleanRadioButtonParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <core/gui/properties/VariantComboBoxParameterUI.h>
#include <core/gui/properties/SubObjectParameterUI.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include "ElasticStrainModifier.h"
#include "ElasticStrainEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, ElasticStrainModifier, StructureIdentificationModifier);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, ElasticStrainModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(ElasticStrainModifier, ElasticStrainModifierEditor);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _inputCrystalStructure, "CrystalStructure", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _calculateDeformationGradients, "CalculateDeformationGradients", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _calculateStrainTensors, "CalculateStrainTensors", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _latticeConstant, "LatticeConstant", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _caRatio, "CtoARatio", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_PROPERTY_FIELD(ElasticStrainModifier, _pushStrainTensorsForward, "PushStrainTensorsForward", PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(ElasticStrainModifier, _patternCatalog, "PatternCatalog", PatternCatalog, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _calculateDeformationGradients, "Output deformation gradient tensors");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _calculateStrainTensors, "Output strain tensors");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _latticeConstant, "Lattice constant");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _caRatio, "c/a ratio");
SET_PROPERTY_FIELD_LABEL(ElasticStrainModifier, _pushStrainTensorsForward, "Strain tensor in spatial frame (push-forward)");
SET_PROPERTY_FIELD_UNITS(ElasticStrainModifier, _latticeConstant, WorldParameterUnit);

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
ElasticStrainModifier::ElasticStrainModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_inputCrystalStructure(StructureAnalysis::LATTICE_FCC), _calculateDeformationGradients(false), _calculateStrainTensors(true),
		_latticeConstant(1), _caRatio(sqrt(8.0/3.0)), _pushStrainTensorsForward(true)
{
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_inputCrystalStructure);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_patternCatalog);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_calculateDeformationGradients);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_calculateStrainTensors);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_latticeConstant);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_caRatio);
	INIT_PROPERTY_FIELD(ElasticStrainModifier::_pushStrainTensorsForward);

	// Create pattern catalog.
	_patternCatalog = new PatternCatalog(dataset);

	// Create the structure types.
	ParticleTypeProperty::PredefinedStructureType predefTypes[] = {
			ParticleTypeProperty::PredefinedStructureType::OTHER,
			ParticleTypeProperty::PredefinedStructureType::FCC,
			ParticleTypeProperty::PredefinedStructureType::HCP,
			ParticleTypeProperty::PredefinedStructureType::BCC,
			ParticleTypeProperty::PredefinedStructureType::CUBIC_DIAMOND,
			ParticleTypeProperty::PredefinedStructureType::HEX_DIAMOND
	};
	OVITO_STATIC_ASSERT(sizeof(predefTypes)/sizeof(predefTypes[0]) == StructureAnalysis::NUM_LATTICE_TYPES);
	for(int id = 0; id < StructureAnalysis::NUM_LATTICE_TYPES; id++) {
		OORef<StructurePattern> stype(new StructurePattern(dataset));
		stype->setId(id);
		stype->setName(ParticleTypeProperty::getPredefinedStructureTypeName(predefTypes[id]));
		stype->setColor(ParticleTypeProperty::getDefaultParticleColor(ParticleProperty::StructureTypeProperty, stype->name(), id));
		stype->setStructureType(StructurePattern::Lattice);
		addStructureType(stype);
		_patternCatalog->addPattern(stype);
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void ElasticStrainModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(ElasticStrainModifier::_inputCrystalStructure) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_calculateDeformationGradients) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_calculateStrainTensors) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_latticeConstant) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_caRatio) ||
			field == PROPERTY_FIELD(ElasticStrainModifier::_pushStrainTensorsForward))
		invalidateCachedResults();
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void ElasticStrainModifier::invalidateCachedResults()
{
	StructureIdentificationModifier::invalidateCachedResults();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> ElasticStrainModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Build list of preferred crystal orientations.
	std::vector<Matrix3> preferredCrystalOrientations;
	if(inputCrystalStructure() == StructureAnalysis::LATTICE_FCC || inputCrystalStructure() == StructureAnalysis::LATTICE_BCC || inputCrystalStructure() == StructureAnalysis::LATTICE_CUBIC_DIAMOND) {
		preferredCrystalOrientations.push_back(Matrix3::Identity());
	}

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<ElasticStrainEngine>(validityInterval, posProperty->storage(),
			simCell->data(), inputCrystalStructure(), std::move(preferredCrystalOrientations),
			calculateDeformationGradients(), calculateStrainTensors(),
			latticeConstant(), axialRatio(), pushStrainTensorsForward());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void ElasticStrainModifier::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);

	ElasticStrainEngine* eng = static_cast<ElasticStrainEngine*>(engine);
	_atomClusters = eng->atomClusters();
	_clusterGraph = eng->clusterGraph();
	_strainTensors = eng->strainTensors();
	_deformationGradients = eng->deformationGradients();
	_volumetricStrainValues = eng->volumetricStrains();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus ElasticStrainModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	if(!_volumetricStrainValues)
		throw Exception(tr("No computation results available."));

	if(outputParticleCount() != _volumetricStrainValues->size())
		throw Exception(tr("The number of input particles has changed. The stored results have become invalid."));

	if(_clusterGraph) {
		// Output cluster graph.
		OORef<ClusterGraphObject> clusterGraphObj(new ClusterGraphObject(dataset(), _clusterGraph.data()));
		output().addObject(clusterGraphObj);
	}

	// Output pattern catalog.
	if(_patternCatalog) {
		output().addObject(_patternCatalog);
	}

	// Output particle properties.
	if(_atomClusters && _atomClusters->size() == outputParticleCount())
		outputStandardProperty(_atomClusters.data());

	if(calculateStrainTensors() && _strainTensors && _strainTensors->size() == outputParticleCount())
		outputCustomProperty(_strainTensors.data());

	if(calculateDeformationGradients() && _deformationGradients && _deformationGradients->size() == outputParticleCount())
		outputCustomProperty(_deformationGradients.data());

	if(_volumetricStrainValues && _volumetricStrainValues->size() == outputParticleCount())
		outputCustomProperty(_volumetricStrainValues.data());

	return PipelineStatus::Success;
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void ElasticStrainModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Elastic strain calculation"), rolloutParams, "particles.modifiers.elastic_strain.html");

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* structureBox = new QGroupBox(tr("Input crystal"));
	layout->addWidget(structureBox);
	QGridLayout* sublayout1 = new QGridLayout(structureBox);
	sublayout1->setContentsMargins(4,4,4,4);
	sublayout1->setSpacing(4);
	sublayout1->setColumnStretch(1,1);
	VariantComboBoxParameterUI* crystalStructureUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_inputCrystalStructure));

	crystalStructureUI->comboBox()->addItem(tr("Face-centered cubic (FCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_FCC));
	crystalStructureUI->comboBox()->addItem(tr("Hexagonal close-packed (HCP)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HCP));
	crystalStructureUI->comboBox()->addItem(tr("Body-centered cubic (BCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_BCC));
	crystalStructureUI->comboBox()->addItem(tr("Diamond cubic / Zinc blende"), QVariant::fromValue((int)StructureAnalysis::LATTICE_CUBIC_DIAMOND));
	crystalStructureUI->comboBox()->addItem(tr("Diamond hexagonal / Wurtzite"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HEX_DIAMOND));
	sublayout1->addWidget(crystalStructureUI->comboBox(), 0, 0, 1, 2);

	FloatParameterUI* latticeConstantUI = new FloatParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_latticeConstant));
	sublayout1->addWidget(latticeConstantUI->label(), 1, 0);
	sublayout1->addLayout(latticeConstantUI->createFieldLayout(), 1, 1);
	latticeConstantUI->setMinValue(0);

	_caRatioUI = new FloatParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_caRatio));
	sublayout1->addWidget(_caRatioUI->label(), 2, 0);
	sublayout1->addLayout(_caRatioUI->createFieldLayout(), 2, 1);
	_caRatioUI->setMinValue(0);

	QGroupBox* outputParamsBox = new QGroupBox(tr("Output settings"));
	layout->addWidget(outputParamsBox);
	QGridLayout* sublayout2 = new QGridLayout(outputParamsBox);
	sublayout2->setContentsMargins(4,4,4,4);
	sublayout2->setSpacing(4);
	sublayout2->setColumnStretch(1, 1);
	sublayout2->setColumnMinimumWidth(0, 12);

	BooleanParameterUI* outputStrainTensorsUI = new BooleanParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_calculateStrainTensors));
	sublayout2->addWidget(outputStrainTensorsUI->checkBox(), 0, 0, 1, 2);

	BooleanRadioButtonParameterUI* pushStrainTensorsForwardUI = new BooleanRadioButtonParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_pushStrainTensorsForward));
	pushStrainTensorsForwardUI->buttonTrue()->setText(tr("in spatial frame"));
	pushStrainTensorsForwardUI->buttonFalse()->setText(tr("in lattice frame"));
	sublayout2->addWidget(pushStrainTensorsForwardUI->buttonTrue(), 1, 1);
	sublayout2->addWidget(pushStrainTensorsForwardUI->buttonFalse(), 2, 1);

	pushStrainTensorsForwardUI->setEnabled(false);
	connect(outputStrainTensorsUI->checkBox(), &QCheckBox::toggled, pushStrainTensorsForwardUI, &BooleanRadioButtonParameterUI::setEnabled);

	BooleanParameterUI* outputDeformationGradientsUI = new BooleanParameterUI(this, PROPERTY_FIELD(ElasticStrainModifier::_calculateDeformationGradients));
	sublayout2->addWidget(outputDeformationGradientsUI->checkBox(), 3, 0, 1, 2);

	// Status label.
	layout->addWidget(statusLabel());

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(structureTypesPUI->tableWidget());

	connect(this, &PropertiesEditor::contentsChanged, this, &ElasticStrainModifierEditor::modifierChanged);
}

/******************************************************************************
* Is called each time the parameters of the modifier have changed.
******************************************************************************/
void ElasticStrainModifierEditor::modifierChanged(RefTarget* editObject)
{
	ElasticStrainModifier* modifier = static_object_cast<ElasticStrainModifier>(editObject);
	_caRatioUI->setEnabled(modifier &&
			(modifier->inputCrystalStructure() == StructureAnalysis::LATTICE_HCP ||
			 modifier->inputCrystalStructure() == StructureAnalysis::LATTICE_HEX_DIAMOND));
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

