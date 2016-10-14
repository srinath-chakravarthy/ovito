#ifndef COORDINATION_HPP
#define COORDINATION_HPP

int calculate_coordination(void* _voronoi_handle, int num_points, const double (*_points)[3], double threshold, bool* is_neighbour);

void* copr_voronoi_initialize_local();
void copr_voronoi_uninitialize_local(void* ptr);

#endif

