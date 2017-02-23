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

#include <gui/GUI.h>
#include <core/dataset/importexport/AttributeFileExporter.h>
#include <core/dataset/DataSetContainer.h>
#include <core/animation/AnimationSettings.h>
#include <gui/utilities/concurrent/ProgressDialog.h>
#include "AttributeFileExporterEditor.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(DataIO)

IMPLEMENT_OVITO_OBJECT(AttributeFileExporterEditor, PropertiesEditor);
SET_OVITO_OBJECT_EDITOR(AttributeFileExporter, AttributeFileExporterEditor);

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void AttributeFileExporterEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("Attributes to export"), rolloutParams);
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

	connect(this, &PropertiesEditor::contentsReplaced, this, &AttributeFileExporterEditor::onContentsReplaced);
	connect(_columnMappingWidget, &QListWidget::itemChanged, this, &AttributeFileExporterEditor::onListChanged);
}

/******************************************************************************
* Is called when the exporter is associated with the editor.
******************************************************************************/
void AttributeFileExporterEditor::onContentsReplaced(Ovito::RefTarget* newEditObject)
{
	_columnMappingWidget->clear();

	// Retrieve the data to be exported.
	AttributeFileExporter* exporter = dynamic_object_cast<AttributeFileExporter>(newEditObject);
	if(!exporter || exporter->outputData().empty()) return;

	for(SceneNode* node : exporter->outputData()) {
		try {
			QVariantMap attrMap;
			ProgressDialog progressDialog(container(), exporter->dataset()->container()->taskManager());
			if(!exporter->getAttributes(node, node->dataset()->animationSettings()->time(), attrMap, progressDialog.taskManager()))
				continue;
			for(const QString& attrName : attrMap.keys())
				insertAttributeItem(attrName, exporter->attributesToExport());
			break;
		}
		catch(const Exception& ex) {
			// Ignore errors.
			_columnMappingWidget->addItems(ex.messages());
		}
	}
}

/******************************************************************************
* Populates the list box with an entry.
******************************************************************************/
void AttributeFileExporterEditor::insertAttributeItem(const QString& displayName, const QStringList& selectedAttributeList)
{
	QListWidgetItem* item = new QListWidgetItem(displayName);
	item->setFlags(Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren));
	item->setCheckState(Qt::Unchecked);
	int sortKey = selectedAttributeList.size();

	for(int c = 0; c < selectedAttributeList.size(); c++) {
		if(selectedAttributeList[c] == displayName) {
			item->setCheckState(Qt::Checked);
			sortKey = c;
			break;
		}
	}

	item->setData(Qt::InitialSortOrderRole, sortKey);
	if(sortKey < selectedAttributeList.size()) {
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
* Is called when the user checked/unchecked an item.
******************************************************************************/
void AttributeFileExporterEditor::onListChanged()
{
	AttributeFileExporter* exporter = dynamic_object_cast<AttributeFileExporter>(editObject());
	if(!exporter) return;

	QStringList newAttributeList;
	for(int index = 0; index < _columnMappingWidget->count(); index++) {
		if(_columnMappingWidget->item(index)->checkState() == Qt::Checked) {
			newAttributeList.push_back(_columnMappingWidget->item(index)->text());
		}
	}
	exporter->setAttributesToExport(newAttributeList);

	// Remember the selection for next time.
	QSettings settings;
	settings.beginGroup("exporter/attributes/");
	settings.setValue("attrlist", newAttributeList);
	settings.endGroup();
}

OVITO_END_INLINE_NAMESPACE
}	// End of namespace
