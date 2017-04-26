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

#pragma once


#include <plugins/particles/gui/ParticlesGui.h>
#include <plugins/particles/gui/modifier/ParticleModifierEditor.h>
#include <plugins/particles/modifier/modify/CreateBondsModifier.h>

namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Modify) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/**
 * \brief A properties editor for the CreateBondsModifier class.
 */
class CreateBondsModifierEditor : public ParticleModifierEditor
{
public:

	/// Default constructor.
	Q_INVOKABLE CreateBondsModifierEditor() {}

protected:

	/// Creates the user interface controls for the editor.
	virtual void createUI(const RolloutInsertionParameters& rolloutParams) override;

protected Q_SLOTS:

	/// Updates the contents of the pair-wise cutoff table.
	void updatePairCutoffList();

	/// Updates the cutoff values in the pair-wise cutoff table.
	void updatePairCutoffListValues();

private:

	class PairCutoffTableModel : public QAbstractTableModel {
	public:
		typedef QVector<QPair<QString,QString>> ContentType;

		PairCutoffTableModel(QObject* parent) : QAbstractTableModel(parent) {}
		virtual int	rowCount(const QModelIndex& parent) const override { return _data.size(); }
		virtual int	columnCount(const QModelIndex& parent) const override { return 3; }
		virtual QVariant data(const QModelIndex& index, int role) const override;
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
			if(orientation != Qt::Horizontal || role != Qt::DisplayRole) return QVariant();
			switch(section) {
			case 0: return CreateBondsModifierEditor::tr("1st type");
			case 1: return CreateBondsModifierEditor::tr("2nd type");
			case 2: return CreateBondsModifierEditor::tr("Cutoff");
			default: return QVariant();
			}
		}
		virtual Qt::ItemFlags flags(const QModelIndex& index) const override {
			if(index.column() != 2)
				return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
			else
				return Qt::ItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
		}
		virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override;
		void setContent(CreateBondsModifier* modifier, const ContentType& data) {
			beginResetModel();
			_modifier = modifier;
			_data = data;
			endResetModel();
		}
		void updateContent() { Q_EMIT dataChanged(index(0,2), index(_data.size()-1,2)); }
	private:
		ContentType _data;
		OORef<CreateBondsModifier> _modifier;
	};

	QTableView* _pairCutoffTable;
	PairCutoffTableModel* _pairCutoffTableModel;

	Q_OBJECT
	OVITO_OBJECT
};

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
}	// End of namespace


