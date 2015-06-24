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
#include "DislocationNetwork.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Copy constructor.
******************************************************************************/
DislocationNetwork::DislocationNetwork(const DislocationNetwork& other)
{
	_clusterGraph = other._clusterGraph;
	for(int segmentIndex = 0; segmentIndex < other.segments().size(); segmentIndex++) {
		DislocationSegment* oldSegment = other.segments()[segmentIndex];
		OVITO_ASSERT(oldSegment->replacedWith == nullptr);
		OVITO_ASSERT(oldSegment->id == segmentIndex);
		DislocationSegment* newSegment = createSegment(oldSegment->burgersVector);
		newSegment->line = oldSegment->line;
		newSegment->coreSize = oldSegment->coreSize;
		OVITO_ASSERT(newSegment->id == oldSegment->id);
	}

	for(int segmentIndex = 0; segmentIndex < other.segments().size(); segmentIndex++) {
		DislocationSegment* oldSegment = other.segments()[segmentIndex];
		DislocationSegment* newSegment = segments()[segmentIndex];
		for(int nodeIndex = 0; nodeIndex < 2; nodeIndex++) {
			DislocationNode* oldNode = oldSegment->nodes[nodeIndex];
			if(oldNode->isDangling()) continue;
			DislocationNode* oldSecondNode = oldNode->junctionRing;
			DislocationNode* newNode = newSegment->nodes[nodeIndex];
			newNode->junctionRing = segments()[oldNode->junctionRing->segment->id]->nodes[oldSecondNode->isForwardNode() ? 0 : 1];
		}
	}

#ifdef OVITO_DEBUG
	for(int segmentIndex = 0; segmentIndex < other.segments().size(); segmentIndex++) {
		DislocationSegment* oldSegment = other.segments()[segmentIndex];
		DislocationSegment* newSegment = segments()[segmentIndex];
		for(int nodeIndex = 0; nodeIndex < 2; nodeIndex++) {
			OVITO_ASSERT(oldSegment->nodes[nodeIndex]->countJunctionArms() == newSegment->nodes[nodeIndex]->countJunctionArms());
		}
	}
#endif
}

/******************************************************************************
* Allocates a new dislocation segment terminated by two nodes.
******************************************************************************/
DislocationSegment* DislocationNetwork::createSegment(const ClusterVector& burgersVector)
{
	DislocationNode* forwardNode  = _nodePool.construct();
	DislocationNode* backwardNode = _nodePool.construct();

	DislocationSegment* segment = _segmentPool.construct(burgersVector, forwardNode, backwardNode);
	segment->id = _segments.size();
	_segments.push_back(segment);

	return segment;
}

/******************************************************************************
* Removes a segment from the list of segments.
******************************************************************************/
void DislocationNetwork::discardSegment(DislocationSegment* segment)
{
	OVITO_ASSERT(segment != nullptr);
	auto i = std::find(_segments.begin(), _segments.end(), segment);
	OVITO_ASSERT(i != _segments.end());
	_segments.erase(i);
}

/******************************************************************************
* Computes the location of a point along the segment line.
******************************************************************************/
Point3 DislocationSegment::getPointOnLine(FloatType t) const
{
	if(line.empty())
		return Point3::Origin();

	t *= calculateLength();

	FloatType sum = 0;
	auto i1 = line.begin();
	for(;;) {
		auto i2 = i1 + 1;
		if(i2 == line.end()) break;
		Vector3 delta = *i2 - *i1;
		FloatType len = delta.length();
		if(sum + len >= t && len != 0) {
			return *i1 + (((t - sum) / len) * delta);
		}
		sum += len;
		i1 = i2;
	}

	return line.back();
}


}	// End of namespace
}	// End of namespace
}	// End of namespace

