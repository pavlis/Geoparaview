//#include "perf.h"
//
//perf.h seems to have an error.  I'll declare the required prototypes here.
//
extern "C" {
extern void daxyp(int, double, double *, int, double *, int);
extern double ddot(int,double *, int, double *, int);
extern double dnrm2(int,double *, int);
}
#include "gclgrid.h"
/* This is an indexing routine for finding the location of a point in 3 space
within what I'm calling here a geographical curvilinear grid (gclgrid).  
This algorithm will work only if the grid defines an object that is 
best thought of as a distorted box filled with bricks of approximately 
uniform initial size.  The grid points index the location of the bricks.
Distortion means the bricks (and object) can be distorted in to 
objects with nonorthogonal sides and variable spacing.  They are still
indexable, however, because the grid points are assumed to be laid
out in a regular order (C order with the last index varying most 
rapidly.  The indices map positions in generalized coordinates in
the grid.)  The algorith used here works only if the grid is not folded 
or multivalued in any sense.  

The basic  algorithm is an iterative one that computes the local 
transformation matrix at each step from a simple forward difference.
That is, it essentially does a shift to the current grid position 
defined by the generalized coordinate index positions i, j, and k.  
At that point it computes a vector direction of a +1 shift in each
grid position to define the number of unit cells to jump from the 
current position.  Because the grid spacing is not assumed to be
uniform or rectilinear this in general requires an iteration.
This is repeated until the requested point is in the cell 
defined by a bounding box defined by the location 
x1[i+1][j+1][k+1], x2[i+1][j+1][k+1], x3[i+1][j+1][k+1]  to
x1[i][j][k], x2[i][j][k], x3[i][j][k]  This algorith converges
rapidly if the initial starting point is not far from the final
point.  For this reason we return the index positions through 
the argument list.  When tracking a regular curve this approach
should be reasonably fast if the previous index position is passed
as the starting point for the search.  

Arguments:

	x,y,z - cartesian coordinates of requested point in GRCgrid
		reference frame

The primary return of this function is the variables ix1, ix2, ix3
defined internally in the GCLgrid3d structure.  They are the index
positions locating the cell that can be interpolated for the
point x,y,z.  


Calls to this function must be enclosed with a try block and a
simple integer catch function.  The error codes that should be
caught are:
	1 - requested point is outside the bounding box (normally this
		should be handled silently)
	-1 - convergence error;  could not locate x,y,z in the grid 
		(this most likely indicates a ill defined grid that
		does not match the expectations of the library)
	2 - requested point is in a grey area near the edge of the 
		grid.  Caller may want to attempt interpolation, but
		should be warned it is dangerous.  i.e. it is 
		algorithm dependent how this case should be handled.
	-2 - total failure.  Attempted nothing because either the
		cartesian grid is not defind.
Returns:
Normal return is 0.  A nonzero return indicates an error that the
calling program will need to handle as desired.
+1 return indicates the point is located outside the grid and the 
returned indices are meaninless
-1 return indicates the algorithm failed to converge.  The return value
is likely bogus.  

Note:  I chose not to use a throw/catch mechanism for this because it
is expected that the 1 condition will happen often.

Both nonzero returns indicate the returned index values are not 
to be ued.  

Author:  Gary Pavlis
Written:  June 2000
Modified:  Debug with minor changes during fall 2002.  Converted to
C++ December 2002.
This brought several major changes too pervasive to be worth documenting.
*/
const int MAXIT=50;	//convergence count limit
void GCLgrid3d::lookup(double x, double y, double z)
{
	int i,j,k;
	double dx1[3],dx2[3],dx3[3];
	double nrmdx1,nrmdx2,nrmdx3;
	double dxunit,dyunit,dzunit;
	int di, dj, dk;
	int ctest;
	int count=0;
	double delta[3];

	if(!cartesian_defined) throw -2;

	/* return immediately if outside the extents bounding box */
	if( (x > (xhigh)) || (x < (xlow)) 
	  ||  (y > (yhigh)) || (y < (ylow)) 
	  ||  (z > (zhigh)) || (z < (zlow)) ) throw 1;

	i = ix1;
	j = ix2;
	k = ix3;
	if(i<0) i=0;
	if(j<0) j=0;
	if(k<0) k=0;
	if(i>=((n1)-1)) i = (n1)-2;
	if(j>=((n2)-1)) j = (n2)-2;
	if(k>=((n3)-1)) k = (n3)-2;


	do
	{
		/* This is the unit cell step directions along x1, x2, and x3 grid lines*/
		dx1[0] = (x1[i+1][j][k]) - (x1[i][j][k]);
		dx1[1] = (x2[i+1][j][k]) - (x2[i][j][k]);
		dx1[2] = (x3[i+1][j][k]) - (x3[i][j][k]);

		dx2[0] = (x1[i][j+1][k]) - (x1[i][j][k]);
		dx2[1] = (x2[i][j+1][k]) - (x2[i][j][k]);
		dx2[2] = (x3[i][j+1][k]) - (x3[i][j][k]);


		dx3[0] = (x1[i][j][k+1]) - (x1[i][j][k]);
		dx3[1] = (x2[i][j][k+1]) - (x2[i][j][k]);
		dx3[2] = (x3[i][j][k+1]) - (x3[i][j][k]);

		nrmdx1 = dnrm2(3,dx1,1);
		nrmdx2 = dnrm2(3,dx2,1);
		nrmdx3 = dnrm2(3,dx3,1);

		delta[0] = x - (x1[i][j][k]);
		delta[1] = y - (x2[i][j][k]);
		delta[2] = z - (x3[i][j][k]);

		/* This might seem wrong, but it is correct to divide
		by dx^2.  One is dot product normalize, second is 
		to get distance in cell units.  Idea is project and
		scale to number of unit cells. */
		dxunit = ddot(3,delta,1,dx1,1)/(nrmdx1*nrmdx1);
		dyunit = ddot(3,delta,1,dx2,1)/(nrmdx2*nrmdx2);
		dzunit = ddot(3,delta,1,dx3,1)/(nrmdx3*nrmdx3);
		di = (int)dxunit;
		dj = (int)dyunit;
		dk = (int)dzunit;
		/* This is necessary because integer truncation jumps
		opposite directions for positive and negative numbers.
		An alternative is nint, but then you have to mess with 
		a special condition between -0.5 and 0 to converge
		correctly.  Here it is automatic.*/
		if(di<0) --di;
		if(dj<0) --dj;
		if(dk<0) --dk;
		i += di;
		j += dj;
		k += dk;
		/* We reset i, j, or k if they move outside the grid
		and just continue the iteration.  We catch nonconvergence
		when the loop is exited and try to decide if the 
		nonconvergence is nonconvergence or a point in the 
		grey zone between the bounding box and the actual grid. */
		if(i >= ((n1)-1) ) i = (n1)-2;
		if(j >= ((n2)-1) ) j = (n2)-2;
		if(k >= ((n3)-1) ) k = (n3)-2;
		if(i<0) i=0;
		if(j<0) j=0;
		if(k<0) k=0;
		ctest = abs(di)+abs(dj)+abs(dk);
		++count;
	}
	while( (ctest>0) && (count<MAXIT) );
	/* points in the grey area between the bounding box and
	the edges of the grid are assumed to never converge and
	reach here.  */
	if((di<=0) && (i==0) )throw 2;
	if((dj<=0) && (j==0) )throw 2;
	if((dk<=0) && (k==0) )throw 2;
	if( di && (i>=((n1)-2))) throw 2;
	if( dj && (j>=((n2)-2))) throw 2;
	if( dk && (k>=((n3)-2))) throw 2;
	if(ctest!=0) throw -1;
	//
	//Note we do not alter the index when errors occur.
	//This is intentional because searches are expected to
	//often be done locally not globally so we hold the last position
	//
	ix1 = i;
	ix2 = j;
	ix3 = k;
}
//
// 2d version here.  General algorithm and symbols are the same but 
// there are major differences in details.  These are noted below.
// Error's thrown have the same pattern as 3d case.
//
void GCLgrid::lookup(double target_lat, double target_lon)
{
	int i,j;
	double dx1[3],dx2[3];
	double nrmdx1,nrmdx2;
	double dxunit,dyunit,dzunit;
	int di, dj, dk;
	int ctest;
	int count=0;
	double delta[3];
	Cartesian_point target_x;
	double r0p;
	double x,y,z;
	//
	//first find the cartesian coordinates of the requested lat/lon
	//cartesian and geographic both need to be defined or we throw
	//an error and give up.  Note both are required here unlike 3d case
	//
	if(!cartesian_defined || !geographic_defined) throw -2;

	//
	// This can incorrectly throw an error if the 2d grid is highly
	// warped, but it seems the safest initial value for this 
	//
	r0p=r0;
	target_x = gtoc(target_lat, target_lon, r0p);
	x=target_x.x1;
	y=target_x.x2;
	z=target_x.x3;

	/* return immediately if outside the extents bounding box */
	if( (x > (xhigh)) || (x < (xlow)) 
	  ||  (y > (yhigh)) || (y < (ylow)) 
	  ||  (z > (zhigh)) || (z < (zlow)) ) throw 1;

	i = ix1;
	j = ix2;
	if(i<0) i=0;
	if(j<0) j=0;
	if(i>=((n1)-1)) i = (n1)-2;
	if(j>=((n2)-1)) j = (n2)-2;


	do
	{
		//
		//  A 2d GCLgrid is a warped surface.  To allow the surface
		//  to be not just be radial shell we have to constantly 
		//  update the radius vector and the effective target vector
		//  Confusing difference from 3D case, but necessary
		//
		r0p=r[i][j];
		target_x = gtoc(target_lat, target_lon, r0p);
		x=target_x.x1;
		y=target_x.x2;
		z=target_x.x3;

		//compute the cell vectors as above, but only need 2 now
		dx1[0] = (x1[i+1][j]) - (x1[i][j]);
		dx1[1] = (x2[i+1][j]) - (x2[i][j]);
		dx1[2] = (x3[i+1][j]) - (x3[i][j]);

		dx2[0] = (x1[i][j+1]) - (x1[i][j]);
		dx2[1] = (x2[i][j+1]) - (x2[i][j]);
		dx2[2] = (x3[i][j+1]) - (x3[i][j]);

		nrmdx1 = dnrm2(3,dx1,1);
		nrmdx2 = dnrm2(3,dx2,1);

		delta[0] = x - (x1[i][j]);
		delta[1] = y - (x2[i][j]);
		delta[2] = z - (x3[i][j]);

		dxunit = ddot(3,delta,1,dx1,1)/(nrmdx1*nrmdx1);
		dyunit = ddot(3,delta,1,dx2,1)/(nrmdx2*nrmdx2);
		di = (int)dxunit;
		dj = (int)dyunit;
		if(di<0) --di;
		if(dj<0) --dj;
		i += di;
		j += dj;

		if(i >= ((n1)-1) ) i = (n1)-2;
		if(j >= ((n2)-1) ) j = (n2)-2;
		if(i<0) i=0;
		if(j<0) j=0;
		ctest = abs(di)+abs(dj);
		++count;
	}
	while( (ctest>0) && (count<MAXIT) );

	if((di<=0) && (i==0) )throw 2;
	if((dj<=0) && (j==0) )throw 2;
	if( di && (i>=((n1)-2))) throw 2;
	if( dj && (j>=((n2)-2))) throw 2;
	if(ctest!=0) throw -1;

	ix1 = i;
	ix2 = j;
}

