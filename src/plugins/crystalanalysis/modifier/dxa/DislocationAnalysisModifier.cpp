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
#include <plugins/particles/objects/SurfaceMesh.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/gui/properties/IntegerParameterUI.h>
#include <core/gui/properties/VariantComboBoxParameterUI.h>
#include <core/gui/properties/SubObjectParameterUI.h>
#include <plugins/particles/objects/SimulationCellObject.h>
#include <plugins/particles/objects/BondsObject.h>
#include <plugins/crystalanalysis/objects/dislocations/DislocationNetworkObject.h>
#include <plugins/crystalanalysis/objects/clusters/ClusterGraphObject.h>
#include <plugins/crystalanalysis/objects/patterns/StructurePattern.h>
#include "DislocationAnalysisModifier.h"
#include "DislocationAnalysisEngine.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(CrystalAnalysis, DislocationAnalysisModifier, StructureIdentificationModifier);
IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, DislocationAnalysisModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(DislocationAnalysisModifier, DislocationAnalysisModifierEditor);
DEFINE_FLAGS_PROPERTY_FIELD(DislocationAnalysisModifier, _inputCrystalStructure, "CrystalStructure", PROPERTY_FIELD_MEMORIZE);
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, _maxTrialCircuitSize, "MaxTrialCircuitSize");
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, _circuitStretchability, "CircuitStretchability");
DEFINE_PROPERTY_FIELD(DislocationAnalysisModifier, _outputInterfaceMesh, "OutputInterfaceMesh");
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _patternCatalog, "PatternCatalog", PatternCatalog, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _dislocationDisplay, "DislocationDisplay", DislocationDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _defectMeshDisplay, "DefectMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _interfaceMeshDisplay, "InterfaceMeshDisplay", SurfaceMeshDisplay, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _smoothDislocationsModifier, "SmoothDislocationsModifier", SmoothDislocationsModifier, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationAnalysisModifier, _smoothSurfaceModifier, "SmoothSurfaceModifier", SmoothSurfaceModifier, PROPERTY_FIELD_ALWAYS_DEEP_COPY|PROPERTY_FIELD_MEMORIZE);
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _inputCrystalStructure, "Input crystal structure");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _maxTrialCircuitSize, "Trial circuit length");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _circuitStretchability, "Circuit stretchability");
SET_PROPERTY_FIELD_LABEL(DislocationAnalysisModifier, _outputInterfaceMesh, "Output interface mesh");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
DislocationAnalysisModifier::DislocationAnalysisModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
		_inputCrystalStructure(StructureAnalysis::LATTICE_FCC), _maxTrialCircuitSize(9), _circuitStretchability(6), _outputInterfaceMesh(false)
{
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_inputCrystalStructure);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_maxTrialCircuitSize);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_circuitStretchability);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_outputInterfaceMesh);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_patternCatalog);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_dislocationDisplay);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_defectMeshDisplay);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_interfaceMeshDisplay);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_smoothDislocationsModifier);
	INIT_PROPERTY_FIELD(DislocationAnalysisModifier::_smoothSurfaceModifier);

	// Create the display objects.
	_dislocationDisplay = new DislocationDisplay(dataset);

	_defectMeshDisplay = new SurfaceMeshDisplay(dataset);
	_defectMeshDisplay->setShowCap(true);
	_defectMeshDisplay->setSmoothShading(true);
	_defectMeshDisplay->setCapTransparency(0.5);
	_defectMeshDisplay->setObjectTitle(tr("Defect mesh"));

	_interfaceMeshDisplay = new SurfaceMeshDisplay(dataset);
	_interfaceMeshDisplay->setShowCap(false);
	_interfaceMeshDisplay->setSmoothShading(false);
	_interfaceMeshDisplay->setCapTransparency(0.5);
	_interfaceMeshDisplay->setObjectTitle(tr("Interface mesh"));

	// Create the internal modifiers.
	_smoothDislocationsModifier = new SmoothDislocationsModifier(dataset);
	_smoothSurfaceModifier = new SmoothSurfaceModifier(dataset);

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
		addStructureType(stype);
		_patternCatalog->addPattern(stype);
	}

	// Create Burgers vector families.
	StructurePattern* fccPattern = _patternCatalog->structureById(StructureAnalysis::LATTICE_FCC);
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<110> (Perfect)"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0,1,1)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<112> (Shockley)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<110> (Stair-rod)"), Vector3(1.0f/6.0f, 1.0f/6.0f, 0.0f/6.0f), Color(0,0,1)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<001> (Hirth)"), Vector3(1.0f/3.0f, 0.0f, 0.0f), Color(1,1,0)));
	fccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/3<111> (Frank)"), Vector3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f), Color(1,0,1)));
	StructurePattern* bccPattern = _patternCatalog->structureById(StructureAnalysis::LATTICE_BCC);
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<111>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 1.0f/2.0f), Color(0,1,0)));
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<100>"), Vector3(1.0f, 0.0f, 0.0f), Color(1, 0.3f, 0.8f)));
	bccPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("<110>"), Vector3(1.0f, 1.0f, 0.0f), Color(0.2f, 0.5f, 1.0f)));
	StructurePattern* cubicDiaPattern = _patternCatalog->structureById(StructureAnalysis::LATTICE_CUBIC_DIAMOND);
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/2<110>"), Vector3(1.0f/2.0f, 1.0f/2.0f, 0.0f), Color(0,1,1)));
	cubicDiaPattern->addBurgersVectorFamily(new BurgersVectorFamily(dataset, tr("1/6<112>"), Vector3(1.0f/6.0f, 1.0f/6.0f, 2.0f/6.0f), Color(0,1,0)));
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void DislocationAnalysisModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute results when the parameters have changed.
	if(field == PROPERTY_FIELD(DislocationAnalysisModifier::_inputCrystalStructure)
			|| field == PROPERTY_FIELD(DislocationAnalysisModifier::_maxTrialCircuitSize)
			|| field == PROPERTY_FIELD(DislocationAnalysisModifier::_circuitStretchability)
			|| field == PROPERTY_FIELD(DislocationAnalysisModifier::_outputInterfaceMesh))
		invalidateCachedResults();
}

