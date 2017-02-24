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


#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <plugins/crystalanalysis/data/ClusterGraph.h>
#include <core/scene/objects/DataObjectWithSharedStorage.h>

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * \brief A graph of atomic clusters.
 */
class OVITO_CRYSTALANALYSIS_EXPORT ClusterGraphObject : public DataObjectWithSharedStorage<ClusterGraph>
{
public:

	/// \brief Constructor.
	Q_INVOKABLE ClusterGraphObject(DataSet* dataset, ClusterGraph* graph = nullptr);

	/// Returns the title of this object.
	virtual QString objectTitle() override { return tr("Clusters"); }

	/// \brief Returns whether this object, when returned as an editable sub-object by another object,
	///        should be displayed in the modification stack.
	///
	/// Return false because this object cannot be edited.
	virtual bool isSubObjectEditable() const override { return false; }

	/// Returns the list of nodes in the cluster graph.
	const std::vector<Cluster*>& clusters() const { return storage()->clusters(); }

	/// Looks up the cluster with the given ID.
	Cluster* findCluster(int id) const { return storage()->findCluster(id); }

private:

	Q_OBJECT
	OVITO_OBJECT
};

}	// End of namespace
}	// End of namespace
}	// End of namespace



