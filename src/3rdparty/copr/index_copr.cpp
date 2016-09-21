#include <cstdint>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cfloat>
#include <cmath>
#include <cassert>
#include <algorithm>
#include "qcprot/qcprot.hpp"
#include "index_copr.hpp"

using namespace std;


static double dot_product(double* u, double* v)
{
	return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
}

static double two_point_rmsd(uint8_t* permutation_P, uint8_t* permutation_Q, double (*P)[3], double (*Q)[3], double* norm_P, double* norm_Q)
{
	double pnorm0 = norm_P[permutation_P[0]];
	double pnorm1 = norm_P[permutation_P[1]];
	double qnorm0 = norm_Q[permutation_Q[0]];
	double qnorm1 = norm_Q[permutation_Q[1]];

	double cosp = dot_product(P[permutation_P[0]], P[permutation_P[1]]) / (pnorm0 * pnorm1);
	double cosq = dot_product(Q[permutation_Q[0]], Q[permutation_Q[1]]) / (qnorm0 * qnorm1);
	double sinp = sqrt(1 - std::min(1.0, cosp*cosp));
	double sinq = sqrt(1 - std::min(1.0, cosq*cosq));

	double p0[2] = {pnorm0, 0};
	double p1[2] = {pnorm1 * cosp, -pnorm1 * sinp};

	double q0[2] = {qnorm0, 0};
	double q1[2] = {qnorm1 * cosq, -qnorm1 * sinq};


	double sint = q1[0]*p1[1] - q1[1]*p1[0];
	double cost = q0[0]*p0[0] + q1[0]*p1[0] + q1[1]*p1[1];
	double normt = sqrt(sint*sint + cost*cost);
	sint /= normt;
	cost /= normt;

	double r0[2] = {p0[0] - q0[0]*cost, -q0[0]*sint};
	double r1[2] = {p1[0] - q1[0]*cost + q1[1]*sint, p1[1] - q1[0]*sint - q1[1]*cost};
	return r0[0]*r0[0] + r0[1]*r0[1] + r1[0]*r1[0] + r1[1]*r1[1];
}

static double evaluate(	int num_fixed, uint8_t* permutation_P, uint8_t* permutation_Q, bool allow_mirroring,
			double E0, double* A, double (*P)[3], double (*Q)[3], double* norm_P, double* norm_Q)
{
	if (num_fixed == 0)
	{
		return 0;
	}
	else if (num_fixed == 1)
	{
		double norm1 = norm_P[permutation_P[0]];
		double norm2 = norm_Q[permutation_Q[0]];
		double d = norm1 - norm2;
		return d*d;
	}
	else if (num_fixed == 2)
	{
		double rmsd = two_point_rmsd(permutation_P, permutation_Q, P, Q, norm_P, norm_Q);
		assert(rmsd == rmsd);
		return rmsd;
	}
	else
	{
		double rmsd;
		FastCalcRMSDAndRotation(num_fixed, E0 / 2, A, &rmsd, NULL, allow_mirroring, NULL);
		assert(rmsd == rmsd);
		return rmsd*rmsd*num_fixed;
	}
}

static bool recurse(	int num_points, int level, uint8_t* permutation_P, uint8_t* permutation_Q, uint8_t* best_permutation_P,
			double* relaxation, bool allow_mirroring, double E0, double* A, double (*P)[3], double (*Q)[3],
			double* norm_P, double* norm_Q, double max_rmsd, int* p_num_nodes_explored, double* p_lowest_rmsd)
{
	double rmsd = evaluate(level, permutation_P, permutation_Q, allow_mirroring, E0, A, P, Q, norm_P, norm_Q) + relaxation[level];
	*p_num_nodes_explored += 1;

	if (level == num_points)
	{
		if (rmsd < *p_lowest_rmsd)
		{
			*p_lowest_rmsd = rmsd;
			memcpy(best_permutation_P, permutation_P, num_points * sizeof(uint8_t));
		}

		return true;
	}
	else if (rmsd > *p_lowest_rmsd || rmsd > max_rmsd)
	{
		return false;
	}

	bool match_found = false;
	int num_swaps = num_points - level;
	for (int i=0;i<num_swaps;i++)
	{
		std::swap(permutation_P[level], permutation_P[level + i]);

		double B[9];
		double F0 = E0 + increment_innerproduct(A, level, P, Q, permutation_P, permutation_Q, B);

		match_found |= recurse(	num_points, level + 1, permutation_P, permutation_Q, best_permutation_P, relaxation,
					allow_mirroring, F0, B, P, Q, norm_P, norm_Q, max_rmsd, p_num_nodes_explored, p_lowest_rmsd);

		std::swap(permutation_P[level], permutation_P[level + i]);
	}

	return match_found;
}

bool copr_register_points_dfs(	int num_points, double (*P)[3], double (*Q)[3], double max_rmsd, bool allow_mirroring,
				uint8_t* best_permutation_P, int* p_num_nodes_explored, double* p_rmsd, double* q, bool* p_mirrored)
{
	assert(num_points <= COPR_MAX_POINTS);

	max_rmsd = max_rmsd * max_rmsd * num_points;

	double norm_P[COPR_MAX_POINTS];
	double norm_Q[COPR_MAX_POINTS];
	for (int i=0;i<num_points;i++)
	{
		double x1 = P[i][0];
		double y1 = P[i][1];
		double z1 = P[i][2];

		double x2 = Q[i][0];
		double y2 = Q[i][1];
		double z2 = Q[i][2];

		norm_P[i] = sqrt(x1*x1 + y1*y1 + z1*z1);
		norm_Q[i] = sqrt(x2*x2 + y2*y2 + z2*z2);
	}


	double relaxation[COPR_MAX_POINTS + 1];
	memset(relaxation, 0, (num_points + 1) * sizeof(double));
	for (int i=0;i<num_points;i++)
	{
		double smin = INFINITY;
		for (int j=0;j<num_points;j++)
		{
			double d = norm_P[j] - norm_Q[i];
			smin = std::min(smin, d*d);
		}

		relaxation[i] = smin;
	}

	for (int i=0;i<num_points;i++)
		for (int j=i+1;j<num_points;j++)
			relaxation[i] += relaxation[j];



	uint8_t permutation_P[COPR_MAX_POINTS];
	uint8_t permutation_Q[COPR_MAX_POINTS];
	for (int i=0;i<num_points;i++)
	{
		permutation_P[i] = i;
		permutation_Q[i] = i;
	}

	*p_num_nodes_explored = 0;
	*p_rmsd = INFINITY;

	double A[9] = {0};
	double rmsd = INFINITY;
	bool match_found = recurse(	num_points, 0, permutation_P, permutation_Q, best_permutation_P, relaxation,
					allow_mirroring, 0.0, A, P, Q, norm_P, norm_Q, max_rmsd, p_num_nodes_explored, &rmsd);
	if (match_found)
	{
		*p_rmsd = sqrt(rmsd / num_points);

		if (q != NULL)
		{
			double A[9];
			double E0 = full_innerproduct(A, num_points, P, Q, best_permutation_P, permutation_Q);
			FastCalcRMSDAndRotation(num_points, E0 / 2, A, &rmsd, q, allow_mirroring, p_mirrored);
		}
	}

	return match_found;
}

