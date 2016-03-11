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
///////////////////////////////////////////////////////////////////////////////
//
//  The code for this modifier has been contributed by
//  Emanuel A. Lazar <mlazar@seas.upenn.edu>
//
///////////////////////////////////////////////////////////////////////////////

#include <plugins/vorotop/VoroTopPlugin.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include <core/gui/properties/BooleanParameterUI.h>
#include <core/gui/properties/BooleanGroupBoxParameterUI.h>
#include <core/gui/properties/IntegerParameterUI.h>
#include <core/gui/properties/FloatParameterUI.h>
#include <plugins/particles/util/NearestNeighborFinder.h>
#include "VoroTopModifier.h"

#include <voro++.hh>

#include <iostream>
#include <fstream>

namespace Ovito { namespace Plugins { namespace VoroTop {

IMPLEMENT_SERIALIZABLE_OVITO_OBJECT(VoroTop, VoroTopModifier, StructureIdentificationModifier);
IMPLEMENT_OVITO_OBJECT(VoroTop, VoroTopModifierEditor, ParticleModifierEditor);
SET_OVITO_OBJECT_EDITOR(VoroTopModifier, VoroTopModifierEditor);
DEFINE_PROPERTY_FIELD(VoroTopModifier, _useRadii, "UseRadii");
SET_PROPERTY_FIELD_LABEL(VoroTopModifier, _useRadii, "Use particle radii");

/******************************************************************************
* Constructs the modifier object.
******************************************************************************/
VoroTopModifier::VoroTopModifier(DataSet* dataset) : StructureIdentificationModifier(dataset),
	_useRadii(false)
{
	INIT_PROPERTY_FIELD(VoroTopModifier::_useRadii);

	// Create the structure types.
	createStructureType(OTHER, ParticleTypeProperty::PredefinedStructureType::OTHER);
	createStructureType(FCC, ParticleTypeProperty::PredefinedStructureType::FCC);
	createStructureType(BCC, ParticleTypeProperty::PredefinedStructureType::BCC);
	createStructureType(HCP, ParticleTypeProperty::PredefinedStructureType::HCP);

	OORef<ParticleType> stype(new ParticleType(dataset));
	stype->setId(FCC_HCP);
	stype->setName(tr("FCC/HCP"));
	stype->setColor(Color(1.0, 0.6, 0.2));
	addStructureType(stype);
}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
std::shared_ptr<AsynchronousParticleModifier::ComputeEngine> VoroTopModifier::createEngine(TimePoint time, TimeInterval validityInterval)
{
	if(structureTypes().size() != NUM_STRUCTURE_TYPES)
		throw Exception(tr("The number of structure types has changed. Please remove this modifier from the modification pipeline and insert it again."));

	// Get the current positions.
	ParticlePropertyObject* posProperty = expectStandardProperty(ParticleProperty::PositionProperty);

	// Get simulation cell.
	SimulationCellObject* inputCell = expectSimulationCell();

	// Get selection particle property.
	ParticlePropertyObject* selectionProperty = nullptr;
	if(onlySelectedParticles())
		selectionProperty = expectStandardProperty(ParticleProperty::SelectionProperty);

	// Get particle radii.
	std::vector<FloatType> radii;
	if(useRadii())
		radii = std::move(inputParticleRadii(time, validityInterval));

	// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
	return std::make_shared<VoroTopAnalysisEngine>(
			validityInterval,
			posProperty->storage(),
			selectionProperty ? selectionProperty->storage() : nullptr,
			std::move(radii),
			inputCell->data());
}

/******************************************************************************
* Processes a single Voronoi cell.
******************************************************************************/
void VoroTopModifier::VoroTopAnalysisEngine::processCell(voro::voronoicell_neighbor& vcell, size_t particleIndex, QMutex* mutex)
{
    // COMPUTES THE TOPOLOGY OF THE VORONOI CELL OF A PARTICLE; THIS IS
    // STORED AS A VECTOR OF INTEGERS, WHICH WE CALL A CODE.  THIS CODE
    // IS THEN COMPARED AGAINST A LIST OF CODES KNOWN TO BE ASSOCIATED
    // WITH PARTICULAR STRUCTURES:
    //   1. FCC
    //   2. BCC
    //   3. FCC-HCP
    //   4. HCP
    
#include "FILTER-FCC-BCC-both-HCP"

    // NO BCC/FCC/HCP TYPE HAS MORE THAN 32 VERTICES
    if(vcell.p>32)
    {
        structures()->setInt(particleIndex, OTHER);
        return;
    }

    int p = vcell.p;        // TOTAL NUMBER OF VERTICES
    int* nu = vcell.nu;     // VERTEX DEGREE ARRAY
    int** ed = vcell.ed;    // EDGE CONNECTIONS ARRAY
    
    unsigned int total_faces         = 0;
    unsigned int total_valence       = 0;
    std::vector<int> pvector(5,0);  // RECORDS FACES WITH PARTICULAR NUMBER OF EDGES

    int min_edges   = 5;         // EVERY CONVEX POLYHEDRON MUST HAVE AT LEAST ONE FACE WITH 5 OR FEWER EDGES
    int max_valence = 3;
    std::vector<int> lowest_adjacent_face_degree(p,6);

    for(int i=0;i<vcell.p;i++)
    {
        total_valence += nu[i];
        if(nu[i]>max_valence)
            max_valence = nu[i];

        // NO TYPE IN OUR LIST HAS VERTICES WITH VALENCE>4
        if(max_valence>4)
        {
            structures()->setInt(particleIndex, OTHER);
            return;
        }
        
        for(int j=0;j<nu[i];j++)
        {
            int k=ed[i][j];
            if(k>=0)
            {
                std::vector<int> temp;

                unsigned int side_count = 1;
                ed[i][j]=-1-k;
                int l=vcell.cycle_up(ed[i][nu[i]+j],k);
                temp.push_back(k);
                do {
                    side_count++;
                    int m=ed[k][l];
                    ed[k][l]=-1-m;
                    l=vcell.cycle_up(ed[k][nu[k]+l],m);
                    temp.push_back(m);
                    k=m;
                } while (k!=i);

                for(int c=0; c<temp.size(); c++)
                    if(side_count<lowest_adjacent_face_degree[temp[c]]) lowest_adjacent_face_degree[temp[c]] = side_count;

                if(side_count<pvector.size()) pvector[side_count]++;
                else
                {
                    pvector.resize(side_count+1,0);
                    pvector[side_count]++;
                }
                if(side_count<min_edges) min_edges=side_count;
                total_faces++;
            }
        }
    }
    
    int i,j;
    for(i=0;i<p;i++) for(j=0;j<nu[i];j++) {
        if(ed[i][j]>=0) voro::voro_fatal_error("Edge reset routine found a previously untested edge",VOROPP_INTERNAL_ERROR);
        ed[i][j]=-1-ed[i][j];
    }
    
    int likely_bcc=0;
    if(total_faces==14 && pvector[4]==6 && pvector[6]==8 && max_valence==3) likely_bcc=1;   // PVECTOR (0,6,0,8,0,...) APPEARS IN 3 DIFFERENT TYPES, WITH SYMMETRIES 4, 8, AND 48

    int code_length;
    int edge_count = total_faces + p - 2;

    // NO BCC/FCC/HCP TYPE HAS MORE THAN 48 EDGES
    if(edge_count > 48)
    {
        structures()->setInt(particleIndex, OTHER);
        return;
    }
    
    int canonical_code[96];
    int all_vertex_temp_label[32];
    
    // THIS TRACKS WHICH VERTEX/EDGE PAIRS WE HAVE VISITED
    int vertex_visited[32][4];
    
    // IF E=edge_count IS THE NUMBER OF EDGES IN THE POLYHEDRON, THEN WE WILL
    // CONSTRUCT 4E CODES, EACH OF LENGTH 2E.
    int first_code =  1;        // IF THIS IS THE FIRST CODE RECORDED
    int finished   =  0;
    int chirality  = -1;
    
    int symmetry_counter=0;     // TRACKS NUMBER OF REPEATS OF A CODE, I.E. SYMMETRY ORDER
    
    for(int orientation=0; orientation<2 && finished==0; orientation++)
    {
        // FOR EACH ORIENTATION, WE CONSTRUCT THREE CODES FOR EACH
        // STARTING VERTEX, ONE FOR EACH EDGE LEAVING IT.
        for(int i=p; i--;)
        {
            if(lowest_adjacent_face_degree[i]>min_edges) continue; // THIS VERTEX IS ADJACENT ONLY TO MANY-EDGED FACES

            for(int j=0; j<nu[i] && finished==0; j++)
            {
                code_length=0;
                int globalhigh=0;
                int continue_code = 0;    // 0: UNDECIDED; 1: GO AHEAD, DO NOT EVEN CHECK.
                
                // FIRST CLEAR ALL LABELS, AND MARK ALL NEIGHBORS OF ALL VERTICES AS UNVISITED
                for(int k=p; k--; )
                {
                    all_vertex_temp_label[k] = -1;
                    for(int l=nu[k]; l--;)
                        vertex_visited[k][l] =  0;
                }
                
                all_vertex_temp_label[i] = globalhigh++;
                canonical_code[code_length]=all_vertex_temp_label[i];
                code_length++;

                vertex_visited[i][j]=1;
                
                int end_flag=0;
                int from = i;
                int next = ed[i][j];
                
                // THIS SECTION BUILDS EACH CODE, FOLLOWING THE WEINBERG RULES FOR TRAVERSING A
                // GRAPH MAKING A HAMILTONIAN PATH, LABELING THE VERTICES ALONG THE WAY, AND
                // RECORDING THE VERTICES VISITED.
                while(end_flag==0)
                {
                    int last=0;
                    for(int k=0; k<nu[next]; k++)
                        if(ed[next][k]==from) last = k;
                    
                    int valence = nu[next];
                    
                    int open = -1;                  // WE DETERMINE THE FIRST OPEN NEIGHBORING PATH
                    for(int k=0; k<valence; k++)
                    {
                        if(last+k<valence)
                        {
                            if(vertex_visited[next][last+k]==0)
                            {
                                open = k;
                                break;
                            }
                        }
                        else
                            if(vertex_visited[next][last+k-valence]==0)
                            {
                                open = k;
                                break;
                            }
                    }
                    
                    if(all_vertex_temp_label[next] < 0)
                    {
                        all_vertex_temp_label[next] = globalhigh++;

                        if(first_code==1) canonical_code[code_length]=all_vertex_temp_label[next];
                        else
                        {
                            if(continue_code==0)
                            {
                                if(all_vertex_temp_label[next]>canonical_code[code_length]) break;
                                if(all_vertex_temp_label[next]<canonical_code[code_length]) { symmetry_counter=0; continue_code=1; if(orientation==1) chirality=1;}
                            }
                            if(continue_code==1) canonical_code[code_length] = all_vertex_temp_label[next];
                        }
                        code_length++;
                        
                        if(last+1<valence) vertex_visited[next][last+1]=1;
                        else               vertex_visited[next][last+1-valence]=1;
                        from = next;
                        
                        if(last+1<valence) next = ed[next][last+1];          // THE LAST+1 IS A RIGHT TURN
                        else               next = ed[next][last+1-valence];  // THE LAST+1 IS A RIGHT TURN
                    }
                    
                    else if(open != -1)
                    {
                        if(last+open<valence) vertex_visited[next][last+open]        =1;
                        else                  vertex_visited[next][last+open-valence]=1;
                        
                        if(first_code==1) canonical_code[code_length]=all_vertex_temp_label[next];
                        else
                        {
                            if(continue_code==0)
                            {
                                if(all_vertex_temp_label[next]>canonical_code[code_length]) break;
                                if(all_vertex_temp_label[next]<canonical_code[code_length]) { symmetry_counter=0; continue_code=1; if(orientation==1) chirality=1;}
                            }
                            if(continue_code==1) canonical_code[code_length] = all_vertex_temp_label[next];
                        }
                        code_length++;
                        
                        from = next;
                        
                        if(last+open<valence) next = ed[next][last+open];         // THE LAST+1 IS A RIGHT TURN
                        else                  next = ed[next][last+open-valence]; // THE LAST+1 IS A RIGHT TURN
                    }
                    
                    else
                    {
                        end_flag=1;
                        if(likely_bcc) if(std::memcmp(filter[26526],canonical_code,sizeof(int)*2*edge_count)==0)
                        {
                            structures()->setInt(particleIndex, BCC);
                            return;
                        }
                        if(chirality==-1 && orientation==1)               { chirality=0; symmetry_counter *= 2; finished=1; }
                        else    symmetry_counter++;
                    }
                }

                first_code=0;
            }
        }
        
        // AFTER MAKING ALL CODES FOR ONE ORIENTATION, FLIP ORIENTATION OF EDGES AT EACH
        // VERTEX, AND REPEAT THE ABOVE FOR THE OPPOSITE ORIENTATION.
        if(orientation==0)
            for(int i=p;i--;)
            {
                for(int j=nu[i]/2; j--;)
                {
                    int temp = ed[i][j];
                    ed[i][j] = ed[i][nu[i]-j-1];
                    ed[i][nu[i]-j-1] = temp;
                }
            }
    }
    
    
    
    // WE NOW HAVE A CANONICAL CODE.  IMPLEMENT A FAST BINARY
    // TO FIND ASSOCIATED TYPE, IF LISTED.
    int type=OTHER;
    int first=0;
    int last=total_types-1;
    int middle = (first+last)/2;
    
    while( first <= last )
    {
        if (std::memcmp(filter[middle],canonical_code,sizeof(int)*2*edge_count)<0) first = middle + 1;
        else if (std::memcmp(filter[middle],canonical_code,sizeof(int)*2*edge_count)==0)
        {
            type = types[middle];
            break;
        }
        else
            last = middle - 1;
        
        middle = (first + last)/2;
    }
    if ( first > last )
        type = OTHER;


    // HERE WE ASSIGN STRUCTURE TYPE
	structures()->setInt(particleIndex, type);
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void VoroTopModifier::VoroTopAnalysisEngine::perform()
{
	setProgressText(tr("Performing VoroTop analysis"));

	if(positions()->size() == 0)
		return;	// Nothing to do

	// Decide whether to use Voro++ container class or our own implementation.
	if(cell().isAxisAligned()) {
		// Use Voro++ container.
		double ax = cell().matrix()(0,3);
		double ay = cell().matrix()(1,3);
		double az = cell().matrix()(2,3);
		double bx = ax + cell().matrix()(0,0);
		double by = ay + cell().matrix()(1,1);
		double bz = az + cell().matrix()(2,2);
		if(ax > bx) std::swap(ax,bx);
		if(ay > by) std::swap(ay,by);
		if(az > bz) std::swap(az,bz);
		double volumePerCell = (bx - ax) * (by - ay) * (bz - az) * voro::optimal_particles / positions()->size();
		double cellSize = pow(volumePerCell, 1.0/3.0);
		int nx = (int)std::ceil((bx - ax) / cellSize);
		int ny = (int)std::ceil((by - ay) / cellSize);
		int nz = (int)std::ceil((bz - az) / cellSize);

		if(_radii.empty()) {
			voro::container voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
					cell().pbcFlags()[0], cell().pbcFlags()[1], cell().pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

			// Insert particles into Voro++ container.
			size_t count = 0;
			for(size_t index = 0; index < positions()->size(); index++) {
				// Skip unselected particles (if requested).
				if(selection() && selection()->getInt(index) == 0) {
					structures()->setInt(index, OTHER);
					continue;
				}
				const Point3& p = positions()->getPoint3(index);
				voroContainer.put(index, p.x(), p.y(), p.z());
				count++;
			}
			if(!count) return;

			setProgressRange(count);
			setProgressValue(0);
			voro::c_loop_all cl(voroContainer);
			voro::voronoicell_neighbor v;
			if(cl.start()) {
				do {
					if(!incrementProgressValue())
						return;
					if(!voroContainer.compute_cell(v,cl))
						continue;
					processCell(v, cl.pid(), nullptr);
					count--;
				}
				while(cl.inc());
			}
			if(count)
				throw Exception(tr("Could not compute Voronoi cell for some particles."));
		}
		else {
			voro::container_poly voroContainer(ax, bx, ay, by, az, bz, nx, ny, nz,
					cell().pbcFlags()[0], cell().pbcFlags()[1], cell().pbcFlags()[2], (int)std::ceil(voro::optimal_particles));

			// Insert particles into Voro++ container.
			size_t count = 0;
			for(size_t index = 0; index < positions()->size(); index++) {
				structures()->setInt(index, OTHER);
				// Skip unselected particles (if requested).
				if(selection() && selection()->getInt(index) == 0) {
					continue;
				}
				const Point3& p = positions()->getPoint3(index);
				voroContainer.put(index, p.x(), p.y(), p.z(), _radii[index]);
				count++;
			}

			if(!count) return;
			setProgressRange(count);
			setProgressValue(0);
			voro::c_loop_all cl(voroContainer);
			voro::voronoicell_neighbor v;
			if(cl.start()) {
				do {
					if(!incrementProgressValue())
						return;
					if(!voroContainer.compute_cell(v,cl))
						continue;
					processCell(v, cl.pid(), nullptr);
					count--;
				}
				while(cl.inc());
			}
			if(count)
				throw Exception(tr("Could not compute Voronoi cell for some particles."));
		}
	}
	else {
		// Prepare the nearest neighbor list generator.
		NearestNeighborFinder nearestNeighborFinder;
		if(!nearestNeighborFinder.prepare(positions(), cell(), selection(), this))
			return;

		// Squared particle radii (input was just radii).
		for(auto& r : _radii)
			r = r*r;

		// This is the size we use to initialize Voronoi cells. Must be larger than the simulation box.
		double boxDiameter = sqrt(
				  cell().matrix().column(0).squaredLength()
				+ cell().matrix().column(1).squaredLength()
				+ cell().matrix().column(2).squaredLength());

		// The normal vectors of the three cell planes.
		std::array<Vector3,3> planeNormals;
		planeNormals[0] = cell().cellNormalVector(0);
		planeNormals[1] = cell().cellNormalVector(1);
		planeNormals[2] = cell().cellNormalVector(2);

		Point3 corner1 = Point3::Origin() + cell().matrix().column(3);
		Point3 corner2 = corner1 + cell().matrix().column(0) + cell().matrix().column(1) + cell().matrix().column(2);

		QMutex mutex;

		// Perform analysis, particle-wise parallel.
		parallelFor(positions()->size(), *this,
				[&nearestNeighborFinder, this, boxDiameter,
				 planeNormals, corner1, corner2, &mutex](size_t index) {

			// Reset structure type.
			structures()->setInt(index, OTHER);

			// Skip unselected particles (if requested).
			if(selection() && selection()->getInt(index) == 0)
				return;

			// Build Voronoi cell.
			voro::voronoicell_neighbor v;

			// Initialize the Voronoi cell to be a cube larger than the simulation cell, centered at the origin.
			v.init(-boxDiameter, boxDiameter, -boxDiameter, boxDiameter, -boxDiameter, boxDiameter);

			// Cut Voronoi cell at simulation cell boundaries in non-periodic directions.
			bool skipParticle = false;
			for(size_t dim = 0; dim < 3; dim++) {
				if(!cell().pbcFlags()[dim]) {
					double r;
					r = 2 * planeNormals[dim].dot(corner2 - positions()->getPoint3(index));
					if(r <= 0) skipParticle = true;
					v.nplane(planeNormals[dim].x() * r, planeNormals[dim].y() * r, planeNormals[dim].z() * r, r*r, -1);
					r = 2 * planeNormals[dim].dot(positions()->getPoint3(index) - corner1);
					if(r <= 0) skipParticle = true;
					v.nplane(-planeNormals[dim].x() * r, -planeNormals[dim].y() * r, -planeNormals[dim].z() * r, r*r, -1);
				}
			}
			// Skip particles that are located outside of non-periodic box boundaries.
			if(skipParticle)
				return;

			// This function will be called for every neighbor particle.
			int nvisits = 0;
			auto visitFunc = [this, &v, &nvisits, index](const NearestNeighborFinder::Neighbor& n, FloatType& mrs) {
				// Skip unselected particles (if requested).
				OVITO_ASSERT(!selection() || selection()->getInt(n.index));
				FloatType rs = n.distanceSq;
				if(!_radii.empty())
					 rs += _radii[index] - _radii[n.index];
				v.nplane(n.delta.x(), n.delta.y(), n.delta.z(), rs, n.index);
				if(nvisits == 0) {
					mrs = v.max_radius_squared();
					nvisits = 100;
				}
				nvisits--;
			};

			// Visit all neighbors of the current particles.
			nearestNeighborFinder.visitNeighbors(nearestNeighborFinder.particlePos(index), visitFunc);

			processCell(v, index, &mutex);
		});
	}
}

/******************************************************************************
* Is called when the value of a property of this object has changed.
******************************************************************************/
void VoroTopModifier::propertyChanged(const PropertyFieldDescriptor& field)
{
	StructureIdentificationModifier::propertyChanged(field);

	// Recompute modifier results when the parameters change.
	if(field == PROPERTY_FIELD(VoroTopModifier::_useRadii))
		invalidateCachedResults();
}

/******************************************************************************
* Sets up the UI widgets of the editor.
******************************************************************************/
void VoroTopModifierEditor::createUI(const RolloutInsertionParameters& rolloutParams)
{
	// Create a rollout.
	QWidget* rollout = createRollout(tr("VoroTop analysis"), rolloutParams);

    // Create the rollout contents.
	QVBoxLayout* layout = new QVBoxLayout(rollout);
	layout->setContentsMargins(4,4,4,4);
	layout->setSpacing(4);

	QGridLayout* gridlayout = new QGridLayout();
	QGridLayout* sublayout;
	gridlayout->setContentsMargins(4,4,4,4);
	gridlayout->setSpacing(4);
	gridlayout->setColumnStretch(1, 1);
	int row = 0;

	// Atomic radii.
	BooleanParameterUI* useRadiiPUI = new BooleanParameterUI(this, PROPERTY_FIELD(VoroTopModifier::_useRadii));
	gridlayout->addWidget(useRadiiPUI->checkBox(), row++, 0, 1, 2);

	// Only selected particles.
	BooleanParameterUI* onlySelectedPUI = new BooleanParameterUI(this, PROPERTY_FIELD(StructureIdentificationModifier::_onlySelectedParticles));
	gridlayout->addWidget(onlySelectedPUI->checkBox(), row++, 0, 1, 2);

	layout->addLayout(gridlayout);

	// Status label.
	layout->addSpacing(6);
	layout->addWidget(statusLabel());

	StructureListParameterUI* structureTypesPUI = new StructureListParameterUI(this);
	layout->addSpacing(10);
	layout->addWidget(new QLabel(tr("Structure types:")));
	layout->addWidget(structureTypesPUI->tableWidget());
	QLabel* label = new QLabel(tr("<p style=\"font-size: small;\">Double-click to change colors. Defaults can be set in the application settings.</p>"));
	label->setWordWrap(true);
	layout->addWidget(label);
}

}	// End of namespace
}	// End of namespace
}	// End of namespace
