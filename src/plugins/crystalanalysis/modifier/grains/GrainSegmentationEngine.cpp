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

#include <plugins/crystalanalysis/CrystalAnalysis.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include <plugins/crystalanalysis/util/DelaunayTessellation.h>
#include <plugins/crystalanalysis/util/ManifoldConstructionHelper.h>
#include "GrainSegmentationEngine.h"
#include "GrainSegmentationModifier.h"

namespace Ovito { namespace Plugins { namespace CrystalAnalysis {

/******************************************************************************
* Constructor.
******************************************************************************/
GrainSegmentationEngine::GrainSegmentationEngine(const TimeInterval& validityInterval,
		ParticleProperty* positions, const SimulationCell& simCell,
		ParticleProperty* selection,
		int inputCrystalStructure, FloatType misorientationThreshold,
		FloatType fluctuationTolerance, int minGrainAtomCount,
		FloatType probeSphereRadius, int meshSmoothingLevel) :
	StructureIdentificationModifier::StructureIdentificationEngine(validityInterval, positions, simCell, QVector<bool>(), selection),
	_structureAnalysis(positions, simCell, (StructureAnalysis::LatticeStructureType)inputCrystalStructure, selection, structures()),
	_inputCrystalStructure(inputCrystalStructure),
	_deformationGradients(new ParticleProperty(positions->size(), qMetaTypeId<FloatType>(), 9, 0, QStringLiteral("Elastic Deformation Gradient"), false)),
	_misorientationThreshold(misorientationThreshold),
	_fluctuationTolerance(fluctuationTolerance),
	_minGrainAtomCount(minGrainAtomCount),
	_probeSphereRadius(probeSphereRadius),
	_meshSmoothingLevel(meshSmoothingLevel)
{
	// Set component names of tensor property.
	deformationGradients()->setComponentNames(QStringList() << "XX" << "YX" << "ZX" << "XY" << "YY" << "ZY" << "XZ" << "YZ" << "ZZ");
}

/******************************************************************************
* Performs the actual analysis. This method is executed in a worker thread.
******************************************************************************/
void GrainSegmentationEngine::perform()
{
	setProgressText(GrainSegmentationModifier::tr("Performing grain segmentation"));

	beginProgressSubSteps({ 360, 97, 7, 1, 35, 83, 143, 1, 10, 170, 2 });
	if(!_structureAnalysis.identifyStructures(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.buildClusters(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.connectClusters(*this))
		return;

	nextProgressSubStep();
	if(!_structureAnalysis.formSuperClusters(*this))
		return;

	nextProgressSubStep();

	// Initialize the working list of grains.
	_grains.resize(positions()->size());

	// Compute the local orientation tensor for all crystalline atoms.
	parallelFor(positions()->size(), *this, [this](size_t particleIndex) {

		Cluster* localCluster = _structureAnalysis.atomCluster(particleIndex);
		if(localCluster->id != 0) {

			Matrix3 idealUnitCellTM = Matrix3::Identity();

			// If the cluster is a defect (stacking fault), find the parent crystal cluster.
			Cluster* parentCluster = nullptr;
			if(localCluster->parentTransition != nullptr) {
				parentCluster = localCluster->parentTransition->cluster2;
				idealUnitCellTM = localCluster->parentTransition->tm;
			}
			else if(localCluster->structure == _inputCrystalStructure) {
				parentCluster = localCluster;
			}

			if(parentCluster != nullptr) {
				OVITO_ASSERT(parentCluster->structure == _inputCrystalStructure);

				// For calculating the cluster orientation.
				Matrix3 orientationV = Matrix3::Zero();
				Matrix3 orientationW = Matrix3::Zero();

				int numneigh = _structureAnalysis.numberOfNeighbors(particleIndex);
				for(int n = 0; n < numneigh; n++) {
					int neighborAtomIndex = _structureAnalysis.getNeighbor(particleIndex, n);
					// Add vector pair to matrices for computing the elastic deformation gradient.
					Vector3 latticeVector = idealUnitCellTM * _structureAnalysis.neighborLatticeVector(particleIndex, n);
					const Vector3& spatialVector = cell().wrapVector(positions()->getPoint3(neighborAtomIndex) - positions()->getPoint3(particleIndex));
					for(size_t i = 0; i < 3; i++) {
						for(size_t j = 0; j < 3; j++) {
							orientationV(i,j) += latticeVector[j] * latticeVector[i];
							orientationW(i,j) += latticeVector[j] * spatialVector[i];
						}
					}
				}

				// Calculate elastic deformation gradient tensor.
				Grain& grain = _grains[particleIndex];
				grain.orientation = orientationW * orientationV.inverse();
				grain.cluster = parentCluster;
				grain.latticeAtomCount = 1;
			}
		}
	});
	if(isCanceled()) return;
	nextProgressSubStep();

	// Build grain graph.
	std::vector<GrainGraphEdge> bulkEdges;
	setProgressMaximum(_grains.size());
	for(int atomA = 0; atomA < _grains.size(); atomA++) {
		if(!setProgressValueIntermittent(atomA)) return;
		Grain& grainA = _grains[atomA];

		// If the current atom is a crystalline atom recognized by the atomic structure identification algorithm,
		// then we connect it with its neighbors.

		// Skip non-crystalline atoms.
		if(grainA.cluster == nullptr) continue;

		// Iterate over all neighbors of the atom.
		int numNeighbors = _structureAnalysis.numberOfNeighbors(atomA);
		for(int ni = 0; ni < numNeighbors; ni++) {
			// Lookup neighbor atom from neighbor list.
			int atomB = _structureAnalysis.getNeighbor(atomA, ni);

			Grain& grainB = _grains[atomB];
			if(grainB.cluster != nullptr) {
				// This test ensures that we will create only one edge per pair of neighbor atoms.
				if(atomB <= atomA)
					continue;

				// Connect the two atoms with an edge.
				GrainGraphEdge edge = { atomA, atomB, calculateMisorientation(grainA, grainB) };
				bulkEdges.push_back(edge);
			}
			else {
				// Add isolated GB atoms to an adjacent lattice grain.
				if(grainB.parent->cluster > grainA.cluster) {
					grainB.parent->atomCount--;
					grainB.parent = &grainB;
				}
				if(grainB.isRoot())
					grainA.join(grainB);
			}
		}
	}
	nextProgressSubStep();

	// Sort edges in order of ascending misorientation.
	std::sort(bulkEdges.begin(), bulkEdges.end());

	// Merge grains.
	for(const GrainGraphEdge& edge : bulkEdges) {
		mergeTest(parentGrain(edge.a), parentGrain(edge.b), false);
		if(isCanceled()) return;
	}
	for(const GrainGraphEdge& edge : bulkEdges) {
		mergeTest(parentGrain(edge.a), parentGrain(edge.b), true);
		if(isCanceled()) return;
	}
	nextProgressSubStep();

	// Dissolve crystal grains that are too small (i.e. number of atoms below the threshold set by user).
	// Also dissolve grains that consist of stacking fault atoms only.
	for(Grain& atomicGrain : _grains) {
		Grain& rootGrain = parentGrain(atomicGrain);
		if(rootGrain.latticeAtomCount < _minGrainAtomCount || rootGrain.cluster->structure != _inputCrystalStructure) {
			atomicGrain.cluster = nullptr;                    // Dissolve grain.
			atomicGrain.latticeAtomCount = 0;
		}
		else
			atomicGrain.parent = &rootGrain;	// Path compression
	}
	if(isCanceled()) return;
	nextProgressSubStep();

	// Prepare the neighbor list builder.
	NearestNeighborFinder neighborFinder(12);
	if(!neighborFinder.prepare(positions(), cell(), nullptr, *this))
		return;

	nextProgressSubStep();
	// Add non-crystalline grain boundary atoms to the grains.
	for(;;) {
		bool done = true;
		boost::dynamic_bitset<> mergedAtoms(positions()->size());
		for(size_t atomA = 0; atomA < positions()->size(); atomA++) {
			if(isCanceled()) return;
			Grain& grainA = parentGrain(atomA);
			if(grainA.cluster != nullptr) {
				int numNeighbors = std::min(12, _structureAnalysis.numberOfNeighbors(atomA));
				for(int ni = 0; ni < numNeighbors; ni++) {
					int atomB = _structureAnalysis.getNeighbor(atomA, ni);

					if(mergedAtoms.test(atomB))
						continue;

					Grain &grainB = parentGrain(atomB);
					if(mergeTest(grainA, grainB, true)) {
						mergedAtoms.set(atomA);
						done = false;
					}
				}
			}
			else {
				NearestNeighborFinder::Query<12> neighQuery(neighborFinder);
				neighQuery.findNeighbors(atomA);
				for(int i = 0; i < neighQuery.results().size(); i++) {
					int atomB = neighQuery.results()[i].index;

					if(mergedAtoms.test(atomB))
						continue;

					Grain &grainB = parentGrain(atomB);
					if(mergeTest(grainA, grainB, true)) {
						mergedAtoms.set(atomA);
						done = false;
					}
				}
			}
		}
		if(done) break;
	}
	nextProgressSubStep();

	// Now assign final contiguous IDs to parent grains.
	_grainCount = assignIdsToGrains();

	qDebug() << "Number of grains:" << _grainCount;

	if(isCanceled()) return;

	static const Color grainColorList[] = {
			Color(255.0f/255.0f,41.0f/255.0f,41.0f/255.0f), Color(153.0f/255.0f,218.0f/255.0f,224.0f/255.0f), Color(71.0f/255.0f,75.0f/255.0f,225.0f/255.0f),
			Color(104.0f/255.0f,224.0f/255.0f,115.0f/255.0f), Color(238.0f/255.0f,250.0f/255.0f,46.0f/255.0f), Color(34.0f/255.0f,255.0f/255.0f,223.0f/255.0f),
			Color(255.0f/255.0f,158.0f/255.0f,41.0f/255.0f), Color(255.0f/255.0f,17.0f/255.0f,235.0f/255.0f), Color(173.0f/255.0f,3.0f/255.0f,240.0f/255.0f),
			Color(180.0f/255.0f,78.0f/255.0f,0.0f/255.0f), Color(162.0f/255.0f,190.0f/255.0f,34.0f/255.0f), Color(0.0f/255.0f,166.0f/255.0f,252.0f/255.0f)
	};

	// Create output cluster graph.
	_outputClusterGraph = new ClusterGraph();
	for(const Grain& grain : _grains) {
		if(grain.isRoot()) {
			if(_outputClusterGraph->findCluster(grain.id) == nullptr) {
				Cluster* cluster = _outputClusterGraph->createCluster(grain.cluster->structure, grain.id);
				cluster->atomCount = grain.atomCount;
				cluster->orientation = grain.orientation;
				cluster->color = grainColorList[grain.id % (sizeof(grainColorList)/sizeof(grainColorList[0]))];
			}
		}
	}

	for(size_t atomIndex = 0; atomIndex < _grains.size(); atomIndex++) {
		atomClusters()->setInt(atomIndex, parentGrain(atomIndex).id);
	}

	endProgressSubSteps();

	if(_probeSphereRadius > 0) {
		setProgressText(GrainSegmentationModifier::tr("Building grain boundary mesh"));
		if(!buildPartitionMesh())
			return;
	}
}

/******************************************************************************
* Calculates the misorientation angle between two lattice orientations.
******************************************************************************/
FloatType GrainSegmentationEngine::calculateMisorientation(const Grain& grainA, const Grain& grainB, Matrix3* alignmentTM)
{
	Cluster* clusterA = grainA.cluster;
	Cluster* clusterB = grainB.cluster;
	OVITO_ASSERT(clusterA != nullptr && clusterB != nullptr);
	Matrix3 inverseOrientationA = grainA.orientation.inverse();

	if(clusterB == clusterA) {
		if(alignmentTM) alignmentTM->setIdentity();
		return angleFromMatrix(grainB.orientation * inverseOrientationA);
	}
	else if(clusterA->structure == clusterB->structure) {
		const StructureAnalysis::LatticeStructure& latticeStructure = _structureAnalysis.latticeStructure(clusterA->structure);
		FloatType smallestAngle = FLOATTYPE_MAX;
		for(const StructureAnalysis::SymmetryPermutation& sge : latticeStructure.permutations) {
			FloatType angle = angleFromMatrix(grainB.orientation * sge.transformation * inverseOrientationA);
			if(angle < smallestAngle) {
				smallestAngle = angle;
				if(alignmentTM)
					*alignmentTM = sge.transformation;
			}
		}
		return smallestAngle;
	}
	else {
		if(ClusterTransition* t = _structureAnalysis.clusterGraph().determineClusterTransition(clusterA, clusterB)) {
			if(alignmentTM) *alignmentTM = t->tm;
			return angleFromMatrix(grainB.orientation * t->tm * inverseOrientationA);
		}
		return FLOATTYPE_MAX;
	}
}

/******************************************************************************
* Computes the angle of rotation from a rotation matrix.
******************************************************************************/
FloatType GrainSegmentationEngine::angleFromMatrix(const Matrix3& tm)
{
	FloatType trace = tm(0,0) + tm(1,1) + tm(2,2) - FloatType(1);
	Vector3 axis(tm(2,1) - tm(1,2), tm(0,2) - tm(2,0), tm(1,0) - tm(0,1));
	FloatType angle = atan2(axis.length(), trace);
	if(angle > FLOATTYPE_PI)
		return (FloatType(2) * FLOATTYPE_PI) - angle;
	else
		return angle;
}

/******************************************************************************
* Tests if two grain should be merged and merges them if deemed necessary.
******************************************************************************/
bool GrainSegmentationEngine::mergeTest(Grain& grainA, Grain& grainB, bool allowForFluctuations)
{
	if(&grainA == &grainB)
		return false;
	if(grainA.cluster == nullptr && grainB.cluster == nullptr)
		return false;

	if(grainA.cluster != nullptr && grainB.cluster != nullptr) {
		Matrix3 alignmentTM;
		FloatType misorientation = calculateMisorientation(grainA, grainB, &alignmentTM);

		if(allowForFluctuations)
			misorientation -= _fluctuationTolerance * sqrt(FloatType(1) / FloatType(grainA.latticeAtomCount) + FloatType(1) / FloatType(grainB.latticeAtomCount));

		if(misorientation >= _misorientationThreshold &&
		   grainA.latticeAtomCount >= _minGrainAtomCount && grainB.latticeAtomCount >= _minGrainAtomCount)
			return false;

		// Join the two grains.
		if(grainA.rank > grainB.rank) {
			grainA.join(grainB, alignmentTM);
		}
		else {
			grainB.join(grainA, alignmentTM.inverse());
			if(grainA.rank == grainB.rank)
				grainB.rank++;
		}
	}
	else {
		// Join the crystal grain and the cluster of disordered atoms.
		if(grainA.cluster != nullptr)
			grainA.join(grainB);
		else
			grainB.join(grainA);
	}

	return true;
}

/******************************************************************************
* Assigns contiguous IDs to all parent grains.
******************************************************************************/
size_t GrainSegmentationEngine::assignIdsToGrains()
{
	size_t numGrains = 0;
	for(size_t atomIndex = 0; atomIndex < _grains.size(); atomIndex++) {
		Grain& atomicGrain = _grains[atomIndex];
		Grain& rootGrain = parentGrain(atomIndex);
		if(rootGrain.cluster != nullptr) {
			OVITO_ASSERT(rootGrain.atomCount >= _minGrainAtomCount);
			if(atomicGrain.isRoot()) {
				rootGrain.id = ++numGrains;
			}
		}
		else {
			rootGrain.id = 0;
		}
	}
	return numGrains;
}

/** Find the most common element in the [first, last) range.

    O(n) in time; O(1) in space.

    [first, last) must be valid sorted range.
    Elements must be equality comparable.
*/
template <class ForwardIterator>
ForwardIterator most_common(ForwardIterator first, ForwardIterator last)
{
	ForwardIterator it(first), max_it(first);
	size_t count = 0, max_count = 0;
	for( ; first != last; ++first) {
		if(*it == *first)
			count++;
		else {
			it = first;
			count = 1;
		}
		if(count > max_count) {
			max_count = count;
			max_it = it;
		}
	}
	return max_it;
}

/******************************************************************************
* Builds the triangle mesh for the grain boundaries.
******************************************************************************/
bool GrainSegmentationEngine::buildPartitionMesh()
{
	double alpha = _probeSphereRadius * _probeSphereRadius;
	FloatType ghostLayerSize = _probeSphereRadius * 3.0f;

	// Check if combination of radius parameter and simulation cell size is valid.
	for(size_t dim = 0; dim < 3; dim++) {
		if(cell().pbcFlags()[dim]) {
			int stencilCount = (int)ceil(ghostLayerSize / cell().matrix().column(dim).dot(cell().cellNormalVector(dim)));
			if(stencilCount > 1)
				throw Exception(GrainSegmentationModifier::tr("Cannot generate Delaunay tessellation. Simulation cell is too small or probe sphere radius parameter is too large."));
		}
	}

	_mesh = new PartitionMeshData();

	// If there are too few particles, don't build Delaunay tessellation.
	// It is going to be invalid anyway.
	size_t numInputParticles = positions()->size();
	if(selection())
		numInputParticles = positions()->size() - std::count(selection()->constDataInt(), selection()->constDataInt() + selection()->size(), 0);
	if(numInputParticles <= 3)
		return true;

	// The algorithm is divided into several sub-steps.
	// Assign weights to sub-steps according to estimated runtime.
	beginProgressSubSteps({ 20, 10, 1 });

	// Generate Delaunay tessellation.
	DelaunayTessellation tessellation;
	if(!tessellation.generateTessellation(cell(), positions()->constDataPoint3(), positions()->size(), ghostLayerSize,
			selection() ? selection()->constDataInt() : nullptr, *this))
		return false;

	nextProgressSubStep();

	// Determines the grain a Delaunay cell belongs to.
	auto tetrahedronRegion = [this, &tessellation](DelaunayTessellation::CellHandle cell) {
		std::array<int,4> clusters;
		for(int v = 0; v < 4; v++)
			clusters[v] = atomClusters()->getInt(tessellation.vertexIndex(tessellation.cellVertex(cell, v)));
		std::sort(std::begin(clusters), std::end(clusters));
		return (*most_common(std::begin(clusters), std::end(clusters)) + 1);
	};

	// Assign triangle faces to grains.
	auto prepareMeshFace = [this, &tessellation](PartitionMeshData::Face* face, const std::array<int,3>& vertexIndices, const std::array<DelaunayTessellation::VertexHandle,3>& vertexHandles, DelaunayTessellation::CellHandle cell) {
		face->region = tessellation.getUserField(cell) - 1;
	};

	// Cross-links adjacent manifolds.
	auto linkManifolds = [](PartitionMeshData::Edge* edge1, PartitionMeshData::Edge* edge2) {
		OVITO_ASSERT(edge1->nextManifoldEdge == nullptr || edge1->nextManifoldEdge == edge2);
		OVITO_ASSERT(edge2->nextManifoldEdge == nullptr || edge2->nextManifoldEdge == edge1);
		OVITO_ASSERT(edge2->vertex2() == edge1->vertex1());
		OVITO_ASSERT(edge2->vertex1() == edge1->vertex2());
		OVITO_ASSERT(edge1->face()->oppositeFace == nullptr || edge1->face()->oppositeFace == edge2->face());
		OVITO_ASSERT(edge2->face()->oppositeFace == nullptr || edge2->face()->oppositeFace == edge1->face());
		edge1->nextManifoldEdge = edge2;
		edge2->nextManifoldEdge = edge1;
		edge1->face()->oppositeFace = edge2->face();
		edge2->face()->oppositeFace = edge1->face();
	};

	ManifoldConstructionHelper<PartitionMeshData, true, true> manifoldConstructor(tessellation, *_mesh, alpha, positions());
	if(!manifoldConstructor.construct(tetrahedronRegion, *this, prepareMeshFace, linkManifolds))
		return false;
	_spaceFillingGrain = manifoldConstructor.spaceFillingRegion();

	nextProgressSubStep();

	std::vector<PartitionMeshData::Edge*> visitedEdges;
	std::vector<PartitionMeshData::Vertex*> visitedVertices;
	size_t oldVertexCount = _mesh->vertices().size();
	for(size_t vertexIndex = 0; vertexIndex < oldVertexCount; vertexIndex++) {
		if(isCanceled())
			return false;

		PartitionMeshData::Vertex* vertex = _mesh->vertices()[vertexIndex];
		visitedEdges.clear();
		// Visit all manifolds that this vertex is part of.
		for(PartitionMeshData::Edge* startEdge = vertex->edges(); startEdge != nullptr; startEdge = startEdge->nextVertexEdge()) {
			if(std::find(visitedEdges.cbegin(), visitedEdges.cend(), startEdge) != visitedEdges.cend()) continue;
			// Traverse the manifold around the current vertex edge by edge.
			// Detect if there are two edges connecting to the same neighbor vertex.
			visitedVertices.clear();
			PartitionMeshData::Edge* endEdge = startEdge;
			PartitionMeshData::Edge* currentEdge = startEdge;
			do {
				OVITO_ASSERT(currentEdge->vertex1() == vertex);
				OVITO_ASSERT(std::find(visitedEdges.cbegin(), visitedEdges.cend(), currentEdge) == visitedEdges.cend());

				if(std::find(visitedVertices.cbegin(), visitedVertices.cend(), currentEdge->vertex2()) != visitedVertices.cend()) {
					// Encountered the same neighbor vertex twice.
					// That means the manifold is self-intersecting and we should split the central vertex

					// Retrieve the other edge where the manifold intersects itself.
					auto iter = std::find_if(visitedEdges.rbegin(), visitedEdges.rend(), [currentEdge](PartitionMeshData::Edge* e) {
						return e->vertex2() == currentEdge->vertex2();
					});
					OVITO_ASSERT(iter != visitedEdges.rend());
					PartitionMeshData::Edge* otherEdge = *iter;

					// Rewire edges to produce two separate manifolds.
					PartitionMeshData::Edge* oppositeEdge1 = otherEdge->unlinkFromOppositeEdge();
					PartitionMeshData::Edge* oppositeEdge2 = currentEdge->unlinkFromOppositeEdge();
					currentEdge->linkToOppositeEdge(oppositeEdge1);
					otherEdge->linkToOppositeEdge(oppositeEdge2);

					// Split the vertex.
					PartitionMeshData::Vertex* newVertex = _mesh->createVertex(vertex->pos());

					// Transfer one group of manifolds to the new vertex.
					std::vector<PartitionMeshData::Edge*> transferredEdges;
					std::deque<PartitionMeshData::Edge*> edgesToBeVisited;
					edgesToBeVisited.push_back(otherEdge);
					do {
						PartitionMeshData::Edge* edge = edgesToBeVisited.front();
						edgesToBeVisited.pop_front();
						PartitionMeshData::Edge* iterEdge = edge;
						do {
							PartitionMeshData::Edge* iterEdge2 = iterEdge;
							do {
								if(std::find(transferredEdges.cbegin(), transferredEdges.cend(), iterEdge2) == transferredEdges.cend()) {
									vertex->transferEdgeToVertex(iterEdge2, newVertex);
									transferredEdges.push_back(iterEdge2);
									edgesToBeVisited.push_back(iterEdge2);
								}
								iterEdge2 = iterEdge2->oppositeEdge()->nextManifoldEdge;
								OVITO_ASSERT(iterEdge2 != nullptr);
							}
							while(iterEdge2 != iterEdge);
							iterEdge = iterEdge->prevFaceEdge()->oppositeEdge();
						}
						while(iterEdge != edge);
					}
					while(!edgesToBeVisited.empty());

					if(otherEdge == endEdge) {
						endEdge = currentEdge;
					}
				}
				visitedVertices.push_back(currentEdge->vertex2());
				visitedEdges.push_back(currentEdge);

				currentEdge = currentEdge->prevFaceEdge()->oppositeEdge();
			}
			while(currentEdge != endEdge);
		}
	}

	// Smooth the generated triangle mesh.
	if(!PartitionMesh::smoothMesh(*_mesh, cell(), _meshSmoothingLevel, *this))
		return false;

	// Make sure every mesh vertex is only part of one surface manifold.
	_mesh->duplicateSharedVertices();

	endProgressSubSteps();

	return true;
}

}	// End of namespace
}	// End of namespace
}	// End of namespace

