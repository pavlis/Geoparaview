#include <vector>
#include "gclgrid.h"
#include "VelocityModel_1d.h"
#include "dmatrix.h"
#include "slowness.h"
#include "Hypocenter.h"
#include "ensemble.h"
class Ray_Transformation_Operator
{
public:
	int npoints;
	Ray_Transformation_Operator(int np);
	// constructor for constant azimuth 
	Ray_Transformation_Operator(GCLgrid&g, dmatrix& path, double azimuth);
	// constructor for depth dependent operator
	Ray_Transformation_Operator(GCLgrid&g, dmatrix& path,double azimuth,
					dmatrix& nup);
	Ray_Transformation_Operator(const Ray_Transformation_Operator& pat);
	dmatrix apply(dmatrix& in);
private:
	vector<dmatrix> U;
};
// function prototypes 
SlownessVector get_stack_slowness(ThreeComponentEnsemble& ensemble);
Hypocenter get_event_hypocenter(ThreeComponentEnsemble& ensemble);
GCLgrid3d *Build_GCLraygrid(bool fixed_u,
	GCLgrid& parent,SlownessVector u,Hypocenter h,
	VelocityModel_1d& vmod,double zmax, double tmax, double dt);
dmatrix *ray_path_tangent(dmatrix&);
