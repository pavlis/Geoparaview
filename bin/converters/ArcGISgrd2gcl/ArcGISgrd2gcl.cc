#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "seispp.h"
#include "Metadata.h"
#include "gclgrid.h"
#include "GCLMasked.h"
#include "LatLong-UTMconversion.h"
void usage()
{
  cerr << "ArcGISgrd2gcl infile outfilebase [-zone xx -gridname gn]" <<endl
    << "Converts txt format regular grid from ArcGIS export to 2D GCLfield"<<endl
    << "infile is the file created by ArcGIS"<<endl
    << "outfilebase is the base name for the converted output gclfield data"
    <<endl
    << "Output is a masked field with depth as the scalar variable"<<endl
    << "Use -zone to change the UTM zone code to xx"<<endl
    << "Use -gridname to change base grid name (default PCStructure)"
    <<endl;
  exit(-1);
}
bool SEISPP::SEISPP_verbose(true);
int main(int argc, char **argv)
{
  int i,j;
  if(argc<3)usage();
  string infile(argv[1]);
  string outbase(argv[2]);
  /* This is correct for great unconformity map.  Letter is superflous. */
  string utmzone("14S");   
  string gridname("PCStructure");
  for(i=3;i<argc;++i)
  {
    string sarg(argv[i]);
    if(sarg=="-zone")
    {
      ++i;
      if(i>=argc)usage();
      utmzone=string(argv[i]);
    }
    else if(sarg=="-zone")
    {
      ++i;
      if(i>=argc)usage();
      gridname=string(argv[i]);
    }
    else
    {
      usage();
    }
  }
  ifstream ifs;
  const int number_header_lines(6);
  /* This was written for a specific file.  Unclear if arcgis adds other
  key,value pairs than the 6 the file I'm converting uses.  If I knew there
  were other posibilities I would make number_header_lines a command line
  argument.  In any case, we blindly read them and convert them to a
  metadata object.*/
  ifs.open(infile.c_str(),ios::in);
  if(ifs.fail())
  {
    cerr << "ArcGISgrd2gcl:  cannot open input file "<<infile<<endl;
    usage();
  }
  string headerlines;
  for(i=0;i<number_header_lines;++i)
  {
    string linebuf;
    getline(ifs,linebuf);
    headerlines=headerlines+"\n"+linebuf;
  }
  Metadata header(headerlines);
  int nx1=header.get_int("ncols");
  int nx2=header.get_int("nrows");
  double xll=header.get_double("xllcorner");
  double yll=header.get_double("yllcorner");
  double cellsize=header.get_double("cellsize");
  int NODATAvalue=header.get_int("NODATA_value");
  /* A sanity check*/
  if(NODATAvalue>0)
  {
    cerr << "ArcGISgrd2gcl:  parsed NODATA_value="<<NODATAvalue
       << " is nonsense"<<endl<<"Expected to be a large negative number"<<endl
       << " Assumptions of this program are violated - bug fix required"
       <<endl
       << "Cannot continue"<<endl;
    exit(-1);
  }
  double NODATAtest;
  NODATAtest=(double)NODATAvalue;
  NODATAtest += 1.0;  //Make test 1 foot above null value to avoid float equal test
  /* Large memory algorithm - eat up the entire file and make sure the
  size matches what is expected from size parameters */
  vector<double> dvector;
  dvector.reserve(nx1*nx2);
  do
  {
    double dval;
    ifs >> dval;
    if(ifs.fail())break;
    dvector.push_back(dval);
  }while(1);
  if(dvector.size() != (nx1*nx2))
  {
    cerr << "Size mismatch in data read from file="<<infile<<endl
      << "Number of values read from file="<<dvector.size()<<endl
      << "Header parameters give ncol="<<nx1<<" and nrows="<<nx2
      << " which means number expected="<<nx1*nx2<<endl;
    exit(-1);
  }
  /* We have to first build a regular GCLgrid before we apply the mask.
  We assume all the dvector values are in units of m.  horizontal coordinates
  are assumed utm in m.  Dante thinks the file starts in the upper left corner
  and online sources confirm this. */
  double lat0,lon0,r0;   //frozen origin here to surface r0 at lower left corner
  /* 23 in this call freezes WGS-84 - not at all elegant*/
  UTMtoLL(23,yll,xll,utmzone.c_str(),lat0,lon0);
  lat0 *= deg2rad;
  lon0 *= deg2rad;
  r0=r0_ellipse(lat0);
  GCLgrid g(nx1,nx2,gridname,lat0,lon0,r0,0.0,cellsize*0.001,cellsize*0.001,0,0);
  GCLscalarfield f(g);
  double easting,northing,elev;
  double lat,lon,r;
  Cartesian_point cp;
  int iv;
  /* To get around a weird arc export feature we need to subtract this 
   * value from the raw elevation number before conversion to km */
  const double zoffset(50000.0);
  /* The precambrian surface data for which this program was written tabulate
   * the depths in feet.   For now we freeze this in this conversion 
   * constant. */
  const double rawz2elev(0.0003048);
  /* Scan order is TV order - start upper left corner, scan left to right,
   * next row down, repeat till bottom.  jj is used for down counting. */
  int jj;
  int number_not_null(0);
  for(j=0,iv=0,jj=nx2-1;j<nx2;++j,--jj)
  {
    for(i=0;i<nx1;++i)
    {
      easting=xll+cellsize*((double)i);
      northing=yll+cellsize*((double)jj);
      UTMtoLL(23,northing,easting,utmzone.c_str(),lat,lon);
      //cout << lat<<" "<<lon<<" ";
      lat*=deg2rad;
      lon*=deg2rad;
      /*Zero masked values */
      if(dvector[iv]<=NODATAtest)
      {
        elev=0;
        f.val[i][jj]=0.0;
      }
      else
      {
        elev=(dvector[iv] - zoffset)*rawz2elev;
        f.val[i][jj]=elev;
        ++number_not_null;
      }
      //cout << elev<<endl;
      r=r0_ellipse(lat)-elev;
      cp=g.gtoc(lat,lon,r);
      f.x1[i][jj]=cp.x1;
      f.x2[i][jj]=cp.x2;
      f.x3[i][jj]=cp.x3;
      ++iv;
    }
  }
  f.compute_extents();
  //f.save(outbase,string("."));
  /* Now apply the mask - this is wrong and needs to be fixed 
   * once I establish the scan order*/
  GCLMask gmask(g);
  for(j=0,iv=0,jj=nx2-1;j<nx2;++j,--jj)
  {
    for(i=0;i<nx1;++i)
    {
      if(dvector[iv]<=NODATAtest)
      {
        gmask.mask_point(i,jj);
      }
      else
      {
          gmask.enable_point(i,jj);
      }
      ++iv;
    }
  }
  GCLMaskedScalarField msf(f,gmask);
  msf.save(outbase);
  cout << "Saved results to "<<outbase<<" with "<<number_not_null
      << " set cells of "<<nx1*nx2<<" total grid points"<<endl;
}
