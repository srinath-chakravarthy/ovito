#ifndef INDEX_COPR_HPP
#define INDEX_COPR_HPP

#define COPR_MAX_POINTS 16

bool copr_register_points_dfs(int num_points, double (*P)[3], double (*Q)[3], double max_rmsd, bool allow_mirroring, uint8_t* best_permutation_P, int* p_num_nodes_explored, double* p_rmsd, double* q, bool* p_mirrored);

#endif