/******************************************************************************
* Handles reference events sent by reference targets of this object.
******************************************************************************/
bool DislocationAnalysisModifier::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	// Do not propagate messages from the attached display objects or the pattern catalog.
	if(source == defectMeshDisplay() || source == interfaceMeshDisplay() || source == dislocationDisplay())
		return false;

	return StructureIdentificationModifier::referenceEvent(source, event);
}

/******************************************************************************
* Resets the modifier's result cache.
******************************************************************************/
void DislocationAnalysisModifier::invalidateCachedResults()
{
	StructureIdentificationModifier::invalidateCachedResults();
	_defectMesh.reset();
	_interfaceMesh.reset();
	_atomClusters.reset();
	_clusterGraph.reset();
	_dislocationNetwork.reset();
	_unassignedEdges.reset();
	_segmentCounts.clear();
	_dislocationLengths.clear();
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> DislocationAnalysisModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	// Get modifier inputs.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);
	SimulationCellObject* simCell = expectSimulationCell();

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<DislocationAnalysisEngine>(validityInterval, posProperty->storage(),
			simCell->data(), inputCrystalStructure(), maxTrialCircuitSize(), circuitStretchability());
}

/******************************************************************************
* Unpacks the results of the computation engine and stores them in the modifier.
******************************************************************************/
void DislocationAnalysisModifier::transferComputationResults(ComputeEngine* engine)
{
	StructureIdentificationModifier::transferComputationResults(engine);

	DislocationAnalysisEngine* eng = static_cast<DislocationAnalysisEngine*>(engine);
	_defectMesh = eng->defectMesh();
	_isGoodEverywhere = eng->isGoodEverywhere();
	_isBadEverywhere = eng->isBadEverywhere();
	_atomClusters = eng->atomClusters();
	_clusterGraph = eng->clusterGraph();
	_dislocationNetwork = eng->dislocationNetwork();
	if(outputInterfaceMesh()) {
		_interfaceMesh = new HalfEdgeMesh<>();
		_interfaceMesh->copyFrom(eng->interfaceMesh());
	}
	else _interfaceMesh.reset();
	_simCell = eng->cell();
	_unassignedEdges = eng->elasticMapping().unassignedEdges();
	_segmentCounts.clear();
	_dislocationLengths.clear();
}

