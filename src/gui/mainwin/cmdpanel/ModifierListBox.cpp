///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2017) Alexander Stukowski
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
#include <core/plugins/PluginManager.h>
#include <core/scene/pipeline/Modifier.h>
#include <core/scene/pipeline/PipelineObject.h>
#include <core/scene/ObjectNode.h>
#include <core/dataset/DataSetContainer.h>
#include <core/animation/AnimationSettings.h>
#include "ModifierListBox.h"
#include "ModificationListModel.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/******************************************************************************
* Initializes the widget.
******************************************************************************/
ModifierListBox::ModifierListBox(QWidget* parent, ModificationListModel* modificationList) : QComboBox(parent),
		_modificationList(modificationList)
{
	setSizeAdjustPolicy(QComboBox::AdjustToContents);

	// A category of modifiers.
	struct ModifierCategory {
		QString name;
		QVector<const OvitoObjectType*> modifierClasses;
	};
	
	QVector<ModifierCategory> modifierCategories;
	ModifierCategory otherCategory;
	otherCategory.name = tr("Others");

	// Retrieve all installed modifier classes.
	for(const OvitoObjectType* clazz : PluginManager::instance().listClasses(Modifier::OOType)) {
		// Sort modifiers into categories.
		if(clazz->qtMetaObject()) {
			int infoIndex = clazz->qtMetaObject()->indexOfClassInfo("ModifierCategory");
			if(infoIndex != -1) {
				QString categoryName = QString::fromLocal8Bit(clazz->qtMetaObject()->classInfo(infoIndex).value());
				// Check if category has already been created.
				bool found = false;
				for(auto& category : modifierCategories) {
					if(category.name == categoryName) {
						category.modifierClasses.push_back(clazz);
						found = true;
						break;
					}
				}
				// Create a new category.
				if(!found) {
					ModifierCategory category;
					category.name = categoryName;
					category.modifierClasses.push_back(clazz);
					modifierCategories.push_back(category);
				}
				continue;
			}
		}

		// Insert modifiers that don't have category information to the "Other" category.
		otherCategory.modifierClasses.push_back(clazz);
	}

	// Sort category list.
	std::sort(modifierCategories.begin(), modifierCategories.end(), [](const ModifierCategory& a, const ModifierCategory& b) {
		return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
	} );

	// Assign modifiers that haven't been assigned to a category yet to the "Other" category.
	if(!otherCategory.modifierClasses.isEmpty())
		modifierCategories.push_back(otherCategory);

	// Sort modifiers in each category.
	for(auto& category : modifierCategories) {
		std::sort(category.modifierClasses.begin(), category.modifierClasses.end(), [](const OvitoObjectType* a, const OvitoObjectType* b) {
			return QString::compare(a->displayName(), b->displayName(), Qt::CaseInsensitive) < 0;
		} );
	}

	// Define font colors etc.
	QFont categoryFont = font();
	categoryFont.setBold(true);
	if(categoryFont.pixelSize() < 0)
		categoryFont.setPointSize(categoryFont.pointSize() * 4 / 5);
	else
		categoryFont.setPixelSize(categoryFont.pixelSize() * 4 / 5);
	QBrush categoryBackgroundBrush(Qt::lightGray, Qt::Dense4Pattern);
	QBrush categoryForegroundBrush(Qt::blue);

	// Populate item model.
	_model = new QStandardItemModel(this);

	// Lists starts with the special "Add modification...", which is used just
	// as a label for the combo box.
	QStandardItem* titleItem = new QStandardItem(tr("Add modification..."));
	titleItem->setFlags(Qt::ItemIsEnabled);
	_model->appendRow(titleItem);

	QStandardItem* mruListItem = new QStandardItem(tr("Most recently used modifiers"));
	mruListItem->setFont(categoryFont);
	mruListItem->setBackground(categoryBackgroundBrush);
	mruListItem->setForeground(categoryForegroundBrush);
	mruListItem->setFlags(Qt::ItemIsEnabled);
	mruListItem->setTextAlignment(Qt::AlignCenter);
	_model->appendRow(mruListItem);

	// Create items for all modifiers and the category titles.
	for(const ModifierCategory& category : modifierCategories) {
		if(category.modifierClasses.empty()) continue;
		
		QStandardItem* categoryItem = new QStandardItem(category.name);
		categoryItem->setFont(categoryFont);
		categoryItem->setBackground(categoryBackgroundBrush);
		categoryItem->setForeground(categoryForegroundBrush);
		categoryItem->setFlags(Qt::ItemIsEnabled);
		categoryItem->setTextAlignment(Qt::AlignCenter);
		_model->appendRow(categoryItem);

		for(const OvitoObjectType* descriptor : category.modifierClasses) {
			QStandardItem* modifierItem = new QStandardItem("   " + descriptor->displayName());
			modifierItem->setData(QVariant::fromValue((void*)descriptor), Qt::UserRole);
			_model->appendRow(modifierItem);
			_modifierItems.push_back(modifierItem);
		}
	}

	// Create category for custom modifiers.
	QStandardItem* categoryItem = new QStandardItem(tr("Custom modifier presets"));
	categoryItem->setFont(categoryFont);
	categoryItem->setBackground(categoryBackgroundBrush);
	categoryItem->setForeground(categoryForegroundBrush);
	categoryItem->setFlags(Qt::ItemIsEnabled);
	categoryItem->setTextAlignment(Qt::AlignCenter);
	_model->appendRow(categoryItem);

	// Append the "Show all modifiers" item at the end of the list.
	QStandardItem* showAllItem = new QStandardItem(tr("Show all modifiers..."));
	QFont boldFont = font();
	boldFont.setBold(true);
	showAllItem->setFont(boldFont);
	showAllItem->setTextAlignment(Qt::AlignCenter);
	_model->appendRow(showAllItem);

	// Filler item to workaround bug in Qt which doesnt fully show all items in the drop-down menu. 
	QStandardItem* fillerItem = new QStandardItem();
	fillerItem->setFlags(Qt::ItemIsEnabled);
	_model->appendRow(fillerItem);

	// Expand list when the "Show all modifiers" entry is selected and update MRU list.
    connect(this, (void (QComboBox::*)(int))&QComboBox::activated, this, [this](int index) {
		if(!showAllModifiers() && index >= count() - 2 && !itemData(index).isValid()) {
			_showAllModifiers = true;
			showPopup();
		}
		else {
			if(itemData(index).isValid()) {
				updateMRUList(itemText(index));
			}
		}
	}, Qt::QueuedConnection);

	// Set up filter model.
	class MyFilterModel : public QSortFilterProxyModel {
	public:
		MyFilterModel(QObject* parent) : QSortFilterProxyModel(parent) {}
	protected:
		virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override {
			// Delegate to parent class.
			return static_cast<ModifierListBox*>(parent())->filterAcceptsRow(source_row, source_parent);
		}
		virtual bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override {
			// Delegate to parent class.
			return static_cast<ModifierListBox*>(parent())->filterSortLessThan(source_left, source_right);
		}
	};
	_filterModel = new MyFilterModel(this);
	_filterModel->setDynamicSortFilter(false);
	_filterModel->sort(0);
	_filterModel->setSourceModel(_model);
	setModel(_filterModel);
}

