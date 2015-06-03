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

#ifndef _OVITO_CA_CLUSTER_VECTOR_H
#define _OVITO_CA_CLUSTER_VECTOR_H

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include "Cluster.h"
#include "ClusterGraph.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/**
 * A Cartesian vector in the stress-free reference configuration of a cluster.
 *
 * Each reference configuration vector is associated with a cluster,
 * which determines the local frame of reference the vector is expressed in.
 *
 * The only exception is the vector (0,0,0), which doesn't need to be associated
 * with a specific frame of reference.
 */
class ClusterVector
{
public:

	/// Initializes the vector to the null vector (0,0,0).
	/// All three components are set to zero. Optionally, a cluster may be associated with the vector,
	/// which determines the frame of reference.
	ClusterVector(Vector3::Zero NULL_VECTOR, Cluster* cluster = nullptr) : _vec(NULL_VECTOR), _cluster(cluster) {}

	/// Initializes the cluster vector to the given Cartesian vector, which is expressed in the frame of
	/// reference of the given cluster.
	explicit ClusterVector(const Vector3& vec, Cluster* cluster) : _vec(vec), _cluster(cluster) {}

	/// Returns the XYZ components of the vector expressed in the local coordinate system of the associated cluster.
	const Vector3& localVec() const { return _vec; }

	/// Returns a reference to the XYZ components of the vector expressed in the local coordinate system of the associated cluster.
	Vector3& localVec() { return _vec; }

	/// Returns the cluster that provides the local frame of reference this reference configuration vector is expressed in.
	Cluster* cluster() const { return _cluster; }

	/// Returns the inverse of the vector.
	ClusterVector operator-() const {
		return ClusterVector(-localVec(), cluster());
	}

	/// Transforms the cluster vector to a spatial vector using the orientation matrix of the cluster.
	Vector3 toSpatialVector() const {
		OVITO_ASSERT(cluster() != nullptr);
		return cluster()->orientation * localVec();
	}

	/// Translates this lattice vector to the frame of reference of another cluster.
	/// Returns true if operation was successful.
	/// Returns false if the transformation could not be computed.
	bool transformToCluster(Cluster* otherCluster, ClusterGraph& graph) {
		OVITO_ASSERT(otherCluster != nullptr);
		OVITO_ASSERT(this->cluster() != nullptr);
		if(this->cluster() == otherCluster) return true;
		ClusterTransition* t = graph.determineClusterTransition(this->cluster(), otherCluster);
		if(!t) return false;
		this->_vec = t->tm * this->localVec();
		this->_cluster = otherCluster;
		return true;
	}

private:

	/// The XYZ components of the vector in the local lattice coordinate system.
	Vector3 _vec;

	/// The cluster which serves as the frame of reference for this vector.
	/// This may be NULL if the vector's components are (0,0,0).
	Cluster* _cluster;
};

/// Outputs a cluster vector to the console.
inline std::ostream& operator<<(std::ostream& stream, const ClusterVector& v)
{
	return stream << v.localVec() << "[cluster " << v.cluster()->id << "]";
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

#endif // _OVITO_CA_CLUSTER_VECTOR_H
