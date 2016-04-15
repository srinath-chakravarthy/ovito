/*
 *  Copyright (c) 2012-2014, Bruno Levy
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *  * Neither the name of the ALICE Project-Team nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  If you modify this software, you should include a notice giving the
 *  name of the person performing the modification, the date of modification,
 *  and the reason for such modification.
 *
 *  Contact: Bruno Levy
 *
 *     Bruno.Levy@inria.fr
 *     http://www.loria.fr/~levy
 *
 *     ALICE Project
 *     LORIA, INRIA Lorraine, 
 *     Campus Scientifique, BP 239
 *     54506 VANDOEUVRE LES NANCY CEDEX 
 *     FRANCE
 *
 */

#include <geogram/delaunay/delaunay.h>
#include <geogram/delaunay/delaunay_3d.h>
#include <geogram/basic/geometry_nd.h>
#include <geogram/basic/algorithm.h>

#include <fstream>
#include <sstream>

namespace {

    using namespace GEO;

    /**
     * \brief Builds the invalid dimension error message
     * \param[in] dimension the specified dimension
     * \param[in] name the name of the Delaunay implementation
     * \param[in] expected the expected dimension
     * \return a string that contains the error message
     */
    std::string invalid_dimension_error(
        coord_index_t dimension,
        const char* name,
        const char* expected
    ) {
        std::ostringstream out;
        out << "Invalid dimension: dimension " << index_t(dimension)
            << " is not supported by the " << name
            << " algorithm. Supported dimension(s): " << expected;
        return out.str();
    }
}

namespace GEO {

    Delaunay::InvalidDimension::InvalidDimension(
        coord_index_t dimension,
        const char* name,
        const char* expected
    ) :
        std::logic_error(invalid_dimension_error(dimension, name, expected)) {
    }

    const char* Delaunay::InvalidDimension::what() const throw () {
        return std::logic_error::what();
    }

    /************************************************************************/

    void Delaunay::initialize() {
        geo_register_Delaunay_creator(Delaunay3d, "BDEL");
        geo_register_Delaunay_creator(RegularWeightedDelaunay3d, "BPOW");
    }

    Delaunay::Delaunay(coord_index_t dimension) {
        set_dimension(dimension);
        vertices_ = nil;
        nb_vertices_ = 0;
        nb_cells_ = 0;
        cell_to_v_ = nil;
        cell_to_cell_ = nil;
        is_locked_ = false;
        store_neighbors_ = false;
        default_nb_neighbors_ = 30;
        do_reorder_ = true;
        store_cicl_ = false;
        keep_infinite_ = false;
        nb_finite_cells_ = 0;
    }

    Delaunay::~Delaunay() {
    }

    void Delaunay::set_vertices(
        index_t nb_vertices, const double* vertices
    ) {
        nb_vertices_ = nb_vertices;
        vertices_ = vertices;
        if(nb_vertices_ < index_t(dimension()) + 1) {
            std::cerr << "Only "
                << nb_vertices
                << " vertices, may be not enough !"
                << std::endl;
        }
    }

    void Delaunay::set_BRIO_levels(const vector<index_t>& levels) {
        geo_argused(levels);
        // Default implementation does nothing
    }

    void Delaunay::set_arrays(
        index_t nb_cells,
        const signed_index_t* cell_to_v, const signed_index_t* cell_to_cell
    ) {
        nb_cells_ = nb_cells;
        cell_to_v_ = cell_to_v;
        cell_to_cell_ = cell_to_cell;

        if(cell_to_cell != nil) {
            if(store_cicl_) {
                update_v_to_cell();
                update_cicl();
            }
            if(store_neighbors_) {
                update_neighbors();
            }
        }
    }

    index_t Delaunay::nearest_vertex(const double* p) const {
        // Unefficient implementation (but at least it works).
        // Derived classes are supposed to overload.
        geo_assert(nb_vertices() > 0);
        index_t result = 0;
        double d = Geom::distance2(vertex_ptr(0), p, dimension());
        for(index_t i = 1; i < nb_vertices(); i++) {
            double cur_d = Geom::distance2(vertex_ptr(i), p, dimension());
            if(cur_d < d) {
                d = cur_d;
                result = i;
            }
        }
        return result;
    }

    void Delaunay::update_neighbors() {
        if(nb_vertices() != neighbors_.nb_arrays()) {
            neighbors_.init(
                nb_vertices(),
                default_nb_neighbors_
            );
            for(index_t i = 0; i < nb_vertices(); i++) {
                neighbors_.resize_array(i, default_nb_neighbors_, false);
            }
        }
        for(index_t i = 0; i < nb_vertices(); i++)
            store_neighbors_CB(i);
    }

    void Delaunay::get_neighbors_internal(
        index_t v, vector<index_t>& neighbors
    ) const {
        // Step 1: traverse the incident cells list, and insert
        // all neighbors (may be duplicated)
        neighbors.resize(0);
        signed_index_t vt = v_to_cell_[v];
        if(vt != -1) { // Happens when there are duplicated vertices.
            index_t t = index_t(vt);
            do {
                index_t lvit = index(t, signed_index_t(v));
                // In the current cell, test all edges incident
                // to current vertex 'it'
                for(index_t lv = 0; lv < cell_size(); lv++) {
                    if(lvit != lv) {
                        signed_index_t neigh = cell_vertex(t, lv);
                        geo_debug_assert(neigh != -1);
                        neighbors.push_back(index_t(neigh));
                    }
                }
                t = index_t(next_around_vertex(t, index(t, signed_index_t(v))));
            } while(t != index_t(vt));
        }

        // Step 2: Sort the neighbors and remove all duplicates
        sort_unique(neighbors);
    }

    void Delaunay::store_neighbors_CB(index_t i) {
        // TODO: this one is not multithread-friendly
        // since it does dynamic memory allocation
        // (but not really a problem, since the one
        // that is used is in Delaunay_ANN).
        vector<index_t> neighbors;
        get_neighbors_internal(i, neighbors);
        neighbors_.set_array(i, neighbors);
    }

    void Delaunay::update_v_to_cell() {
        geo_assert(!is_locked_);  // Not thread-safe
        is_locked_ = true;
        v_to_cell_.assign(nb_vertices(), -1);
        for(index_t c = 0; c < nb_cells(); c++) {
            for(index_t lv = 0; lv < cell_size(); lv++) {
                v_to_cell_[cell_vertex(c, lv)] = signed_index_t(c);
            }
        }
        is_locked_ = false;
    }

    void Delaunay::update_cicl() {
        geo_assert(!is_locked_);  // Not thread-safe
        is_locked_ = true;
        cicl_.resize(cell_size() * nb_cells());
        for(index_t v = 0; v < nb_vertices(); ++v) {
            signed_index_t t = v_to_cell_[v];
            if(t != -1) {
                index_t lv = index(index_t(t), signed_index_t(v));
                set_next_around_vertex(index_t(t), lv, index_t(t));
            }
        }
        for(index_t t = 0; t < nb_cells(); ++t) {
            bool skip = false;
            if(keep_infinite_) {
                for(index_t lv=0; lv < cell_size(); ++lv) {
                    if(cell_vertex(t,lv) < 0) {
                        skip = true;
                        break;
                    }
                }
            }
            if(!skip) {
                for(index_t lv = 0; lv < cell_size(); ++lv) {
                    index_t v = index_t(cell_vertex(t, lv));
                    if(v_to_cell_[v] != signed_index_t(t)) {
                        index_t t1 = index_t(v_to_cell_[v]);
                        index_t lv1 = index(t1, signed_index_t(v));
                        index_t t2 = index_t(next_around_vertex(t1, lv1));
                        set_next_around_vertex(t1, lv1, t);
                        set_next_around_vertex(t, lv, t2);
                    }
                }
            }
        }
        is_locked_ = false;
    }

    bool Delaunay::cell_is_infinite(index_t c) const {
        geo_debug_assert(c < nb_cells());
        for(index_t lv=0; lv < cell_size(); ++lv) {
            if(cell_vertex(c,lv) == -1) {
                return true;
            }
        }
        return false;
    }
    
}