/******************************************************************************
* Lets the modifier insert the cached computation results into the
* modification pipeline.
******************************************************************************/
PipelineStatus DislocationAnalysisModifier::applyComputationResults(TimePoint time, TimeInterval& validityInterval)
{
	StructureIdentificationModifier::applyComputationResults(time, validityInterval);

	if(!_dislocationNetwork)
		throw Exception(tr("No computation results available."));

	// Output defect mesh.
	OORef<SurfaceMesh> defectMeshObj(new SurfaceMesh(dataset(), _defectMesh.data()));
	defectMeshObj->setCompletelySolid(_isBadEverywhere);
	if(smoothSurfaceModifier() && smoothSurfaceModifier()->isEnabled() && smoothSurfaceModifier()->smoothingLevel() > 0)
		defectMeshObj->smoothMesh(_simCell, smoothSurfaceModifier()->smoothingLevel());
	defectMeshObj->setDisplayObject(_defectMeshDisplay);
	output().addObject(defectMeshObj);

	// Output interface mesh.
	if(_interfaceMesh) {
		OORef<SurfaceMesh> interfaceMeshObj(new SurfaceMesh(dataset(), _interfaceMesh.data()));
		interfaceMeshObj->setCompletelySolid(_isBadEverywhere);
		interfaceMeshObj->setDisplayObject(_interfaceMeshDisplay);
		output().addObject(interfaceMeshObj);
	}

	// Output cluster graph.
	OORef<ClusterGraphObject> clusterGraphObj(new ClusterGraphObject(dataset(), _clusterGraph.data()));
	output().addObject(clusterGraphObj);

	// Output dislocations.
	OORef<DislocationNetworkObject> dislocationsObj(new DislocationNetworkObject(dataset(), _dislocationNetwork.data()));
	dislocationsObj->setDisplayObject(_dislocationDisplay);
	if(smoothDislocationsModifier() && smoothDislocationsModifier()->isEnabled())
		smoothDislocationsModifier()->smoothDislocationLines(dislocationsObj);
	output().addObject(dislocationsObj);

	// Classify, count and measure length of dislocation segments.
	_segmentCounts.clear();
	_dislocationLengths.clear();
	FloatType totalLineLength = 0;
	int totalSegmentCount = 0;
	for(DislocationSegment* segment : dislocationsObj->storage()->segments()) {
		FloatType len = segment->calculateLength();
		totalLineLength += len;
		totalSegmentCount++;

		Cluster* cluster = segment->burgersVector.cluster();
		OVITO_ASSERT(cluster != nullptr);
		StructurePattern* pattern = _patternCatalog->structureById(cluster->structure);
		if(pattern == nullptr) continue;
		BurgersVectorFamily* family = pattern->defaultBurgersVectorFamily();
		for(BurgersVectorFamily* f : pattern->burgersVectorFamilies()) {
			if(f->isMember(segment->burgersVector.localVec())) {
				family = f;
				break;
			}
		}
		_segmentCounts[family]++;
		_dislocationLengths[family] += len;
	}

	// Output pattern catalog.
	if(_patternCatalog)
		output().addObject(_patternCatalog);

	// Output particle properties.
	if(_atomClusters)
		outputStandardProperty(_atomClusters.data());

	if(_unassignedEdges) {
		OORef<BondsObject> bondsObj(new BondsObject(dataset(), _unassignedEdges.data()));
		output().addObject(bondsObj);
	}

	if(totalSegmentCount == 0)
		return PipelineStatus(PipelineStatus::Success, tr("No dislocations found"));
	else
		return PipelineStatus(PipelineStatus::Success, tr("Found %1 dislocation segments\nTotal line length: %2").arg(totalSegmentCount).arg(totalLineLength));
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void DislocationAnalysisModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create the rollout.
	QWidget* rollout = createRollout(tr("Dislocation analysis"), rolloutParams, "particles.modifiers.dislocation_analysis.html");

    QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(6);

	QGroupBox* structureBox = new QGroupBox(tr("Input crystal type"));
	layout->addWidget(structureBox);
	QVBoxLayout* sublayout1 = new QVBoxLayout(structureBox);
	sublayout1->setContentsMargins(4,4,4,4);
	VariantComboBoxParameterUI* crystalStructureUI = new VariantComboBoxParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_inputCrystalStructure));

	crystalStructureUI->comboBox()->addItem(tr("Face-centered cubic (FCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_FCC));
	crystalStructureUI->comboBox()->addItem(tr("Hexagonal close-packed (HCP)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HCP));
	crystalStructureUI->comboBox()->addItem(tr("Body-centered cubic (BCC)"), QVariant::fromValue((int)StructureAnalysis::LATTICE_BCC));
	crystalStructureUI->comboBox()->addItem(tr("Diamond cubic / Zinc blende"), QVariant::fromValue((int)StructureAnalysis::LATTICE_CUBIC_DIAMOND));
	crystalStructureUI->comboBox()->addItem(tr("Diamond hexagonal / Wurtzite"), QVariant::fromValue((int)StructureAnalysis::LATTICE_HEX_DIAMOND));
	sublayout1->addWidget(crystalStructureUI->comboBox());

	QGroupBox* dxaParamsBox = new QGroupBox(tr("DXA parameters"));
	layout->addWidget(dxaParamsBox);
	QGridLayout* sublayout = new QGridLayout(dxaParamsBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(1, 1);

	IntegerParameterUI* maxTrialCircuitSizeUI = new IntegerParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_maxTrialCircuitSize));
	sublayout->addWidget(maxTrialCircuitSizeUI->label(), 0, 0);
	sublayout->addLayout(maxTrialCircuitSizeUI->createFieldLayout(), 0, 1);
	maxTrialCircuitSizeUI->setMinValue(3);

	IntegerParameterUI* circuitStretchabilityUI = new IntegerParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_circuitStretchability));
	sublayout->addWidget(circuitStretchabilityUI->label(), 1, 0);
	sublayout->addLayout(circuitStretchabilityUI->createFieldLayout(), 1, 1);
	circuitStretchabilityUI->setMinValue(0);

	QGroupBox* advancedParamsBox = new QGroupBox(tr("Advanced settings"));
	layout->addWidget(advancedParamsBox);
	sublayout = new QGridLayout(advancedParamsBox);
	sublayout->setContentsMargins(4,4,4,4);
	sublayout->setSpacing(4);
	sublayout->setColumnStretch(0, 1);

	BooleanParameterUI* outputInterfaceMeshUI = new BooleanParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_outputInterfaceMesh));
	sublayout->addWidget(outputInterfaceMeshUI->checkBox(), 0, 0);

	// Status label.
	layout->addWidget(statusLabel());
	//statusLabel()->setMinimumHeight(60);

	// Structure list.
	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(new QLabel(tr("Atomic structure analysis:")));
	layout->addWidget(structureTypesPUI->tableWidget());

	// Open a sub-editor for the mesh display object.
	//new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_defectMeshDisplay), rolloutParams.after(rollout));

	// Open a sub-editor for the internal surface smoothing modifier.
	new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_smoothSurfaceModifier), rolloutParams.after(rollout).setTitle(tr("Post-processing")));

	// Open a sub-editor for the dislocation display object.
	//new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_dislocationDisplay), rolloutParams.after(rollout));

	// Open a sub-editor for the internal line smoothing modifier.
	new SubObjectParameterUI(this, PROPERTY_FIELD(DislocationAnalysisModifier::_smoothDislocationsModifier), rolloutParams.after(rollout).setTitle(tr("Post-processing")));

	// Burgers vector list.
	_burgersFamilyListUI.reset(new DislocationTypeListParameterUI());
	layout->addSpacing(10);
	layout->addWidget(new QLabel(tr("Dislocation analysis:")));
	layout->addWidget(_burgersFamilyListUI->tableWidget());
	connect(this, &PropertiesEditor::contentsChanged, [this](RefTarget* editObject) {
		_burgersFamilyListUI->setModifier(static_object_cast<DislocationAnalysisModifier>(editObject));
	});
}

