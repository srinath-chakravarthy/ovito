///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2016) Alexander Stukowski
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

#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/export/FileColumnParticleExporter.h>
#include <core/animation/AnimationSettings.h>
#include <core/dataset/DataSetContainer.h>
#include <gui/utilities/concurrent/ProgressDialog.h>
#include "FileColumnParticleExporterEditor.h"

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Export)

IMPLEMENT_OVITO_OBJECT(FileColumnParticleExporterEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(FileColumnParticleExporter, FileColumnParticleExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void FileColumnParticleExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Particle properties"), rolloutParams);
	QGridLayout* columnsGroupBoxLayout = new QGridLayout(rollout);

	_columnMappingWidget = new QListWidget();
	columnsGroupBoxLayout->addWidget(_columnMappingWidget, 0, 0, 5, 1);
	columnsGroupBoxLayout->setRowStretch(2, 1);

	QPushButton* moveUpButton = new QPushButton(tr("Move up"), rollout);
	QPushButton* moveDownButton = new QPushButton(tr("Move down"), rollout);
	QPushButton* selectAllButton = new QPushButton(tr("Select all"), rollout);
	QPushButton* selectNoneButton = new QPushButton(tr("Unselect all"), rollout);
	columnsGroupBoxLayout->addWidget(moveUpButton, 0, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(moveDownButton, 1, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(selectAllButton, 3, 1, 1, 1);
	columnsGroupBoxLayout->addWidget(selectNoneButton, 4, 1, 1, 1);
	moveUpButton->setEnabled(_columnMappingWidget->currentRow() >= 1);
	moveDownButton->setEnabled(_columnMappingWidget->currentRow() >= 0 && _columnMappingWidget->currentRow() < _columnMappingWidget->count() - 1);

	connect(_columnMappingWidget, &QListWidget::itemSelectionChanged, [moveUpButton, moveDownButton, this]() {
		moveUpButton->setEnabled(_columnMappingWidget->currentRow() >= 1);
		moveDownButton->setEnabled(_columnMappingWidget->currentRow() >= 0 && _columnMappingWidget->currentRow() < _columnMappingWidget->count() - 1);
	});

	connect(moveUpButton, &QPushButton::clicked, [this]() {
		int currentIndex = _columnMappingWidget->currentRow();
		QListWidgetItem* currentItem = _columnMappingWidget->takeItem(currentIndex);
		_columnMappingWidget->insertItem(currentIndex - 1, currentItem);
		_columnMappingWidget->setCurrentRow(currentIndex - 1);
		onListChanged();
	});

	connect(moveDownButton, &QPushButton::clicked, [this]() {
		int currentIndex = _columnMappingWidget->currentRow();
		QListWidgetItem* currentItem = _columnMappingWidget->takeItem(currentIndex);
		_columnMappingWidget->insertItem(currentIndex + 1, currentItem);
		_columnMappingWidget->setCurrentRow(currentIndex + 1);
		onListChanged();
	});

	connect(selectAllButton, &QPushButton::clicked, [this]() {
		for(int index = 0; index < _columnMappingWidget->count(); index++)
			_columnMappingWidget->item(index)->setCheckState(Qt::Checked);
	});

	connect(selectNoneButton, &QPushButton::clicked, [this]() {
		for(int index = 0; index < _columnMappingWidget->count(); index++)
			_columnMappingWidget->item(index)->setCheckState(Qt::Unchecked);
	});

	connect(this, &PropertiesEditor::contentsReplaced, this, &FileColumnParticleExporterEditor::onContentsReplaced);
	connect(_columnMappingWidget, &QListWidget::itemChanged, this, &FileColumnParticleExporterEditor::onListChanged);
}

/******************************************************************************
* Is called when the exporter is associated with the editor.
******************************************************************************/
void FileColumnParticleExporterEditor::onContentsReplaced(Ovito::RefTarget* newEditObject)
{
	_columnMappingWidget->clear();

	// Retrieve the data to be exported.
	FileColumnParticleExporter* particleExporter = dynamic_object_cast<FileColumnParticleExporter>(newEditObject);
	if(!particleExporter || particleExporter->outputData().empty()) return;

	for(SceneNode* node : particleExporter->outputData()) {
		try {
			ProgressDialog progressDialog(container(), particleExporter->dataset()->container()->taskManager());
			PipelineFlowState state;
			if(!particleExporter->getParticleData(node, node->dataset()->animationSettings()->time(), state, progressDialog.taskManager()))
				continue;
			bool hasParticleIdentifiers = false;
			for(DataObject* o : state.objects()) {
				ParticlePropertyObject* property = dynamic_object_cast<ParticlePropertyObject>(o);
				if(!property) continue;
				if(property->componentCount() == 1) {
					insertPropertyItem(ParticlePropertyReference(property), property->name(), particleExporter->columnMapping());
					if(property->type() == ParticleProperty::IdentifierProperty)
						hasParticleIdentifiers = true;
				}
				else {
					for(int vectorComponent = 0; vectorComponent < (int)property->componentCount(); vectorComponent++) {
						QString propertyName = property->nameWithComponent(vectorComponent);
						ParticlePropertyReference propRef(property, vectorComponent);
						insertPropertyItem(propRef, propertyName, particleExporter->columnMapping());
					}
				}
			}
			if(!hasParticleIdentifiers)
				insertPropertyItem(ParticleProperty::IdentifierProperty, tr("Particle index"), particleExporter->columnMapping());
			break;
		}
		catch(const Exception& ex) {
			// Ignore errors.
			_columnMappingWidget->addItems(ex.messages());
		}
	}

	// Update the settings stored in the exporter to match the current settings in the UI.
	saveChanges(particleExporter);	
}

/******************************************************************************
* Populates the column mapping list box with an entry.
******************************************************************************/
void FileColumnParticleExporterEditor::insertPropertyItem(ParticlePropertyReference propRef, const QString& displayName, const OutputColumnMapping& columnMapping)
{
	QListWidgetItem* item = new QListWidgetItem(displayName);
	item->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren));
	item->setCheckState(Qt::Unchecked);
	item->setData(Qt::UserRole, qVariantFromValue(propRef));
	int sortKey = columnMapping.size();

	for(int c = 0; c < (int)columnMapping.size(); c++) {
		if(columnMapping[c] == propRef) {
			item->setCheckState(Qt::Checked);
			sortKey = c;
			break;
		}
	}

	item->setData(Qt::InitialSortOrderRole, sortKey);
	if(sortKey < (int)columnMapping.size()) {
		int insertIndex = 0;
		for(; insertIndex < _columnMappingWidget->count(); insertIndex++) {
			int k = _columnMappingWidget->item(insertIndex)->data(Qt::InitialSortOrderRole).value<int>();
			if(sortKey < k)
				break;
		}
		_columnMappingWidget->insertItem(insertIndex, item);
	}
	else {
		_columnMappingWidget->addItem(item);
	}
}

/******************************************************************************
* This writes the settings made in the UI back to the exporter.
******************************************************************************/
void FileColumnParticleExporterEditor::saveChanges(FileColumnParticleExporter* particleExporter)
{
	OutputColumnMapping newMapping;
	for(int index = 0; index < _columnMappingWidget->count(); index++) {
		if(_columnMappingWidget->item(index)->checkState() == Qt::Checked) {
			newMapping.push_back(_columnMappingWidget->item(index)->data(Qt::UserRole).value<ParticlePropertyReference>());
		}
	}
	particleExporter->setColumnMapping(newMapping);
}

/******************************************************************************
* Is called when the user checked/unchecked an item.
******************************************************************************/
void FileColumnParticleExporterEditor::onListChanged()
{
	FileColumnParticleExporter* particleExporter = dynamic_object_cast<FileColumnParticleExporter>(editObject());
	if(!particleExporter) return;

	// Stores current settings in exporter object.
	saveChanges(particleExporter);

	// Remember the output column mapping for the next time.
	QSettings settings;
	settings.beginGroup("exporter/particles/");
	settings.setValue("columnmapping", particleExporter->columnMapping().toByteArray());
	settings.endGroup();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace
