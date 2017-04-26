///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (2013) Alexander Stukowski
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

#pragma once


#include <gui/GUI.h>
#include <core/object/OvitoObjectType.h>

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

class ModificationListModel;

/**
 * A combo-box widget that lets the user insert new modifiers into the modification pipeline.
 */
class ModifierListBox : public QComboBox
{
public:

	/// Initializes the widget.
	ModifierListBox(QWidget* parent, ModificationListModel* modificationList);

	/// Is called just before the drop-down box is activated.
	virtual void showPopup() override {
		updateAvailableModifiers();
		_filterModel->invalidate();
		setMaxVisibleItems(_model->rowCount());
		_showAllModifiers = false;
		QComboBox::showPopup();
	}

	/// Indicates whether the complete list of modifiers should be shown.
	bool showAllModifiers() const {
		return (_showAllModifiers || _mostRecentlyUsedModifiers.size() < 4);
	}

private Q_SLOTS:

	/// Updates the list box of modifier classes that can be applied to the currently selected
	/// item in the modification list.
	void updateAvailableModifiers();

	/// Updates the MRU list after the user has selected a modifier.
	void updateMRUList(const QString& selectedModifierName);

private:

	/// Filters the full list of modifiers to show only most recently used ones.
	bool filterAcceptsRow(int source_row, const QModelIndex& source_parent);

	/// Determines the sort order of the modifier list. 
	bool filterSortLessThan(const QModelIndex& source_left, const QModelIndex& source_right);

	/// The modification list model.
	ModificationListModel* _modificationList;

	/// The list items representing modifier types.
	QVector<QStandardItem*> _modifierItems;

	/// The item model containing all entries of the combo box.
	QStandardItemModel* _model;

	/// The item model used for filtering/storting the displayed list of modifiers.
	QSortFilterProxyModel* _filterModel;

	/// This flag asks updateAvailableModifiers() to list all modifiers, not just the most recently used ones.
	bool _showAllModifiers = false;

	/// The number of custom modifier presets in the list.
	int _numCustomModifiers = 0;

	/// MRU list of modifiers.
	QStringList _mostRecentlyUsedModifiers; 

	/// Maximum number of modifiers shown in the MRU list.
	int _maxMRUSize = 8;

	Q_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace


