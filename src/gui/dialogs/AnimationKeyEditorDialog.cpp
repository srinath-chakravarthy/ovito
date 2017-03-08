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

#include <gui/GUI.h>
#include <gui/widgets/general/SpinnerWidget.h>
#include <gui/mainwin/MainWindow.h>
#include <gui/properties/PropertiesPanel.h>
#include <gui/properties/NumericalParameterUI.h>
#include <gui/dialogs/AnimationSettingsDialog.h>
#include <core/animation/controller/KeyframeController.h>
#include <core/utilities/units/UnitsManager.h>
#include "AnimationKeyEditorDialog.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Gui) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

class NumericalItemDelegate : public QStyledItemDelegate
{
	ParameterUnit* _units;
	FloatType _minValue, _maxValue;

public:

	NumericalItemDelegate(QObject* parent, ParameterUnit* units, FloatType minValue, FloatType maxValue) : QStyledItemDelegate(parent), _units(units), _minValue(minValue), _maxValue(maxValue) {}

	virtual QString displayText(const QVariant& value, const QLocale& locale) const override {
		if(_units)
			return _units->formatValue(_units->nativeToUser(value.value<FloatType>()));
		return value.toString();
	}

	virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		QWidget* container = new QWidget(parent);
		QHBoxLayout* layout = new QHBoxLayout(container);
		layout->setSpacing(0);
		layout->setContentsMargins(0,0,0,0);
		QLineEdit* edit = new QLineEdit();
		edit->setFrame(false);
		layout->addWidget(edit, 1);
		container->setFocusProxy(edit);
		SpinnerWidget* spinner = new SpinnerWidget(nullptr, edit);
		if(_units) spinner->setUnit(_units);
		spinner->setMinValue(_minValue);
		spinner->setMaxValue(_maxValue);
		layout->addWidget(spinner);
		connect(spinner, &SpinnerWidget::spinnerValueChanged, [this,container]() {
			Q_EMIT const_cast<NumericalItemDelegate*>(this)->commitData(container);
		});
		return container;
	}

	virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override {
		SpinnerWidget* spinner = editor->findChild<SpinnerWidget*>();
		QVariant data = index.data(Qt::EditRole);
		if(data.userType() == qMetaTypeId<FloatType>())
			spinner->setFloatValue(data.value<FloatType>());
		else if(data.userType() == qMetaTypeId<int>())
			spinner->setFloatValue(data.toInt());
	}

	virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override {
		SpinnerWidget* spinner = editor->findChild<SpinnerWidget*>();
		QVariant data = index.data(Qt::EditRole);
		if(data.userType() == qMetaTypeId<FloatType>())
			model->setData(index, QVariant::fromValue(spinner->floatValue()));
		else if(data.userType() == qMetaTypeId<int>())
			model->setData(index, QVariant::fromValue(spinner->intValue()));
	}

	virtual void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		editor->setGeometry(option.rect);
	}
};

class AnimationKeyModel : public QAbstractTableModel
{
public:

	/// Constructor.
	AnimationKeyModel(KeyframeController* ctrl, const PropertyFieldDescriptor* propertyField, QObject* parent = nullptr)
		: QAbstractTableModel(parent), _propertyField(propertyField) {
		_ctrl.setTarget(ctrl);
		_ctrlType = ctrl->controllerType();
		_keys.setTargets(ctrl->keys());
		connect(&_ctrl, &RefTargetListener<KeyframeController>::notificationEvent, this, &AnimationKeyModel::onCtrlEvent);
		connect(&_keys, &VectorRefTargetListener<AnimationKey>::notificationEvent, this, &AnimationKeyModel::onKeyEvent);
	}

	/// Returns the animation controller being edited.
	KeyframeController* ctrl() const { return _ctrl.target(); }

	/// Returns the list of animation keys.
	const QVector<AnimationKey*>& keys() const { return _keys.targets(); }

	/// Returns the number of rows.
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override {
		if(parent.isValid()) return 0;
		return keys().size();
	}

	/// Returns the number of columns.
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override {
		if(parent.isValid()) return 0;
		else if(_ctrlType == Controller::ControllerTypeFloat || _ctrlType == Controller::ControllerTypeInt) return 1;
		else if(_ctrlType == Controller::ControllerTypeVector3 || _ctrlType == Controller::ControllerTypePosition) return 3;
		else if(_ctrlType == Controller::ControllerTypeRotation) return 4;
		else return 0;
	}

	/// Returns the data stored.
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
		if(!index.isValid()) return QVariant();
		if(role == Qt::DisplayRole || role == Qt::EditRole) {
			int row = index.row();
			if(row >= 0 && row < keys().size()) {
				const AnimationKey* key = keys()[row];
				if(_ctrlType == Controller::ControllerTypeFloat || _ctrlType == Controller::ControllerTypeInt) {
					return key->valueQVariant();
				}
				else if(_ctrlType == Controller::ControllerTypeVector3) {
					Vector3 v = static_object_cast<Vector3AnimationKey>(key)->value();
					return QVariant::fromValue(v[index.column()]);
				}
				else if(_ctrlType == Controller::ControllerTypePosition) {
					Vector3 v = static_object_cast<PositionAnimationKey>(key)->value();
					return QVariant::fromValue(v[index.column()]);
				}
				else if(_ctrlType == Controller::ControllerTypeRotation) {
					Rotation r = static_object_cast<RotationAnimationKey>(key)->value();
					if(index.column() < 3)
						return QVariant::fromValue(r.axis()[index.column()]);
					else
						return QVariant::fromValue(r.angle());
				}
			}
		}
		return QVariant();
	}

	/// Returns the item flags for the given index.
	virtual Qt::ItemFlags flags(const QModelIndex& index) const override {
		if(!index.isValid())
			return QAbstractTableModel::flags(index);
		return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
	}

	/// Sets the role data for the item at index to value.
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override {
		if(index.isValid() && role == Qt::EditRole) {
			int row = index.row();
			if(row >= 0 && row < keys().size()) {
				try {
					AnimationKey* key = keys()[row];
					if(_ctrlType == Controller::ControllerTypeFloat || _ctrlType == Controller::ControllerTypeInt) {
						return key->setValueQVariant(value);
					}
					else if(_ctrlType == Controller::ControllerTypeVector3) {
						Vector3 vec = static_object_cast<Vector3AnimationKey>(key)->value();
						vec[index.column()] = value.value<FloatType>();
						static_object_cast<Vector3AnimationKey>(key)->setValue(vec);
						return true;
					}
					else if(_ctrlType == Controller::ControllerTypePosition) {
						Vector3 vec = static_object_cast<PositionAnimationKey>(key)->value();
						vec[index.column()] = value.value<FloatType>();
						static_object_cast<PositionAnimationKey>(key)->setValue(vec);
						return true;
					}
					else if(_ctrlType == Controller::ControllerTypeRotation) {
						Rotation r = static_object_cast<RotationAnimationKey>(key)->value();
						if(index.column() < 3) {
							Vector3 axis = r.axis();
							axis[index.column()] = value.value<FloatType>();
							axis.normalizeSafely();
							r.setAxis(axis);
						}
						else if(index.column() == 3) {
							r.setAngle(value.value<FloatType>());
						}
						static_object_cast<RotationAnimationKey>(key)->setValue(r);
						return true;
					}
				}
				catch(const Exception& ex) {
					ex.reportError();
				}
			}
		}
		return false;
	}

	/// Returns the data for the given role and section in the header with the specified orientation.
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
		if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
			if(_ctrlType == Controller::ControllerTypeFloat || _ctrlType == Controller::ControllerTypeInt) {
				return _propertyField->displayName();
			}
			else if(_ctrlType == Controller::ControllerTypeVector3 || _ctrlType == Controller::ControllerTypePosition) {
				if(section == 0) return _propertyField->displayName() + QString(" (X)");
				else if(section == 1) return _propertyField->displayName() + QString(" (Y)");
				else if(section == 2) return _propertyField->displayName() + QString(" (Z)");
			}
			else if(_ctrlType == Controller::ControllerTypeRotation) {
				if(section == 0) return _propertyField->displayName() + QString(" (Axis X)");
				else if(section == 1) return _propertyField->displayName() + QString(" (Axis Y)");
				else if(section == 2) return _propertyField->displayName() + QString(" (Axis Z)");
				else if(section == 3) return _propertyField->displayName() + QString(" (Angle)");
			}
		}
		else if(orientation == Qt::Vertical && role == Qt::DisplayRole) {
			if(section >= 0 && section < keys().size()) {
				const AnimationKey* key = keys()[section];
				return QVariant::fromValue(tr("Time: %1").arg(key->dataset()->animationSettings()->timeToString(key->time())));
			}
		}
		return QAbstractTableModel::headerData(section, orientation, role);
	}

	/// Is called when the animation controller generates a notification event.
	void onCtrlEvent(ReferenceEvent* event) {
		if(event->type() == ReferenceEvent::ReferenceRemoved) {
			ReferenceFieldEvent* refEvent = static_cast<ReferenceFieldEvent*>(event);
			if(refEvent->field() == PROPERTY_FIELD(KeyframeController::keys)) {
				int index = keys().indexOf(static_object_cast<AnimationKey>(refEvent->oldTarget()));
				if(index >= 0) {
					beginRemoveRows(QModelIndex(), index, index);
					_keys.remove(index);
					endRemoveRows();
				}
				OVITO_ASSERT(keys().size() == ctrl()->keys().size());
			}
		}
		else if(event->type() == ReferenceEvent::ReferenceAdded) {
			ReferenceFieldEvent* refEvent = static_cast<ReferenceFieldEvent*>(event);
			if(refEvent->field() == PROPERTY_FIELD(KeyframeController::keys)) {
				OVITO_ASSERT(keys().size() == ctrl()->keys().size() - 1);
				beginInsertRows(QModelIndex(), refEvent->index(), refEvent->index());
				_keys.insert(refEvent->index(), static_object_cast<AnimationKey>(refEvent->newTarget()));
				endInsertRows();
			}
		}
	}

	/// Is called when an animation key generates a notification event.
	void onKeyEvent(RefTarget* source, ReferenceEvent* event) {
		if(event->type() == ReferenceEvent::TargetChanged) {
			int index = keys().indexOf(static_object_cast<AnimationKey>(source));
			OVITO_ASSERT(index >= 0);
			Q_EMIT dataChanged(createIndex(index, 0), createIndex(index, columnCount() - 1));
			Q_EMIT headerDataChanged(Qt::Vertical, index, index);
		}
		else if(event->type() == ReferenceEvent::TargetDeleted) {
			int index = keys().indexOf(static_object_cast<AnimationKey>(source));
			OVITO_ASSERT(index >= 0);
			beginRemoveRows(QModelIndex(), index, index);
			_keys.remove(index);
			endRemoveRows();
		}
	}

private:

	/// The controller whose keys are being edited.
	RefTargetListener<KeyframeController> _ctrl;

	/// The list of keys.
	VectorRefTargetListener<AnimationKey> _keys;

	/// The type of controller being edited.
	Controller::ControllerType _ctrlType;

	/// The property field being animated.
	const PropertyFieldDescriptor* _propertyField;
};

/******************************************************************************
* The constructor of the dialog widget.
******************************************************************************/
AnimationKeyEditorDialog::AnimationKeyEditorDialog(KeyframeController* ctrl, const PropertyFieldDescriptor* propertyField, QWidget* parent, MainWindow* mainWindow) :
	QDialog(parent),
	UndoableTransaction(ctrl->dataset()->undoStack(), tr("Edit animatable parameter"))
{
	setWindowTitle(tr("Parameter animation: %1").arg(propertyField->displayName()));
	_ctrl.setTarget(ctrl);

	// Make sure the controller has at least one animation key.
	if(ctrl->keys().empty()) {
		try { ctrl->createKey(0); }
		catch(const Exception&) {}
	}

	QVBoxLayout* mainLayout = new QVBoxLayout(this);

	mainLayout->addWidget(new QLabel(tr("Animation keys:")));
	_tableWidget = new QTableView();
	AnimationKeyModel* model = new AnimationKeyModel(ctrl, propertyField, _tableWidget);
	_model = model;
	QItemSelectionModel* selModel = _tableWidget->selectionModel();
	_tableWidget->setModel(_model);
	delete selModel;
	_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	_tableWidget->setEditTriggers(QAbstractItemView::AllEditTriggers);

	mainLayout->addStrut(model->columnCount() * 120 + 100);

	ParameterUnit* units = nullptr;
	FloatType minValue = FLOATTYPE_MIN;
	FloatType maxValue = FLOATTYPE_MAX;
	if(propertyField->numericalParameterInfo()) {
		minValue = propertyField->numericalParameterInfo()->minValue;
		maxValue = propertyField->numericalParameterInfo()->maxValue;
		if(propertyField->numericalParameterInfo()->unitType != nullptr)
			units = ctrl->dataset()->unitsManager().getUnit(propertyField->numericalParameterInfo()->unitType);
	}

	if(ctrl->controllerType() != Controller::ControllerTypeRotation) {
		NumericalItemDelegate* numericalDelegate = new NumericalItemDelegate(_tableWidget, units, minValue, maxValue);
		for(int col = 0; col < model->columnCount(); col++)
			_tableWidget->setItemDelegateForColumn(col, numericalDelegate);
	}
	else {
		NumericalItemDelegate* axisDelegate = new NumericalItemDelegate(_tableWidget, ctrl->dataset()->unitsManager().worldUnit(), FLOATTYPE_MIN, FLOATTYPE_MAX);
		NumericalItemDelegate* angleDelegate = new NumericalItemDelegate(_tableWidget, ctrl->dataset()->unitsManager().angleUnit(), FLOATTYPE_MIN, FLOATTYPE_MAX);
		_tableWidget->setItemDelegateForColumn(0, axisDelegate);
		_tableWidget->setItemDelegateForColumn(1, axisDelegate);
		_tableWidget->setItemDelegateForColumn(2, axisDelegate);
		_tableWidget->setItemDelegateForColumn(3, angleDelegate);
	}

	_tableWidget->resizeColumnsToContents();

	QHBoxLayout* hlayout = new QHBoxLayout();
	hlayout->setContentsMargins(0,0,0,0);
	hlayout->setSpacing(0);
	hlayout->addWidget(_tableWidget, 1);

	QToolBar* toolbar = new QToolBar();
	toolbar->setOrientation(Qt::Vertical);
	toolbar->setFloatable(false);
	_addKeyAction = toolbar->addAction(QIcon(":/gui/actions/animation/add_animation_key.png"), tr("Create animation key"));
	connect(_addKeyAction, &QAction::triggered, this, &AnimationKeyEditorDialog::onAddKey);
	_deleteKeyAction = toolbar->addAction(QIcon(":/gui/actions/animation/delete_animation_key.png"), tr("Delete animation key"));
	_deleteKeyAction->setEnabled(false);
	connect(_deleteKeyAction, &QAction::triggered, this, &AnimationKeyEditorDialog::onDeleteKey);

	toolbar->addSeparator();
	QAction* animSettingsAction = toolbar->addAction(QIcon(":/gui/actions/animation/animation_settings.png"), tr("Animation settings..."));
	connect(animSettingsAction, &QAction::triggered, [this]() {
		AnimationSettingsDialog(this->ctrl()->dataset()->animationSettings(), this).exec();
	});

	hlayout->addWidget(toolbar);
	mainLayout->addLayout(hlayout);

	_keyPropPanel = new PropertiesPanel(this, mainWindow);
	mainLayout->addWidget(_keyPropPanel);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help, Qt::Horizontal, this);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &AnimationKeyEditorDialog::onOk);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &AnimationKeyEditorDialog::reject);

	// Implement Help button.
	connect(buttonBox, &QDialogButtonBox::helpRequested, []() {
		MainWindow::openHelpTopic(QStringLiteral("usage.animation.html"));
	});

	mainLayout->addWidget(buttonBox);

	connect(_tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged, [this]() {
		QModelIndexList selection = _tableWidget->selectionModel()->selectedRows();
		if(_model->rowCount() > 1 && selection.empty() == false) {
			_deleteKeyAction->setEnabled(true);
			const QModelIndex& index = selection.first();
			OVITO_ASSERT(index.row() >= 0 && index.row() < this->ctrl()->keys().size());
			_keyPropPanel->setEditObject(this->ctrl()->keys()[index.row()]);
		}
		else {
			_deleteKeyAction->setEnabled(false);
			_keyPropPanel->setEditObject(nullptr);
		}
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

/******************************************************************************
* Handles the 'Add key' button.
******************************************************************************/
void AnimationKeyEditorDialog::onAddKey()
{
	QDialog dlg(this);
	dlg.setWindowTitle(tr("New animation key"));
	QVBoxLayout* mainLayout = new QVBoxLayout(&dlg);
	QHBoxLayout* subLayout = new QHBoxLayout();
	subLayout->setContentsMargins(0,0,0,0);
	subLayout->setSpacing(0);
	subLayout->addWidget(new QLabel(tr("Create key at animation time:")));
	subLayout->addSpacing(4);
	QLineEdit* timeEdit = new QLineEdit();
	subLayout->addWidget(timeEdit);
	SpinnerWidget* timeSpinner = new SpinnerWidget();
	timeSpinner->setTextBox(timeEdit);
	timeSpinner->setUnit(ctrl()->dataset()->unitsManager().timeUnit());
	timeSpinner->setIntValue(ctrl()->dataset()->animationSettings()->time());
	timeSpinner->setMinValue(ctrl()->dataset()->animationSettings()->animationInterval().start());
	timeSpinner->setMaxValue(ctrl()->dataset()->animationSettings()->animationInterval().end());
	subLayout->addWidget(timeSpinner);
	mainLayout->addLayout(subLayout);
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);
	connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
	mainLayout->addWidget(buttonBox);
	if(dlg.exec() == QDialog::Accepted) {
		try {
			int index = ctrl()->createKey(timeSpinner->intValue());
			_tableWidget->selectRow(index);
		}
		catch(const Exception& ex) {
			ex.reportError();
		}
	}
}

/******************************************************************************
* Handles the 'Delete key' button.
******************************************************************************/
void AnimationKeyEditorDialog::onDeleteKey()
{
	QModelIndexList selection = _tableWidget->selectionModel()->selectedRows();
	try {
		QVector<AnimationKey*> keysToDelete;
		for(const QModelIndex& index : selection) {
			OVITO_ASSERT(index.row() >= 0 && index.row() < ctrl()->keys().size());
			keysToDelete.push_back(ctrl()->keys()[index.row()]);
		}
		ctrl()->deleteKeys(keysToDelete);
	}
	catch(const Exception& ex) {
		ex.reportError();
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