/******************************************************************************
* Filters the full list of modifiers to show only most recently used ones.
******************************************************************************/
bool ModifierListBox::filterAcceptsRow(int source_row, const QModelIndex& source_parent)
{
	if(showAllModifiers()) {
		// Don't show the "Most recently used" entry if all modifier are shown.
		if(source_row == 1)
			return false;
		// Don't show the "Show all modifiers" entry if all are already shown.
		if(source_row >= _model->rowCount(source_parent) - 2) 
			return false; 
		// Don't show the "Custom modifier presets" category if there are no custom modifiers.
		if(_numCustomModifiers == 0 && source_row == _model->rowCount(source_parent) - 3)
			return false;
		return true;
	}
	// Always show the "Add modification..." entry.
	if(source_row == 0) 
		return true; 

	// Always show the "Most recently used" entry.
	if(source_row == 1) 
		return true; 

	// Show the "Show all modifiers" entry
	if(source_row >= _model->rowCount(source_parent) - 2) 
		return true;

	// Don't show modifier categories.
	if(!_model->index(source_row, 0, source_parent).data(Qt::UserRole).isValid())
		return false;

	// Only show modifiers from MRU list.
	QString modifierName = _model->index(source_row, 0, source_parent).data().toString();
	return _mostRecentlyUsedModifiers.contains(modifierName); 
}

/******************************************************************************
* Determines the sort order of the modifier list.
******************************************************************************/
bool ModifierListBox::filterSortLessThan(const QModelIndex& source_left, const QModelIndex& source_right)
{
	if(showAllModifiers() || source_left.row() <= 1 || source_right.row() <= 1 || source_left.row() >= _model->rowCount()-2 || source_right.row() >= _model->rowCount()-2) {
		return source_left.row() < source_right.row();
	}
	else {
		return source_left.data().toString().localeAwareCompare(source_right.data().toString()) < 0;
	}
}

