#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <cell.hh>
using namespace voro;
using namespace std;

//#define DEBUG
#define MAX_POINTS 19

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif


//todo: change voronoi code to return errors rather than exiting
static int calculate_voronoi_face_areas(int num_points, const double (*_points)[3], double* normsq, double max_norm, voronoicell_neighbor* v, vector<int>& nbr_indices, vector<int>& face_vertices, vector<double>& vertices)
{
	const double k = 1000 * max_norm;
	v->init(-k,k,-k,k,-k,k);

	for (int i=1;i<num_points;i++)
	{
		double x = _points[i][0] - _points[0][0];
		double y = _points[i][1] - _points[0][1];
		double z = _points[i][2] - _points[0][2];
		v->nplane(x,y,z,normsq[i],i);
	}

	v->neighbors(nbr_indices);
	v->face_vertices(face_vertices);
	v->vertices(0, 0, 0, vertices);
	return 0;
}

static double dot_product(double* a, double* b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void cross_product(double* a, double* b, double* c)
{
	c[0] = a[1] * b[2] - a[2] * b[1];
	c[1] = a[2] * b[0] - a[0] * b[2];
	c[2] = a[0] * b[1] - a[1] * b[0];
}

static double calculate_solid_angle(double* R1, double* R2, double* R3)	//norms of R1-R3 must be 1
{
	double R2R3[3];
	cross_product(R2, R3, R2R3);
	double numerator = dot_product(R1, R2R3);

	double r1r2 = dot_product(R1, R2);
	double r2r3 = dot_product(R2, R3);
	double r3r1 = dot_product(R3, R1);

	double denominator = 1 + r1r2 + r3r1 + r2r3;
	return fabs(2 * atan2(numerator, denominator));
}

int calculate_coordination(void* _voronoi_handle, int num_points, const double (*_points)[3], double threshold, bool* is_neighbour)
{
	assert(num_points <= MAX_POINTS);
	assert(threshold >= 0 && threshold <= 1);

	voronoicell_neighbor* voronoi_handle = (voronoicell_neighbor*)_voronoi_handle;

	double max_norm = 0;
	double points[MAX_POINTS][3];
	double normsq[MAX_POINTS];
	for (int i = 0;i<num_points;i++)
	{
		double x = _points[i][0] - _points[0][0];
		double y = _points[i][1] - _points[0][1];
		double z = _points[i][2] - _points[0][2];
		points[i][0] = x;
		points[i][1] = y;
		points[i][2] = z;

		normsq[i] = x*x + y*y + z*z;
		max_norm = max(max_norm, normsq[i]);
#ifdef DEBUG
		printf("point %d: %f\t%f\t%f\t%f\n", i, x, y, z, x*x + y*y + z*z);
#endif
	}

	max_norm = sqrt(max_norm);
	vector<int> nbr_indices;
	vector<int> face_vertices;
	vector<double> vertices;

	int ret = calculate_voronoi_face_areas(num_points, points, normsq, max_norm, voronoi_handle, nbr_indices, face_vertices, vertices);
	if (ret != 0)
		return ret;

	size_t num_vertices = vertices.size() / 3;
	for (size_t i=0;i<num_vertices;i++)
	{
		double x = vertices[i * 3 + 0];
		double y = vertices[i * 3 + 1];
		double z = vertices[i * 3 + 2];

		double s = sqrt(x*x + y*y + z*z);
		vertices[i * 3 + 0] /= s;
		vertices[i * 3 + 1] /= s;
		vertices[i * 3 + 2] /= s;
	}

	int num_faces = voronoi_handle->number_of_faces();
#ifdef DEBUG
	printf("num_faces=%d\n", num_faces);
#endif

	bool is_voronoi_neighbour[MAX_POINTS] = {false};
	double solid_angles[MAX_POINTS] = {0};
	size_t c = 0;
	for (int current_face = 0;current_face<num_faces;current_face++)
	{
		int point_index = nbr_indices[current_face];
		if (point_index <= 0)
			continue;

		is_voronoi_neighbour[point_index] = true;

		double solid_angle = 0;
		int num = face_vertices[c++];
		int u = face_vertices[c];
		int v = face_vertices[c+1];
		for (int i=2;i<num;i++)
		{
			int w = face_vertices[c+i];
			double omega = calculate_solid_angle(&vertices[u*3], &vertices[v*3], &vertices[w*3]);
			solid_angle += omega;

			v = w;
		}

		solid_angles[point_index] = solid_angle;
		c += num;

#ifdef DEBUG
		printf("%d\t%f\t%f\n", point_index, solid_angle, solid_angle / (4 * M_PI));
#endif
	}

	assert(c == face_vertices.size());

	double solid_angle_threshold = threshold * 4 * M_PI;
	for (int i=0;i<num_points;i++) {
		is_neighbour[i] = is_voronoi_neighbour[i] && solid_angles[i] > solid_angle_threshold;
	}

	return ret;
}

void* copr_voronoi_initialize_local()
{
	voronoicell_neighbor* ptr = new voronoicell_neighbor;
	return (void*)ptr;
}

void copr_voronoi_uninitialize_local(void* _ptr)
{
	voronoicell_neighbor* ptr = (voronoicell_neighbor*)_ptr;
	delete ptr;
}

