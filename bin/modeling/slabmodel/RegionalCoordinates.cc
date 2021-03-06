#include <math.h>
#include "coords.h"
#include "RegionalCoordinates.h"
RegionalCoordinates::RegionalCoordinates()
{
    lat0=0.0;
    lon0=0.0;
    r0=0.0;
    azimuth_y=0.0;
    int i,j;
    for(i=0;i<3;++i)
        for(j=0;j<3;++j)
        {
            gtoc_rmatrix[i][j]=0.0;
        }
    for(i=0;i<3;++i)
    {
        gtoc_rmatrix[i][i]=1.0;
        translation_vector[i]=0.0;
    }
}
RegionalCoordinates::RegionalCoordinates(double olat, double olon,
        double origr, double oaz)
{
    lat0=olat;
    lon0=olon;
    r0=origr;
    azimuth_y=oaz;
    double pole_lat,pole_lon;
    double x0[3], xpole[3], xcros[3];

    /* This is copied from set_transformation_matrix in gclgrid library.
       latlon call defines great circle path to a pole defined by 
       azimuth_y.  Rest is vector manipulation to set up Cartesian 
       system with x2 along azimuth_y at origin */
    latlon(lat0, lon0, M_PI_2,azimuth_y, &pole_lat, &pole_lon);
    dsphcar(lon0,lat0,x0);
    dsphcar(pole_lon, pole_lat, xpole);
    dr3cros(xpole, x0, xcros);
    dcopy(3,x0,1,gtoc_rmatrix[2],1);
    dcopy(3,xpole,1,gtoc_rmatrix[1],1);
    dcopy(3,xcros,1,gtoc_rmatrix[0],1);
    /* Now we define the translation vector */
    dcopy(3,x0,1,translation_vector,1);
    dscal(3,r0,translation_vector,1);
}
RegionalCoordinates::RegionalCoordinates(const RegionalCoordinates& parent)
{
    int i,j;
    for(i=0;i<3;++i)
    {
        translation_vector[i]=parent.translation_vector[i];
        for(j=0;j<3;++j)
        {
            gtoc_rmatrix[i][j]=parent.gtoc_rmatrix[i][j];
        }
    }
    lat0=parent.lat0;
    lon0=parent.lon0;
    r0=parent.r0;
    azimuth_y=parent.azimuth_y;
}
RegionalCoordinates& RegionalCoordinates::operator=(const RegionalCoordinates& parent)
{
    if(this!=&parent)
    {
        int i,j;
        for(i=0;i<3;++i)
        {
            translation_vector[i]=parent.translation_vector[i];
            for(j=0;j<3;++j)
            {
                gtoc_rmatrix[i][j]=parent.gtoc_rmatrix[i][j];
            }
        }
        lat0=parent.lat0;
        lon0=parent.lon0;
        r0=parent.r0;
        azimuth_y=parent.azimuth_y;
    }
    return(*this);
}
Cartesian_point RegionalCoordinates::cartesian(double lat, double lon, double r)
{
    Cartesian_point p;
    double xp[3],dxp[3];
    int i;
    //dsphcar computes a unit vector.  We then scale it by radius
    dsphcar(lon, lat, xp);
    for(i=0;i<3;++i) xp[i]*=r;
    //
    //Remove the origin translation vector
    //
    dr3sub(xp,translation_vector,dxp);
    //
    //Rotate by transformation matrix and that is it
    //
    p.x1=ddot(3,gtoc_rmatrix[0],1,dxp,1);
    p.x2=ddot(3,gtoc_rmatrix[1],1,dxp,1);
    p.x3=ddot(3,gtoc_rmatrix[2],1,dxp,1);
    return(p);
}
Cartesian_point RegionalCoordinates::cartesian(Geographic_point gp)
{
    return(this->cartesian(gp.lat,gp.lon,gp.r));
}
Geographic_point RegionalCoordinates::geographic(double xin[3])
{
    Geographic_point  p;
    double dxp[3], x[3];
    int i;

    /*
    First apply rotation matrix to put this point in the
    standard geographic reference frame.  We use a safe
    BLAS algorithm for the multiply of the transpose 
    */
    for(i=0;i<3;++i) dxp[i]=0.0;
    daxpy(3,xin[0],gtoc_rmatrix[0],1,dxp,1);
    daxpy(3,xin[1],gtoc_rmatrix[1],1,dxp,1);
    daxpy(3,xin[2],gtoc_rmatrix[2],1,dxp,1);
    /*dxp now contains the relative location vector in standard
    //coordinates.  We have to apply a translation vector
    //relative to the center of the earth before we can convert
    //to correct latitude and longitude
    */
    for(i=0;i<3;++i) x[i]=dxp[i]+translation_vector[i];
    dcarsph(x,&(p.lon),&(p.lat));
    p.r = dnrm2(3,x,1);
    return(p);
}
Geographic_point RegionalCoordinates::geographic(Cartesian_point cp)
{
    double x[3];
    x[0]=cp.x1;
    x[1]=cp.x2;
    x[2]=cp.x3;
    return(this->geographic(x));
}
/* This method is effecively a format converter from a vector to a struct*/
Cartesian_point RegionalCoordinates::cartesian(double x[3])
{
    Cartesian_point cp;
    cp.x1=x[0];
    cp.x2=x[1];
    cp.x3=x[2];
    return(cp);
}
Geographic_point RegionalCoordinates::origin()
{
    Geographic_point gp;
    gp.lat=lat0;
    gp.lon=lon0;
    gp.r=r0;
    return(gp);
}