/******************************************************************************
* Updates the MRU list after the user has selected a modifier.
******************************************************************************/
void ModifierListBox::updateMRUList(const QString& selectedModifierName)
{
	QSettings settings;
	settings.beginGroup("core/modifier/mru/");
	if(!settings.value("enable_mru", true).toBool())
		return;
	
	int index = _mostRecentlyUsedModifiers.indexOf(selectedModifierName);
	if(index >= 0) {
		_mostRecentlyUsedModifiers.removeAt(index);
	}
	else if(_mostRecentlyUsedModifiers.size() >= _maxMRUSize) {
		_mostRecentlyUsedModifiers.pop_back();
	}
	_mostRecentlyUsedModifiers.push_front(selectedModifierName);

	// Store MRU list in application settings.
	settings.setValue("list", QVariant::fromValue(_mostRecentlyUsedModifiers));

	// Update list of modifiers shown in the combo box.
	_filterModel->invalidate();
}

/******************************************************************************
* Updates the list box of modifier classes that can be applied to the current selected
* item in the modification list.
******************************************************************************/
void ModifierListBox::updateAvailableModifiers()
{
	// Always select the "Add modification..." entry by default.
	setCurrentIndex(0);

	// Should we show MRU list?
	QSettings settings;
	settings.beginGroup("core/modifier/mru/");
	if(settings.value("enable_mru", true).toBool())
		_mostRecentlyUsedModifiers = settings.value("list").toStringList();
	else
		_mostRecentlyUsedModifiers.clear();
	settings.endGroup();

	// Retrieve the input state which a newly inserted modifier would be applied to.
	// This is used to filter the list of available modifiers.
	ModificationListItem* currentItem = _modificationList->selectedItem();
	while(currentItem && currentItem->parent()) {
		currentItem = currentItem->parent();
	}
	PipelineFlowState inputState;
	DataSet* dataset = _modificationList->datasetContainer().currentSet();
	if(!dataset) return;

	if(dynamic_object_cast<Modifier>(currentItem->object())) {
		for(ModifierApplication* modApp : currentItem->modifierApplications()) {
			PipelineObject* pipelineObj = modApp->pipelineObject();
			OVITO_CHECK_OBJECT_POINTER(pipelineObj);
			inputState = pipelineObj->evaluateImmediately(PipelineEvalRequest(dataset->animationSettings()->time(), false, modApp, true));
			break;
		}
	}
	else if(dynamic_object_cast<DataObject>(currentItem->object())) {
		DataObject* dataObj = static_object_cast<DataObject>(currentItem->object());
		OVITO_CHECK_OBJECT_POINTER(dataObj);
		inputState = dataObj->evaluateImmediately(PipelineEvalRequest(dataset->animationSettings()->time(), false));
	}
	else {
		for(RefTarget* objNode : _modificationList->selectedNodes()) {
			inputState = static_object_cast<ObjectNode>(objNode)->evaluatePipelineImmediately(PipelineEvalRequest(dataset->animationSettings()->time(), false));
			break;
		}
	}

	// Update state of combo box items.
	for(QStandardItem* item : _modifierItems) {
		const OvitoObjectType* descriptor = static_cast<const OvitoObjectType*>(item->data(Qt::UserRole).value<void*>());
		OVITO_ASSERT(descriptor);

		// Create an instance of the modifier to call its isApplicableTo() method.
		OORef<Modifier> modifier = static_object_cast<Modifier>(descriptor->createInstance(dataset));
		OVITO_CHECK_OBJECT_POINTER(modifier);
		item->setEnabled(modifier && modifier->isApplicableTo(inputState));
	}

	// Load custom modifier presets.
	settings.beginGroup("core/modifier/presets/");
	QStringList keys = settings.childKeys();
	settings.endGroup();
	int numCustom = 0;
	for(const QString& name : keys) {
		QStandardItem* modifierItem;
		if(numCustom < _numCustomModifiers)
			modifierItem = _model->item(_model->rowCount() - 2 - _numCustomModifiers + numCustom);
		else {
			modifierItem = new QStandardItem("   " + name);
			_model->insertRow(_model->rowCount() - 2, modifierItem);
		}
		modifierItem->setData(QVariant::fromValue(name), Qt::UserRole);
		numCustom++;
	}
	// Remove unused entries.
	if(numCustom < _numCustomModifiers)
		_model->removeRows(_model->rowCount() - 2 - _numCustomModifiers + numCustom, _numCustomModifiers - numCustom);
	_numCustomModifiers = numCustom;
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
