#include <iostream>
#include "gclgrid.h"
#include "dmatrix.h"
#include "seispp.h"
#include "vmodel.h"
#include "ray1d.h"
#include "pf.h"


/* Cavalier copy of a ray path, path, to a GCLgrid3d object at offset i,j
(i.e. for the first two indices that define the surface grid position)
The input path is reversed so that the surface point will be at 
index i,j,n3-1.  This is a strong potential confusion but it keeps the
generalized coordinates right handed at each grid box.  I was not sure
this was necessary, but did it under a "better safe than sorry" assumption.
It matters only for code maintenance because the user would not normally 
care about this sign detail.  

Arguments:
	path - input path (assumed starting at surface an oriented down
	g - pointer to grid to hold results
	i,j - index to copy path to.
*/

void copy_path(dmatrix *path,GCLgrid3d& g,int i, int j)
{
	Geographic_point geo;
	int k,kk;
	// assume we already checked that grid size is consistent with columns in path
	for(k=0;k<g.n3;++k)
	for(k=0,kk=(g.n3)-1;k<g.n3;++k,--k)
	{
		g.x1[i][j][kk]=(*path)(0,k);
		g.x2[i][j][kk]=(*path)(1,k);
		g.x3[i][j][kk]=(*path)(2,k);
	}
	// now we have to get the geo points 
	for(k=0;k<g.n3;++i)
	{
		geo=g.ctog(g.x1[i][j][k],g.x2[i][j][k],g.x3[i][j][k]);
		g.lat[i][j][k] = geo.lat;
		g.lon[i][j][k] = geo.lon;
		g.r[i][j][k] = geo.r;
	}
}
/* This function acts like a constructor, but it isn't made and explicit part of the object
definition because it is a special construction used for this program.  

Arguments:
	parent - 2d surface GCLgrid of pseudostation grid points 
	u - slowness vector used to generate this ray grid (note for large grids this points in
		different directions at different points even though the vector is fixed)
	vmod - 1d velocity model used for ray tracing 
	zmax, tmax - depth, time maxima used to define ray trace limits.

Note that the number of points that define each ray path is set by the ray tracing.  The
same ray, in fact, is used for all elements but the projection is spatially dependent because
of earth curvature and geographical coordinate conventions on how slowness vector are 
traditionally defined.  This algorithm is a bit at odds with the gclgrid model where the
object is to reduce artifacts for where on earth the grid happened to be positioned.  This 
approach may cause problems for grids near the pole and will definitely not work for a grid
that spans the pole.  Since neither is likely in the near future this seems a minor 
concession to simplicity.  

The number of points in n3 direction is determined by the ray tracing and is implicitly 
returned as the n3 variable in the newly created GCLgrid3d object.  Note carefully that
this function acts like new and the object must eventually be deleted explicitly.  

It is also VERY important to recognize this constructor produces ray grids that 
are oriented from the bottom up.  This is useful to keep the set of basis vectors
at all points right handed.  

NOTE TO GLP DURING DEBUG:  I DON'T THINK THE RAYTRACE FUNCTIONS ARE CONSISTENT
WITH THIS SIGN CONVENTION.  THIS WILL REQUIRE SOME CHECKING WHEN I HAVE A BIG
ENOUGH BLOCK OF TIME TO COVER ALL THE PIECES IN ONE SITTING.  

Author:  Gary Pavlis
*/  


GCLgrid3d *Build_GCLraygrid(GCLgrid& parent,Slowness_vector u, Velocity_Model_1d& vmod,
		double zmax, double tmax, double dt)
{
	int i,j;
	double umag, theta;  // used many times so we make temporaries of these
	umag = u.mag();
	theta = u.baz();
	// First trace the basic ray object that will be projected.  This gets dimensions.
	RayPathSphere base_ray(vmod, u.mag(), zmax, tmax, dt, "t");

	// call the simple, parameterized GCLgrid constructor that allocs space but has no content
	GCLgrid *rgptr = new GCLgrid3d(parent.n1, parent.n2, base_ray.npts);
	GCLgrid raygrid& = *rptr;
	// clone these parent grid variable
	strcpy(raygrid.name,parent.name);
	raygrid.lat0=parent.lat0;
	raygrid.lon0=parent.lon0;
	raygrid.r0=parent.r0;
	raygrid.azimuth_y=parent.azimuth_y;
	raygrid.dx1_nom=parent.dx1_nom;
	raygrid.dx2_nom=parent.dx2_nom;
	raygrid.n1=parent.n1;
	raygrid.n2=parent.n2;
	raygrid.i0=parent.i0;
	raygrid.j0=parent.j0;
	raygrid.cartesian_defined=parent.cartesian_defined;
	raygrid.geographic_defined=parent.geographic_defined;
	/*  k0 must be set to the point at the earth's surface.  This is necesary
	to set the transformation at this point to keep all grids consistent. 
	The ray bottom will depend on the slowness vector */
	raygrid.k0 = base_ray.npts - 1;  // earth's surface or this will be
					// inconsistent with lat0,lon0
	// need to explicitly set the transformation matrix like this 
	// as it can only be set through this mechanism
	raygrid.set_transformation_matrix();
	//
	// now start filling up the grid with the cartesian points
	//
	dmatrix *path;
	for(i=0;i<n1;++i)
		for(j=0;j<n2;++j)
		{
			//GCLGRID NEEDS A FIX FOR THIS LINE 
			try {
				path = GCLgrid_Ray_Project(parent,base_ray&, 
					theta, i,j)
				copy_path(path,raygrid,i,j);
			}
			catch (GCLgrid_error err)
			{
				err.log_error();
				elog_die(0,"Cannot continue\n");
			}
			delete path;
		}


	// Before we return we have to recompute the grid extents.
	raygrid.compute_extents();

	return(rgptr);

}