IMPLEMENT_OVITO_OBJECT(CrystalAnalysis, DislocationTypeListParameterUI, RefTargetListParameterUI);
DEFINE_FLAGS_REFERENCE_FIELD(DislocationTypeListParameterUI, _modifier, "Modifier", DislocationAnalysisModifier, PROPERTY_FIELD_NO_UNDO | PROPERTY_FIELD_WEAK_REF | PROPERTY_FIELD_NO_CHANGE_MESSAGE);

/******************************************************************************
* Constructor.
******************************************************************************/
DislocationTypeListParameterUI::DislocationTypeListParameterUI(QObject* parent)
	: RefTargetListParameterUI(parent, PROPERTY_FIELD(StructurePattern::_burgersVectorFamilies))
{
	INIT_PROPERTY_FIELD(DislocationTypeListParameterUI::_modifier);

	connect(tableWidget(160), &QTableWidget::doubleClicked, this, &DislocationTypeListParameterUI::onDoubleClickDislocationType);
	tableWidget()->setAutoScroll(false);
}

/******************************************************************************
* Sets the modifier whose results should be displayed.
******************************************************************************/
void DislocationTypeListParameterUI::setModifier(DislocationAnalysisModifier* modifier)
{
	if(modifier)
		setEditObject(modifier->patternCatalog()->structureById(modifier->inputCrystalStructure()));
	else
		setEditObject(nullptr);
	_modifier = modifier;
}

