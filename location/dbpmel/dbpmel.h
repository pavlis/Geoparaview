/* This structure definition should ultimately end up in location.h
I put it here only temporarily */
/* This structure is used to define an indexed matrix that is called
S in the original PMEL paper.  The S matrix has nrow rows (normally
this is ndata-4*nevents) and ncol columns.  The two associative arrays,
phase_index and sta_index, are used to index the columns through a 
double indexing scheme.  The S matrix should be thought of as partitioned
by columns into nphases submatrices, one for each seismic phase to 
solve for.  phase_index contains the column index for the first element
of the matrix for each named phase.  Within the submatrix the RELATIVE
position of the column associated with a given station is obtained
from the sta_index array.  e.g. if we wanted phase S for station AAK
and we found the phase index was 25 and the station index was 4 we
would know that 25+4 was the column position for this station:phase.
*/
typedef struct scmatrix {
	Arr *phase_index;  /* Contains integer index for phase */
	Arr *sta_index;  /* Same for station names.  */ 
	int nphases; 
	int nsta;
	int nrow,ncol;  /* ncol=nsta*nphases, but we keep it anyway*/
	double rmsraw,sswrodgf;
	double ndgf;
	double *scref;  /* Vector of length ncol of reference station 
			corrections (used for bias correction projectors)*/
	double *sc;  /* Holds a current estimate of path anomalies */
	double *scbias,*scdata;  /* components of sc derived from sref 
					and data respectively */
	double *S;
} SCMatrix; 


/* Function prototypes */
int check_required_pf(Pf *);
int create_Smatrix(SCMatrix *, Arr *);
void clear_station_corrections(Arr *);
int initialize_station_corrections(Arr *,Arr *, Arr *,Hypocenter *);
Dbptr dbform_working_view(Dbptr, Pf *, char *);
Arr *dbpmel_load_stations(Dbptr, Pf *);
int dbpmel_process(Dbptr, Tbl *, Pf *);
Tbl *pmel(int, Tbl **, Hypocenter *, char **,Hypocenter *,SCMatrix *,Pf *);