int main(int argc, char **argv)
{
	char *pfin=NULL;
	char *dbname;
	Dbptr db;
	Dbptr dbo;  // output special table for slowness grids
	char *gridname;
	int ierr;
	int i,j,k,l;   // four loop levels in this program.
	int nt;  // target number of points on ray paths
	double dt;  // sample interval of time steps desired
	// target depths and time.  note tmax should be > n*dt 
	// as 1d to 3d conversion will cause errors otherwise
	double zmax, tmax;  
	char *griddir;  // directory to store GCLgrid coordinates
	char *fielddir;  // write 1d travel times here
	char *dfile;  // file to store results (all results are placed in this same file)
	char *Pfieldname="1D-Pwave_travel_times";
	char *Sfieldname="1D-Swave_travel_times";
	char *Pbase_grid_name;  // used as base name for output grids in form XXXXXXXX_III_JJJ with III and JJJ index I and J
	char *Sbase_grid_name;  // same for S
	char *modname;  // 1D Velocity model name 
	Pf *pf;

	ios::sync_with_stdio();
	elog_init(argc,argv);

        if(argc < 2) usage();
        dbname = argv[1];

        for(i=2;i<argc;++i)
        {
                if(!strcmp(argv[i],"-V"))
                        usage();
                else if(!strcmp(argv[i],"-pf"))
                {
                        ++i;
                        if(i>=argc) usage();
                        pfin = argv[i];
                }
                else
                        usage();
        }
        /* this sets defaults */
        if(pfin == NULL) pfin = strdup("pwmigprecompute");
	if(pfread(pfin,&pf)) die(1,(char *)"pfread error\n");
	// This builds a slowness grid object from pf.  A bad thing here is there
	// is now way to assure this grid is consistent with the one used by pwstack
	// could database this, but don't expect need for large numbers of these
	Rectangular_Slowness_Grid ugrid(pf); 
	ierr=dbopen(dbname,(char *)"r+",&db);
	if(ierr==dbINVALID) die(1,(char *)"Cannot open database %s\n",dbname);
	dbo=dblookup(db,0,"slowgrid",0,0);
	if(dbo.record==dbINVALID) die(1,"Lookup failed for required extension table slowgrid\n");

	gridname=pfget_string(pf,"pseudostation_gridname");
	dt = pfget_double(pf,"nominal_time_step");
	nt = pfget_int(pf,"number_time_steps");
	zmax = pfget_double(pf,"depth_floor");
	tmax = pfget_double(pf,"maximum_time_cutoff");
	griddir = pfget_string(pf,"GCLgrid_directory");
	fielddir = pfget_string(pf,"Travel_time_field_directory");
	dfile = pfget_string(pf,"Travel_time_field_filename");
	Pbase_grid_name = pfget_string(pf,"P_slowness_grid_base_name");
	Sbase_grid_name = pfget_string(pf,"S_slowness_grid_base_name");
	modname=pfget_string(pf,"1d_velocity_model_name");

	try{
		GCLgrid pstat(db,gridname);
		Velocity_Model_1d Vp(db,modname,"P");
		Velocity_Model_1d Vs(db,modname,"S");
		if(!pstat.cartesian_defined || !pstat.geographic_defined)
			die(1,"GCLgrid must have both cartesian and geographic components\n");
		// These are the main outputs that define ray geometry.  They are created now
		// because the parameters other than the coordinates do not vary with the 
		// slowness vector.  The paths inside the earth change but the GCLgrid parent
		// properties are fixed.
		GCLscalarfield3d Praygrid(pstat.n1,pstat.n2,nt);  
		GCLscalarfield3d Sraygrid(pstat.n1,pstat.n2,nt); 

		clone_parent_grid_properties(&pstat,&Praygrid);
		clone_parent_grid_properties(&pstat,&Sraygrid);
		// Now we have to fill in some things manually.
		Praygrid.n3=nt;
		Sraygrid.n3=nt;
		Praygrid.dx3_nom=zmax/((double)nt);
		Sraygrid.dx3_nom=Praygrid.dx3_nom;
		Praygrid.k0=0;
		Sraygrid.k0=0;

		// main loop over slowness grid
		for(i=0;i<ugrid.nux;++i)
		{
			for(j=0;j<ugrid.nuy;++j)
			{
				double u;
				// base paths rotated and translated below.  They are 
				// done as pointers to make them dynamic.  Note delete at end
				RayPathSphere *Pray0, *Sray0; 
				dmatrix U();  // rotation matrix used below (always 3x3)
				dmatrix Pcoords(),Scoords();  //hold P and S ray paths referenced at origin
				double xp[3];  // Cartesian coordinate used for translation vector

				sprintf(Praygrid.name,"%s_%d_%d", Pbase_grid_name,i,j);
				sprintf(Sraygrid.name,"%s_%d_%d", Sbase_grid_name,i,j);

				u=hypot(ugrid.ux(i),ugrid.uy(j));  // slowness magnitude
				Pray0=new RayPathSphere(&Vp,u,zmax,tmax,dt,"t");
				Sray0=new RayPathSphere(&Vs,u,zmax,tmax,dt,"t");
				// This is heavy handed but given that this program
				// would always be run offline standalone it is 
				// the right approach
				if(Pray0->npts < nt)
					die(0,"P ray path for slowness %lf was truncated\nNeeded %d points but got only %d\nFix parameters and rerun\n",
						u,nt, Pray0->npts);
				if(Sray0->npts < nt)
					die(0,"S ray path for slowness %lf was truncated\nNeeded %d points but got only %d\nFix parameters and rerun\n",
						u,nt, Sray0->npts);
				// This function returns Cartesian coordinates in the 
				// GCLgrid cartesian reference frame with the path laid
				// out along the trajectory of the x1 axis.  
				Pcoords = GCLgrid_Ray_gtoc(pstat,Pray0);
				Scoords = GCLgrid_Ray_gotc(pstat,Sray0);
				
				for(k=0;k<pstat.n1;++k)
				    for(l=0;l<psstat.n2;++l)
				    {
					int m;
					dmatrix newpath();
					// We now compute the rotation and translation vectors
					// to take the standard ray and place it's top point
					// at this grid point and rotated by the correct angle
					// for spherical geometry.  Note a memory leak will
					// occur if the = operator for dmatrix is not defined
					// correctly here
					U=compute_unit_spherical_transformation(pstat,pstat.lat[k][l],
								pstat.lon[k][l]);
					xp[0]=pstat.x1[k][l];
					xp[1]=pstat.x2[k][l];
					xp[2]=pstat.x3[k][l];

					newpath=U*Pcoords;
					translate_path(&newpath,xp);
					copy_path(&newpath,&Praygrid,k,l);
					copy_path(&newpath,&Sraygrid,k,l);
					for(m=0;m<Praygrid.n3;++m) Praygrid.val[k][l][m]=Pray0->t[m];
					for(m=0;m<Sraygrid.n3;++m) Sraygrid.val[k][l][m]=Sray0->t[m];
				    }
				
				Praygrid.dbsave(db,griddir,fielddir,Pfieldname,dfile);
				Sraygrid.dbsave(db,griddir,fielddir,Sfieldname,dfile);
				if( dbaddv(dbo,0,"gridname",Praygrid.name,
					"ix",i,
					"iy",j,
					"ux",ugrid.ux(i),
					"uy",ugrid.uy(j),0) == dbINVALID)
						die(1,"dbaddv error writing slowgrid table for grid point (%d,%d) for P rays\n",
							i,j);
				if( dbaddv(dbo,0,"gridname",Sraygrid.name,
					"ix",i,
					"iy",j,
					"ux",ugrid.ux(i),
					"uy",ugrid.uy(j),0) == dbINVALID)
						die(1,"dbaddv error writing slowgrid table for grid point (%d,%d) for S rays\n",
							i,j);
				delete Pray0;
				delete Sray0;
			}
		}
	} 
	// Collection of error handlers.  The int handler is way to generic for long
	// term stability, but is correct for the current code done while I was still
	// a beginner at C++
	catch (int ipserr) 
		die(1,"Constructor failure acquiring grid %s from database",gridname);
	catch (GCLgrid_dberror gcldberr)
	{
		gcldberr.log_error();
		die(1,"GCLerrors cannot be recovered.  Check database\n");
	}
	catch (Velocity_Model_1d_dberror vmdbe)
	{
		vmdbe.log_error();
		die(1,"Check database and parameter file and try again\n");
	}
}