/******************************************************************************
* Returns a data item from the list data model.
******************************************************************************/
QVariant DislocationTypeListParameterUI::getItemData(RefTarget* target, const QModelIndex& index, int role)
{
	BurgersVectorFamily* family = dynamic_object_cast<BurgersVectorFamily>(target);
	if(family && _modifier) {
		if(role == Qt::DisplayRole) {
			if(index.column() == 1) {
				return family->name();
			}
			else if(index.column() == 2) {
				auto entry = _modifier->segmentCounts().find(family);
				if(entry != _modifier->segmentCounts().end())
					return entry->second;
			}
			else if(index.column() == 3) {
				auto entry = _modifier->dislocationLengths().find(family);
				if(entry != _modifier->dislocationLengths().end())
					return QString::number(entry->second);
			}
		}
		else if(role == Qt::DecorationRole) {
			if(index.column() == 0)
				return (QColor)family->color();
		}
	}
	return QVariant();
}

/******************************************************************************
* This method is called when a reference target changes.
******************************************************************************/
bool DislocationTypeListParameterUI::referenceEvent(RefTarget* source, ReferenceEvent* event)
{
	if(source == _modifier && event->type() == ReferenceEvent::ObjectStatusChanged) {
		// Update the result columns.
		_model->updateColumns(2, 3);
	}
	return RefTargetListParameterUI::referenceEvent(source, event);
}

/******************************************************************************
* Is called when the user has double-clicked on one of the dislocation
* types in the list widget.
******************************************************************************/
void DislocationTypeListParameterUI::onDoubleClickDislocationType(const QModelIndex& index)
{
	// Let the user select a color for the structure type.
	BurgersVectorFamily* family = static_object_cast<BurgersVectorFamily>(selectedObject());
	if(!family) return;

	QColor oldColor = (QColor)family->color();
	QColor newColor = QColorDialog::getColor(oldColor, _viewWidget);
	if(!newColor.isValid() || newColor == oldColor) return;

	undoableTransaction(tr("Change dislocation type color"), [family, newColor]() {
		family->setColor(Color(newColor));
	});
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

