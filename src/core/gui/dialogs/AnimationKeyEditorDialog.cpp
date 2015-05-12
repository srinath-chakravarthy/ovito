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

#include <core/Core.h>
#include <core/gui/widgets/general/SpinnerWidget.h>
#include <core/gui/mainwin/MainWindow.h>
#include <core/animation/controller/KeyframeController.h>
#include <core/utilities/units/UnitsManager.h>
#include "AnimationKeyEditorDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

class NumericalItemDelegate : public QStyledItemDelegate
{
public:
	NumericalItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

	virtual QString displayText(const QVariant& value, const QLocale& locale) const override {
		return value.toString();
	}

	virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const {
		SpinnerWidget* spinner = new SpinnerWidget(parent);
		return spinner;
	}

	virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override {

	}

	virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const {

	}

	virtual void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		editor->setGeometry(option.rect);
	}
};

class AnimationKeyModel : public QAbstractTableModel
{
public:

	/// Constructor.
	AnimationKeyModel(AnimationSettings* animSettings, Controller::ControllerType ctrlType, QObject* parent = nullptr)
		: QAbstractTableModel(parent), _animSettings(animSettings), _ctrlType(ctrlType) {}

	/// Returns the number of rows.
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override {
		if(parent.isValid()) return 0;
		return _keys.size();
	}

	/// Returns the number of columns.
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override {
		if(parent.isValid()) return 0;
		if(_ctrlType == Controller::ControllerTypeFloat || _ctrlType == Controller::ControllerTypeInt) return 2;
		if(_ctrlType == Controller::ControllerTypeVector3 || _ctrlType == Controller::ControllerTypePosition) return 4;
		return 0;
	}

	/// Returns the data stored.
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
		if(!index.isValid()) return QVariant();
		if(role == Qt::DisplayRole || role == Qt::EditRole) {
			int row = index.row();
			if(index.column() == 0) return QVariant::fromValue(_animSettings->timeToString(_keys[row].first));
			if(_ctrlType == Controller::ControllerTypeFloat || _ctrlType == Controller::ControllerTypeInt) {
				return _keys[row].second;
			}
			else if(_ctrlType == Controller::ControllerTypeVector3 || _ctrlType == Controller::ControllerTypePosition) {
				Vector3 v = _keys[row].second.value<Vector3>();
				return QVariant::fromValue(v[index.column()-1]);
			}
		}
		return QVariant();
	}

	/// Returns the item flags for the given index.
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override {
		if(!index.isValid())
			return QAbstractTableModel::flags(index);
		if(index.column() == 0)
			return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		else
			return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	}

	/// Sets the role data for the item at index to value.
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override {
		if(index.isValid() && role == Qt::EditRole) {
			int row = index.row();
			OVITO_ASSERT(index.column() != 0);
			if(_ctrlType == Controller::ControllerTypeFloat || _ctrlType == Controller::ControllerTypeInt) {
				_keys[row].second = QVariant::fromValue(value.value<FloatType>());
				dataChanged(index, index);
				return true;
			}
			else if(_ctrlType == Controller::ControllerTypeVector3 || _ctrlType == Controller::ControllerTypePosition) {
				Vector3 v = _keys[row].second.value<Vector3>();
				v[index.column() - 1] = value.value<FloatType>();
				_keys[row].second = QVariant::fromValue(v);
				dataChanged(index, index);
				return true;
			}
		}
		return false;
	}

	/// Returns the data for the given role and section in the header with the specified orientation.
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
		if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
			if(section == 0) return tr("Time");
			if(_ctrlType == Controller::ControllerTypeFloat || _ctrlType == Controller::ControllerTypeInt) {
				if(section == 1) return tr("Value");
			}
			else if(_ctrlType == Controller::ControllerTypeVector3 || _ctrlType == Controller::ControllerTypePosition) {
				if(section == 1) return tr("Value (X)");
				else if(section == 2) return tr("Value (Y)");
				else if(section == 3) return tr("Value (Z)");
			}
		}
		return QAbstractTableModel::headerData(section, orientation, role);
	}

private:

	/// The global animation settings.
	AnimationSettings* _animSettings;
	/// The list of animation keys.
	std::vector<std::pair<TimePoint, QVariant>> _keys;
	/// The data type of the animation keys.
	Controller::ControllerType _ctrlType;

	friend class AnimationKeyEditorDialog;
};

/******************************************************************************
* The constructor of the dialog widget.
******************************************************************************/
AnimationKeyEditorDialog::AnimationKeyEditorDialog(KeyframeController* ctrl, const PropertyFieldDescriptor* propertyField, QWidget* parent) :
	QDialog(parent),
	UndoableTransaction(ctrl->dataset()->undoStack(), tr("Edit animatable parameter"))
{
	setWindowTitle(tr("Animatable parameter: %1").arg(propertyField->displayName()));

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	mainLayout->addWidget(new QLabel(tr("Animation keys:")));
	_tableWidget = new QTableView();
	AnimationKeyModel* model = new AnimationKeyModel(ctrl->dataset()->animationSettings(), ctrl->controllerType(), _tableWidget);
	for(const AnimationKey* key : ctrl->keys())
		model->_keys.push_back(std::make_pair(key->time(), key->qvariant_value()));

	_model = model;
	QItemSelectionModel* selModel = _tableWidget->selectionModel();
	_tableWidget->setModel(_model);
	delete selModel;
	_tableWidget->verticalHeader()->hide();
	_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

	if(model->columnCount() >= 4)
		mainLayout->addStrut(480);

	NumericalItemDelegate* numericalDelegate = new NumericalItemDelegate(_tableWidget);
	for(int col = 1; col < model->columnCount(); col++)
		_tableWidget->setItemDelegateForColumn(col, numericalDelegate);

	QHBoxLayout* hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(0,0,0,0);
	hlayout->setSpacing(0);
	hlayout->addWidget(_tableWidget, 1);

	QToolBar* toolbar = new QToolBar();
	toolbar->setOrientation(Qt::Vertical);
	toolbar->setFloatable(false);
	_addKeyAction = toolbar->addAction(QIcon(":/core/actions/animation/add_animation_key.png"), tr("Add animation key"));
	_deleteKeyAction = toolbar->addAction(QIcon(":/core/actions/animation/delete_animation_key.png"), tr("Delete animation key"));
	_deleteKeyAction->setEnabled(false);

	hlayout->addWidget(toolbar);
	mainLayout->addLayout(hlayout);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &AnimationKeyEditorDialog::onOk);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &AnimationKeyEditorDialog::reject);
	mainLayout->addWidget(buttonBox);

	connect(_tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged, [this]() {
		QModelIndexList selection = _tableWidget->selectionModel()->selectedRows();
		_deleteKeyAction->setEnabled(_model->rowCount() > 1 && selection.empty() == false);
	});
	if(_model->rowCount() >= 0)
		_tableWidget->selectRow(_model->rowCount() - 1);
}

/******************************************************************************
* Event handler for the Ok button.
******************************************************************************/
void AnimationKeyEditorDialog::onOk()
{
	commit();
	accept();
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
