#include "db.h"
#ifdef	__cplusplus

typedef struct geo_{
	double lat;
	double lon;
	double r;
} Geographic_point;
typedef struct cart_ {
	double x1;
	double x2;
	double x3;
} Cartesian_point;

extern int GCLverbose;  //verbosity level.  0 terse, 1 verbose, 2 very verbose

class GCLgrid
{
	public:
		char name[10];  // name assigned to this coordinate grid
		double lat0, lon0, r0;  // geographical location of origin 
		double azimuth_y;  // Azimuth of positive y axis 
		double dx1_nom, dx2_nom;  // nominal grid spacing 
		int n1,n2;  // grid size in each component direction
		int i0, j0;  // origin location in grid 
		double xlow, xhigh, ylow, yhigh, zlow, zhigh;// bounding box 
		int cartesian_defined, geographic_defined;
		double **x1, **x2, **x3; //cartesian coordinates of nodes
		double **lat, **lon, **r;  //geographical coordinates of nodes

		GCLgrid(){
			n1=0;n2=0;x1=NULL;x2=NULL;x3=NULL;lat=NULL;lon=NULL;r=NULL;
		};
		GCLgrid(int n1size, int n2size);
		GCLgrid(int n1size, int n2size, char *n, double la0, double lo0,
			double radius0, double az, double dx1n, double dx2n, 
			int iorigin, int jorigin);
		GCLgrid(Dbptr db, char *nm);  // acquire from Antelope database 
		GCLgrid(const GCLgrid&);  //standard copy constructor
		GCLgrid& operator=(const GCLgrid& );
		void dbsave(GCLgrid&, Dbptr, char *);
		void lookup(double, double);
		void reset_index() {ix1=i0; ix2=j0;};
		void get_index(int *ind) {ind[0]=ix1; ind[1]=ix2;};
		Geographic_point ctog(double, double, double);
		Cartesian_point gtoc(double, double, double);
		void GCLset_transformation_matrix();
		~GCLgrid();
		//
		//These derived classes need private access so are declared
		//friends
		//
		friend class GCLgrid3d;
		friend class GCLscalarfield;
		friend class GCLvectorfield;
		friend class GCLscalarfield3d;
		friend class GCLvectorfield3d;
	private:
		int ix1, ix2;
		double gtoc_rmatrix[3][3];
		double translation_vector[3];
};
//3d version is identical except it requires 3 indexes instead of 2 for
//coordinates.  We use inheritance to simply this description.
class GCLgrid3d : public GCLgrid
{
	public:
		double dx3_nom;
		int n3;
		int k0;
		double ***x1, ***x2, ***x3;
		double ***lat, ***lon, ***r;

		GCLgrid3d(){
			n1=0;n2=0;n3=0;
			x1=NULL;x2=NULL;x3=NULL;lat=NULL;lon=NULL;r=NULL;
		};
		GCLgrid3d(int n1size, int n2size, int n3size);
		GCLgrid3d(int n1size, int n2size, int n3size, 
			char *n, double la0, double lo0,
			double radius0, double az, 
			double dx1n, double dx2n, double dx3n,
			int iorigin, int jorigin);
		GCLgrid3d(Dbptr db, char *nm); 
		GCLgrid3d(const GCLgrid3d&); 
		GCLgrid3d& operator=(const GCLgrid3d& );
		void dbsave(GCLgrid3d&, Dbptr, char *);
		void lookup(double, double, double);
		void reset_index() {ix1=i0; ix2=j0; ix3=k0;};
		void get_index(int *ind) {ind[0]=ix1; ind[1]=ix2; ind[2]=ix3;};
		~GCLgrid3d();
	private:
		int ix1, ix2, ix3;
};	  		
//
//These are generic scalar and vector fields defined on a GCLgrid object
//of 2 or 3d
//
class GCLscalarfield :  public GCLgrid
{
	public:
		double **val;

		GCLscalarfield();
		GCLscalarfield(int, int);
		GCLscalarfield(GCLgrid& );
		GCLscalarfield& operator=(const GCLscalarfield&);
		GCLscalarfield operator+=(const GCLscalarfield&);
		GCLscalarfield operator*=(double);
		double interpolate(double,double,double);
		~GCLscalarfield();
};
class GCLvectorfield : public GCLgrid
{
	public:
		int nv;
		double ***val;

		GCLvectorfield();
		GCLvectorfield(int,int,int);
		GCLvectorfield(GCLgrid &,int);
		GCLvectorfield& operator=(const GCLvectorfield&);
		GCLvectorfield operator+=(const GCLvectorfield&);
		GCLvectorfield operator*=(double);
		double *interpolate(double,double,double);
		~GCLvectorfield();
};
class GCLscalarfield3d : public GCLgrid3d 
{
	public:
		double ***val;

		GCLscalarfield3d();
		GCLscalarfield3d(int,int,int);
		GCLscalarfield3d(GCLgrid3d &);
		GCLscalarfield3d& operator=(const GCLscalarfield3d&);
		GCLscalarfield3d operator+=(const GCLscalarfield3d&);
		GCLscalarfield3d operator*=(double);
		double interpolate(double,double,double);
		~GCLscalarfield3d();
};
class GCLvectorfield3d : public GCLgrid3d
{
	public:
		int nv;
		double ****val;

		GCLvectorfield3d();
		GCLvectorfield3d(int,int,int,int);
		GCLvectorfield3d(GCLgrid3d &,int);
		GCLvectorfield3d& operator=(const GCLvectorfield3d&);
		GCLvectorfield3d operator+=(const GCLvectorfield3d&);
		GCLvectorfield3d operator*=(double);
		double *interpolate(double,double,double);
		~GCLvectorfield3d();
};
//
//C++ helpers
//
#endif

#ifdef	__cplusplus
extern "C" {
#endif

	

//
// plain C helper function prototypes 
//
double r0_ellipse(double);
double ****create_4dgrid_contiguous(int, int, int, int);
double ***create_3dgrid_contiguous(int, int, int);
double **create_2dgrid_contiguous(int, int);
void GCLlookup_error_handler(int);
#ifdef  __cplusplus
}
#endif

