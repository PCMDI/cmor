#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "uuid.h"
#include <unistd.h>
#include <string.h>
#include "json.h"
#include "json_tokener.h"
#include "cmor.h"
#include "cmor_locale.h"
#include <netcdf.h>
#include <udunits2.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#include <sys/types.h>
#include <unistd.h>


/* ==================================================================== */
/*      this is defining NETCDF4 variable if we are                     */
/*      using NETCDF3 not used anywhere else                            */
/* ==================================================================== */
#ifndef NC_NETCDF4
#define NC_NETCDF4 0
#define NC_CLASSIC_MODEL 0
int nc_def_var_deflate( int i, int j, int k, int l, int m ) {
    return( 0 );
};
int nc_def_var_chunking( int i, int j, int k, size_t * l ) {
    return( 0 );
};
#endif

/* -------------------------------------------------------------------- */
/*      function declaration                                            */
/* -------------------------------------------------------------------- */
extern int cmor_set_cur_dataset_attribute_internal( char *name,
						    char *value,
						    int optional );

extern int cmor_set_variable_attribute_internal( int id,
						 char *attribute_name,
						 char type, void *value );
extern int cmor_history_contains( int var_id, char *add );

extern void cdCompAdd( cdCompTime comptime,
		       double value,
		       cdCalenType calendar, cdCompTime * result );

extern void cdCompAddMixed( cdCompTime ct,
			    double value, cdCompTime * result );

/* -------------------------------------------------------------------- */
/*      Global Variable                                                 */
/* -------------------------------------------------------------------- */
int USE_NETCDF_4;
int cleanup_varid = -1;

const char CMOR_VALID_CALENDARS[CMOR_N_VALID_CALS][CMOR_MAX_STRING] =
    { "gregorian",
    "standard",
    "proleptic_gregorian",
    "noleap",
    "365_day",
    "360_day",
    "julian",
    "none"
};

int CMOR_HAS_BEEN_SETUP = 0;
ut_system *ut_read = NULL;
FILE *output_logfile;


/* -------------------------------------------------------------------- */
/*      Variable related to cmor                                        */
/* -------------------------------------------------------------------- */
cmor_dataset_def cmor_current_dataset;
cmor_table_t cmor_tables[CMOR_MAX_TABLES];
cmor_var_t cmor_vars[CMOR_MAX_VARIABLES];
cmor_axis_t cmor_axes[CMOR_MAX_AXES];
cmor_grid_t cmor_grids[CMOR_MAX_GRIDS];

int CMOR_MODE;
int CMOR_TABLE;
int CMOR_VERBOSITY;
int CMOR_NETCDF_MODE;

int cmor_naxes;
int cmor_nvars;
int cmor_ntables;
int cmor_ngrids;

int cmor_nerrors;
int cmor_nwarnings;

int did_history = 0;

int CMOR_CREATE_SUBDIRECTORIES = 1;

char cmor_input_path[CMOR_MAX_STRING];
char cmor_traceback_info[CMOR_MAX_STRING];

/**************************************************************************/
/*                cmor_mkdir()                                            */
/**************************************************************************/
int cmor_mkdir(const char *dir) {
        char tmp[PATH_MAX];
        char *p = NULL;
        size_t len;
        int ierr;
        cmor_add_traceback( "cmor_mkdir" );

        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++){
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, ( S_IRWXU | S_IRWXG | S_IRWXO ));
                        *p = '/';
                }
        }
        ierr = mkdir(tmp, ( S_IRWXU | S_IRWXG | S_IRWXO ));
        cmor_pop_traceback(  );
        return(ierr);
}


/**************************************************************************/
/*                                                                        */
/*                 cmorstringstring()                                     */
/*                                                                        */
/**************************************************************************/
int cmor_stringinstring( char *dest, char *src ) {
/* -------------------------------------------------------------------- */
/*      returns 1 if dest contains the words of src.                    */
/*      The end of a word is either a space, a period or null.          */
/*                                                                      */
/*      This will not give the desired results if                       */
/*      period is used internal to a word.                              */
/* -------------------------------------------------------------------- */
    char *pstr = dest;
    cmor_add_traceback( "cmor_stringinstring" );

    while( ( pstr = strstr( pstr, src ) ) ) {
/* -------------------------------------------------------------------- */
/*      if this word is at the beginning of msg or preceeded by a       */
/*      space                                                           */
/* -------------------------------------------------------------------- */

	if( pstr == dest || pstr[-1] == ' ' ) {
/* -------------------------------------------------------------------- */
/*      if it isn't a substring match                                   */
/* -------------------------------------------------------------------- */
	    if( ( pstr[strlen( src )] == ' ' ) ||
		( pstr[strlen( src )] == 0 ) ||
		( pstr[strlen( src )] == '.' ) ) {
/* -------------------------------------------------------------------- */
/*      then return( 1 ) to indicate string found                          */
/* -------------------------------------------------------------------- */
		return( 1 );
	    }
	}
	pstr++;			/* In which case, skip to the next char */
    }
/* -------------------------------------------------------------------- */
/*      nothing like src is in dest, and so return the location of the end*/
/*      of string                                                       */
/* -------------------------------------------------------------------- */
    cmor_pop_traceback(  );
    return( 0 );
}

/************************************************************************/
/*                                                                      */
/*      cmor_cat_unique_string()                                        */
/*                                                                      */
/************************************************************************/
void cmor_cat_unique_string( char *dest, char *src ) {
    int offset;
    int spare_space;
    cmor_add_traceback( "cmor_cat_unique_string" );

/* -------------------------------------------------------------------- */
/*      if this string is in msg                                        */
/* -------------------------------------------------------------------- */
    if( !cmor_stringinstring( dest, src ) ) {
	if( ( offset = strlen( dest ) ) ) {
	    strcat( dest + offset, " " );
	    offset++;
	    spare_space = CMOR_MAX_STRING - offset - 1;
	    strncat( dest + offset, src, spare_space );
	} else {
	    strncpy( dest, src, CMOR_MAX_STRING );
	}
    }
    cmor_pop_traceback();
}

/**************************************************************************/
/*                                                                        */
/*             cmor_check_forcing_validity()                              */
/*                                                                        */
/**************************************************************************/
void cmor_check_forcing_validity( int table_id, char *value ) {
    int i, j, n, found = 0;
    char msg[CMOR_MAX_STRING];
    char astr[CMOR_MAX_STRING];
    char **bstr;

    cmor_add_traceback("cmor_check_forcing_validity");
    if( cmor_tables[table_id].nforcings == 0 ) {
        cmor_pop_traceback();
	return;
    }
    strcpy( astr, value );
    found = 0;
    
    for( i = 0; i < strlen( astr ); i++ ) {
	if( astr[i] == ',' )
	    astr[i] = ' ';

/* -------------------------------------------------------------------- */
/*      removes everything  after first paranthesis                     */
/* -------------------------------------------------------------------- */
	if( astr[i] == '(' )
	    astr[i] = '\0';
    }
    cmor_convert_string_to_list( astr, 'c', ( void ** ) &bstr, &n );
    if( n == 0 ){
        cmor_pop_traceback();
	return;
    }
    for( i = 0; i < n; i++ ) {
	found = 0;
	for( j = 0; j < cmor_tables[table_id].nforcings; j++ ) {
	    if( strcmp( bstr[i], cmor_tables[table_id].forcings[j] ) == 0 ) {
		found = 1;
		break;
	    }
	}
	if( found == 0 ) {
	    sprintf( msg, "forcing attribute elt %i (%s) is not valid for\n! "
		     "table %s, valid values are:", i,
		     bstr[i], cmor_tables[table_id].szTable_id );
	    for( j = 0; j < cmor_tables[table_id].nforcings; j++ ) {
		strncat( msg, " ", CMOR_MAX_STRING - strlen( msg ) );
		strncat( msg, cmor_tables[table_id].forcings[j],
			 CMOR_MAX_STRING - strlen( msg ) );
		strncat( msg, ",", CMOR_MAX_STRING - strlen( msg ) );
	    }
	    msg[strlen( msg ) - 1] = '\0';
	    cmor_handle_error( msg, CMOR_CRITICAL );
	}
    }
/* -------------------------------------------------------------------- */
/*      Ok now we need to clean up the memory allocations....           */
/* -------------------------------------------------------------------- */
    for( i = 0; i < n; i++ ) {
	free( bstr[i] );
    }
    free( bstr );
    cmor_pop_traceback();
    return;
}

/**************************************************************************/
/*                  cmor_check_expt_id()                                  */
/**************************************************************************/
int cmor_check_expt_id( char *szExptID, int nTableID,
			char *szGblAttLong, char *szGblAttShort ) {
    int i, j;
    char szTableExptID[CMOR_MAX_STRING];
    char szTableShtExptID[CMOR_MAX_STRING];

    cmor_add_traceback( "cmor_check_expt_id" );
    j = 1;
    for( i = 0; i <= cmor_tables[nTableID].nexps; i++ ) {

        strncpy(szTableExptID,
                cmor_tables[nTableID].expt_ids[i],
                CMOR_MAX_STRING);

        strncpy(szTableShtExptID,
                cmor_tables[nTableID].sht_expt_ids[i],
                CMOR_MAX_STRING);

	if( ( strncmp( szTableExptID, szExptID, CMOR_MAX_STRING ) == 0 ) ||
	    ( strncmp( szTableShtExptID, szExptID, CMOR_MAX_STRING ) == 0 ) ) {
		j = 0;
		cmor_set_cur_dataset_attribute_internal( szGblAttLong,
							 szTableExptID, 0 );
		cmor_set_cur_dataset_attribute_internal( szGblAttShort,
							 szTableShtExptID, 1 );
		strncpy( szExptID, szTableShtExptID,CMOR_MAX_STRING );
		break;
	    }
    }

    cmor_pop_traceback();

    return( j );
}

/************************************************************************/
/*                            strncpytrim()                             */
/************************************************************************/
int strncpytrim( char *out, char *in, int max ) {

    int i, n, j, k;

    cmor_add_traceback("strncpytrim");
    j = 0;
    n = strlen( in );

    if( n > max ) {
	n = max;
    }

/* -------------------------------------------------------------------- */
/*      Look for first space position and last space position and       */
/*      copy interval characters in out.                                */
/* -------------------------------------------------------------------- */
    while( ( in[j] == ' ' ) && ( j < n ) ) {
	j++;
    }

    k = n - 1;

    while( ( in[k] == ' ' ) && ( k > 0 ) ) {
	k--;
    }

    for( i = j; i <= k; i++ ) {
	out[i - j] = in[i];
    }

    out[i - j] = '\0';

    cmor_pop_traceback();
    return( 0 );
}


/************************************************************************/
/*                           cmor_is_setup()                            */
/************************************************************************/
void cmor_is_setup( void ) {

    extern int CMOR_HAS_BEEN_SETUP;
    char msg[CMOR_MAX_STRING];
    extern void cmor_handle_error( char error_msg[CMOR_MAX_STRING],
				   int level );

    cmor_add_traceback( "cmor_is_setup" );

    if( CMOR_HAS_BEEN_SETUP == 0 ) {
	snprintf( msg, CMOR_MAX_STRING,
		"You need to run cmor_setup before calling any cmor_function" );
	cmor_handle_error( msg, CMOR_CRITICAL );
    }
    cmor_pop_traceback(  );
    return;
}

/************************************************************************/
/*                         cmor_add_traceback()                         */
/************************************************************************/
void cmor_add_traceback( char *name ) {
    char tmp[CMOR_MAX_STRING];


    if( strlen( cmor_traceback_info ) == 0 ) {
        sprintf( cmor_traceback_info, "%s\n! ", name );
    } else {
        sprintf( tmp, "%s\n! called from: %s", name, cmor_traceback_info );
        strncpy( cmor_traceback_info, tmp, CMOR_MAX_STRING );
    }
    return;
}

/************************************************************************/
/*                         cmor_pop_traceback()                         */
/************************************************************************/
void cmor_pop_traceback( void ) {
    int i;
    char tmp[CMOR_MAX_STRING];

    strcpy( tmp, "" );
    for( i = 0; i < strlen( cmor_traceback_info ); i++ ) {
	if( strncmp( &cmor_traceback_info[i], "called from: ", 13 ) == 0 ) {
	    strcpy( tmp, &cmor_traceback_info[i + 13] );
	    break;
	}
    }
    strcpy( cmor_traceback_info, tmp );
    return;
}

/************************************************************************/
/*                         cmor_have_NetCDF4()                          */
/************************************************************************/
int cmor_have_NetCDF4( void ) {
    char version[50];
    int major;

    cmor_pop_traceback();
    strncpy( version, nc_inq_libvers(  ), 50 );
    sscanf( version, "%1d%*s", &major );
    if( major != 4 ){
        cmor_pop_traceback();
	return( 1 );
    }
    cmor_pop_traceback();

    return( 0 );
}


/************************************************************************/
/*                          cmor_prep_units()                           */
/************************************************************************/
int cmor_prep_units( char *uunits,
                     char *cunits,
                     ut_unit ** user_units,
                     ut_unit ** cmor_units,
                     cv_converter ** ut_cmor_converter ) {

    extern ut_system *ut_read;
    char local_unit[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    extern void cmor_handle_error( char error_msg[CMOR_MAX_STRING],
                                   int level );
    cmor_add_traceback( "cmor_prep_units" );
    cmor_is_setup(  );
    *cmor_units = ut_parse( ut_read, cunits, UT_ASCII );
    if( ut_get_status(  ) != UT_SUCCESS ) {
        snprintf( msg, CMOR_MAX_STRING,
                  "Udunits: analyzing units from cmor (%s)", cunits );
        cmor_handle_error( msg, CMOR_CRITICAL );
        cmor_pop_traceback(  );
        return( 1 );
    }

    strncpy( local_unit, uunits, CMOR_MAX_STRING );
    ut_trim( local_unit, UT_ASCII );
    *user_units = ut_parse( ut_read, local_unit, UT_ASCII );

    if( ut_get_status(  ) != UT_SUCCESS ) {
        snprintf( msg, CMOR_MAX_STRING,
                  "Udunits: analyzing units from user (%s)", local_unit );
        cmor_handle_error( msg, CMOR_CRITICAL );
        cmor_pop_traceback(  );
        return( 1 );
    }

    if( ut_are_convertible( *cmor_units, *user_units ) == 0 ) {
        snprintf( msg, CMOR_MAX_STRING,
                  "Udunits: cmor and user units are incompatible: %s and %s",
                  cunits, uunits );
        cmor_handle_error( msg, CMOR_CRITICAL );
        cmor_pop_traceback(  );
        return( 1 );
    }

    *ut_cmor_converter = ut_get_converter( *user_units, *cmor_units );

    if( *ut_cmor_converter == NULL ) {
    }

    if( ut_get_status(  ) != UT_SUCCESS ) {
        snprintf( msg, CMOR_MAX_STRING,
                  "Udunits: Error getting converter from %s to %s", cunits,
                  local_unit );
        cmor_handle_error( msg, CMOR_CRITICAL );
        cmor_pop_traceback(  );
        return( 1 );
    }

    cmor_pop_traceback(  );
    return( 0 );
}


/************************************************************************/
/*                       cmor_have_NetCDF41min()                        */
/************************************************************************/
int cmor_have_NetCDF41min( void ) {
    char version[50];
    int major, minor;

    cmor_add_traceback("cmor_have_NetCDF41min");
    strncpy( version, nc_inq_libvers(  ), 50 );
    sscanf( version, "%1d%*c%1d%*s", &major, &minor );
    if( major > 4 ){
        cmor_pop_traceback();
	return( 0 );
    }
    if( major < 4 ) {
        cmor_pop_traceback();
	return( 1 );
    }
    if( minor < 1 ) {
        cmor_pop_traceback();
	return( 1 );
    }
    cmor_pop_traceback();
    return( 0 );
}

/************************************************************************/
/*                         cmor_handle_error()                          */
/************************************************************************/
void cmor_handle_error( char error_msg[CMOR_MAX_STRING], int level ) {
    int i;
    char msg[CMOR_MAX_STRING];
    extern FILE *output_logfile;

    if( output_logfile == NULL )
	output_logfile = stderr;

    msg[0] = '\0';
    if( CMOR_VERBOSITY != CMOR_QUIET ) {
	fprintf( output_logfile, "\n" );
    }
    if( level == CMOR_WARNING ) {
	cmor_nwarnings++;
	if( CMOR_VERBOSITY != CMOR_QUIET ) {
	    
#ifdef COLOREDOUTPUT
	    fprintf( output_logfile, "%c[%d;%dm", 0X1B, 2, 34 );
#endif
	    
	    fprintf( output_logfile, "C Traceback:\nIn function: %s",
		     cmor_traceback_info );
	    
#ifdef COLOREDOUTPUT
	    fprintf( output_logfile, "%c[%dm", 0X1B, 0 );
#endif
	    
	    fprintf( output_logfile, "\n\n" );
	    
#ifdef COLOREDOUTPUT
	    fprintf( output_logfile, "%c[%d;%d;%dm", 0X1B, 1, 34, 47 );
#endif
	    
	    snprintf( msg, CMOR_MAX_STRING, "! Warning: %s", error_msg );
	}
    } else {
	cmor_nerrors++;
	
#ifdef COLOREDOUTPUT
	fprintf( output_logfile, "%c[%d;%d;%dm", 0X1B, 2, 31, 47 );
#endif
	
	fprintf( output_logfile, "C Traceback:\n! In function: %s",
		 cmor_traceback_info );

#ifdef COLOREDOUTPUT
	fprintf( output_logfile, "%c[%dm", 0X1B, 0 );
#endif

	fprintf( output_logfile, "\n\n" );

#ifdef COLOREDOUTPUT
	fprintf( output_logfile, "%c[%d;%d;%dm", 0X1B, 1, 31, 47 );
#endif

	snprintf( msg, CMOR_MAX_STRING, "! Error: %s", error_msg );
    }
    if( CMOR_VERBOSITY != CMOR_QUIET || level != CMOR_WARNING ) {
	for( i = 0; i < 25; i++ ) {
	    fprintf( output_logfile, "!" );
	}
	fprintf( output_logfile, "\n" );
	fprintf( output_logfile, "!\n" );
	fprintf( output_logfile, "%s\n", msg );
	fprintf( output_logfile, "!\n" );
	
	for( i = 0; i < 25; i++ )
	    fprintf( output_logfile, "!" );
	
#ifdef COLOREDOUTPUT
	fprintf( output_logfile, "%c[%dm", 0X1B, 0 );
#endif
	
	fprintf( output_logfile, "\n\n" );
    }
    
    if( ( CMOR_MODE == CMOR_EXIT_ON_WARNING )
	|| ( level == CMOR_CRITICAL ) ) {
	exit( 1 );
    }
}


/************************************************************************/
/*                        cmor_reset_variable()                         */
/************************************************************************/
void cmor_reset_variable( int var_id ) {
    extern cmor_var_t cmor_vars[];
    int j;

    cmor_vars[var_id].self                             = -1;
    cmor_vars[var_id].grid_id                          = -1;
    cmor_vars[var_id].sign                             =  1;
    cmor_vars[var_id].zfactor                          = -1;
    cmor_vars[var_id].ref_table_id                     = -1;
    cmor_vars[var_id].ref_var_id                       = -1;
    cmor_vars[var_id].initialized                      = -1;
    cmor_vars[var_id].closed                           =  0;
    cmor_vars[var_id].nc_var_id                        = -999;
    
    for( j = 0; j < CMOR_MAX_VARIABLES; j++ ) {
	cmor_vars[var_id].nc_zfactors[j]               = -999;
    }
    
    cmor_vars[var_id].nzfactor                         = 0;
    cmor_vars[var_id].ntimes_written                   = 0;
    
    for( j = 0; j < 10; j++ ) {
	cmor_vars[var_id].ntimes_written_coords[j]     = -1;
	cmor_vars[var_id].associated_ids[j]            = -1;
	cmor_vars[var_id].ntimes_written_associated[j] = 0;
    }
    
    cmor_vars[var_id].time_nc_id                       = -999;
    cmor_vars[var_id].time_bnds_nc_id                  = -999;
    cmor_vars[var_id].id[0]                            = '\0';
    cmor_vars[var_id].ndims                            = 0;
    
    for( j = 0; j < CMOR_MAX_DIMENSIONS; j++ ) {
/* -------------------------------------------------------------------- */
/*      place holder for singleton axes ids                             */
/* -------------------------------------------------------------------- */
	cmor_vars[var_id].singleton_ids[j]             = -1;
	cmor_vars[var_id].axes_ids[j]                  = -1;
	cmor_vars[var_id].original_order[j]            = -1;

    }
    
    for( j = 0; j < CMOR_MAX_ATTRIBUTES; j++ ) {
	cmor_vars[var_id].attributes_values_char[j][0] = '\0';
	cmor_vars[var_id].attributes_values_num[j]     = -999.;
	cmor_vars[var_id].attributes_type[j]           = '\0';
	cmor_vars[var_id].attributes[j][0]             = '\0';
    }
    
    cmor_vars[var_id].nattributes                      = 0;
    cmor_vars[var_id].type                             = '\0';
    cmor_vars[var_id].itype                            = 'N';
    cmor_vars[var_id].missing                          = 1.e20;
    cmor_vars[var_id].omissing                         = 1.e20;
    cmor_vars[var_id].tolerance                        = 1.e-4;
    cmor_vars[var_id].valid_min                        = 1.e20;
    cmor_vars[var_id].valid_max                        = 1.e20;
    cmor_vars[var_id].ok_min_mean_abs                  = 1.e20;
    cmor_vars[var_id].ok_max_mean_abs                  = 1.e20;
    cmor_vars[var_id].shuffle                          = 0;
    cmor_vars[var_id].deflate                          = 1;
    cmor_vars[var_id].deflate_level                    = 1;
    cmor_vars[var_id].nomissing                        = 1;
    cmor_vars[var_id].iunits[0]                        = '\0';
    cmor_vars[var_id].ounits[0]                        = '\0';
    cmor_vars[var_id].isbounds                         = 0;
    cmor_vars[var_id].needsinit                        = 1;
    cmor_vars[var_id].zaxis                            = -1;
    
    if( cmor_vars[var_id].values != NULL ) {
	free( cmor_vars[var_id].values );
    }
    
    cmor_vars[var_id].values                           = NULL;
    cmor_vars[var_id].first_time                       = -999.;
    cmor_vars[var_id].last_time                        = -999.;
    cmor_vars[var_id].first_bound                      = 1.e20;
    cmor_vars[var_id].last_bound                       = 1.e20;
    cmor_vars[var_id].base_path[0]                     = '\0';
    cmor_vars[var_id].current_path[0]                  = '\0';
    cmor_vars[var_id].suffix[0]                        = '\0';
    cmor_vars[var_id].suffix_has_date                  = 0;
}


/************************************************************************/
/*                             cmor_setup()                             */
/************************************************************************/
int cmor_setup( char *path,
		int *netcdf,
		int *verbosity,
		int *mode,
		char *logfile,
		int *create_subdirectories ) {


    extern cmor_axis_t cmor_axes[];
    extern int CMOR_TABLE, cmor_ntables;
    extern ut_system *ut_read;
    extern cmor_dataset_def cmor_current_dataset;

    ut_unit *dimlessunit = NULL, *perunit = NULL, *newequnit = NULL;
    ut_status myutstatus;

    int i, j;
    char msg[CMOR_MAX_STRING];
    char msg2[CMOR_MAX_STRING];
    char tmplogfile[CMOR_MAX_STRING];
    uuid_t *myuuid;
    uuid_fmt_t fmt;
    void *myuuid_str = NULL;
    size_t uuidlen;
    time_t lt;

    struct stat buf;
    struct tm *ptr;
    extern FILE *output_logfile;
    extern int did_history;

    strcpy( cmor_traceback_info, "" );
    cmor_add_traceback( "cmor_setup" );

/* -------------------------------------------------------------------- */
/*      ok we need to know if we are using NC3 or 4                     */
/* -------------------------------------------------------------------- */
    USE_NETCDF_4 = -1;
    if( cmor_have_NetCDF4( ) == 0 ) {
        USE_NETCDF_4 = 1;
        if( cmor_have_NetCDF41min( ) != 0 ) {
            sprintf( msg,
                    "You are using a wrong version of NetCDF4 (%s), \n! "
                    "you need 4.1",
                    nc_inq_libvers( ) );
            cmor_handle_error( msg, CMOR_CRITICAL );
        }
    } else {
        sprintf( msg,
                "You are using a wrong version of NetCDF (%s), you need 3.6.3\n! "
                "or 4.1",
                nc_inq_libvers( ) );
        cmor_handle_error( msg, CMOR_CRITICAL );
    }

    did_history = 0;
    CMOR_HAS_BEEN_SETUP = 1;
    CMOR_TABLE = -1;
    cmor_ngrids = -1;
    cmor_nvars = -1;
    cmor_naxes = -1;
    cmor_ntables = -1;
    cmor_nerrors = 0;
    cmor_nwarnings = 0;

    if (logfile == NULL) {
        output_logfile = NULL;
    } else {

#ifdef COLOREDOUTPUT
#undef COLOREDOUTPUT
#endif

        cmor_trim_string(logfile, tmplogfile);
        output_logfile = NULL;
        output_logfile = fopen(tmplogfile, "r");
        if (output_logfile != NULL) {
/* -------------------------------------------------------------------- */
/*      logfile already exists need to rename it                        */
/*      Figure out the time                                             */
/* -------------------------------------------------------------------- */
            stat(tmplogfile, &buf);
            lt = buf.st_ctime;
            ptr = localtime(&lt);

            snprintf(msg, CMOR_MAX_STRING, "%s_%.4i-%.2i-%.2iT%.2i:%.2i:%.2i",
                    tmplogfile, ptr->tm_year + 1900, ptr->tm_mon + 1,
                    ptr->tm_mday, ptr->tm_hour, ptr->tm_min, ptr->tm_sec);

            fclose(output_logfile);
            rename(tmplogfile, msg);

            snprintf(msg2, CMOR_MAX_STRING,
                    "Logfile %s already existed. Renamed to: %s", tmplogfile,
                    msg);
            output_logfile = NULL;
            output_logfile = fopen(tmplogfile, "w");

            if (output_logfile == NULL) {
                snprintf(msg2, CMOR_MAX_STRING,
                        "Could not open logfile %s for writing", tmplogfile);
                cmor_handle_error(msg2, CMOR_CRITICAL);
            }
            cmor_handle_error(msg2, CMOR_WARNING);
        } else {
            output_logfile = fopen(tmplogfile, "w");

            if (output_logfile == NULL) {
                snprintf(msg2, CMOR_MAX_STRING,
                        "Could not open logfile %s for writing", tmplogfile);
                cmor_handle_error(msg2, CMOR_CRITICAL);
            }
        }
    }

    if (mode == NULL) {
        CMOR_MODE = CMOR_NORMAL;

    } else {

        if (*mode != CMOR_EXIT_ON_WARNING && *mode != CMOR_EXIT_ON_MAJOR
                && *mode != CMOR_NORMAL) {

            snprintf(msg, CMOR_MAX_STRING,
                    "exit mode can be either CMOR_EXIT_ON_WARNING CMOR_NORMAL "
                    "or CMOR_EXIT_ON_MAJOR");
            cmor_handle_error(msg, CMOR_CRITICAL);
        }

        CMOR_MODE = *mode;

    }
    if (verbosity == NULL) {
        CMOR_VERBOSITY = CMOR_NORMAL;

    } else {

        if (*verbosity != CMOR_QUIET && *verbosity != CMOR_NORMAL) {
            snprintf(msg, CMOR_MAX_STRING,
                    "verbosity mode can be either CMOR_QUIET or CMOR_NORMAL");
            cmor_handle_error(msg, CMOR_NORMAL);
        }
        CMOR_VERBOSITY = *verbosity;
    }

    if (netcdf == NULL) {
        CMOR_NETCDF_MODE = CMOR_PRESERVE;

    } else {
        if (*netcdf != CMOR_PRESERVE_4 && *netcdf != CMOR_APPEND_4
                && *netcdf != CMOR_REPLACE_4 && *netcdf != CMOR_PRESERVE_3
                && *netcdf != CMOR_APPEND_3 && *netcdf != CMOR_REPLACE_3) {
            snprintf(msg, CMOR_MAX_STRING,
                    "file mode can be either CMOR_PRESERVE, CMOR_APPEND, "
                    "CMOR_REPLACE, CMOR_PRESERVE_4, CMOR_APPEND_4, "
                    "CMOR_REPLACE_4, CMOR_PRESERVE_3, CMOR_APPEND_3 or "
                    "CMOR_REPLACE_3");
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
        CMOR_NETCDF_MODE = *netcdf;

    }

    /* Make sure we are not trying to use NETCDF4 mode while linked against NetCDF3 */
    if (((CMOR_NETCDF_MODE == CMOR_PRESERVE_4)
            || (CMOR_NETCDF_MODE == CMOR_REPLACE_4)
            || (CMOR_NETCDF_MODE == CMOR_APPEND_4)) && (USE_NETCDF_4 == 0)) {
        sprintf(msg,
                "You are trying to use a NetCDF4 mode but linked against "
                "NetCDF3 libraries");
        cmor_handle_error(msg, CMOR_CRITICAL);

    }

    if ((path == NULL) || (strcmp(path, "") == 0)) {
        strncpy(cmor_input_path, ".", CMOR_MAX_STRING);
    } else {
        strncpytrim(cmor_input_path, path, CMOR_MAX_STRING);
    }

    for (i = 0; i < CMOR_MAX_VARIABLES; i++) {
        cmor_reset_variable(i);
    }

    for (i = 0; i < CMOR_MAX_AXES; i++) {
        cmor_axes[i].ref_table_id = -1;
        cmor_axes[i].ref_axis_id = -1;
        cmor_axes[i].isgridaxis = -1;
        cmor_axes[i].axis = '\0';
        cmor_axes[i].iunits[0] = '\0';
        cmor_axes[i].id[0] = '\0';

        if (cmor_axes[i].values != NULL)
            free(cmor_axes[i].values);

        cmor_axes[i].values = NULL;

        if (cmor_axes[i].bounds != NULL)
            free(cmor_axes[i].bounds);

        cmor_axes[i].bounds = NULL;

        if (cmor_axes[i].cvalues != NULL) {
            for (j = 0; j < cmor_axes[i].length; j++) {
                if (cmor_axes[i].cvalues[j] != NULL) {
                    free(cmor_axes[i].cvalues[j]);
                    cmor_axes[i].cvalues[j] = NULL;
                }
            }
            free(cmor_axes[i].cvalues);
        }

        cmor_axes[i].cvalues = NULL;
        cmor_axes[i].length = 0;
        cmor_axes[i].revert = 1; /* 1 means no reverse -1 means reverse */
        cmor_axes[i].offset = 0;
        cmor_axes[i].type = '\0';

        for (j = 0; j < CMOR_MAX_ATTRIBUTES; j++) {
            cmor_axes[i].attributes_values_char[j][0] = '\0';
            cmor_axes[i].attributes_values_num[j] = -999.;
            cmor_axes[i].attributes_type[j] = '\0';
            cmor_axes[i].attributes[j][0] = '\0';
        }

        cmor_axes[i].nattributes = 0;
        cmor_axes[i].hybrid_in = 0;
        cmor_axes[i].hybrid_out = 0;
        cmor_axes[i].store_in_netcdf = 1;
        cmor_axes[i].wrapping = NULL;
        cmor_set_axis_attribute(i, "units", 'c', "");
        cmor_set_axis_attribute(i, "interval", 'c', "");
    }

    if (create_subdirectories != NULL) {
        CMOR_CREATE_SUBDIRECTORIES = *create_subdirectories;
    }

    if ((CMOR_CREATE_SUBDIRECTORIES != 1)
            && (CMOR_CREATE_SUBDIRECTORIES != 0)) {
        snprintf(msg, CMOR_MAX_STRING,
                "cmor_setup: create_subdirectories must be 0 or 1");
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

/* -------------------------------------------------------------------- */
/*      initialize the udunits                                          */
/* -------------------------------------------------------------------- */
    if (ut_read != NULL) {
        ut_free_system(ut_read);
    }

    ut_set_error_message_handler(ut_ignore);

    ut_read = ut_read_xml(NULL);

    if (ut_get_status() != UT_SUCCESS) {
        snprintf(msg, CMOR_MAX_STRING, "Udunits: Error reading units system");
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

    ut_set_error_message_handler(ut_ignore);

    if (newequnit != NULL) {
        ut_free(newequnit);
    }

    newequnit = ut_new_base_unit(ut_read);

    if (ut_get_status() != UT_SUCCESS) {
        snprintf(msg, CMOR_MAX_STRING,
                "Udunits: creating dimlessnew base unit");
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

    myutstatus = ut_map_name_to_unit("eq", UT_ASCII, newequnit);
    ut_free(newequnit);

    if (myutstatus != UT_SUCCESS) {
        snprintf(msg, CMOR_MAX_STRING,
                "Udunits: Error mapping dimless 'eq' unit");
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

    if (dimlessunit != NULL)
        ut_free(dimlessunit);

    dimlessunit = ut_new_dimensionless_unit(ut_read);

    if (ut_get_status() != UT_SUCCESS) {
        snprintf(msg, CMOR_MAX_STRING, "Udunits: creating dimless unit");
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

    myutstatus = ut_map_name_to_unit("dimless", UT_ASCII, dimlessunit);

    if (myutstatus != UT_SUCCESS) {
        snprintf(msg, CMOR_MAX_STRING, "Udunits: Error mapping dimless unit");
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

    if (perunit != NULL)
        ut_free(perunit);

    perunit = ut_scale(.01, dimlessunit);
    if (myutstatus != UT_SUCCESS) {
        snprintf(msg, CMOR_MAX_STRING, "Udunits: Error creating percent unit");
        cmor_handle_error(msg, CMOR_CRITICAL);
    }
    myutstatus = ut_map_name_to_unit("%", UT_ASCII, perunit);
    if (myutstatus != UT_SUCCESS) {
        snprintf(msg, CMOR_MAX_STRING, "Udunits: Error mapping percent unit");
        cmor_handle_error(msg, CMOR_CRITICAL);
    }
    ut_free(dimlessunit);
    ut_free(perunit);

/* -------------------------------------------------------------------- */
/*      initialized dataset                                             */
/* -------------------------------------------------------------------- */
    for (i = 0; i < CMOR_MAX_ATTRIBUTES; i++) {
        cmor_current_dataset.attributes_names[i][0] = '\0';
        cmor_current_dataset.attributes_values[i][0] = '\0';
    }
    cmor_current_dataset.nattributes = 0;
    cmor_current_dataset.realization = 0;
    cmor_current_dataset.leap_year = 0;
    cmor_current_dataset.leap_month = 0;
    cmor_current_dataset.associate_file = 0;
    cmor_current_dataset.associated_file = -1;
/* -------------------------------------------------------------------- */
/*      generates a unique id                                           */
/* -------------------------------------------------------------------- */
    uuid_create(&myuuid);
    uuid_make(myuuid, 4);
    myuuid_str = NULL;
    fmt = UUID_FMT_STR;
    uuid_export(myuuid, fmt, &myuuid_str, &uuidlen);
    strncpy(cmor_current_dataset.tracking_id, (char *) myuuid_str,
            CMOR_MAX_STRING);
    free(myuuid_str);
    uuid_destroy(myuuid);
    strncpy(cmor_current_dataset.associated_file_name, "", CMOR_MAX_STRING);

    for (i = 0; i < 12; i++)
        cmor_current_dataset.month_lengths[i] = 0;
    cmor_current_dataset.initiated = 0;

    for (i = 0; i < CMOR_MAX_GRIDS; i++) {
        strncpy(cmor_grids[i].mapping, "", CMOR_MAX_STRING);
        cmor_grids[i].ndims = 0;
        cmor_grids[i].nattributes = 0;

        for (j = 0; j < CMOR_MAX_GRID_ATTRIBUTES; j++) {
            cmor_grids[i].attributes_values[j] = 1.e20;
            cmor_grids[i].attributes_names[j][0] = '\0';
        }

        if (cmor_grids[i].lats != NULL)
            free(cmor_grids[i].lats);
        if (cmor_grids[i].lons != NULL)
            free(cmor_grids[i].lons);
        if (cmor_grids[i].blats != NULL)
            free(cmor_grids[i].blats);
        if (cmor_grids[i].blons != NULL)
            free(cmor_grids[i].blons);

        cmor_grids[i].lats = NULL;
        cmor_grids[i].lons = NULL;
        cmor_grids[i].blats = NULL;
        cmor_grids[i].blons = NULL;

        cmor_grids[i].istimevarying = 0;
        cmor_grids[i].nvertices = 0;

        for (j = 0; j < 6; j++)
            cmor_grids[i].associated_variables[j] = -1;
    }
    cmor_pop_traceback();
    return( 0 );
}

/************************************************************************/
/*                     cmor_open_inpathFile()                           */
/************************************************************************/
json_object *cmor_open_inpathFile(char *szFilename ) {
    FILE *table_file;
    char szFullName[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char *buffer;
    int  nFileSize;
    json_object *oJSON;

    cmor_add_traceback( "cmor_open_inpathFile" );

    /* -------------------------------------------------------------------- */
    /*      Try to find file in current directory or in cmor_iput_path      */
    /* -------------------------------------------------------------------- */
    strcpy(szFullName, szFilename);
    table_file = fopen( szFullName, "r" );
    int read_size;

    if( table_file == NULL ) {
        if( szFilename[0] != '/' ) {
            snprintf( szFullName, CMOR_MAX_STRING, "%s/%s", cmor_input_path,
                      szFilename );
            table_file = fopen( szFullName, "r" );
        }

        if( table_file == NULL ) {
            snprintf( szFullName, CMOR_MAX_STRING, "Could not find table: %s",
                      szFilename );
            cmor_handle_error( szFullName, CMOR_NORMAL );
            cmor_ntables -= 1;
            cmor_pop_traceback(  );
            return( NULL );
        }
    }

/* -------------------------------------------------------------------- */
/*      Read the entire file in memory                                 */
/* -------------------------------------------------------------------- */
    fseek(table_file,0,SEEK_END);
    nFileSize = ftell( table_file );
    rewind( table_file );
    buffer = (char *) malloc( sizeof(char) * (nFileSize + 1) );
    read_size = fread( buffer, sizeof(char), nFileSize, table_file );
    buffer[nFileSize] = '\0';

/* -------------------------------------------------------------------- */
/*      print errror and exist if this is not a JSON file               */
/* -------------------------------------------------------------------- */
    if(buffer[0]!= '{') {
        free(buffer);
        buffer=NULL;
        snprintf( msg, CMOR_MAX_STRING,
                  "Could not understand file \"%s\" Is this a JSON CMIP6 table?",
                   szFullName  );
            cmor_handle_error( msg, CMOR_CRITICAL );
            cmor_ntables--;
            cmor_pop_traceback(  );
        return(NULL);
    }
/* -------------------------------------------------------------------- */
/*      print error and exit if file was not completely read            */
/* -------------------------------------------------------------------- */
    if( nFileSize != read_size ) {
        free(buffer);
        buffer=NULL;
        snprintf( msg, CMOR_MAX_STRING,
                  "Could not read file %s check file permission",
                   szFullName  );
            cmor_handle_error( msg, CMOR_CRITICAL );
            cmor_ntables--;
            cmor_pop_traceback(  );
        return(NULL);
    }

/* -------------------------------------------------------------------- */
/*     parse buffer into json object                                    */
/* -------------------------------------------------------------------- */
    oJSON = json_tokener_parse(buffer);
    if( oJSON == NULL ){
        snprintf( msg, CMOR_MAX_STRING,
                "Please validate JSON File!\n! "
                "USE: http://jsonlint.com/\n! "
                "Syntax Error in file: %s\n!  "
                "%s",szFullName, buffer );
        cmor_handle_error( msg, CMOR_CRITICAL );
    }
    cmor_pop_traceback();

    return(oJSON);
}

/************************************************************************/
/*                      CreateDrivingPathTag()                          */
/************************************************************************/
int CreateDrivingPathTag(){
    char szDrSrcID[CMOR_MAX_STRING];
    char szDrVarID[CMOR_MAX_STRING];
    char szNewTag[CMOR_MAX_STRING];

    cmor_add_traceback("CreateDrivingPathTag");
    cmor_is_setup();

    szDrSrcID[0] = '\0';
    szDrVarID[0] = '\0';
    szNewTag[0] = '\0';

    cmor_get_cur_dataset_attribute(GLOBAL_ATT_DRIVING_SOURCE_ID, szDrSrcID);
    if(strncmp(szDrSrcID, GLOBAL_ATT_VAL_NODRIVER, CMOR_MAX_STRING) != 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_DRIVING_VARIANT_ID, szDrVarID);
    }

/* -------------------------------------------------------------------- */
/*   Create Path tag "driving_source_id-driving_variant_id"             */
/*                                                                      */
/*   if driving_source_id to "no-driver" srcDrVarID is not required     */
/*                                                                      */
/* -------------------------------------------------------------------- */
    if(strcmp(szDrSrcID, "no-driver") == 0) {
        strcpy(szNewTag, szDrSrcID);
    } else if((strlen(szDrSrcID) != 0 ) && strlen(szDrVarID) != 0 ) {
        strncat(szNewTag, szDrSrcID, strlen(szDrSrcID));
        strncat(szNewTag, "-", 1);
        strncat(szNewTag, szDrVarID, strlen(szDrVarID));
    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_DRIVING_PATH,
            szNewTag, 0);
    cmor_get_cur_dataset_attribute(GLOBAL_ATT_DRIVING_PATH,
            szDrVarID);
    cmor_pop_traceback();
    return (0);
}
/************************************************************************/
/*                         cmor_dataset_json()                          */
/************************************************************************/
int cmor_dataset_json(char * ressource){
    extern cmor_dataset_def cmor_current_dataset;

    int ierr;
    cmor_add_traceback( "cmor_dataset_json" );
    cmor_is_setup();

    char szVal[CMOR_MAX_STRING];
    json_object *json_obj;

    strncpytrim( cmor_current_dataset.path_template,
                        CMOR_DEFAULT_PATH_TEMPLATE,
                        CMOR_MAX_STRING);

    strncpytrim( cmor_current_dataset.file_template,
                        CMOR_DEFAULT_FILE_TEMPLATE,
                        CMOR_MAX_STRING );

    json_obj = cmor_open_inpathFile(ressource);
    if(json_obj == NULL) {
        cmor_handle_error( "JSON resource file not found", CMOR_CRITICAL );
        cmor_pop_traceback();
        return( 1 );
    }
    json_object_object_foreach(json_obj, key, value) {
        if(key[0] == '#') {
            continue;
        }
        strcpy(szVal, json_object_get_string(value));

        if(strcmp(key, FILE_OUTPUTPATH) == 0 ) {
            strncpytrim( cmor_current_dataset.outpath,
                    szVal,
                    CMOR_MAX_STRING );
            continue;
        } else if(strcmp(key, FILE_PATH_TEMPLATE ) == 0 ){
            strncpytrim( cmor_current_dataset.path_template,
                    szVal,
                    CMOR_MAX_STRING );
            continue;
        } else if(strcmp(key, FILE_NAME_TEMPLATE) == 0 ){
            strncpytrim( cmor_current_dataset.file_template,
                    szVal,
                    CMOR_MAX_STRING );
            continue;
        } else if(strcmp(key, GLOBAL_ATT_ACTIVITY_ID) == 0 ) {
            char *pValue;
            char szValueCpy[CMOR_MAX_STRING];
            strncpy(szValueCpy, szVal, CMOR_MAX_STRING);
            strncpy( cmor_current_dataset.activity_id, szVal, CMOR_MAX_STRING);

            pValue = strtok(szValueCpy, "-");
            if(pValue != NULL) {
                cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_ACTIVITY_SEG1,
                        pValue, 1);
            }

            pValue = strtok( NULL, "-");
            if(pValue != NULL) {
                cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_ACTIVITY_SEG2,
                        pValue, 1);
            }

        } else if((strcmp(key, GLOBAL_ATT_DRIVING_SOURCE_ID) == 0) ||
                  (strcmp(key, GLOBAL_ATT_DRIVING_VARIANT_ID) == 0)){

        }

        cmor_set_cur_dataset_attribute_internal(key, szVal, 1);
    }
    /* Create Special tag for path */
    CreateDrivingPathTag();

    cmor_set_cur_dataset_attribute_internal(key, szVal, 1);
    cmor_current_dataset.initiated = 1;
    cmor_current_dataset.realization = 1;
    cmor_set_cur_dataset_attribute_internal( TABLE_HEADER_TRACKING_ID,
                                             cmor_current_dataset.tracking_id,
                                             0 );
/* -------------------------------------------------------------------- */
/*  Verify if the output directory exist                                */
/* -------------------------------------------------------------------- */
    ierr =   cmor_outpath_exist(cmor_current_dataset.outpath);
    if( ierr ) {
        cmor_pop_traceback();
        return (1);
    }
    cmor_pop_traceback();
    return( 0 );
}

/************************************************************************/
/*                     cmor_put_nc_num_attribute()                      */
/************************************************************************/
int cmor_put_nc_num_attribute(int ncid, int nc_var_id, char *name, char type,
        double value, char *var_name) {
    char msg[CMOR_MAX_STRING];
    int ierr;

    cmor_add_traceback("cmor_put_nc_num_attribute");
    ierr = 0;
    if (type == 'i') {
        ierr = nc_put_att_double(ncid, nc_var_id, name, NC_INT, 1, &value);
    } else if (type == 'l') {
        ierr = nc_put_att_double(ncid, nc_var_id, name, NC_INT, 1, &value);
    } else if (type == 'f') {
        ierr = nc_put_att_double(ncid, nc_var_id, name, NC_FLOAT, 1, &value);
    } else if (type == 'd') {
        ierr = nc_put_att_double(ncid, nc_var_id, name, NC_DOUBLE, 1, &value);
    }
    if (ierr != NC_NOERR) {
        snprintf(msg, CMOR_MAX_STRING,
                "NetCDF Error (%i: %s) setting numerical attribute"
                        " %s on variable %s", ierr, nc_strerror(ierr), name,
                var_name);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }
    cmor_pop_traceback();
    return( ierr );
}

/************************************************************************/
/*                     cmor_put_nc_char_attribute()                     */
/************************************************************************/
int cmor_put_nc_char_attribute(int ncid,
        int nc_var_id,
        char *name,
        char *value,
        char *var_name) {
    int k, ierr;
    char msg[CMOR_MAX_STRING];

    ierr = 0;
    cmor_add_traceback("cmor_put_nc_char_attribute");
    k = strlen(value);
    if (k != 0) {
        value[k] = '\0';
        ierr = nc_put_att_text(ncid, nc_var_id, name, k + 1, value);
        if (ierr != NC_NOERR) {
            snprintf(msg, CMOR_MAX_STRING,
                    "NetCDF Error (%i: %s) setting attribute: '%s' "
                    "on variable (%s)",
                    ierr, nc_strerror(ierr), name, var_name);
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
    }
    cmor_pop_traceback();
    return( ierr) ;
}

/************************************************************************/
/*              cmor_set_cur_dataset_attribute_internal()               */
/************************************************************************/
int cmor_set_cur_dataset_attribute_internal(char *name, char *value,
        int optional) {
    int i, n;
    char msg[CMOR_MAX_STRING];
    extern cmor_dataset_def cmor_current_dataset;

    cmor_add_traceback("cmor_set_cur_dataset_attribute_internal");
    cmor_is_setup();

    cmor_trim_string(value, msg);

    if ((int) strlen(name) > CMOR_MAX_STRING) {
        snprintf(msg, CMOR_MAX_STRING,
                "CMOR Dataset error, attribute name: %s; length (%i) is "
                "greater than limit: %i",
                name, (int) strlen(name), CMOR_MAX_STRING);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return( 1 );
    }

    if ((value == NULL) || (msg[0] == '\0')) {
        if (optional == 1) {
            cmor_pop_traceback();
            return( 0 );
        } else {
            snprintf(msg, CMOR_MAX_STRING,
                    "CMOR Dataset error, required attribute %s was not "
                    "passed or blanked",
                    name);
            cmor_handle_error(msg, CMOR_CRITICAL);
            cmor_pop_traceback();
            return( 1 );
        }
    }

    cmor_trim_string(name, msg);
    n = cmor_current_dataset.nattributes;

    for (i = 0; i <= cmor_current_dataset.nattributes; i++) {
        if (strcmp(msg, cmor_current_dataset.attributes_names[i]) == 0) {
            n = i;
            cmor_current_dataset.nattributes -= 1;
            break;
        }
    }

    if (n >= CMOR_MAX_ATTRIBUTES) {
        sprintf(msg,
                "Setting dataset attribute: %s, we already have %i "
                "elements set which is the max, this element won't be set",
                name, CMOR_MAX_ELEMENTS);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return( 1 );
    }

    strncpy(cmor_current_dataset.attributes_names[n], msg, CMOR_MAX_STRING);
    cmor_trim_string(value, msg);
    strncpytrim(cmor_current_dataset.attributes_values[n], msg, CMOR_MAX_STRING);
    cmor_current_dataset.nattributes += 1;
    cmor_pop_traceback();
    return( 0 );
}

/************************************************************************/
/*                   cmor_get_cur_dataset_attribute()                   */
/************************************************************************/
int cmor_get_cur_dataset_attribute( char *name, char *value ) {
    int i, n;
    char msg[CMOR_MAX_STRING];
    extern cmor_dataset_def cmor_current_dataset;

    cmor_add_traceback( "cmor_get_cur_dataset_attribute" );
    cmor_is_setup(  );
    if( strlen( name ) > CMOR_MAX_STRING ) {
	snprintf( msg, CMOR_MAX_STRING,
		  "CMOR Dataset: %s length is greater than limit: %i",
		  name, CMOR_MAX_STRING );
	cmor_handle_error( msg, CMOR_NORMAL );
	cmor_pop_traceback(  );
	return( 1 );
    }
    n = -1;
    for( i = 0; i <= cmor_current_dataset.nattributes; i++ ) {
	if( strcmp( name, cmor_current_dataset.attributes_names[i] ) == 0 )
	    n = i;
    }
    if( n == -1 ) {
	snprintf( msg, CMOR_MAX_STRING,
		  "CMOR Dataset: current dataset does not have attribute : %s",
		  name );
	cmor_handle_error( msg, CMOR_NORMAL );
	cmor_pop_traceback(  );
	return( 1 );
    }
    strncpy( value, cmor_current_dataset.attributes_values[n],
	     CMOR_MAX_STRING );
    cmor_pop_traceback(  );
    return( 0 );
}

/************************************************************************/
/*                cmor_has_required_global_attributes()                 */
/************************************************************************/
void cmor_has_required_global_attributes( int table_id ) {
    int i, j, n, found;
    char msg[CMOR_MAX_STRING];
    char msg2[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char expt_id[CMOR_MAX_STRING];

    cmor_add_traceback( "cmor_has_required_global_attributes" );
    if( cmor_tables[table_id].required_gbl_att[0] == '\0' ) {
	cmor_pop_traceback(  );
	return;			/* not required */
    }
    cmor_get_cur_dataset_attribute( GLOBAL_ATT_EXPERIMENTID, expt_id );
    
/* -------------------------------------------------------------------- */
/*      ok we want to make sure it is the sht id                        */
/* -------------------------------------------------------------------- */
    for( j = 0; j <= cmor_tables[table_id].nexps; j++ ) {
	if( strcmp( expt_id, cmor_tables[table_id].expt_ids[j] ) == 0 ) {
	    strncpy( expt_id, cmor_tables[table_id].sht_expt_ids[j],
		     CMOR_MAX_STRING );
	    break;
	}
    }

    n = strlen( cmor_tables[table_id].required_gbl_att );

    msg[0] = '\0';
    j = 0;
    i = 0;
    msg2[0] = '\0';
    while( i < n ) {
        while( (( cmor_tables[table_id].required_gbl_att[i] == '[')
                || (cmor_tables[table_id].required_gbl_att[i] == ']')
                || (cmor_tables[table_id].required_gbl_att[i] == '"')
                || (cmor_tables[table_id].required_gbl_att[i] == ',')
                || (cmor_tables[table_id].required_gbl_att[i] == ' ')) ) {
            i++;
        }

	while( (( cmor_tables[table_id].required_gbl_att[i] != '"' )
	       && ( cmor_tables[table_id].required_gbl_att[i] != '\0' ) ) ) {
	    msg[j] = cmor_tables[table_id].required_gbl_att[i];
	    msg[j + 1] = '\0';
	    j++;
	    i++;
	}
	i++;

	found = 0;

	for( j = 0; j < cmor_current_dataset.nattributes; j++ ) {
	    if( strcmp( msg, cmor_current_dataset.attributes_names[j] ) == 0 ) {
		cmor_get_cur_dataset_attribute( msg, ctmp );
		if( strcmp( ctmp, "not specified" ) != 0 ) {
		    found = 1;
		    break;
		}
	    }
	}

	if( found == 0 ) {
	    snprintf( ctmp, CMOR_MAX_STRING,
		      "Required global attribute %s is missing please check call(s) "
		      "to cmor_dataset_json",
		      msg );
	    cmor_handle_error( ctmp, CMOR_CRITICAL );
	}
	strncpy( msg2, msg, CMOR_MAX_STRING );
	j = 0;
    }
    cmor_pop_traceback(  );
    return;
}

/************************************************************************/
/*                 cmor_is_required_global_attribute()                  */
/************************************************************************/
int cmor_is_required_global_attribute( char *name, int table_id ) {
    int i, j, n, req;
    char msg[CMOR_MAX_STRING];

    cmor_add_traceback( "cmor_is_required_global_attribute" );
    if( cmor_tables[table_id].required_gbl_att[0] == '\0' ) {
	cmor_pop_traceback(  );
	return( 1 );		/* not required */
    }

    n = strlen( cmor_tables[table_id].required_gbl_att );

    msg[0] = '\0';
    i = 0;
    j = 0;
    req = 1;
    while( i < n ) {
	while( ( cmor_tables[table_id].required_gbl_att[i] != ' ' ) &&
	       ( cmor_tables[table_id].required_gbl_att[i] != '\0' ) ) {
	    msg[j] = cmor_tables[table_id].required_gbl_att[i];
	    msg[j + 1] = '\0';
	    j++;
	    i++;
	}
	i++;
	j = 0;
	if( strcmp( msg, name ) == 0 ) {
	    req = 0;
	    i = n;
	}
    }
    cmor_pop_traceback(  );
    return req;
}



/************************************************************************/
/*                   cmor_has_cur_dataset_attribute()                   */
/************************************************************************/
int cmor_has_cur_dataset_attribute( char *name ) {
    int i, n;
    char msg[CMOR_MAX_STRING];
    extern cmor_dataset_def cmor_current_dataset;

    cmor_add_traceback( "cmor_has_cur_dataset_attribute" );
    cmor_is_setup(  );
    if( ( int ) strlen( name ) > CMOR_MAX_STRING ) {
	snprintf( msg, CMOR_MAX_STRING,
		  "CMOR Dataset: attribute name (%s) length\n! "
	          "(%i) is greater than limit: %i",
		  name, ( int ) strlen( name ), CMOR_MAX_STRING );
	cmor_handle_error( msg, CMOR_NORMAL );
	cmor_pop_traceback(  );
	return( 1 );
    }
    n = -1;
    for( i = 0; i <= cmor_current_dataset.nattributes; i++ ) {
	if( strcmp( name, cmor_current_dataset.attributes_names[i] ) == 0 )
	    n = i;
    }
    if( n == -1 ) {
	cmor_pop_traceback(  );
	return( 1 );
    }
    cmor_pop_traceback(  );
    return( 0 );
}

/************************************************************************/
/*                      cmor_outpath_exist()                            */
/************************************************************************/
int  cmor_outpath_exist(char *outpath) {
    struct stat buf;
    char msg[CMOR_MAX_STRING];
    FILE *test_file = NULL;

    cmor_add_traceback( "cmor_outpath_exist" );
    cmor_is_setup(  );

/* -------------------------------------------------------------------- */
/*      Very first thing is to make sure the output path does exist     */
/* -------------------------------------------------------------------- */
    if( stat( cmor_current_dataset.outpath, &buf ) == 0 ) {
        if( S_ISREG( buf.st_mode ) != 0 ) {
            sprintf( msg,
                     "You defined your output directory to be: '%s',\n! "
                     "but it appears to be a regular file not a directory",
                     cmor_current_dataset.outpath );
            cmor_handle_error( msg, CMOR_CRITICAL );
            cmor_pop_traceback(  );
            return(1);
        } else if( S_ISDIR( buf.st_mode ) == 0 ) {
            sprintf( msg,
                     "You defined your output directory to be: '%s',\n! "
                     "but it appears to be a special file not a directory",
                     cmor_current_dataset.outpath );
            cmor_handle_error( msg, CMOR_CRITICAL );
            cmor_pop_traceback(  );
            return(1);

        }
/* -------------------------------------------------------------------- */
/*      ok if not root then test permssions                             */
/* -------------------------------------------------------------------- */
        if( getuid(  ) != 0 ) {
            strcpy( msg, cmor_current_dataset.outpath );
            strncat( msg, "/tmp.cmor.test", CMOR_MAX_STRING );
            test_file = fopen( msg, "w" );
            if( test_file == NULL ) {

                sprintf( msg,
                         "You defined your output directory to be: '%s', but\n! "
                        "you do not have read/write permissions on it",
                         cmor_current_dataset.outpath );
                cmor_handle_error( msg, CMOR_CRITICAL );
                cmor_pop_traceback(  );
                return(1);

            } else {
                fclose( test_file );
                remove( msg );
            }
        }
    } else if( errno == ENOENT ) {
        sprintf( msg,
                 "You defined your output directory to be: '%s', but this\n! "
                "directory does not exist",
                 cmor_current_dataset.outpath );
        cmor_handle_error( msg, CMOR_CRITICAL );
        cmor_pop_traceback(  );
        return(1);


    } else if( errno == EACCES ) {
        sprintf( msg,
                 "You defined your output directory to be: '%s', but we\n! "
                "cannot access it, please check permissions",
                 cmor_current_dataset.outpath );
        cmor_handle_error( msg, CMOR_CRITICAL );
        cmor_pop_traceback(  );
        return(1);

    }
    cmor_pop_traceback(  );
    return(0);
}


/************************************************************************/
/*                    cmor_convert_string_to_list()                     */
/************************************************************************/
int cmor_convert_string_to_list( char *invalues, char type, void **target,
				 int *nelts ) {
    int i, j, k, itmp;
    long l;
    double d;
    float f;
    char values[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char msg2[CMOR_MAX_STRING];

    j = 1;
    cmor_add_traceback( "cmor_convert_string_to_list" );
    
/* -------------------------------------------------------------------- */
/*      trim this so no extra spaces after or before                    */
/* -------------------------------------------------------------------- */
    strncpytrim( values, invalues, CMOR_MAX_STRING );
    k = 1;			/* 1 means we are on characters */
    for( i = 0; i < strlen( values ); i++ ) {
	if( values[i] == ' ' ) {
	    if( k == 1 ) {
		j++;
		k = 0;
	    }
	    while( values[i + 1] == ' ' )
		i++;
	} else {
	    k = 1;
	}
    }

    *nelts = j;

    if( type == 'i' )
	*target = malloc( j * sizeof ( int ) );
    else if( type == 'f' )
	*target = malloc( j * sizeof ( float ) );
    else if( type == 'l' )
	*target = malloc( j * sizeof ( long ) );
    else if( type == 'd' )
	*target = malloc( j * sizeof ( double ) );
    else if( type == 'c' )
	*target = ( char ** ) malloc( j * sizeof ( char * ) );
    else {
	snprintf( msg, CMOR_MAX_STRING,
		  "unknown conversion '%c' for list: %s", type, values );
	cmor_handle_error( msg, CMOR_CRITICAL );
    }

    if( *target == NULL ) {
	snprintf( msg, CMOR_MAX_STRING, "mallocing '%c' for list: %s",
		  type, values );
	cmor_handle_error( msg, CMOR_CRITICAL );
    }

    j = 0;
    msg[0] = '\0';
    k = 0;
    itmp = 1;
    for( i = 0; i < strlen( values ); i++ ) {
	if( values[i] == ' ' ) {	/* ok next world */
	    if( itmp == 1 ) {
		itmp = 0;
		msg[i - k] = '\0';
		strncpytrim( msg2, msg, CMOR_MAX_STRING );
		if( type == 'i' ) {
		    sscanf( msg2, "%d", &itmp );
		    ( ( int * ) *target )[j] = ( int ) itmp;
		} else if( type == 'l' ) {
		    sscanf( msg2, "%ld", &l );
		    ( ( long * ) *target )[j] = ( long ) l;
		} else if( type == 'f' ) {
		    sscanf( msg2, "%f", &f );
		    ( ( float * ) *target )[j] = ( float ) f;
		} else if( type == 'd' ) {
		    sscanf( msg2, "%lf", &d );
		    ( ( double * ) *target )[j] = ( double ) d;
		} else if( type == 'c' ) {
		    ( ( char ** ) *target )[j] =
			( char * ) malloc( 13 * sizeof ( char ) );
		    strncpy( ( ( char ** ) *target )[j], msg2, 12 );
		}
		j++;
	    }
	    while( values[i + 1] == ' ' )
		i++;
	    k = i + 1;
	} else {
	    msg[i - k] = values[i];
	    itmp = 1;
	}
    }
/* -------------------------------------------------------------------- */
/*      ok now the last one                                             */
/* -------------------------------------------------------------------- */
    msg[i - k] = '\0';

    strncpytrim( msg2, msg, CMOR_MAX_STRING );
    if( type == 'i' ) {
	sscanf( msg2, "%d", &itmp );
	( ( int * ) *target )[j] = ( int ) itmp;
    } else if( type == 'l' ) {
	sscanf( msg2, "%ld", &l );
	( ( long * ) *target )[j] = ( long ) l;
    } else if( type == 'f' ) {
	sscanf( msg2, "%f", &f );
	( ( float * ) *target )[j] = ( float ) f;
    } else if( type == 'd' ) {
	sscanf( msg2, "%lf", &d );
	( ( double * ) *target )[j] = ( double ) d;
    } else if( type == 'c' ) {
	( ( char ** ) *target )[j] =
	    ( char * ) malloc( 13 * sizeof ( char ) );
	strncpy( ( ( char ** ) *target )[j], msg2, 12 );
    }
    cmor_pop_traceback(  );
    return( 0 );
}

/************************************************************************/
/*                     cmor_define_zfactors_vars()                      */
/************************************************************************/
int cmor_define_zfactors_vars( int var_id, int ncid, int *nc_dim,
			       char *formula_terms, int *nzfactors,
			       int *zfactors, int *nc_zfactors, int i,
			       int dim_bnds ) {
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    int ierr = 0, l, m, k, n, j, m2, found, nelts, *int_list = NULL;
    int dim_holder[CMOR_MAX_VARIABLES];
    int lnzfactors;
    int ics, icd, icdl, ia;

    cmor_add_traceback( "cmor_define_zfactors_vars" );
    cmor_is_setup(  );
    lnzfactors = *nzfactors;
    
/* -------------------------------------------------------------------- */
/*      now figures out the variables for z_factor and loops thru it    */
/* -------------------------------------------------------------------- */
    n = strlen( formula_terms );
    for( j = 0; j < n; j++ ) {
	while( ( formula_terms[j] != ':' ) && ( j < n ) ) {
	    j++;
	}
/* -------------------------------------------------------------------- */
/*      at this point we skipped the name thingy                        */
/* -------------------------------------------------------------------- */
	j++;
	while( formula_terms[j] == ' ' ) {
	    j++;
	}			/* ok we skipped the blanks as well */
	/* ok now we can start scanning the zvar name */
	k = j;
	while( ( formula_terms[j] != ' ' ) && ( formula_terms[j] != '\0' ) ) {
	    ctmp[j - k] = formula_terms[j];
	    j++;
	}
	
/* -------------------------------------------------------------------- */
/*      all right here we reach a  blank, the name is finsihed          */
/* -------------------------------------------------------------------- */
	ctmp[j - k] = '\0';
	
/* -------------------------------------------------------------------- */
/*      here we try to match with the actual variable                   */
/* -------------------------------------------------------------------- */
	l = -1;
	for( k = 0; k < cmor_nvars + 1; k++ ) {
	    if( strcmp( ctmp, cmor_vars[k].id ) == 0 ) {
/* -------------------------------------------------------------------- */
/*      ok that is not enough! We need to know if the dims match!       */
/* -------------------------------------------------------------------- */
		nelts = 0;
		for( m = 0; m < cmor_vars[k].ndims; m++ ) {
		    for( m2 = 0; m2 < cmor_vars[var_id].ndims; m2++ ) {
			if( cmor_vars[k].axes_ids[m] ==
			    cmor_vars[var_id].axes_ids[m2] ) {
			    nelts += 1;
			    break;
			}
		    }
		}
		if( nelts == cmor_vars[k].ndims ) {
		    l = k;
		    break;
		}
	    }
	}

	if( l == -1 ) {
/* -------------------------------------------------------------------- */
/*      ok this looks bad! last hope is that the                        */
/*      zfactor is actually a coordinate!                               */
/* -------------------------------------------------------------------- */
	    found = 0;
	    for( m = 0; m < cmor_vars[var_id].ndims; m++ ) {
		if( strcmp( cmor_axes[cmor_vars[var_id].axes_ids[m]].id,
		        ctmp ) == 0 ) {
		    found = 1;
		    break;
		}
/* -------------------------------------------------------------------- */
/*      ok this axes has bounds let's check against his name + _bnds then*/
/* -------------------------------------------------------------------- */
		if( cmor_axes[cmor_vars[var_id].axes_ids[m]].bounds != NULL ) {

		    sprintf( msg, "%s_bnds",
			     cmor_axes[cmor_vars[var_id].axes_ids[m]].id );
		    if( strcmp( msg, ctmp ) == 0 ) {
			found = 1;
			break;
		    }
		}
	    }
	    if( found == 0 ) {
		snprintf( msg, CMOR_MAX_STRING,
		        "could not find the zfactor variable: %s,\n! "
		        "please define it first, while defining zfactors\n! "
		        "for variable %s (table %s)",
		        ctmp, cmor_vars[var_id].id,
		        cmor_tables[cmor_vars[var_id].
		                    ref_table_id].szTable_id );
		cmor_handle_error( msg, CMOR_CRITICAL );
		cmor_pop_traceback(  );
		return( 1 );
	    }
	} else {
	    found = 0;
	}
/* -------------------------------------------------------------------- */
/*      now figure out if we already defined this zfactor var           */
/* -------------------------------------------------------------------- */

	for( k = 0; k < lnzfactors; k++ )
	    if( zfactors[k] == l )
		found = 1;
	if( found == 0 ) {
/* -------------------------------------------------------------------- */
/*      ok it is a new one                                              */
/* -------------------------------------------------------------------- */

	    zfactors[lnzfactors] = l;
/* -------------------------------------------------------------------- */
/*      ok we need to figure out the dimensions of that zfactor         */
/*      and then define the variable                                    */
/* -------------------------------------------------------------------- */
	    for( k = 0; k < cmor_vars[l].ndims; k++ ) {
		found = 0;
		for( m = 0; m < cmor_vars[var_id].ndims; m++ ) {
		    if( strcmp
			( cmor_axes[cmor_vars[var_id].axes_ids[m]].id,
			  cmor_axes[cmor_vars[l].axes_ids[k]].id ) == 0 ) {
			found = 1;
			dim_holder[k] = nc_dim[m];
/* -------------------------------------------------------------------- */
/*      ok here we mark this factor has time varying if necessary       */
/*      so that we can count the number of time written and make        */
/*      sure it matches the variable                                    */
/* -------------------------------------------------------------------- */
			if( cmor_axes[cmor_vars[var_id].axes_ids[m]].axis=='T'){
			    for( ia = 0; ia < 10; ia++ ) {
				if( cmor_vars[var_id].associated_ids[ia]== -1) {
				    cmor_vars[var_id].associated_ids[ia] = l;
				    break;
				}
			    }
			}
			break;
		    }
		}
		if( found == 0 ) {
		    snprintf( msg, CMOR_MAX_STRING,
		            "variable \"%s\" (table: %s) has axis \"%s\"\n! "
		            "defined with formula terms, but term \"%s\"\n! "
		            "depends on axis \"%s\" which is not part of\n! "
		            "the variable",
		            cmor_vars[var_id].id,
		            cmor_tables[cmor_vars[var_id].
		                        ref_table_id].szTable_id,
		                        cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
		                        ctmp,
		                        cmor_axes[cmor_vars[l].axes_ids[k]].id );
		    cmor_handle_error( msg, CMOR_CRITICAL );
		}
	    }
/* -------------------------------------------------------------------- */
/*      at that point we can define the var                             */
/* -------------------------------------------------------------------- */
	    if( dim_bnds == -1 ) {	/* we are not defining a bnds one */
		if( cmor_vars[l].type == 'd' )
		    ierr =
			nc_def_var( ncid, cmor_vars[l].id, NC_DOUBLE,
				    cmor_vars[l].ndims, &dim_holder[0],
				    &nc_zfactors[lnzfactors] );
		else if( cmor_vars[l].type == 'f' )
		    ierr =
			nc_def_var( ncid, cmor_vars[l].id, NC_FLOAT,
				    cmor_vars[l].ndims, &dim_holder[0],
				    &nc_zfactors[lnzfactors] );
		else if( cmor_vars[l].type == 'l' )
		    ierr =
			nc_def_var( ncid, cmor_vars[l].id, NC_INT,
				    cmor_vars[l].ndims, &dim_holder[0],
				    &nc_zfactors[lnzfactors] );
		else if( cmor_vars[l].type == 'i' )
		    ierr =
			nc_def_var( ncid, cmor_vars[l].id, NC_INT,
				    cmor_vars[l].ndims, &dim_holder[0],
				    &nc_zfactors[lnzfactors] );
		if( ierr != NC_NOERR ) {
		    snprintf( msg, CMOR_MAX_STRING,
		            "NC Error (%i: %s) for variable %s (table %s)\n! "
		            "error defining zfactor var: %i (%s)",
		            ierr, nc_strerror( ierr ),
		            cmor_vars[var_id].id,
		            cmor_tables[cmor_vars[var_id].
		                        ref_table_id].szTable_id,
		                        lnzfactors, cmor_vars[l].id );
		    cmor_handle_error( msg, CMOR_CRITICAL );
		}

/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */
		if( ( CMOR_NETCDF_MODE != CMOR_REPLACE_3 ) &&
		     ( CMOR_NETCDF_MODE != CMOR_PRESERVE_3 ) &&
		     ( CMOR_NETCDF_MODE != CMOR_APPEND_3 ) ) {
		    if( cmor_vars[l].ndims > 0 ) {
		        int nTableID = cmor_vars[l].ref_table_id;
			ics =
			    cmor_tables[nTableID].vars[nTableID].shuffle;
			icd =
			    cmor_tables[nTableID].vars[nTableID].deflate;
			icdl =
			    cmor_tables[nTableID].vars[nTableID].deflate_level;
			ierr = nc_def_var_deflate( ncid,
			        nc_zfactors[lnzfactors],
			        ics, icd, icdl );
			if( ierr != NC_NOERR ) {
			    snprintf( msg, CMOR_MAX_STRING,
			            "NCError (%i: %s) defining compression\n! "
			            "parameters for zfactor variable %s for\n! "
			            "variable '%s' (table %s)",
			            ierr, nc_strerror( ierr ),
			            cmor_vars[l].id,
			            cmor_vars[var_id].id,
			            cmor_tables[nTableID].szTable_id );
			    cmor_handle_error( msg, CMOR_CRITICAL );
			}
		    }
		}

/* -------------------------------------------------------------------- */
/*      Creates attribute related to that variable                      */
/* -------------------------------------------------------------------- */
		for( k = 0; k < cmor_vars[l].nattributes; k++ ) {
/* -------------------------------------------------------------------- */
/*      first of all we need to make sure it is not an empty attribute  */
/* -------------------------------------------------------------------- */
		    if( cmor_has_variable_attribute
			( l, cmor_vars[l].attributes[k] ) != 0 ) {
/* -------------------------------------------------------------------- */
/*      deleted attribute continue on                                   */
/* -------------------------------------------------------------------- */
			continue;
		    }
		    if( strcmp( cmor_vars[l].attributes[k], "flag_values" )
			== 0 ) {
/* -------------------------------------------------------------------- */
/*      ok we need to convert the string to a list of int               */
/* -------------------------------------------------------------------- */
			ierr = cmor_convert_string_to_list(
			        cmor_vars[l].attributes_values_char[k],
			        'i',
			        ( void * ) &int_list,
			        &nelts );

			ierr = nc_put_att_int(
			        ncid,
			        nc_zfactors[lnzfactors],
			        "flag_values",
			        NC_INT,
			        nelts,
			        int_list );

			if( ierr != NC_NOERR ) {
			    snprintf( msg, CMOR_MAX_STRING,
				      "NetCDF Error (%i: %s) setting flags\n! "
			              "numerical attribute on zfactor\n! "
			              "variable %s for variable %s (table %s)",
				      ierr, nc_strerror( ierr ),
				      cmor_vars[l].id,
				      cmor_vars[var_id].id,
				      cmor_tables[cmor_vars[var_id].ref_table_id].
				      szTable_id );
			    cmor_handle_error( msg, CMOR_CRITICAL );
			}
			free( int_list );
		    } else if( cmor_vars[l].attributes_type[k] == 'c' ) {
			ierr = cmor_put_nc_char_attribute(
			        ncid,
			        nc_zfactors[lnzfactors],
			        cmor_vars[l].attributes[k],
			        cmor_vars[l].attributes_values_char[k],
			        cmor_vars[l].id );
		    } else {
			ierr = cmor_put_nc_num_attribute(
			        ncid,
			        nc_zfactors[lnzfactors],
			        cmor_vars[l].attributes[k],
			        cmor_vars[l].attributes_type[k],
			        cmor_vars[l].attributes_values_num[k],
			        cmor_vars[l].id );
		    }
		}
		lnzfactors += 1;
	    } else {
/* -------------------------------------------------------------------- */
/*      ok now we need to see if we have bounds on that variable        */
/* -------------------------------------------------------------------- */
		dim_holder[cmor_vars[l].ndims] = dim_bnds;
		if( cmor_vars[l].type == 'd' )
		    ierr =
			nc_def_var( ncid, cmor_vars[l].id, NC_DOUBLE,
				    cmor_vars[l].ndims + 1, &dim_holder[0],
				    &nc_zfactors[lnzfactors] );
		else if( cmor_vars[l].type == 'f' )
		    ierr =
			nc_def_var( ncid, cmor_vars[l].id, NC_FLOAT,
				    cmor_vars[l].ndims + 1, &dim_holder[0],
				    &nc_zfactors[lnzfactors] );
		else if( cmor_vars[l].type == 'l' )
		    ierr =
			nc_def_var( ncid, cmor_vars[l].id, NC_INT,
				    cmor_vars[l].ndims + 1, &dim_holder[0],
				    &nc_zfactors[lnzfactors] );
		else if( cmor_vars[l].type == 'i' )
		    ierr =
			nc_def_var( ncid, cmor_vars[l].id, NC_INT,
				    cmor_vars[l].ndims + 1, &dim_holder[0],
				    &nc_zfactors[lnzfactors] );
		if( ierr != NC_NOERR ) {
		    snprintf( msg, CMOR_MAX_STRING,
			      "NC Error (%i: %s) for variable %s (table: %s),\n! "
		            "error defining zfactor var: %i (%s)",
			      ierr, nc_strerror( ierr ),
			      cmor_vars[var_id].id,
			      cmor_tables[cmor_vars[var_id].
					  ref_table_id].szTable_id,
			      lnzfactors, cmor_vars[l].id );
		    cmor_handle_error( msg, CMOR_CRITICAL );
		}

/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */

		if( ( CMOR_NETCDF_MODE != CMOR_REPLACE_3 ) &&
		    ( CMOR_NETCDF_MODE != CMOR_PRESERVE_3 ) &&
		    ( CMOR_NETCDF_MODE != CMOR_APPEND_3 ) ) {
                    int nTableID = cmor_vars[l].ref_table_id;

		    ics = cmor_tables[nTableID].vars[nTableID].shuffle;
		    icd = cmor_tables[nTableID].vars[nTableID].deflate;
		    icdl = cmor_tables[nTableID].vars[nTableID].deflate_level;
		    ierr = nc_def_var_deflate( ncid, nc_zfactors[lnzfactors],
		            ics, icd, icdl );

		    if( ierr != NC_NOERR ) {
			snprintf( msg, CMOR_MAX_STRING,
				  "NCError (%i: %s) defining\n! "
			          "compression parameters\n! "
			          "for zfactor variable %s for\n! "
			          "variable '%s' (table %s)",
				  ierr, nc_strerror( ierr ),
				  cmor_vars[l].id, cmor_vars[var_id].id,
				  cmor_tables[cmor_vars[var_id].ref_table_id].
				  szTable_id );
			cmor_handle_error( msg, CMOR_CRITICAL );
		    }
		}

/* -------------------------------------------------------------------- */
/*      Creates attribute related to that variable                      */
/* -------------------------------------------------------------------- */
		for( k = 0; k < cmor_vars[l].nattributes; k++ ) {
/* -------------------------------------------------------------------- */
/*      first of all we need to make sure it is not an empty attribute  */
/* -------------------------------------------------------------------- */
		    if( cmor_has_variable_attribute( l,
		            cmor_vars[l].attributes[k] ) != 0 ) {
/* -------------------------------------------------------------------- */
/*      deleted attribute continue on                                   */
/* -------------------------------------------------------------------- */
			continue;
		    }
		    if(strcmp(cmor_vars[l].attributes[k], "flag_values")== 0 ) {
/* -------------------------------------------------------------------- */
/*      ok we need to convert the string to a list of int               */
/* -------------------------------------------------------------------- */
			ierr =cmor_convert_string_to_list(
			        cmor_vars[l].attributes_values_char[k],
			        'i',
			        ( void * ) &int_list,
			        &nelts );

			ierr = nc_put_att_int( ncid,
			        nc_zfactors[lnzfactors],
			        "flag_values",
			        NC_INT, nelts,
			        int_list );

			if( ierr != NC_NOERR ) {
			    snprintf(
			            msg,
			            CMOR_MAX_STRING,
			            "NetCDF Error (%i: %s) setting flags numerical "
			            "attribute on zfactor variable %s for variable "
			            "%s (table: %s)",
			            ierr, nc_strerror( ierr ),
			            cmor_vars[l].id,
			            cmor_vars[var_id].id,
			            cmor_tables[cmor_vars[var_id].ref_table_id].
			            szTable_id );
			    cmor_handle_error( msg, CMOR_CRITICAL );
			}
			free( int_list );

		    } else if( cmor_vars[l].attributes_type[k] == 'c' ) {
			ierr = cmor_put_nc_char_attribute(
			        ncid,
			        nc_zfactors[lnzfactors],
			        cmor_vars[l].attributes[k],
			        cmor_vars[l].attributes_values_char[k],
			        cmor_vars[l].id );
		    } else {
			ierr = cmor_put_nc_num_attribute(
			        ncid,
			        nc_zfactors[lnzfactors],
			        cmor_vars[l].attributes[k],
			        cmor_vars[l].attributes_type[k],
			        cmor_vars[l].attributes_values_num[k],
			        cmor_vars[l].id );
		    }
		}

		lnzfactors += 1;
	    }
	}

	while( ( formula_terms[j] == ' ' )
	       && ( formula_terms[j] != '\0' ) ) {
	    j++;
	}			/* skip the other whites */
    }
    *nzfactors = lnzfactors;
    cmor_pop_traceback(  );
    return( 0 );
}

/************************************************************************/
/*                          cmor_flip_hybrid()                          */
/************************************************************************/
void cmor_flip_hybrid( int var_id, int i, char *a, char *b, char *abnds,
		       char *bbnds ) {
    int doflip, j, k, l = 0;
    double tmp;
    extern cmor_var_t cmor_vars[CMOR_MAX_VARIABLES];
    extern cmor_axis_t cmor_axes[CMOR_MAX_AXES];

    cmor_add_traceback( "cmor_flip_hybrid" );

/* -------------------------------------------------------------------- */
/*      here we need to look and see if we need to flip the             */
/*      levels again since we overwrote this stuff                      */
/* -------------------------------------------------------------------- */
    doflip = 0;
    cmor_axis_t *pVarAxis;
    cmor_axis_def_t *pTableAxis;
    int nVarAxisTableID;

    pVarAxis = &cmor_axes[cmor_vars[var_id].axes_ids[i]];
    nVarAxisTableID = pVarAxis->ref_table_id;

    pTableAxis = &cmor_tables[nVarAxisTableID].axes[nVarAxisTableID];


    if( pTableAxis->stored_direction == 'd' ) {
/* -------------------------------------------------------------------- */
/*      decrease stuff                                                  */
/* -------------------------------------------------------------------- */
	if( pVarAxis->values[1] > pVarAxis->values[0] )
	    doflip = 1;
    } else {
/* -------------------------------------------------------------------- */
/*      increase stuff                                                  */
/* -------------------------------------------------------------------- */
	if( pVarAxis->values[1] < pVarAxis->values[0] )
	    doflip = 1;
    }

    if( doflip == 1 ) {
/* -------------------------------------------------------------------- */
/*      look for a coeff                                                */
/* -------------------------------------------------------------------- */
	k = -1;
	for( j = 0; j <= cmor_nvars; j++ )
	    if( ( strcmp( cmor_vars[j].id, a ) == 0 ) &&
		( cmor_vars[j].zaxis == cmor_vars[var_id].axes_ids[i] ) ) {
		k = j;
		break;
	    }
/* -------------------------------------------------------------------- */
/*      look for b coeff                                                */
/* -------------------------------------------------------------------- */
	if( b != NULL ) {
	    l = -1;
	    for( j = 0; j <= cmor_nvars; j++ ) {
		if( ( strcmp( cmor_vars[j].id, b ) == 0 ) &&
		    ( cmor_vars[j].zaxis == cmor_vars[var_id].axes_ids[i] ) ) {
		    l = j;
		    break;
		}
	    }
	}

	for( j = 0; j < pVarAxis->length / 2; j++ ) {

	    tmp = pVarAxis->values[j];
	    pVarAxis->values[j] = pVarAxis->values[pVarAxis->length - 1 - j];
	    pVarAxis->values[pVarAxis->length -1 - j] = tmp;

	    tmp = cmor_vars[k].values[j];
	    cmor_vars[k].values[j] =
	                cmor_vars[k].values[pVarAxis->length - 1 - j];
	    cmor_vars[k].values[pVarAxis->length - 1 - j] = tmp;

	    if( b != NULL ) {
		tmp = cmor_vars[l].values[j];
		cmor_vars[l].values[j] =
		          cmor_vars[l].values[pVarAxis->length - 1 - j];
		cmor_vars[l].values[pVarAxis->length - 1 - j] = tmp;
	    }
	}

	if( pVarAxis->bounds != NULL ) {
	    k = -1;
	    for( j = 0; j <= cmor_nvars; j++ ) {
		if( ( strcmp( cmor_vars[j].id, abnds ) == 0 ) &&
		    ( cmor_vars[j].zaxis == cmor_vars[var_id].axes_ids[i] ) ) {
		    k = j;
		    break;
		}
	    }

	    if( bbnds != NULL ) {
		l = -1;
		for( j = 0; j <= cmor_nvars; j++ ) {
		    if( ( strcmp( cmor_vars[j].id, bbnds ) == 0 ) &&
		       ( cmor_vars[j].zaxis == cmor_vars[var_id].axes_ids[i])) {
			l = j;
			break;
		    }
		}
	    }

	    for( j = 0; j < pVarAxis->length; j++ ) {
	        int length = pVarAxis->length;
		tmp = pVarAxis->bounds[j];
		pVarAxis->bounds[j] = pVarAxis->bounds[length * 2 - 1 - j];
		pVarAxis->bounds[length * 2 - 1 - j] = tmp;

		tmp = cmor_vars[k].values[j];
		cmor_vars[k].values[j] = cmor_vars[k].values[length * 2 - 1 - j];
		cmor_vars[k].values[length * 2 - 1 - j] = tmp;

		if( bbnds != NULL ) {
		    tmp = cmor_vars[l].values[j];
		    cmor_vars[l].values[j] = cmor_vars[l].values[length*2-1-j];
		    cmor_vars[l].values[length*2-1-j] = tmp;
		}
	    }
	}
    }
    cmor_pop_traceback(  );
    return;
}

/************************************************************************/
/*                          cmor_set_refvar( )                          */
/************************************************************************/
int  cmor_set_refvar( int var_id, int *refvar, int ntimes_passed ) {

/* -------------------------------------------------------------------- */
/*  Return either associated variable id or passed variable id          */
/* -------------------------------------------------------------------- */
    int nRefVarID = var_id;
    char msg[CMOR_MAX_STRING];
    int nVarRefTblID = cmor_vars[var_id].ref_table_id;
    int ierr;

    cmor_add_traceback("cmor_set_refvar");
    if( refvar != NULL ) {
	nRefVarID = ( int ) *refvar;

	if( cmor_vars[nRefVarID].initialized == -1 ) {
	    snprintf( msg, CMOR_MAX_STRING,
	            "You are trying to write variable \"%s\" in association\n! "
	            "with variable \"%s\" (table %s), but you you need to\n! "
	            "write the associated variable first in order to\n! "
	            "initialize the file and dimensions.",
	            cmor_vars[nRefVarID].id,
	            cmor_vars[var_id].id,
	            cmor_tables[nVarRefTblID].szTable_id );
	    cmor_handle_error( msg, CMOR_CRITICAL );
	}
/* -------------------------------------------------------------------- */
/*      ok now we need to scan the netcdf file                          */
/*      to figure the ncvarid associated                                */
/* -------------------------------------------------------------------- */
	ierr = nc_inq_varid( cmor_vars[nRefVarID].initialized,
	                     cmor_vars[var_id].id,
	                     &cmor_vars[var_id].nc_var_id );

	if( ierr != NC_NOERR ) {
	    sprintf( msg,
		     "Could not find variable: '%s' (table: %s) in file of\n! "
	            "associated variable: '%s'",
		     cmor_vars[var_id].id,
		     cmor_tables[nVarRefTblID].szTable_id,
		     cmor_vars[*refvar].id );
	    cmor_handle_error( msg, CMOR_CRITICAL );
	}
	cmor_vars[var_id].ntimes_written =
	    cmor_vars[nRefVarID].ntimes_written - ntimes_passed;
    }
    cmor_pop_traceback();
    return (nRefVarID);
}
/************************************************************************/
/*                    cmor_validate_activity_id()                       */
/************************************************************************/
int cmor_validate_activity_id(int  nVarRefTblID) {
    char szTableActivity[CMOR_MAX_STRING];
    char szDatasetActivity[CMOR_MAX_STRING];
    char *pszTokenTable;
    char *pszTokenDataset;
    int ierr=0;

    cmor_add_traceback("cmor_validate_activity_id");
    ierr = cmor_get_cur_dataset_attribute(GLOBAL_ATT_ACTIVITY_ID,
                                          szDatasetActivity);
    if( strcmp(szDatasetActivity, "") == 0) {
        return(ierr);
    }
    strncpy(szTableActivity,
            cmor_tables[nVarRefTblID].activity_id,
            CMOR_MAX_STRING);

    pszTokenTable  = strtok(szTableActivity, "-");
    pszTokenDataset  = strtok(szDatasetActivity, "-");

    if(strcmp(pszTokenTable, pszTokenDataset) != 0) {
        ierr = -1;
    } else {
        /* replace with user specified activity for netCDF GLOBAL attributes */
        strcpy(cmor_tables[nVarRefTblID].activity_id, szDatasetActivity);
    }
    cmor_pop_traceback();
    return ierr;
}

/************************************************************************/
/*                       cmor_checkMissing()                            */
/************************************************************************/
void cmor_checkMissing(int varid, int var_id, char type) {
    char msg[CMOR_MAX_STRING];
    int nVarRefTblID;

    cmor_add_traceback("cmor_checkMissing");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;

    if (cmor_vars[varid].nomissing == 0) {
        if (cmor_vars[varid].itype != type) {
            snprintf(msg, CMOR_MAX_STRING,
                    "You defined variable \"%s\" (table %s) with a missing\n! "
                            "value of type \"%c\", but you are now writing data of\n! "
                            "type: \"%c\" this may lead to some spurious handling\n! "
                            "of the missing values", cmor_vars[varid].id,
                    cmor_tables[nVarRefTblID].szTable_id,
                    cmor_vars[varid].itype, type);
            cmor_handle_error(msg, CMOR_WARNING);
        }
    }
    cmor_pop_traceback();
}
/************************************************************************/
/*                    cmor_validateFilename()                           */
/************************************************************************/
int cmor_validateFilename(char *outname, int var_id) {
    int cmode;
    int ierr;
    FILE *fperr;
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    size_t starts[2];
    size_t nctmp;
    int ncid;
    int i;
    cmor_add_traceback("cmor_validateFilename");
    ncid = -1;
    if (USE_NETCDF_4 == 1) {
        cmode = NC_NETCDF4 | NC_CLASSIC_MODEL;
        if ((CMOR_NETCDF_MODE == CMOR_REPLACE_3)
                || (CMOR_NETCDF_MODE == CMOR_PRESERVE_3)
                || (CMOR_NETCDF_MODE == CMOR_APPEND_3)) {
            cmode = NC_CLOBBER;
        }
    } else {
        cmode = NC_CLOBBER;
    }

    if ((CMOR_NETCDF_MODE == CMOR_REPLACE_4)
            || (CMOR_NETCDF_MODE == CMOR_REPLACE_3)) {

        ierr = nc_create(outname, NC_CLOBBER | cmode, &ncid);

    } else if ((CMOR_NETCDF_MODE == CMOR_PRESERVE_4)
            || (CMOR_NETCDF_MODE == CMOR_PRESERVE_3)) {
/* -------------------------------------------------------------------- */
/*      ok first let's check if the file does exists or not             */
/* -------------------------------------------------------------------- */
        fperr = NULL;
        fperr = fopen(outname, "r");
        if (fperr != NULL) {
            snprintf(msg, CMOR_MAX_STRING,
                    "Output file ( %s ) already exists, remove file\n! "
                            "or use CMOR_REPLACE or CMOR_APPEND for\n! "
                            "CMOR_NETCDF_MODE value in cmor_setup", outname);
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
        ierr = nc_create(outname, NC_NOCLOBBER | cmode, &ncid);
    } else if ((CMOR_NETCDF_MODE == CMOR_APPEND_4)
            || (CMOR_NETCDF_MODE == CMOR_APPEND_3)) {
/* -------------------------------------------------------------------- */
/*      ok first let's check if the file does exists or not             */
/* -------------------------------------------------------------------- */
        fperr = NULL;
        fperr = fopen(outname, "r");
        if (fperr == NULL) {

/* -------------------------------------------------------------------- */
/*      ok it does not exists... we will open as new                    */
/* -------------------------------------------------------------------- */
            ierr = nc_create(outname, NC_CLOBBER | cmode, &ncid);

        } else { /*ok it was there already */

            ierr = fclose(fperr);
            ierr = nc_open(outname, NC_WRITE, &ncid);

            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) opening file: %s", ierr,
                        nc_strerror(ierr), outname);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }

            ierr = nc_inq_dimid(ncid, "time", &i);

            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) looking for time\n! "
                                "dimension in file: %s", ierr,
                        nc_strerror(ierr), outname);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }

            ierr = nc_inq_dimlen(ncid, i, &nctmp);
            cmor_vars[var_id].ntimes_written = (int) nctmp;

            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) looking for time\n! "
                                "dimension length in file: %s", ierr,
                        nc_strerror(ierr), outname);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }

            ierr = nc_inq_varid(ncid, cmor_vars[var_id].id,
                    &cmor_vars[var_id].nc_var_id);

            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) looking for variable\n! "
                                "'%s' in file: %s", ierr, nc_strerror(ierr),
                        cmor_vars[var_id].id, outname);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }

            ierr = nc_inq_varid(ncid, "time", &cmor_vars[var_id].time_nc_id);

            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) looking for time of\n! "
                                "variable '%s' in file: %s", ierr,
                        nc_strerror(ierr), cmor_vars[var_id].id, outname);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }

/* -------------------------------------------------------------------- */
/*      ok now we need to read the first time in here                   */
/* -------------------------------------------------------------------- */
            starts[0] = 0;
            ierr = nc_get_var1_double(ncid, cmor_vars[var_id].time_nc_id,
                    &starts[0], &cmor_vars[var_id].first_time);

            starts[0] = cmor_vars[var_id].ntimes_written - 1;
            ierr = nc_get_var1_double(ncid, cmor_vars[var_id].time_nc_id,
                    &starts[0], &cmor_vars[var_id].last_time);

            if (cmor_tables[cmor_axes[cmor_vars[var_id].axes_ids[i]].
                            ref_table_id].axes[cmor_axes[cmor_vars[var_id].
                                                         axes_ids[i]].
                                                         ref_axis_id].
                                                         climatology
                    == 1) {

                snprintf(msg, CMOR_MAX_STRING, "climatology");
                strncpy(ctmp, "climatology_bnds", CMOR_MAX_STRING);
            } else {
                strncpy(ctmp, "time_bnds", CMOR_MAX_STRING);
            }

            ierr = nc_inq_varid(ncid, ctmp, &i);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) looking for time bounds\n! "
                                "of variable '%s' in file: %s", ierr,
                        nc_strerror(ierr), cmor_vars[var_id].id, outname);
                cmor_handle_error(msg, CMOR_WARNING);
                ierr = NC_NOERR;
            } else {
                cmor_vars[var_id].time_bnds_nc_id = i;
/* -------------------------------------------------------------------- */
/*      Here I need to store first/last bounds for appending issues     */
/* -------------------------------------------------------------------- */

                starts[0] = cmor_vars[var_id].ntimes_written - 1;
                starts[1] = 1;
                ierr = nc_get_var1_double(ncid,
                        cmor_vars[var_id].time_bnds_nc_id, &starts[0],
                        &cmor_vars[var_id].last_bound);
                starts[1] = 0;
                ierr = nc_get_var1_double(ncid,
                        cmor_vars[var_id].time_bnds_nc_id, &starts[0],
                        &cmor_vars[var_id].first_bound);
            }
            cmor_vars[var_id].initialized = ncid;
        }
    } else {
        snprintf(msg, CMOR_MAX_STRING, "Unknown CMOR_NETCDF_MODE file mode: %i",
                CMOR_NETCDF_MODE);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }
    if (ierr != NC_NOERR) {
        snprintf(msg, CMOR_MAX_STRING,
                "NetCDF Error (%i: %s) creating file: %s", ierr,
                nc_strerror(ierr), outname);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }
    cmor_pop_traceback();
    return( ncid );
}


/************************************************************************/
/*                      cmor_writeGblAttr()                             */
/************************************************************************/
int cmor_WriteGblAttr(int var_id, int ncid, int ncafid) {
    struct tm *ptr;
    time_t lt;
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char ctmp2[CMOR_MAX_STRING];
    char ctmp5[CMOR_MAX_STRING];
    char ctmp6[CMOR_MAX_STRING];
    double tmps[2];
    int i;
    int ierr;
    float afloat, d;

    int nVarRefTblID;

    cmor_add_traceback("cmor_WriteGblAttr");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;

/* -------------------------------------------------------------------- */
/*      ok writes global attributes                                     */
/*      first figures out time                                          */
/* -------------------------------------------------------------------- */
    lt = time(NULL);
    ptr = gmtime(&lt);
    snprintf(msg, CMOR_MAX_STRING, "%.4i-%.2i-%.2iT%.2i:%.2i:%.2iZ",
            ptr->tm_year + 1900, ptr->tm_mon + 1, ptr->tm_mday, ptr->tm_hour,
            ptr->tm_min, ptr->tm_sec);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_CREATION_DATE, msg, 0);

    if (did_history == 0) {
        snprintf(ctmp, CMOR_MAX_STRING,
                "%s CMOR rewrote data to comply with CF standards "
                        "and %s requirements.", msg,
                cmor_tables[nVarRefTblID].activity_id);

        if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_HISTORY) == 0) {
            cmor_get_cur_dataset_attribute(GLOBAL_ATT_HISTORY, msg);
            snprintf(ctmp2, CMOR_MAX_STRING, "%s %s", msg, ctmp);
            strncpy(ctmp, ctmp2, CMOR_MAX_STRING);
        }
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_HISTORY, ctmp, 0);
        did_history = 1;
    }
/* -------------------------------------------------------------------- */
/*    Set attribute Conventions for netCDF file metadata                */
/* -------------------------------------------------------------------- */

    snprintf(msg, CMOR_MAX_STRING, "%s", cmor_tables[nVarRefTblID].Conventions);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_CONVENTIONS, msg, 0);

/* -------------------------------------------------------------------- */
/*    Set attribute data_specs_versions for netCDF file (CMIP6)         */
/* -------------------------------------------------------------------- */
    if( cmor_tables[nVarRefTblID].data_specs_version != '\0' ) {
            snprintf(msg, CMOR_MAX_STRING, "%s",
                    cmor_tables[nVarRefTblID].data_specs_version);
            cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_DATASPECSVERSION,
                    msg, 0);
    }
/* -------------------------------------------------------------------- */
/*    Set attribute frequency for netCDF file (CMIP6)                   */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, "%s",
            cmor_tables[nVarRefTblID].frequency);
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_FREQUENCY, msg, 0);
/* -------------------------------------------------------------------- */
/*    Set attribute variable_id for netCDF file (CMIP6)                 */
/* -------------------------------------------------------------------- */

    snprintf(msg, CMOR_MAX_STRING, "%s",
            cmor_vars[var_id].id);
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_VARIABLE_ID, msg, 0);


    snprintf(msg, CMOR_MAX_STRING, "Table %s (%s) ",
            cmor_tables[nVarRefTblID].szTable_id,
            cmor_tables[nVarRefTblID].date);

    for (i = 0; i < 16; i++)
        sprintf(&ctmp[2 * i], "%02x", cmor_tables[nVarRefTblID].md5[i]);

    ctmp[32] = '\0';

    strcat(msg, ctmp);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_TABLE_ID, msg, 0);

    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID, ctmp);
    } else {
        ctmp[0] = '\0';
    }

    cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENT, ctmp2);

    snprintf(msg, CMOR_MAX_STRING, "%s model output prepared for %s %s", ctmp,
            cmor_tables[nVarRefTblID].activity_id, ctmp2);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_TITLE, msg, 0);
/* -------------------------------------------------------------------- */
/*      check table cf version vs ours                                  */
/* -------------------------------------------------------------------- */
    afloat = CMOR_CF_VERSION_MAJOR;
    d = CMOR_CF_VERSION_MINOR;
    while (d > 1.) {
        d /= 10.;
    }
    afloat += d;

    if (cmor_tables[nVarRefTblID].cf_version > afloat) {
        snprintf(msg, CMOR_MAX_STRING,
                "Your table (%s) claims to enforce CF version %f but\n! "
                        "this version of the library is designed for CF up\n! "
                        "to: %i.%i, you were writing variable: %s\n! ",
                cmor_tables[nVarRefTblID].szTable_id,
                cmor_tables[nVarRefTblID].cf_version, CMOR_CF_VERSION_MAJOR,
                CMOR_CF_VERSION_MINOR, cmor_vars[var_id].id);
        cmor_handle_error(msg, CMOR_WARNING);
    }
/* -------------------------------------------------------------------- */
/*      Ok now we need to check the parent_experiment_id is valid       */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_PARENT_EXPT_ID) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_PARENT_EXPT_ID, msg);
/* -------------------------------------------------------------------- */
/*      did the user pass an expt?                                      */
/* -------------------------------------------------------------------- */
        if (strcmp(msg, "N/A") != 0) {
            cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENTID, ctmp);
            if (strcmp(msg, ctmp) == 0) {
                sprintf(ctmp, "Your parent_experiment id matches your current "
                        "experiment_id, they are both set to: %s; you "
                        "were writing variable %s (table: %s)", msg,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_handle_error(ctmp, CMOR_NORMAL);
                cmor_pop_traceback();
                return (1);
            } else {
                cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENT, ctmp);
                if (strcmp(msg, ctmp) == 0) {
                    sprintf(ctmp, "Your parent_experiment id matches your "
                            "current experiment_id, they are both set "
                            "to: %s; you were writing variable %s "
                            "(table: %s)", msg, cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(ctmp, CMOR_NORMAL);
                    cmor_pop_traceback();
                    return (1);
                } else {
/* -------------------------------------------------------------------- */
/*      ok now we can check it is a valid "other" experiment            */
/*  CMIP6 does not have parent_experiment parameter anymore             */
/* -------------------------------------------------------------------- */
                    if (cmor_check_expt_id(msg, nVarRefTblID,
                            GLOBAL_INT_ATT_PARENT_EXPT, GLOBAL_ATT_PARENT_EXPT_ID)
                            != 0) {

                        snprintf(ctmp, CMOR_MAX_STRING,
                                "Invalid dataset parent experiment "
                                        "id: %s, check against table: %s, you "
                                        "were writing variable: %s", msg,
                                cmor_tables[nVarRefTblID].szTable_id,
                                cmor_vars[var_id].id);
                        cmor_handle_error(ctmp, CMOR_NORMAL);
                        cmor_pop_traceback();
                        return (1);
                    }
                }
            }
        } else {
            if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_BRANCH_TIME) == 0) {
                cmor_get_cur_dataset_attribute(GLOBAL_ATT_BRANCH_TIME, msg);
                sscanf(msg, "%lf", &tmps[0]);
                if (tmps[0] != 0.) {
                    sprintf(msg,
                            "when dataset attribute parent_experiment_id\n! "
                                    "is set to N/A, branch_time must be 0., you\n! "
                                    "passed: %lf, we are resetting to 0. for\n! "
                                    "variable %s (table: %s)", tmps[0],
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_WARNING);
                    cmor_set_cur_dataset_attribute_internal(
                            GLOBAL_ATT_BRANCH_TIME,
                            "0.",
                            1);
                }
            }
            cmor_set_cur_dataset_attribute_internal(GLOBAL_INT_ATT_PARENT_EXPT,
                    "N/A",
                    1);
        }
    }

    cmor_has_required_global_attributes(nVarRefTblID);

/* -------------------------------------------------------------------- */
/*     check source and model_id are identical                          */
/* -------------------------------------------------------------------- */
    if (strcmp(cmor_tables[nVarRefTblID].activity_id, CMIP6) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID, ctmp5);
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE, ctmp6);
        if (strncmp(ctmp5, ctmp6, strlen(ctmp5)) != 0) {
            snprintf(msg, CMOR_MAX_STRING,
                    "while writing variable %s (table: %s), source\n! "
                            "attribute does not start with '%s', it\n! "
                            "should start with: %s", cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id, GLOBAL_ATT_SOURCE_ID,
                    ctmp5);
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
    }

/* -------------------------------------------------------------------- */
/*      first check if the variable itself has a realm                  */
/* -------------------------------------------------------------------- */

    if (cmor_tables[nVarRefTblID].vars[nVarRefTblID].realm[0] != '\0') {
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_REALM,
                cmor_tables[nVarRefTblID].vars[nVarRefTblID].realm, 0);
    } else {
/* -------------------------------------------------------------------- */
/*      ok it didn't so we're using the value from the table            */
/* -------------------------------------------------------------------- */
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_REALM,
                cmor_tables[nVarRefTblID].realm, 0);
    }
    cmor_generate_uuid(ncid, ncafid, var_id);

/* -------------------------------------------------------------------- */
/*      realization                                                     */
/* -------------------------------------------------------------------- */
    ierr = nc_put_att_int(ncid, NC_GLOBAL, GLOBAL_ATT_REALIZATION, NC_INT, 1,
            &cmor_current_dataset.realization);
    if (ierr != NC_NOERR) {
        snprintf(msg, CMOR_MAX_STRING,
                "NetCDF error (%i: %s) writing variable %s (table: %s)\n! "
                        "global att realization (%i)", ierr, nc_strerror(ierr),
                cmor_vars[var_id].id, cmor_tables[nVarRefTblID].szTable_id,
                cmor_current_dataset.realization);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

/* -------------------------------------------------------------------- */
/*      cmor_ver                                                        */
/* -------------------------------------------------------------------- */
    snprintf(msg, CMOR_MAX_STRING, "%i.%i.%i", CMOR_VERSION_MAJOR,
            CMOR_VERSION_MINOR, CMOR_VERSION_PATCH);
    ierr = nc_put_att_text(ncid, NC_GLOBAL, GLOBAL_ATT_CMORVERSION,
            strlen(msg) + 1, msg);
    if (ierr != NC_NOERR) {
        snprintf(msg, CMOR_MAX_STRING,
                "NetCDF error (%i: %s) writing variable %s (table: %s)\n! "
                        "global att cmor_version (%f)", ierr, nc_strerror(ierr),
                cmor_vars[var_id].id, cmor_tables[nVarRefTblID].szTable_id,
                afloat);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

    if (ncid != ncafid) {
        ierr = nc_put_att_int(ncafid, NC_GLOBAL, GLOBAL_ATT_REALIZATION, NC_INT,
                1, &cmor_current_dataset.realization);
        if (ierr != NC_NOERR) {
            snprintf(msg, CMOR_MAX_STRING,
                    "NetCDF error (%i: %s) writing variable %s\n! "
                            "(table: %s) global att realization (%i) to metafile",
                    ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id,
                    cmor_current_dataset.realization);
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
/* -------------------------------------------------------------------- */
/*      cmor_ver                                                        */
/* -------------------------------------------------------------------- */
        ierr = nc_put_att_text(ncid, NC_GLOBAL, GLOBAL_ATT_CMORVERSION,
                strlen(msg) + 1, msg);
        if (ierr != NC_NOERR) {
            snprintf(msg, CMOR_MAX_STRING,
                    "NetCDF error (%i: %s) writing variable %s\n! "
                            "(table: %s) global att cmor_version (%f)", ierr,
                    nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id, afloat);
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
    }
    cmor_pop_traceback();
    return (0);
}
/************************************************************************/
/*                     cmor_generate_uuid()                             */
/************************************************************************/
void cmor_generate_uuid(int ncid, int ncafid, int var_id){
    int nVarRefTblID;
    int ierr;
    char msg[CMOR_MAX_STRING];
    double tmps[2];
    uuid_t *myuuid;
    uuid_fmt_t fmt;
    void *myuuid_str = NULL;
    size_t uuidlen;
    int i;
    int itmp2;

    cmor_add_traceback("cmor_generate_uuid");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;

/* -------------------------------------------------------------------- */
/*      generates a new unique id                                       */
/* -------------------------------------------------------------------- */
    uuid_create(&myuuid);
    uuid_make(myuuid, 4);

    myuuid_str = NULL;
    fmt = UUID_FMT_STR;

/* -------------------------------------------------------------------- */
/*      Write tracking_id and tracking_prefix                           */
/* -------------------------------------------------------------------- */
    uuid_export(myuuid, fmt, &myuuid_str, &uuidlen);

    if (cmor_tables[nVarRefTblID].tracking_prefix != '\0') {
        strncpy(cmor_current_dataset.tracking_id,
                cmor_tables[nVarRefTblID].tracking_prefix, CMOR_MAX_STRING);
        strcat(cmor_current_dataset.tracking_id, "/");
        strcat(cmor_current_dataset.tracking_id, (char *) myuuid_str);
    } else {
        strncpy(cmor_current_dataset.tracking_id, (char *) myuuid_str,
                CMOR_MAX_STRING);
    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_TRACKING_ID,
            cmor_current_dataset.tracking_id, 0);
    free(myuuid_str);
    uuid_destroy(myuuid);

    for (i = 0; i < cmor_current_dataset.nattributes; i++) {
/* -------------------------------------------------------------------- */
/* Skip "calendar" global attribute                                     */
/* -------------------------------------------------------------------- */
        if (strcmp(cmor_current_dataset.attributes_names[i], "calendar") != 0) {
/* -------------------------------------------------------------------- */
/* Write physics or initialization as int attribute                     */
/* -------------------------------------------------------------------- */
            if ((strcmp(cmor_current_dataset.attributes_names[i],
                    GLOBAL_ATT_PHYSICS_IDX) == 0)
                    || (strcmp(cmor_current_dataset.attributes_names[i],
                            GLOBAL_ATT_INITIA_IDX) == 0)) { /* these two are actually int not char */
                sscanf(cmor_current_dataset.attributes_values[i], "%i", &itmp2);

                ierr = nc_put_att_int(ncid, NC_GLOBAL,
                        cmor_current_dataset.attributes_names[i], NC_INT, 1,
                        &itmp2);

                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF error (%i: %s) for variable %s\n! "
                                    "(table: %s) writing global att: %s (%s)\n! ",
                            ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id,
                            cmor_current_dataset.attributes_names[i],
                            cmor_current_dataset.attributes_values[i]);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
                if (ncid != ncafid) {
                    ierr = nc_put_att_int(ncafid, NC_GLOBAL,
                            cmor_current_dataset.attributes_names[i], NC_INT, 1,
                            &itmp2);
                    if (ierr != NC_NOERR) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "NetCDF error (%i: %s) for variable %s\n! "
                                        "(table: %s) writing global att to\n! "
                                        "metafile: %s (%s)", ierr,
                                nc_strerror(ierr), cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id,
                                cmor_current_dataset.attributes_names[i],
                                cmor_current_dataset.attributes_values[i]);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                }
/* -------------------------------------------------------------------- */
/*  Write Branch_Time as double attribute                               */
/* -------------------------------------------------------------------- */
            } else if (strcmp(cmor_current_dataset.attributes_names[i],
                    GLOBAL_ATT_BRANCH_TIME) == 0) {
                sscanf(cmor_current_dataset.attributes_values[i], "%lf",
                        &tmps[0]);
                ierr = nc_put_att_double(ncid, NC_GLOBAL,
                        cmor_current_dataset.attributes_names[i], NC_DOUBLE, 1,
                        &tmps[0]);
                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF error (%i: %s) for variable %s\n! "
                                    "(table: %s)  writing global att: %s (%s)\n! ",
                            ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id,
                            cmor_current_dataset.attributes_names[i],
                            cmor_current_dataset.attributes_values[i]);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
                if (ncid != ncafid) {
                    ierr = nc_put_att_double(ncafid, NC_GLOBAL,
                            cmor_current_dataset.attributes_names[i], NC_DOUBLE,
                            1, &tmps[0]);
                    if (ierr != NC_NOERR) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "NetCDF error (%i: %s) for variable\n! "
                                        "%s (table: %s), writing global att\n! "
                                        "to metafile: %s (%s)", ierr,
                                nc_strerror(ierr), cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id,
                                cmor_current_dataset.attributes_names[i],
                                cmor_current_dataset.attributes_values[i]);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                }
            } else {
                itmp2 = strlen(cmor_current_dataset.attributes_values[i]);
                if (itmp2 < CMOR_DEF_ATT_STR_LEN) {
                    int nNbAttrs = strlen(
                            cmor_current_dataset.attributes_values[i]);
                    for (itmp2 = nNbAttrs; itmp2 < CMOR_DEF_ATT_STR_LEN;
                            itmp2++) {
                        cmor_current_dataset.attributes_values[i][itmp2] = '\0';
                    }
                    itmp2 = CMOR_DEF_ATT_STR_LEN;
                }
/* -------------------------------------------------------------------- */
/*  Write all "text" attributes                                         */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*      Skip attributes starting with "_"                               */
/* -------------------------------------------------------------------- */
                if (cmor_current_dataset.attributes_names[i][0] != '_') {
                    ierr = nc_put_att_text(ncid, NC_GLOBAL,
                            cmor_current_dataset.attributes_names[i], itmp2,
                            cmor_current_dataset.attributes_values[i]);

                    if (ierr != NC_NOERR) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "NetCDF error (%i: %s) for variable %s\n! "
                                        "(table: %s)  writing global att: %s (%s)",
                                ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id,
                                cmor_current_dataset.attributes_names[i],
                                cmor_current_dataset.attributes_values[i]);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                    if (ncid != ncafid) {
                        ierr = nc_put_att_text(ncafid, NC_GLOBAL,
                                cmor_current_dataset.attributes_names[i], itmp2,
                                cmor_current_dataset.attributes_values[i]);
                        if (ierr != NC_NOERR) {
                            snprintf(msg, CMOR_MAX_STRING,
                                    "NetCDF error (%i: %s) for variable %s\n! "
                                            "(table %s), writing global att to\n! "
                                            "metafile: %s (%s)", ierr,
                                    nc_strerror(ierr), cmor_vars[var_id].id,
                                    cmor_tables[nVarRefTblID].szTable_id,
                                    cmor_current_dataset.attributes_names[i],
                                    cmor_current_dataset.attributes_values[i]);
                            cmor_handle_error(msg, CMOR_CRITICAL);
                        }
                    }
                }
            }
        }
    }
    cmor_pop_traceback();
}

/************************************************************************/
/*                cmor_define_dimensions()                              */
/************************************************************************/
void cmor_define_dimensions(int var_id, int ncid,
                            int ncafid, double *time_bounds,
                            int *nc_dim,
                            int *nc_vars, int *nc_bnds_vars,
                            int *nc_vars_af,
                            size_t *nc_dim_chunking, int *dim_bnds,
                            int *zfactors, int *nc_zfactors,
                            int *nc_dim_af){
    int i,j,k,l,n;
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char ctmp2[CMOR_MAX_STRING];
    char ctmp3[CMOR_MAX_STRING];

    int ierr;
    int tmp_dims[2];
    int dims_bnds_ids[2];
    int nzfactors = 0;
    int nVarRefTblID = cmor_vars[var_id].ref_table_id;
    int ics, icd, icdl;
    int itmpmsg, itmp2, itmp3;

    cmor_add_traceback("cmor_define_dimensions");
    for (i = 0; i < cmor_vars[var_id].ndims; i++) {
/* -------------------------------------------------------------------- */
/*      did we flip that guy?                                           */
/* -------------------------------------------------------------------- */
        if (cmor_axes[cmor_vars[var_id].axes_ids[i]].revert == -1) {
            sprintf(msg, "Inverted axis: %s",
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id);
/* -------------------------------------------------------------------- */
/*      fiddle to avoid duplicated effort here if it's already inverted */
/* -------------------------------------------------------------------- */
            if (!cmor_history_contains(var_id, msg)) {
                cmor_update_history(var_id, msg);
            }
        }
        int nAxisID = cmor_vars[var_id].axes_ids[i];
/* -------------------------------------------------------------------- */
/*      Axis length                                                     */
/* -------------------------------------------------------------------- */
        j = cmor_axes[nAxisID].length;
        if ((i == 0) && (cmor_axes[nAxisID].axis == 'T'))
            j = NC_UNLIMITED;

        if ((cmor_axes[nAxisID].axis == 'X') || (cmor_axes[nAxisID].axis == 'Y')) {
            nc_dim_chunking[i] = j;
        } else if (cmor_axes[nAxisID].isgridaxis == 1) {
            nc_dim_chunking[i] = j;
        } else {
            nc_dim_chunking[i] = 1;
        }

        ierr = nc_def_dim(ncid, cmor_axes[nAxisID].id, j, &nc_dim[i]);
        if (ierr != NC_NOERR) {
            snprintf(msg, CMOR_MAX_STRING,
                    "NetCDF error (%i:%s) for dimension definition of\n! "
                            "axis: %s (%i), for variable %i (%s, table: %s)",
                    ierr, nc_strerror(ierr), cmor_axes[nAxisID].id, nAxisID,
                    var_id, cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
            ierr = nc_enddef(ncid);
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
        nc_dim_af[i] = nc_dim[i];
        if (ncid != ncafid) {
            ierr = nc_def_dim(ncafid, cmor_axes[nAxisID].id, j, &nc_dim_af[i]);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF error (%i: %s) for dimension definition\n! "
                                "of axis: %s (%i) in metafile, variable %s "
                                "(table: %s)", ierr, nc_strerror(ierr),
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].id, i,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      creates the bounds dim (only in metafile?)                      */
/* -------------------------------------------------------------------- */
    ierr = nc_def_dim(ncafid, "bnds", 2, dim_bnds);
    if (ierr != NC_NOERR) {
        snprintf(msg, CMOR_MAX_STRING,
                "NC error (%i: %s), error creating bnds dimension to\n! "
                        "metafile, variable %s (table: %s)", ierr,
                nc_strerror(ierr), cmor_vars[var_id].id,
                cmor_tables[nVarRefTblID].szTable_id);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

/* -------------------------------------------------------------------- */
/*      Now define the variable corresponding to                        */
/*      store the dimensions values                                     */
/* -------------------------------------------------------------------- */
    for (i = 0; i < cmor_vars[var_id].ndims; i++) {
        cmor_axis_t *pAxis;
        pAxis = &cmor_axes[cmor_vars[var_id].axes_ids[i]];
        if (pAxis->store_in_netcdf == 0)
            continue;

        if (pAxis->cvalues == NULL) {
/* -------------------------------------------------------------------- */
/*       first we need to figure out the output type                    */
            /* -------------------------------------------------------------------- */
            switch (cmor_tables[pAxis->ref_table_id].axes[pAxis->ref_axis_id].type) {

            case ('f'):
                j = NC_FLOAT;
                break;
            case ('d'):
                j = NC_DOUBLE;
                break;
            case ('i'):
                j = NC_INT;
                break;
            default:
                j = NC_DOUBLE;
                break;
            }
            ierr = nc_def_var(ncid, pAxis->id, j, 1, &nc_dim[i], &nc_vars[i]);

            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) for variable %s\n! "
                                "(table: %s) error defining dim var: %i (%s)",
                        ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }


/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */
            if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3) &&
                    (CMOR_NETCDF_MODE != CMOR_PRESERVE_3) &&
                    (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {

                int nVarID = cmor_vars[var_id].ref_var_id;
                cmor_var_def_t *pVar;

                pVar = &cmor_tables[nVarRefTblID].vars[nVarID];

                ics = pVar->shuffle;
                icd = pVar->deflate;
                icdl = pVar->deflate_level;

                ierr = nc_def_var_deflate(ncid, nc_vars[i], ics, icd, icdl);

                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NCError (%i: %s) defining compression\n! "
                                    "parameters for dimension %s for variable\n! "
                                    "'%s' (table: %s)", ierr, nc_strerror(ierr),
                            pAxis->id, cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
            }

            nc_vars_af[i] = nc_vars[i];
            if (ncid != ncafid) {
                ierr = nc_def_var(ncafid, pAxis->id, j, 1, &nc_dim_af[i],
                        &nc_vars_af[i]);

                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF Error (%i: %s ) for variable %s\n! "
                                    "(table: %s) error defining dim var: %i\n! "
                                    "(%s) in metafile", ierr, nc_strerror(ierr),
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }

/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */

                if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
                        && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
                        && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {
                    cmor_var_def_t *pVar;
                    pVar =
                            &cmor_tables[nVarRefTblID].vars[cmor_vars[var_id].ref_var_id];
                    ics = pVar->shuffle;
                    icd = pVar->deflate;
                    icdl = pVar->deflate_level;
                    ierr = nc_def_var_deflate(ncafid, nc_vars_af[i], ics, icd,
                            icdl);
                    if (ierr != NC_NOERR) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "NCError (%i: %s) defining compression\n! "
                                        "parameters for dimension %s for\n! "
                                        "variable '%s' (table: %s)", ierr,
                                nc_strerror(ierr), pAxis->id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                }
            }

        } else {
/* -------------------------------------------------------------------- */
/*      ok at this point i'm assuming only 1 string dimension!          */
/*      might need to be revised                                        */
/*      so i only create 1 strlen dim                                   */
/*      first need to figure out if the "region name is defined         */
/* -------------------------------------------------------------------- */
            strcpy(ctmp,
                    cmor_tables[pAxis->ref_table_id].axes[pAxis->ref_axis_id].cname);

            if (ctmp[0] == '\0') {
                strcpy(ctmp, "geo_region");
            }

            if (cmor_has_variable_attribute(var_id, "coordinates") == 0) {
                cmor_get_variable_attribute(var_id, "coordinates", msg);
                l = 0;
                for (j = 0; j < strlen(msg) - strlen(ctmp) + 1; j++) {
                    if (strncmp(ctmp, &msg[j], strlen(ctmp)) == 0) {
                        l = 1;
                        break;
                    }
                }
                if (l == 0) {
                    strncat(msg, " ", CMOR_MAX_STRING - strlen(msg));
                    strncat(msg, ctmp, CMOR_MAX_STRING - strlen(msg));
                }
            } else {
                strncpy(msg, ctmp, CMOR_MAX_STRING);
            }

            cmor_set_variable_attribute_internal(var_id,
                    VARIABLE_ATT_COORDINATES,
                    'c',
                    msg);

            l = 0;
            for (j = 0; j < pAxis->length; j++) {
                strncpy(msg, pAxis->cvalues[j], CMOR_MAX_STRING);
                k = strlen(msg);
                if (k > l) {
                    l = k;
                }
            }
/* -------------------------------------------------------------------- */
/*      ok so now i can create the dummy dim strlen                     */
/* -------------------------------------------------------------------- */
            ierr = nc_def_dim(ncid, "strlen", l, &tmp_dims[1]);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF error (%i: %s) for dummy 'strlen'\n! "
                                "dimension definition of axis: %s (%i) in\n! "
                                "metafile, while writing variable %s (table: %s)",
                        ierr, nc_strerror(ierr), pAxis->id, i,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }
            tmp_dims[0] = nc_dim[i];
            ierr = nc_def_var(ncid, ctmp, NC_CHAR, 2, &tmp_dims[0],
                    &nc_vars[i]);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) for variable %s\n! "
                                "(table: %s) error defining dim var: %i (%s)",
                        ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }

            nc_vars_af[i] = nc_vars[i];

            if (ncid != ncafid) {

                ierr = nc_def_dim(ncafid, "strlen", l, &tmp_dims[1]);

                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF error (%i: %s) for dummy 'strlen'\n! "
                                    "dimension definition of axis: %s (%i) in\n! "
                                    "metafile, while writing variable %s "
                                    "(table: %s)", ierr, nc_strerror(ierr),
                            pAxis->id, i, cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
                tmp_dims[0] = nc_dim_af[i];

                ierr = nc_def_var(ncafid, ctmp, NC_CHAR, 1, &tmp_dims[0],
                        &nc_vars_af[i]);

                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF Error (%i: %s) for variable %s\n! "
                                    "(table: %s) error defining dim var:\n! "
                                    "%i (%s) in metafile", ierr,
                            nc_strerror(ierr), cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
            }
        }
/* -------------------------------------------------------------------- */
/*      ok do we have bounds on this axis?                              */
/* -------------------------------------------------------------------- */
        if ((pAxis->bounds != NULL) || ((i == 0) && (time_bounds != NULL))) {
            strncpy(ctmp, pAxis->id, CMOR_MAX_STRING);
            strncat(ctmp, "_bnds", CMOR_MAX_STRING - strlen(ctmp));
            snprintf(msg, CMOR_MAX_STRING, "bounds");
            if (i == 0) {
/* -------------------------------------------------------------------- */
/*      Ok here we need to see if it is a climatological                */
/*      variable in order to change                                     */
/*      the "bounds" attribute into "climatology"                       */
/* -------------------------------------------------------------------- */
                if (cmor_tables[pAxis->ref_table_id].axes[pAxis->ref_axis_id].climatology
                        == 1) {
                    snprintf(msg, CMOR_MAX_STRING, "climatology");
                    strncpy(ctmp, "climatology_bnds", CMOR_MAX_STRING);
                }
            }
            dims_bnds_ids[0] = nc_dim[i];
            dims_bnds_ids[1] = *dim_bnds;
            switch (cmor_tables[pAxis->ref_table_id].axes[pAxis->ref_axis_id].type) {
            case ('f'):
                j = NC_FLOAT;
                break;
            case ('d'):
                j = NC_DOUBLE;
                break;
            case ('i'):
                j = NC_INT;
                break;
            default:
                j = NC_DOUBLE;
                break;
            }
            ierr = nc_def_var(ncafid, ctmp, j, 2, &dims_bnds_ids[0],
                    &nc_bnds_vars[i]);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) for variable %s\n! "
                                "(table: %s) error defining bounds dim var: %i (%s)",
                        ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }

/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */
            if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
                    && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
                    && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {

                ics = cmor_tables[nVarRefTblID].vars[nVarRefTblID].shuffle;
                icd = cmor_tables[nVarRefTblID].vars[nVarRefTblID].deflate;
                icdl =
                        cmor_tables[nVarRefTblID].vars[nVarRefTblID].deflate_level;

                ierr = nc_def_var_deflate(ncafid, nc_bnds_vars[i], ics, icd,
                        icdl);
                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NCError (%i: %s) defining compression\n! "
                                    "parameters for bounds variable %s for\n! "
                                    "variable '%s' (table: %s)", ierr,
                            nc_strerror(ierr), ctmp, cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
            }
/* -------------------------------------------------------------------- */
/* sets the bounds attribute of parent var                              */
/* -------------------------------------------------------------------- */

            if (i == 0)
                cmor_vars[var_id].time_bnds_nc_id = nc_bnds_vars[i];
            ierr = nc_put_att_text(ncafid, nc_vars[i], msg, strlen(ctmp) + 1,
                    ctmp);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) for variable %s\n! "
                                "(table: %s) error defining bounds attribute\n! "
                                "var: %i (%s)", ierr, nc_strerror(ierr),
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id, i, pAxis->id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }
        }
/* -------------------------------------------------------------------- */
/*      Creates attribute related to that axis                          */
/* -------------------------------------------------------------------- */

        for (j = 0; j < cmor_axes[cmor_vars[var_id].axes_ids[i]].nattributes;
                j++) {
            if (strcmp(cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes[j],
                    "z_factors") == 0) {
/* -------------------------------------------------------------------- */
/*      ok this part checks for z_factor things                         */
/*      creates the formula terms attriubte                             */
/* -------------------------------------------------------------------- */
                strncpy(msg,
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes_values_char[j],
                        CMOR_MAX_STRING);
                n = strlen(msg) + 1;
                ierr = nc_put_att_text(ncid, nc_vars[i], "formula_terms", n,
                        msg);
                if (ierr != NC_NOERR) {
                    snprintf(ctmp, CMOR_MAX_STRING,
                            "NetCDF error (%i: %s) writing formula term "
                                    "att (%s) for axis %i (%s), variable %s "
                                    "(table: %s)", ierr, nc_strerror(ierr), msg,
                            i, cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }

                if (ncid != ncafid) {
                    ierr = nc_put_att_text(ncafid, nc_vars_af[i],
                            "formula_terms", n, msg);
                    if (ierr != NC_NOERR) {
                        snprintf(ctmp, CMOR_MAX_STRING,
                                "NetCDF error (%i: %s) writing formula "
                                        "term att (%s) for axis %i (%s), variable "
                                        "%s (table: %s)", ierr,
                                nc_strerror(ierr), msg, i,
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(ctmp, CMOR_CRITICAL);
                    }
                }
                ierr = cmor_define_zfactors_vars(var_id, ncafid, &nc_dim_af[0],
                        msg, &nzfactors, &zfactors[0], &nc_zfactors[0], i, -1);
            } else if (strcmp(
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes[j],
                    "z_bounds_factors") == 0) {
                cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[i],
                        "formula", 'c', &msg);
                n = strlen(msg) + 1;
                ierr = nc_put_att_text(ncafid, nc_bnds_vars[i], "formula", n,
                        msg);
                cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[i],
                        "standard_name", 'c', &msg);
                n = strlen(msg);
                ierr = nc_put_att_text(ncafid, nc_bnds_vars[i], "standard_name",
                        n, msg);
                cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[i], "units",
                        'c', &msg);
                n = strlen(msg) + 1;
                ierr = nc_put_att_text(ncafid, nc_bnds_vars[i], "units", n,
                        msg);
                /*formula terms */
                strncpy(msg,
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes_values_char[j],
                        CMOR_MAX_STRING);
                n = strlen(msg) + 1;
                ierr = nc_put_att_text(ncafid, nc_bnds_vars[i], "formula_terms",
                        n, msg);
                ierr = cmor_define_zfactors_vars(var_id, ncafid, nc_dim, msg,
                        &nzfactors, &zfactors[0], &nc_zfactors[0], i, *dim_bnds);
            } else if (strcmp(
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes[j],
                    "interval") == 0) {
                if (cmor_has_variable_attribute(var_id, "cell_methods") == 0) {
                    cmor_get_variable_attribute(var_id, "cell_methods", msg);
                } else {
                    strcpy(msg, "");
                }
                strncpy(ctmp, cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                        CMOR_MAX_STRING);
                strncat(ctmp, ":", CMOR_MAX_STRING - strlen(ctmp));
                icd = strlen(ctmp);
                itmpmsg = strlen(msg);
                for (ics = 0; ics < (itmpmsg - icd); ics++) {
                    for (icdl = 0; icdl < icd; icdl++) {
                        ctmp2[icdl] = msg[ics + icdl];
                        ctmp2[icdl + 1] = '\0';
                    }
                    if (strcmp(ctmp2, ctmp) == 0) {
                        itmp2 = strlen(ctmp);
                        for (icdl = 0; icdl < (ics + itmp2 + 1); icdl++) {
                            ctmp2[icdl] = msg[icdl];
                        }
                        while ((msg[icdl] != ' ') && (msg[icdl] != '\0')) {
                            ctmp2[icdl] = msg[icdl];
                            icdl++;
                        }
                        ctmp2[icdl] = '\0';

                        icd = strlen(ctmp2);
/* -------------------------------------------------------------------- */
/*      ok now we need to know if the user passed an                    */
/*      interval or not in order to add it                              */
/* -------------------------------------------------------------------- */
                        cmor_get_axis_attribute(cmor_vars[var_id].axes_ids[i],
                                "interval", 'c', ctmp);

                        cmor_trim_string(ctmp, ctmp3);

                        if (strcmp(ctmp3, "") != 0) {
                            strncat(ctmp2, " (interval: ",
                                    CMOR_MAX_STRING - strlen(ctmp2));
                            strncat(ctmp2, ctmp,
                                    CMOR_MAX_STRING - strlen(ctmp2));
                            strncat(ctmp2, ")",
                                    CMOR_MAX_STRING - strlen(ctmp2));
                        }

                        ierr = strlen(ctmp2) - icd;
                        itmp3 = strlen(msg);
                        for (icdl = icd; icdl < itmp3; icdl++) {
                            ctmp2[icdl + ierr] = msg[icdl];
                            ctmp2[icdl + 1 + ierr] = '\0';
                        }
                        cmor_set_variable_attribute_internal(var_id,
                                "cell_methods",
                                'c',
                                ctmp2);
                        break;
                    }
                }
            } else {
                if ((cmor_tables[cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_table_id].axes[cmor_axes[cmor_vars[var_id].axes_ids[i]].ref_axis_id].type
                        == 'c')
                        && (strcmp(
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].attributes[j],
                                "units") == 0)) {
/* -------------------------------------------------------------------- */
/*      passing we do not want the units attribute                      */
/* -------------------------------------------------------------------- */
                } else {
                    cmor_axis_t *pAxis;
                    pAxis = &cmor_axes[cmor_vars[var_id].axes_ids[i]];

                    if (pAxis->attributes_type[j] == 'c') {
                        ierr = cmor_put_nc_char_attribute(ncid, nc_vars[i],
                                pAxis->attributes[j],
                                pAxis->attributes_values_char[j],
                                cmor_vars[var_id].id);

                        if (ncid != ncafid) {
                            ierr = cmor_put_nc_char_attribute(ncafid,
                                    nc_vars_af[i], pAxis->attributes[j],
                                    pAxis->attributes_values_char[j],
                                    cmor_vars[var_id].id);
                        }
                    } else {
                        ierr = cmor_put_nc_num_attribute(ncid, nc_vars[i],
                                pAxis->attributes[j], pAxis->attributes_type[j],
                                pAxis->attributes_values_num[j],
                                cmor_vars[var_id].id);

                        if (ncid != ncafid) {
                            ierr = cmor_put_nc_num_attribute(ncafid,
                                    nc_vars_af[i], pAxis->attributes[j],
                                    pAxis->attributes_type[j],
                                    pAxis->attributes_values_num[j],
                                    cmor_vars[var_id].id);
                        }
                    }
                }
            }
        }
    }
    cmor_pop_traceback();
}

/************************************************************************/
/*                         cmor_grids_def()                             */
/************************************************************************/
int cmor_grids_def(int var_id, int nGridID, int ncafid, int *nc_dim_af) {
    int ierr;
    int m;
    char msg[CMOR_MAX_STRING];
    double tmps[2];
    int i,j,k,l;
    int nc_associated_vars[6];
    int nc_dims_associated[CMOR_MAX_AXES];
    int nVarRefTblID = cmor_vars[var_id].ref_table_id;
    int m2[5];
    int *int_list = NULL;
    char mtype;
    int nelts;
    int ics, icd, icdl;

    cmor_add_traceback("cmor_define_dimensions");
/* -------------------------------------------------------------------- */
/*      first of all checks for grid_mapping                            */
/* -------------------------------------------------------------------- */

    if (strcmp(cmor_grids[nGridID].mapping, "") != 0) {
/* -------------------------------------------------------------------- */
/*      ok we need to create this dummy variable                        */
/*      that contains all the info                                      */
/* -------------------------------------------------------------------- */

        cmor_set_variable_attribute_internal(var_id,
                VARIALBE_ATT_GRIDMAPPING,
                'c',
                cmor_grids[nGridID].mapping);

        ierr = nc_def_var(ncafid, cmor_grids[nGridID].mapping, NC_INT, 0,
                &nc_dims_associated[0], &m);

        if (ierr != NC_NOERR) {
            snprintf(msg, CMOR_MAX_STRING,
                    "NetCDF error (%i: %s) while defining\n! "
                            "associated grid mapping variable %s for\n! "
                            "variable %s (table: %s)", ierr, nc_strerror(ierr),
                    cmor_grids[nGridID].mapping, cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
/* -------------------------------------------------------------------- */
/*      Creates attributes related to that variable                     */
/* -------------------------------------------------------------------- */
        ierr = cmor_put_nc_char_attribute(ncafid, m, "grid_mapping_name",
                cmor_grids[nGridID].mapping, cmor_vars[var_id].id);
        for (k = 0; k < cmor_grids[cmor_vars[var_id].grid_id].nattributes;
                k++) {
            if (strcmp(cmor_grids[nGridID].attributes_names[k],
                    "standard_parallel1") == 0
                    || strcmp(cmor_grids[nGridID].attributes_names[k],
                            "standard_parallel2") == 0) {

                i = -nGridID - CMOR_MAX_GRIDS;
                if ((cmor_has_grid_attribute(i, "standard_parallel1") == 0)
                        && (cmor_has_grid_attribute(i, "standard_parallel2")
                                == 0)) {
                    cmor_get_grid_attribute(i, "standard_parallel1", &tmps[0]);
                    cmor_get_grid_attribute(i, "standard_parallel2", &tmps[1]);
                    ierr = nc_put_att_double(ncafid, m, "standard_parallel",
                            NC_DOUBLE, 2, &tmps[0]);
                } else if (cmor_has_grid_attribute(i, "standard_parallel1")
                        == 0) {
                    cmor_get_grid_attribute(i, "standard_parallel1", &tmps[0]);
                    ierr = nc_put_att_double(ncafid, m, "standard_parallel",
                            NC_DOUBLE, 1, &tmps[0]);
                } else {
                    cmor_get_grid_attribute(i, "standard_parallel2", &tmps[0]);
                    ierr = nc_put_att_double(ncafid, m, "standard_parallel",
                            NC_DOUBLE, 1, &tmps[0]);
                }
                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF Error (%i: %s) writing\n! "
                                    "standard_parallel to file, variable:\n! "
                                    "%s (table: %s)", ierr, nc_strerror(ierr),
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_NORMAL);
                    cmor_pop_traceback();
                    return (1);
                }
            } else {
                ierr = cmor_put_nc_num_attribute(ncafid, m,
                        cmor_grids[nGridID].attributes_names[k], 'd',
                        cmor_grids[nGridID].attributes_values[k],
                        cmor_grids[nGridID].mapping);
            }
        }
    }
/* -------------------------------------------------------------------- */
/*      Preps the marker for vertices dimensions                        */
/* -------------------------------------------------------------------- */

    m = 0;
/* -------------------------------------------------------------------- */
/*      At this point creates the associated variables                  */
/*      all is done is associated file                                  */
/* -------------------------------------------------------------------- */
    for (i = 0; i < 5; i++) {
        m2[i] = 0;
        j = cmor_grids[nGridID].associated_variables[i];

        if (j != -1) {
/* -------------------------------------------------------------------- */
/*      ok we need to define this variable                              */
/* -------------------------------------------------------------------- */

            l = 0;
/* -------------------------------------------------------------------- */
/*      first we need to figure out the actual                          */
/*      grid dimensions and their netcdf eq                             */
/* -------------------------------------------------------------------- */

            for (k = 0; k < cmor_vars[var_id].ndims; k++) {
                if (cmor_axes[cmor_vars[var_id].axes_ids[k]].isgridaxis == 1) {
                    nc_dims_associated[l] = nc_dim_af[k];

                    if (m2[i] == 0 && (i == 0 || i == 1)) {
                        if (cmor_has_variable_attribute(var_id, "coordinates")
                                == 0) {
                            cmor_get_variable_attribute(var_id, "coordinates", &msg);
                            cmor_cat_unique_string(msg,
                                    cmor_vars[cmor_grids[nGridID].associated_variables[i]].id);
                        } else {
                            strncpy(msg,
                                    cmor_vars[cmor_grids[nGridID].associated_variables[i]].id,
                                    CMOR_MAX_STRING - strlen(msg));
                        }
                        cmor_set_variable_attribute_internal(var_id,
                                "coordinates",
                                'c',
                                msg);
                        m2[i] = 1;
                    }
                    l++;
                }
            }
/* -------------------------------------------------------------------- */
/*      vertices need to be added                                       */
/* -------------------------------------------------------------------- */

            if (((i == 2) || (i == 3)) && (m == 0)) {
/* -------------------------------------------------------------------- */
/*      ok now it has been defined                                      */
/* -------------------------------------------------------------------- */

                m = 1;
                ierr = nc_def_dim(ncafid, "vertices",
                                cmor_axes[cmor_vars[j].axes_ids[cmor_vars[j].ndims
                                        - 1]].length, &nc_dims_associated[l]);
                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF error (%i: %s) while defining\n! "
                                    "vertices dimension, variable %s\n! "
                                    "(table: %s)", ierr, nc_strerror(ierr),
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
            }
            mtype = cmor_vars[j].type;
            if (mtype == 'd')
                ierr = nc_def_var(ncafid, cmor_vars[j].id, NC_DOUBLE,
                        cmor_vars[j].ndims, &nc_dims_associated[0],
                        &nc_associated_vars[i]);
            else if (mtype == 'f')
                ierr = nc_def_var(ncafid, cmor_vars[j].id, NC_FLOAT,
                        cmor_vars[j].ndims, &nc_dims_associated[0],
                        &nc_associated_vars[i]);
            else if (mtype == 'l')
                ierr = nc_def_var(ncafid, cmor_vars[j].id, NC_INT,
                        cmor_vars[j].ndims, &nc_dims_associated[0],
                        &nc_associated_vars[i]);
            else if (mtype == 'i')
                ierr = nc_def_var(ncafid, cmor_vars[j].id, NC_INT,
                        cmor_vars[j].ndims, &nc_dims_associated[0],
                        &nc_associated_vars[i]);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF error (%i: %s) while defining\n! "
                                "associated variable %s, of variable\n! "
                                "%s (table: %s)", ierr, nc_strerror(ierr),
                        cmor_vars[j].id, cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }

/* -------------------------------------------------------------------- */
/*      Creates attributes related to that variable                     */
/* -------------------------------------------------------------------- */
            for (k = 0; k < cmor_vars[j].nattributes; k++) {

/* -------------------------------------------------------------------- */
/*      first of all we need to make sure it is not an empty attribute  */
/* -------------------------------------------------------------------- */
                if (cmor_has_variable_attribute(j, cmor_vars[j].attributes[k])
                        != 0) {
/* -------------------------------------------------------------------- */
/*      deleted attribute continue on                                   */
/* -------------------------------------------------------------------- */
                    continue;
                }
                if (strcmp(cmor_vars[j].attributes[k], "flag_values") == 0) {
/* -------------------------------------------------------------------- */
/*      ok we need to convert the string to a list of int               */
/* -------------------------------------------------------------------- */
                    ierr = cmor_convert_string_to_list(
                            cmor_vars[j].attributes_values_char[k], 'i',
                            (void *) &int_list, &nelts);

                    ierr = nc_put_att_int(ncafid, nc_associated_vars[i],
                            "flag_values", NC_INT, nelts, int_list);

                    if (ierr != NC_NOERR) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "NetCDF Error (%i: %s) setting\n! "
                                        "flags numerical attribute on\n! "
                                        "associated variable %s, for\n! "
                                        "variable %s (table: %s)", ierr,
                                nc_strerror(ierr), cmor_vars[j].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                    free(int_list);
                } else if (cmor_vars[j].attributes_type[k] == 'c') {
                    ierr = cmor_put_nc_char_attribute(ncafid,
                            nc_associated_vars[i], cmor_vars[j].attributes[k],
                            cmor_vars[j].attributes_values_char[k],
                            cmor_vars[j].id);
                } else {
                    ierr = cmor_put_nc_num_attribute(ncafid,
                            nc_associated_vars[i], cmor_vars[j].attributes[k],
                            cmor_vars[j].attributes_type[k],
                            cmor_vars[j].attributes_values_num[k],
                            cmor_vars[j].id);
                }
            }
/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */

            if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
                    && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
                    && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {
                if (cmor_vars[j].ndims > 0) {
                    ics =
                            cmor_tables[cmor_vars[j].ref_table_id].vars[cmor_vars[j].ref_var_id].shuffle;
                    icd =
                            cmor_tables[cmor_vars[j].ref_table_id].vars[cmor_vars[j].ref_var_id].deflate;
                    icdl =
                            cmor_tables[cmor_vars[j].ref_table_id].vars[cmor_vars[j].ref_var_id].deflate_level;
                    ierr = nc_def_var_deflate(ncafid, nc_associated_vars[i],
                            ics, icd, icdl);
                    if (ierr != NC_NOERR) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "NetCDF Error (%i: %s) defining\n! "
                                        "compression parameters for\n! "
                                        "associated variable '%s' for\n! "
                                        "variable %s (table: %s)", ierr,
                                nc_strerror(ierr), cmor_vars[j].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                }
            }
        }
    }
    cmor_pop_traceback();
    return(0);
}
/************************************************************************/
/*                create_singleton_dimensions()                         */
/************************************************************************/
void create_singleton_dimensions(int var_id, int ncid, int *nc_singletons,
                                 int *nc_singletons_bnds, int *dim_bnds){
    int ierr;
    int i,j,k;
    char msg[CMOR_MAX_STRING];
    int nVarRefTblID;

    cmor_add_traceback("create_singleton_dimensions");
    nVarRefTblID = cmor_vars[var_id].ref_table_id;

/* -------------------------------------------------------------------- */
/*      Creates singleton dimension variables                           */
/* -------------------------------------------------------------------- */
    for (i = 0; i < CMOR_MAX_DIMENSIONS; i++) {
        j = cmor_vars[var_id].singleton_ids[i];
        if (j != -1) {
            if (cmor_tables[cmor_axes[j].ref_table_id].axes[cmor_axes[j].ref_axis_id].type
                    == 'c') {
                ierr =  nc_def_dim(ncid, "strlen", strlen(
         cmor_tables[cmor_axes[j].ref_table_id].axes[cmor_axes[j].ref_axis_id].cvalue),
         &k);
                ierr = nc_def_var(ncid, cmor_axes[j].id, NC_CHAR, 1, &k,
                        &nc_singletons[i]);
            } else {
                ierr = nc_def_var(ncid, cmor_axes[j].id, NC_DOUBLE, 0,
                        &nc_singletons[i], &nc_singletons[i]);
            }
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) defining scalar variable\n! "
                                "%s for variable %s (table: %s)", ierr,
                        nc_strerror(ierr), cmor_axes[j].id,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }
/* -------------------------------------------------------------------- */
/*      now  puts on its attributes                                     */
/* -------------------------------------------------------------------- */
            for (k = 0; k < cmor_axes[j].nattributes; k++) {
                if (cmor_axes[j].attributes_type[k] == 'c') {
                    ierr = cmor_put_nc_char_attribute(ncid, nc_singletons[i],
                            cmor_axes[j].attributes[k],
                            cmor_axes[j].attributes_values_char[k],
                            cmor_vars[var_id].id);
                } else {
                    ierr = cmor_put_nc_num_attribute(ncid, nc_singletons[i],
                            cmor_axes[j].attributes[k],
                            cmor_axes[j].attributes_type[k],
                            cmor_axes[j].attributes_values_num[k],
                            cmor_vars[var_id].id);
                }
            }
/* -------------------------------------------------------------------- */
/*      ok we need to see if there's bounds as well...                  */
/* -------------------------------------------------------------------- */

            if (cmor_axes[j].bounds != NULL) { /*yep */
                snprintf(msg, CMOR_MAX_STRING, "%s_bnds", cmor_axes[j].id);
                ierr = cmor_put_nc_char_attribute(ncid, nc_singletons[i],
                        "bounds", msg, cmor_vars[var_id].id);
                ierr = nc_def_var(ncid, msg, NC_DOUBLE, 1, dim_bnds,
                        &nc_singletons_bnds[i]);
                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF Error (%i: %s) defining scalar\n! "
                                    "bounds variable %s for variable %s (table: %s)",
                            ierr, nc_strerror(ierr), cmor_axes[j].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
            }
        }
    }
    cmor_pop_traceback();

}
/************************************************************************/
/*                             cmor_write()                             */
/************************************************************************/
int cmor_write( int var_id, void *data, char type,
		int ntimes_passed, double *time_vals, double *time_bounds,
		int *refvar ) {
    extern cmor_var_t cmor_vars[CMOR_MAX_VARIABLES];
    extern cmor_axis_t cmor_axes[CMOR_MAX_AXES];
    extern int cmor_nvars;
    extern cmor_dataset_def cmor_current_dataset;

    int i,  ierr = 0, ncid, ncafid;
    char outname[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char ctmp2[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char appending_to[CMOR_MAX_STRING];
    size_t nc_dim_chunking[CMOR_MAX_AXES];
    int nc_vars[CMOR_MAX_VARIABLES];
    int nc_vars_af[CMOR_MAX_VARIABLES];
    int nc_dim_af[CMOR_MAX_DIMENSIONS];
    int nc_associated_vars[6];
    int nc_bnds_vars[CMOR_MAX_VARIABLES];
    int dim_bnds;
    int nc_singletons[CMOR_MAX_DIMENSIONS];
    int nc_singletons_bnds[CMOR_MAX_DIMENSIONS];
    char mtype;
    int nzfactors = 0;
    int nc_dim[CMOR_MAX_AXES];
    int zfactors[CMOR_MAX_VARIABLES];
    int nc_zfactors[CMOR_MAX_VARIABLES];
    int refvarid;
    int isfixed = 0;
    int origRealization = 0;
    uuid_t *myuuid;
    uuid_fmt_t fmt;
    void *myuuid_str = NULL;
    size_t uuidlen;

    int  nVarRefTblID;
    extern int cmor_convert_char_to_hyphen( char c );
    char szPathTemplate[CMOR_MAX_STRING];
    char outpath[CMOR_MAX_STRING];

    cmor_add_traceback( "cmor_write" );


    nVarRefTblID = cmor_vars[var_id].ref_table_id;

    strcpy( appending_to, "" );	/* initialize to nothing */
    strcpy( outname, "" );
    strcpy( ctmp, "" );
    strcpy( msg, "" );
    strcpy( ctmp2, "" );
    strcpy( outpath,"" );

    cmor_is_setup(  );
    if( var_id > cmor_nvars ) {

	cmor_handle_error( "var_id %i not defined", CMOR_CRITICAL );
	cmor_pop_traceback(  );
	return -1;
    };

    ierr = cmor_validate_activity_id(nVarRefTblID);
    if( ierr != 0) {
        char szActivity[CMOR_MAX_STRING];
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_ACTIVITY_ID, szActivity);

        snprintf( msg, CMOR_MAX_STRING,
                "You defined an activity_id of \"%s\" but your input table\n! "
                "%s has an activity ID starting with \"%s\".  The first segment\n! "
                "both strings need to match, make sure you are using the\n! "
                "right table for this activity.\n!\n! "
                "verify your json input file and make sure you call \n! "
                "dataset_json(<file.json>)\n!",
                szActivity,
                cmor_tables[nVarRefTblID].szTable_id,
                cmor_tables[nVarRefTblID].activity_id);

        cmor_handle_error( msg, CMOR_CRITICAL);

    }

    ierr = cmor_addVersion();
    ierr = cmor_addRIPF(ctmp);

/* -------------------------------------------------------------------- */
/*    Make sure that variable_id is set Global Attributes and for       */
/*    File and Path template                                            */
/* -------------------------------------------------------------------- */
    cmor_set_cur_dataset_attribute_internal(
                    GLOBAL_ATT_VARIABLE_ID,
                    cmor_vars[var_id].id,
                    1);
    ctmp[0] = '\0';
/* -------------------------------------------------------------------- */
/*      here we check that the variable actually has all                */
/*      the required attributes set                                     */
/* -------------------------------------------------------------------- */
    cmor_has_required_variable_attributes( var_id );

/* -------------------------------------------------------------------- */
/*  Do we have associated variables (z_factors)?                        */
/* -------------------------------------------------------------------- */
    refvarid = cmor_set_refvar(var_id, refvar, ntimes_passed);

/* -------------------------------------------------------------------- */
/*      Here we check that the types are consistent between             */
/*      the missing value passed and the type passed now                */
/* -------------------------------------------------------------------- */
    cmor_checkMissing(refvarid, var_id, type);

/* -------------------------------------------------------------------- */
/*      Variable never been thru cmor_write,                            */
/*      we need to define everything                                    */
/* -------------------------------------------------------------------- */
    if( cmor_vars[refvarid].initialized == -1 ) {

	if( cmor_vars[refvarid].type != type ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "Converted type from '%c' to '%c'", type,
		      cmor_vars[refvarid].type );
	    cmor_update_history( refvarid, msg );
	}

	if( cmor_has_cur_dataset_attribute( GLOBAL_ATT_FORCING ) == 0 ) {
	    cmor_get_cur_dataset_attribute( GLOBAL_ATT_FORCING, ctmp2 );
	    cmor_check_forcing_validity( nVarRefTblID,
					 ctmp2 );
	}

/* -------------------------------------------------------------------- */
/*      need to store the product type                                  */
/* -------------------------------------------------------------------- */
	strncpy( ctmp2, cmor_tables[nVarRefTblID].product, CMOR_MAX_STRING );
	cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_PRODUCT, ctmp2, 1);

/* -------------------------------------------------------------------- */
/*      we will need the expt_id for the filename                       */
/*      so we check its validity here                                   */
/* -------------------------------------------------------------------- */
	cmor_get_cur_dataset_attribute( GLOBAL_ATT_EXPERIMENTID, ctmp2 );

/* -------------------------------------------------------------------- */
/*      ok here we check the exptid is ok                               */
/* -------------------------------------------------------------------- */
	ierr = cmor_check_expt_id( ctmp2,
	            nVarRefTblID,
                    GLOBAL_ATT_EXPERIMENT,
                    GLOBAL_ATT_EXPERIMENTID );

	if( ierr != 0 ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "Invalid dataset experiment id: %s, while writing\n! "
	              "variable %s, check against table: %s",
		      ctmp2, cmor_vars[var_id].id,
		      cmor_tables[nVarRefTblID].szTable_id );
	    cmor_handle_error( msg, CMOR_NORMAL );
	    cmor_pop_traceback(  );
	    return( 1 );
	}

/* -------------------------------------------------------------------- */
/*      Figures out path                                                */
/* -------------------------------------------------------------------- */
	strncpy(szPathTemplate, cmor_current_dataset.path_template,
	        CMOR_MAX_STRING);

	if( CMOR_CREATE_SUBDIRECTORIES == 1 ) {
	    cmor_CreateFromTemplate(var_id, szPathTemplate,
	                                outname, "/");
	} else {
	    cmor_CreateFromTemplate( var_id, szPathTemplate, msg, "/");
	    strncpytrim( outname, cmor_current_dataset.outpath,
			 CMOR_MAX_STRING );
	}
	ierr = cmor_mkdir(outname);
        if( (ierr != 0) && (errno != EEXIST ) ) {
            sprintf( ctmp,
                    "creating outpath: %s, for variable %s (table: %s). "
                    "Not enough permission?",
                    outname, cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].
                                ref_table_id].szTable_id );
            cmor_handle_error( ctmp, CMOR_CRITICAL );
        }

	strncat( outname, "/", CMOR_MAX_STRING - strlen( outname ) );
/* -------------------------------------------------------------------- */
/*    Verify that var name does not contain "_" or "-"                  */
/* -------------------------------------------------------------------- */
	for( i = 0; i < strlen( cmor_vars[var_id].id ); i++ ) {
	    if( ( cmor_vars[var_id].id[i] == '_' ) ||
	        ( cmor_vars[var_id].id[i] == '-' ) ) {
		snprintf( outname, CMOR_MAX_STRING,
			  "var_id cannot contain %c you passed: %s "
		          "(table: %s). Please check your input tables\n! ",
			  cmor_vars[var_id].id[i], cmor_vars[var_id].id,
			  cmor_tables[nVarRefTblID].szTable_id );
		cmor_handle_error( outname, CMOR_CRITICAL );
	    }
	}
/* -------------------------------------------------------------------- */
/*    Create/Save filename                                              */
/* -------------------------------------------------------------------- */
	ierr = cmor_CreateFromTemplate(var_id,
	        cmor_current_dataset.file_template,
	        outname, "_");

	strcat(outpath, outname);
	strncpy(outname, outpath, CMOR_MAX_STRING);
	strncpytrim(cmor_vars[var_id].base_path, outname, CMOR_MAX_STRING);

/* -------------------------------------------------------------------- */
/*      Add Process ID and a random number to filename                  */
/* -------------------------------------------------------------------- */
	sprintf( msg, "%d", ( int ) getpid(  ) );
	strncat( outname, msg, CMOR_MAX_STRING - strlen( outname ) );

/* -------------------------------------------------------------------- */
/*      Add the '.nc' extension                                         */
/* -------------------------------------------------------------------- */
	strncat( outname, ".nc", CMOR_MAX_STRING - strlen( outname ) );
	strncpytrim( cmor_vars[var_id].current_path, outname, CMOR_MAX_STRING );
/* -------------------------------------------------------------------- */
/*      Decides NetCDF mode                                             */
/* -------------------------------------------------------------------- */
	ncid = cmor_validateFilename(outname, var_id);

/* -------------------------------------------------------------------- */
/*      we closed and reopened the same test, in case we                */
/*      were appending, in which case all declaration have              */
/*      been done if the open loop                                      */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*      Variable never been thru cmor_write, we need to                 */
/*      define everything                                               */
/*                                                                      */
/*      define global attributes                                        */
/* -------------------------------------------------------------------- */
	if( cmor_current_dataset.initiated == 0 ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "you need to initialize the dataset by calling "
	              "cmor_dataset before calling cmor_write" );
	    cmor_handle_error( msg, CMOR_NORMAL );
	    cmor_pop_traceback(  );
	    return( 1 );
	}

	cleanup_varid = var_id;
	ncafid = ncid;

/* -------------------------------------------------------------------- */
/*      make sure we are in def mode                                    */
/* -------------------------------------------------------------------- */
	ierr = nc_redef( ncafid );
	if( ierr != NC_NOERR && ierr != NC_EINDEFINE ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "NetCDF Error (%i: %s) putting metadata file (%s) in\n! "
	              "def mode, nc file id was: %i, you were writing\n! "
	              "variable %s (table: %s)",
		      ierr, nc_strerror( ierr ),
		      cmor_current_dataset.associated_file_name, ncafid,
		      cmor_vars[var_id].id,
		      cmor_tables[nVarRefTblID].szTable_id );
	    cmor_handle_error( msg, CMOR_CRITICAL );
	}

	ierr = cmor_WriteGblAttr(var_id, ncid, ncafid);
	if(ierr != 0) {
	    return ierr;
	}

        if( isfixed == 1 ) {
            cmor_current_dataset.realization = origRealization;
        }
/* -------------------------------------------------------------------- */
/*      store netcdf file id associated with this variable              */
/* -------------------------------------------------------------------- */
	cmor_vars[var_id].initialized = ncid;
/* -------------------------------------------------------------------- */
/*      define dimensions in NetCDF file                                */
/* -------------------------------------------------------------------- */

        cmor_define_dimensions(var_id, ncid, ncafid, time_bounds, nc_dim,
                nc_vars, nc_bnds_vars, nc_vars_af, nc_dim_chunking, &dim_bnds,
                zfactors, nc_zfactors,nc_dim_af);

/* -------------------------------------------------------------------- */
/*      Store the dimension id for reuse when writting                  */
/*      over multiple call to cmor_write                                */
/* -------------------------------------------------------------------- */
	cmor_vars[var_id].time_nc_id = nc_vars[0];

        int nGridID;
        nGridID = cmor_vars[var_id].grid_id;
/* -------------------------------------------------------------------- */
/*      check if it is a grid thing                                     */
/* -------------------------------------------------------------------- */
	if( nGridID > -1 ) {
	    ierr = cmor_grids_def(var_id, nGridID,ncafid, nc_dim_af);
	    if(ierr) return(ierr);
	}

	create_singleton_dimensions(var_id, ncid,
	        nc_singletons, nc_singletons_bnds, &dim_bnds);


/* -------------------------------------------------------------------- */
/*      Creating variable to write                                      */
/* -------------------------------------------------------------------- */
	mtype = cmor_vars[var_id].type;
	if( mtype == 'd' )
	    ierr =
		nc_def_var( ncid, cmor_vars[var_id].id, NC_DOUBLE,
			    cmor_vars[var_id].ndims, &nc_dim[0],
			    &nc_vars[cmor_vars[var_id].ndims] );
	else if( mtype == 'f' )
	    ierr =
		nc_def_var( ncid, cmor_vars[var_id].id, NC_FLOAT,
			    cmor_vars[var_id].ndims, &nc_dim[0],
			    &nc_vars[cmor_vars[var_id].ndims] );
	else if( mtype == 'l' )
	    ierr =
		nc_def_var( ncid, cmor_vars[var_id].id, NC_INT,
			    cmor_vars[var_id].ndims, &nc_dim[0],
			    &nc_vars[cmor_vars[var_id].ndims] );
	else if( mtype == 'i' )
	    ierr =
		nc_def_var( ncid, cmor_vars[var_id].id, NC_INT,
			    cmor_vars[var_id].ndims, &nc_dim[0],
			    &nc_vars[cmor_vars[var_id].ndims] );
	if( ierr != NC_NOERR ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "NetCDF Error (%i: %s) writing variable: %s (table: %s)",
		      ierr, nc_strerror( ierr ), cmor_vars[var_id].id,
		      cmor_tables[nVarRefTblID].szTable_id );
	    cmor_handle_error( msg, CMOR_CRITICAL );
	}

/* -------------------------------------------------------------------- */
/*      Store the var id for reuse when writting                        */
/*      over multiple call to cmor_write and for cmor_close             */
/* -------------------------------------------------------------------- */
	cmor_vars[var_id].nc_var_id = nc_vars[cmor_vars[var_id].ndims];

	cmor_create_var_attributes(var_id, ncid, ncafid, nc_vars, nc_bnds_vars,
	        nc_vars_af, nc_associated_vars, nc_singletons, nc_singletons_bnds,
	        nc_zfactors, zfactors, nzfactors, nc_dim_chunking, outname);



    } else {
/* -------------------------------------------------------------------- */
/*      Variable already been thru cmor_write,                          */
/*      we just get the netcdf file id                                  */
/* -------------------------------------------------------------------- */

	ncid = cmor_vars[refvarid].initialized;

/* -------------------------------------------------------------------- */
/*      generates a new unique id                                       */
/* -------------------------------------------------------------------- */
	uuid_create( &myuuid );
	uuid_make( myuuid, 4 );
	myuuid_str = NULL;
	fmt = UUID_FMT_STR;
	uuid_export( myuuid, fmt, &myuuid_str, &uuidlen );
	if( cmor_tables[nVarRefTblID].tracking_prefix != '\0' ) {
	    strncpy( cmor_current_dataset.tracking_id,
		     cmor_tables[nVarRefTblID].tracking_prefix,
		     CMOR_MAX_STRING );
	    strcat( cmor_current_dataset.tracking_id, "/" );
	    strcat( cmor_current_dataset.tracking_id,
		    ( char * ) myuuid_str );
	} else {
	    strncpy( cmor_current_dataset.tracking_id,
		     ( char * ) myuuid_str, CMOR_MAX_STRING );
	}
	cmor_set_cur_dataset_attribute_internal( GLOBAL_ATT_TRACKING_ID,
	        cmor_current_dataset.tracking_id,
	        0 );

	ierr =
	    nc_put_att_text( ncid, NC_GLOBAL, GLOBAL_ATT_TRACKING_ID,
			     ( int ) uuidlen, myuuid_str );
	if( ierr != NC_NOERR ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "NetCDF error (%i: %s) for variable %s (table: %s)\n! "
	              "writing global att: %s (%s)",
		      ierr, nc_strerror( ierr ), cmor_vars[var_id].id,
		      cmor_tables[nVarRefTblID].szTable_id,
		      "tracking_id",
		     (char *) myuuid_str );
	    cmor_handle_error( msg, CMOR_CRITICAL );
	}
        free( myuuid_str );
        uuid_destroy( myuuid );


/* -------------------------------------------------------------------- */
/*      in case we are doing a zfactor var                              */
/* -------------------------------------------------------------------- */

	cmor_vars[var_id].time_nc_id = cmor_vars[refvarid].time_nc_id;
	cmor_vars[var_id].time_bnds_nc_id = cmor_vars[refvarid].time_bnds_nc_id;
    }

/* -------------------------------------------------------------------- */
/*      here we add the number of time                                  */
/*      written for the associated variable                             */
/* -------------------------------------------------------------------- */
    if( ( refvar != NULL ) && ( cmor_vars[refvarid].grid_id > -1 )
	&& ( cmor_grids[cmor_vars[refvarid].grid_id].istimevarying == 1 ) ) {
	for( i = 0; i < 4; i++ ) {
	    if( cmor_grids[cmor_vars[refvarid].grid_id].associated_variables
		[i] == var_id ) {
		if( cmor_vars[refvarid].ntimes_written_coords[i] == -1 ) {
		    cmor_vars[refvarid].ntimes_written_coords[i] =
			ntimes_passed;
		} else {
		    cmor_vars[refvarid].ntimes_written_coords[i] +=
			ntimes_passed;
		}
	    }
	}
    }
    if( refvar != NULL ) {
	for( i = 0; i < 10; i++ ) {
	    if( cmor_vars[*refvar].associated_ids[i] == var_id ) {
		if( cmor_vars[*refvar].ntimes_written_associated[i] == 0 ) {
		    cmor_vars[*refvar].ntimes_written_associated[i] =
			ntimes_passed;
		} else {
		    cmor_vars[*refvar].ntimes_written_associated[i] +=
			ntimes_passed;
		}
	    }
	}
    }
    cmor_write_var_to_file( ncid, &cmor_vars[var_id], data, type,
			    ntimes_passed, time_vals, time_bounds );
    cmor_pop_traceback(  );
    return( 0 );
};
/************************************************************************/
/*                 cmor_create_var_attributes()                         */
/************************************************************************/
void cmor_create_var_attributes(int var_id, int ncid, int ncafid,
        int *nc_vars, int *nc_bnds_vars,
        int *nc_vars_af, int *nc_associated_vars,
        int *nc_singletons, int *nc_singletons_bnds,
        int *nc_zfactors, int *zfactors,    int nzfactors,
        size_t *nc_dim_chunking, char *outname) {

    size_t starts[2], counts[2];
    int i, j, k, l,  ho;
    int ierr;
    char msg[CMOR_MAX_STRING];
    int nVarRefTblID = cmor_vars[var_id].ref_table_id;
    int nelts;
    int *int_list = NULL;
    int ics, icd, icdl;
    cmor_add_traceback("cmor_create_var_attributes");
/* -------------------------------------------------------------------- */
/*      Creates attributes related to that variable                     */
/* -------------------------------------------------------------------- */
    for (j = 0; j < cmor_vars[var_id].nattributes; j++) {
/* -------------------------------------------------------------------- */
/*      first of all we need to make sure it is not an empty attribute  */
/* -------------------------------------------------------------------- */
        if (cmor_has_variable_attribute(var_id, cmor_vars[var_id].attributes[j])
                != 0) {
/* -------------------------------------------------------------------- */
/*      deleted attribute continue on                                   */
/* -------------------------------------------------------------------- */
            continue;
        }
        if (strcmp(cmor_vars[var_id].attributes[j], "flag_values") == 0) {
/* -------------------------------------------------------------------- */
/*      ok we need to convert the string to a list of int               */
/* -------------------------------------------------------------------- */

            ierr = cmor_convert_string_to_list(
                    cmor_vars[var_id].attributes_values_char[j], 'i',
                    (void *) &int_list, &nelts);
            ierr = nc_put_att_int(ncid, cmor_vars[var_id].nc_var_id,
                    "flag_values", NC_INT, nelts, int_list);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) setting flags numerical\n! "
                                "attribute on variable %s (table: %s)", ierr,
                        nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }
            free(int_list);

        } else if (cmor_vars[var_id].attributes_type[j] == 'c') {
            ierr = cmor_put_nc_char_attribute(ncid,
                    cmor_vars[var_id].nc_var_id,
                    cmor_vars[var_id].attributes[j],
                    cmor_vars[var_id].attributes_values_char[j],
                    cmor_vars[var_id].id);
        } else {
            ierr = cmor_put_nc_num_attribute(ncid,
                    cmor_vars[var_id].nc_var_id,
                    cmor_vars[var_id].attributes[j],
                    cmor_vars[var_id].attributes_type[j],
                    cmor_vars[var_id].attributes_values_num[j],
                    cmor_vars[var_id].id);
        }
    }

    if ((CMOR_NETCDF_MODE != CMOR_REPLACE_3)
            && (CMOR_NETCDF_MODE != CMOR_PRESERVE_3)
            && (CMOR_NETCDF_MODE != CMOR_APPEND_3)) {
/* -------------------------------------------------------------------- */
/*      Compression stuff                                               */
/* -------------------------------------------------------------------- */
        ics = cmor_tables[nVarRefTblID].vars[nVarRefTblID].shuffle;
        icd = cmor_tables[nVarRefTblID].vars[nVarRefTblID].deflate;
        icdl = cmor_tables[nVarRefTblID].vars[nVarRefTblID].deflate_level;
        ierr = nc_def_var_deflate(ncid, cmor_vars[var_id].nc_var_id, ics, icd,
                icdl);

        if (ierr != NC_NOERR) {
            snprintf(msg, CMOR_MAX_STRING,
                    "NetCDF Error (%i: %s) defining compression\n! "
                            "parameters for variable '%s' (table: %s)", ierr,
                    nc_strerror(ierr), cmor_vars[var_id].id,
                    cmor_tables[nVarRefTblID].szTable_id);
            cmor_handle_error(msg, CMOR_CRITICAL);
        }
/* -------------------------------------------------------------------- */
/*      Chunking stuff                                                  */
        /* -------------------------------------------------------------------- */

#ifndef NC_CHUNKED
#define NC_CHUNKED 0
#endif
        if (!((cmor_vars[var_id].grid_id > -1)
                && (cmor_grids[cmor_vars[var_id].grid_id].istimevarying == 1))) {
            ierr = nc_def_var_chunking(ncid, cmor_vars[var_id].nc_var_id,
                    NC_CHUNKED, &nc_dim_chunking[0]);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) defining chunking\n! "
                                "parameters for variable '%s' (table: %s)",
                        ierr, nc_strerror(ierr), cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }
        }
    }


/* -------------------------------------------------------------------- */
/*      Done with NetCDF file definitions                               */
/* -------------------------------------------------------------------- */

    ierr = nc_enddef(ncid);
    if (ierr != NC_NOERR && ierr != NC_ENOTINDEFINE) {
        snprintf(msg, CMOR_MAX_STRING,
                "NetCDF Error (%i: %s) leaving definition mode for file %s",
                ierr, nc_strerror(ierr), outname);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }
    ierr = nc_enddef(ncafid);
    if (ierr != NC_NOERR && ierr != NC_ENOTINDEFINE) {
        snprintf(msg, CMOR_MAX_STRING,
                "NetCDF Error (%i: %s) leaving definition mode for metafile %s",
                ierr, nc_strerror(ierr),
                cmor_current_dataset.associated_file_name);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

/* -------------------------------------------------------------------- */
/*      Write non time dimension of variable into the NetCDF file       */
/* -------------------------------------------------------------------- */
    l = 1;
    if (cmor_axes[cmor_vars[var_id].axes_ids[0]].axis != 'T') {
        l = 0;
    }
    for (i = l; i < cmor_vars[var_id].ndims; i++) {
/* -------------------------------------------------------------------- */
/*      at this point we need to check if the values of the             */
/*      axis need to be replaced (hybrid coords                         */
/*      we only need to do this if ho != hi                             */
/* -------------------------------------------------------------------- */
        if (cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_out
                != cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_in) {
            ho = 0;
            if (cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_out != 0) {
                ho = cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_out;
            } else if (cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_in
                    != 0) {
                ho = cmor_axes[cmor_vars[var_id].axes_ids[i]].hybrid_in;
            }
            if (ho != 0) {
/* -------------------------------------------------------------------- */
/*      yep need to change them                                         */
/* -------------------------------------------------------------------- */
/* -------------------------------------------------------------------- */
/*      std hyb sigma                                                   */
/* -------------------------------------------------------------------- */
                if (ho == 1) {
/* -------------------------------------------------------------------- */
/*      Look for a coeff                                                */
/* -------------------------------------------------------------------- */
                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "a") == 0)
                                && (cmor_vars[j].zaxis
                                        == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "could not find 'a' coeff for axis: %s,\n! "
                                        "for variable %s (table: %s)",
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                    for (j = 0;
                            j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                            j++)
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] =
                                cmor_vars[k].values[j];
/* -------------------------------------------------------------------- */
/*      look for b coeff                                                */
/* -------------------------------------------------------------------- */

                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "b") == 0)
                                && (cmor_vars[j].zaxis
                                        == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "could find 'b' coeff for axis: %s,\n! "
                                        "for variable %s (table: %s)",
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                    for (j = 0;
                            j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                            j++) {
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] +=
                                cmor_vars[k].values[j];
                    }
/* -------------------------------------------------------------------- */
/*      do we have bounds to treat as well?                             */
/* -------------------------------------------------------------------- */

                    if (cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds != NULL) {
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "a_bnds") == 0)
                                    && (cmor_vars[j].zaxis
                                            == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            snprintf(msg, CMOR_MAX_STRING,
                                    "could not find 'a_bnds' coeff for\n! "
                                            "axis: %s, for variable %s (table: %s)",
                                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                    cmor_vars[var_id].id,
                                    cmor_tables[nVarRefTblID].szTable_id);
                            cmor_handle_error(msg, CMOR_CRITICAL);
                        }
                        for (j = 0;
                                j
                                        < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                                                * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds[j] =
                                    cmor_vars[k].values[j];
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "b_bnds") == 0)
                                    && (cmor_vars[j].zaxis
                                            == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            snprintf(msg, CMOR_MAX_STRING,
                                    "could find 'b_bnds' coef for axis:\n! "
                                            " %s, for variable %s (table: %s)",
                                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                    cmor_vars[var_id].id,
                                    cmor_tables[nVarRefTblID].szTable_id);
                            cmor_handle_error(msg, CMOR_CRITICAL);
                        }
                        for (j = 0;
                                j
                                        < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                                                * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds[j] +=
                                    cmor_vars[k].values[j];
                    }
                    cmor_flip_hybrid(var_id, i, "a", "b", "a_bnds", "b_bnds");
/* -------------------------------------------------------------------- */
/*      alternate hyb sigma                                             */
/* -------------------------------------------------------------------- */
                } else if (ho == 2) {
/* -------------------------------------------------------------------- */
/*      look for ap coeff                                               */
/* -------------------------------------------------------------------- */
                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "ap") == 0)
                                && (cmor_vars[j].zaxis
                                        == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "could not find 'ap' coeff for axis:\n! "
                                        "%s, for variable %s (table: %s)",
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                    for (j = 0;
                            j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                            j++)
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] =
                                cmor_vars[k].values[j] / cmor_vars[l].values[0];
/* -------------------------------------------------------------------- */
/*      look for b coeff                                                */
/* -------------------------------------------------------------------- */
                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "b") == 0)
                                && (cmor_vars[j].zaxis
                                        == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "could find 'b' coef for axis: %s,\n! "
                                        "for variable %s (table: %s)",
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                    for (j = 0;
                            j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                            j++)
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] +=
                                cmor_vars[k].values[j];

/* -------------------------------------------------------------------- */
/*      deals with bounds                                               */
/* -------------------------------------------------------------------- */

                    if (cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds != NULL) {
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "ap_bnds") == 0)
                                    && (cmor_vars[j].zaxis
                                            == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            snprintf(msg, CMOR_MAX_STRING,
                                    "could not find 'ap_bnds' coeff for\n! "
                                            "axis: %s, for variable %s\n! "
                                            "(table: %s)",
                                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                    cmor_vars[var_id].id,
                                    cmor_tables[nVarRefTblID].szTable_id);
                            cmor_handle_error(msg, CMOR_CRITICAL);
                        }
                        for (j = 0;
                                j
                                        < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                                                * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds[j] =
                                    cmor_vars[k].values[j]
                                            / cmor_vars[l].values[0];
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "b_bnds") == 0)
                                    && (cmor_vars[j].zaxis
                                            == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            snprintf(msg, CMOR_MAX_STRING,
                                    "could find 'b_bnds' coef for axis:\n! "
                                            "%s, for variable %s (table: %s)",
                                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                    cmor_vars[var_id].id,
                                    cmor_tables[nVarRefTblID].szTable_id);
                            cmor_handle_error(msg, CMOR_CRITICAL);
                        }
                        for (j = 0;
                                j
                                        < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                                                * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds[j] +=
                                    cmor_vars[k].values[j];
                    }
                    cmor_flip_hybrid(var_id, i, "ap", "b", "ap_bnds", "b_bnds");
/* -------------------------------------------------------------------- */
/*      sigma                                                           */
/* -------------------------------------------------------------------- */
                } else if (ho == 3) {
                    k = -1;
                    for (j = 0; j <= cmor_nvars; j++)
                        if ((strcmp(cmor_vars[j].id, "sigma") == 0)
                                && (cmor_vars[j].zaxis
                                        == cmor_vars[var_id].axes_ids[i])) {
                            k = j;
                            break;
                        }
                    if (k == -1) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "could not find 'sigma' coeff for axis:\n! "
                                        "%s, for variable %s (table: %s)",
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                    for (j = 0;
                            j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                            j++)
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values[j] =
                                cmor_vars[k].values[j];
/* -------------------------------------------------------------------- */
/*      deals with bounds                                               */
/* -------------------------------------------------------------------- */

                    if (cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds != NULL) {
                        k = -1;
                        for (j = 0; j <= cmor_nvars; j++)
                            if ((strcmp(cmor_vars[j].id, "sigma_bnds") == 0)
                                    && (cmor_vars[j].zaxis
                                            == cmor_vars[var_id].axes_ids[i])) {
                                k = j;
                                break;
                            }
                        if (k == -1) {
                            snprintf(msg, CMOR_MAX_STRING,
                                    "could not find 'sigma_bnds' coeff\n! "
                                            "for axis: %s, for variable %s (table: %s)",
                                    cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                    cmor_vars[var_id].id,
                                    cmor_tables[nVarRefTblID].szTable_id);
                            cmor_handle_error(msg, CMOR_CRITICAL);
                        }
                        for (j = 0;
                                j
                                        < cmor_axes[cmor_vars[var_id].axes_ids[i]].length
                                                * 2; j++)
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds[j] =
                                    cmor_vars[k].values[j];
                    }
                }
                cmor_flip_hybrid(var_id, i, "sigma", NULL, "sigma_bnds", NULL);
            }
        }
        if (cmor_axes[cmor_vars[var_id].axes_ids[i]].cvalues == NULL) {
            if (cmor_axes[cmor_vars[var_id].axes_ids[i]].store_in_netcdf == 1) {
                ierr = nc_put_var_double(ncid, nc_vars[i],
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].values);
                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF Error (%i: %s) writing axis '%s'\n! "
                                    "values for variable %s (table: %s)", ierr,
                            nc_strerror(ierr),
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
                if (ncid != ncafid) {
                    ierr = nc_put_var_double(ncafid, nc_vars_af[i],
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].values);
                    if (ierr != NC_NOERR) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "NetCDF Error (%i: %s) writing axis '%s'\n! "
                                        "values to metafile, for variable %s "
                                        "(table: %s)", ierr, nc_strerror(ierr),
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                }
            }
        } else {
            for (j = 0; j < cmor_axes[cmor_vars[var_id].axes_ids[i]].length;
                    j++) {
                starts[0] = j;
                starts[1] = 0;
                counts[0] = 1;
                counts[1] = strlen(
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].cvalues[j]);
                ierr = nc_put_vara_text(ncid, nc_vars[i], starts, counts,
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].cvalues[j]);
                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF Error (%i: %s) writing axis '%s'\n! "
                                    "value number %d (%s), for variable %s "
                                    "(table: %s)", ierr, nc_strerror(ierr),
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].id, j,
                            cmor_axes[cmor_vars[var_id].axes_ids[i]].cvalues[j],
                            cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
                if (ncid != ncafid) {
                    ierr =
                            nc_put_vara_text(ncafid, nc_vars_af[i], starts,
                                    counts,
                                    cmor_axes[cmor_vars[var_id].axes_ids[i]].cvalues[j]);
                    if (ierr != NC_NOERR) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "NetCDF Error (%i: %s) writing axis '%s'\n! "
                                        "values to metafile, for variable %s\n! "
                                        "(table: %s)", ierr, nc_strerror(ierr),
                                cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                                cmor_vars[var_id].id,
                                cmor_tables[nVarRefTblID].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    }
                }
            }
        }
/* -------------------------------------------------------------------- */
/*      ok do we have bounds on this axis?                              */
/* -------------------------------------------------------------------- */
        if (cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds != NULL) {
            ierr = nc_put_var_double(ncafid, nc_bnds_vars[i],
                    cmor_axes[cmor_vars[var_id].axes_ids[i]].bounds);
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NC error (%i: %s) on variable %s writing\n! "
                                "bounds for dim %i (%s), for variable %s\n! "
                                "(table: %s)", ierr, nc_strerror(ierr),
                        cmor_vars[var_id].id, i,
                        cmor_axes[cmor_vars[var_id].axes_ids[i]].id,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      ok now need to write grid variables                             */
/* -------------------------------------------------------------------- */
    if (cmor_vars[var_id].grid_id > -1) {
        if (cmor_grids[cmor_vars[var_id].grid_id].istimevarying == 0) {
            for (i = 0; i < 4; i++) {
                j =
                        cmor_grids[cmor_vars[var_id].grid_id].associated_variables[i];
                if (j != -1) { /* we need to write this variable */
                    cmor_vars[j].nc_var_id = nc_associated_vars[i];
                    switch (i) {
                    case (0):
                        cmor_write_var_to_file(ncafid, &cmor_vars[j],
                                cmor_grids[cmor_vars[var_id].grid_id].lats, 'd',
                                0, NULL, NULL);
                        break;
                    case (1):
                        cmor_write_var_to_file(ncafid, &cmor_vars[j],
                                cmor_grids[cmor_vars[var_id].grid_id].lons, 'd',
                                0, NULL, NULL);
                        break;
                    case (2):
                        cmor_write_var_to_file(ncafid, &cmor_vars[j],
                                cmor_grids[cmor_vars[var_id].grid_id].blats,
                                'd', 0, NULL, NULL);
                        break;
                    case (3):
                        cmor_write_var_to_file(ncafid, &cmor_vars[j],
                                cmor_grids[cmor_vars[var_id].grid_id].blons,
                                'd', 0, NULL, NULL);
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      ok now write the zfactor values if necessary                    */
/* -------------------------------------------------------------------- */

    for (i = 0; i < nzfactors; i++) {
/* -------------------------------------------------------------------- */
/*      ok this one has value defined we need to store it               */
/* -------------------------------------------------------------------- */
        if (cmor_vars[zfactors[i]].values != NULL) {
            cmor_vars[zfactors[i]].nc_var_id = nc_zfactors[i];
            cmor_write_var_to_file(ncafid, &cmor_vars[zfactors[i]],
                    cmor_vars[zfactors[i]].values, 'd', 0, NULL, NULL);
        }
    }
/* -------------------------------------------------------------------- */
/*      Write singleton dimension variables                             */
/* -------------------------------------------------------------------- */
    for (i = 0; i < CMOR_MAX_DIMENSIONS; i++) {
        int nRefAxTableID;
        int nRefAxisID;

        nRefAxTableID = cmor_axes[j].ref_table_id;
        nRefAxisID = cmor_axes[j].ref_axis_id;

        j = cmor_vars[var_id].singleton_ids[i];
        if (j != -1) {
            if (cmor_tables[nRefAxTableID].axes[nRefAxisID].type == 'c') {
                ierr = nc_put_var_text(ncid, nc_singletons[i],
                        cmor_tables[nRefAxTableID].axes[nRefAxisID].cvalue);
            } else {
                ierr = nc_put_var_double(ncid, nc_singletons[i],
                        cmor_axes[j].values);
            }
            if (ierr != NC_NOERR) {
                snprintf(msg, CMOR_MAX_STRING,
                        "NetCDF Error (%i: %s) writing scalar variable\n! "
                                "%s for variable %s (table: %s), value: %lf",
                        ierr, nc_strerror(ierr), cmor_axes[j].id,
                        cmor_vars[var_id].id,
                        cmor_tables[nVarRefTblID].szTable_id,
                        cmor_axes[j].values[0]);
                cmor_handle_error(msg, CMOR_CRITICAL);
            }
/* -------------------------------------------------------------------- */
/*      now see if we need bounds                                       */
/* -------------------------------------------------------------------- */
            if (cmor_axes[j].bounds != NULL) {
                ierr = nc_put_var_double(ncid, nc_singletons_bnds[i],
                        cmor_axes[j].bounds);
                if (ierr != NC_NOERR) {
                    snprintf(msg, CMOR_MAX_STRING,
                            "NetCDF Error (%i: %s) writing scalar bounds\n! "
                                    "variable %s for variable %s (table: %s),\n! "
                                    "values: %lf, %lf", ierr, nc_strerror(ierr),
                            cmor_axes[j].id, cmor_vars[var_id].id,
                            cmor_tables[nVarRefTblID].szTable_id,
                            cmor_axes[j].bounds[0], cmor_axes[j].bounds[1]);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
            }
        }
    }
    cmor_current_dataset.associate_file = ncafid;
    cmor_pop_traceback();
}
/************************************************************************/
/*                    cmor_CreateFromTemplate()                         */
/************************************************************************/
int cmor_CreateFromTemplate(int var_id, char *template,
                            char *szJoin, char *separator ){
    char *szToken;
    char tmp[CMOR_MAX_STRING];
    char path_template[CMOR_MAX_STRING];
    cmor_table_t *pTable;
    cmor_var_t *pVariable;

    pTable = &cmor_tables[cmor_vars[var_id].ref_table_id];
    pVariable = &cmor_vars[var_id];

    cmor_add_traceback( "cmor_CreateFromTemplate" );
    cmor_is_setup();

    strcpy(path_template, template);

/* -------------------------------------------------------------------- */
/*    Get rid of <> characters from template and add "information"      */
/*    to path                                                           */
/* -------------------------------------------------------------------- */
    szToken = strtok(path_template, GLOBAL_SEPARATORS);
    int optional = 0;
    while(szToken) {

/* -------------------------------------------------------------------- */
/*      Is this an optional token.                                      */
/* -------------------------------------------------------------------- */
        if(strncmp(szToken, GLOBAL_OPENOPTIONAL, 1) == 0 ) {
            optional = 1;

        } else if(strncmp(szToken, GLOBAL_CLOSEOPTIONAL, 1) == 0 ) {
            optional = 0;
/* -------------------------------------------------------------------- */
/*      This token must be a global attribute, a table header attribute */
/*      or an internal attribute.  Otherwise we just skip it and the    */
/*      user get a warning.                                             */
/* -------------------------------------------------------------------- */
        } else if( cmor_has_cur_dataset_attribute( szToken ) == 0 ) {
            cmor_get_cur_dataset_attribute( szToken, tmp );
            strncat(szJoin, tmp, CMOR_MAX_STRING);
            strcat(szJoin, separator);

        } else if (cmor_get_table_attr(szToken, pTable, tmp ) == 0 ){
            strncat(szJoin, tmp, CMOR_MAX_STRING);
            strcat(szJoin, separator);

        } else if( strcmp(szToken, OUTPUT_TEMPLATE_RIPF) == 0) {
            cmor_addRIPF(szJoin);
            strcat(szJoin, separator);

        } else if(strcmp(szToken, GLOBAL_ATT_VARIABLE_ID) == 0) {
            strncat(szJoin, pVariable->id, CMOR_MAX_STRING);
            strcat(szJoin, separator);

        } else {
            char szInternalAtt[CMOR_MAX_STRING];
            strcpy(szInternalAtt, GLOBAL_INTERNAL);
            strncat(szInternalAtt, szToken, strlen(szToken));
            if( cmor_has_cur_dataset_attribute( szInternalAtt ) == 0 ) {
                cmor_get_cur_dataset_attribute( szInternalAtt, tmp );
                if( !optional) {
                    strncat(szJoin,tmp,CMOR_MAX_STRING);
                    strcat(szJoin, separator );
                }
                else {

/* -------------------------------------------------------------------- */
/*      Skip "no-driver for filename if optional is set to 1            */
/* -------------------------------------------------------------------- */
                    if(strcmp(tmp, GLOBAL_ATT_VAL_NODRIVER)!=0) {
                        strncat(szJoin,tmp,CMOR_MAX_STRING);
                        strcat(szJoin, separator );
                    }

                }
            }
        }

        szToken = strtok(NULL, "><");
    }

    cmor_pop_traceback(  );
    return (0);
}
/************************************************************************/
/*                          cmor_addVersion()                           */
/************************************************************************/
int cmor_addVersion(){
    time_t t;
    struct tm* tm;
    char szVersion[CMOR_MAX_STRING];;
    char szDate[CMOR_MAX_STRING];

    cmor_add_traceback( "cmor_addVersion" );
    cmor_is_setup();

    time(&t);
    tm = localtime(&t);
    strcpy(szVersion,"v");
    strftime(szDate, CMOR_MAX_STRING, "%Y%m%d", tm);
    strcat(szVersion, szDate);

    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_VERSION, szVersion, 1);
    cmor_pop_traceback();
    return(0);
}
/************************************************************************/
/*                          cmor_addRIPF()                              */
/************************************************************************/
int cmor_addRIPF(char *variant) {
    char tmp[CMOR_MAX_STRING];
    int initialization_index;
    int physics_index;
    int forcing_index;

    cmor_add_traceback( "cmor_addRipf" );
    cmor_is_setup();

/* -------------------------------------------------------------------- */
/*      realization                                                     */
/* -------------------------------------------------------------------- */
    snprintf(tmp, CMOR_MAX_STRING, "r%d", cmor_current_dataset.realization);
    strncat(variant, tmp, CMOR_MAX_STRING - strlen(variant));

/* -------------------------------------------------------------------- */
/*      initialization id (required)                                    */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_INITIA_IDX) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_INITIA_IDX, tmp);
        sscanf(tmp, "%i", &initialization_index);
        snprintf(tmp, CMOR_MAX_STRING, "i%d", initialization_index);
        strncat(variant, tmp, CMOR_MAX_STRING - strlen(variant));
    }

/* -------------------------------------------------------------------- */
/*      physics id (required)                                           */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_PHYSICS_IDX) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_PHYSICS_IDX, tmp);
        sscanf(tmp, "%i", &physics_index);
        snprintf(tmp, CMOR_MAX_STRING, "p%d", physics_index);
        strncat(variant, tmp, CMOR_MAX_STRING - strlen(variant));
    }
/* -------------------------------------------------------------------- */
/*      forcing id (required)                                           */
/* -------------------------------------------------------------------- */
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_FORCING_IDX) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_FORCING_IDX, tmp);
        sscanf(tmp, "%i", &forcing_index);
        snprintf(tmp, CMOR_MAX_STRING, "f%d", forcing_index);
        strncat(variant, tmp, CMOR_MAX_STRING - strlen(variant));
    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_VARIANT_ID, variant, 1);
    cmor_pop_traceback();
    return(0);

}

/************************************************************************/
/*                        cmor_close_variable()                         */
/************************************************************************/
int cmor_close_variable( int var_id, char *file_name, int *preserve ) {
    int ierr;
    extern int cmor_nvars;
    char outname[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char msg2[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    char ctmp2[CMOR_MAX_STRING];
    cdCalenType icalo;
    cdCompTime comptime;
    int i, j, n;
    double interval;
    struct stat buf;
    off_t sz;
    long maxsz = ( long ) pow( 2, 32 ) - 1;

    cmor_add_traceback( "cmor_close_variable" );
    cmor_is_setup(  );

    cleanup_varid = var_id;

    if( cmor_vars[var_id].initialized != -1 ) {
	ierr = nc_close( cmor_vars[var_id].initialized );

	if( ierr != NC_NOERR ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "NetCDF Error (%i: %s) closing variable %s (table: %s)\n! ",
		      ierr, nc_strerror( ierr ), cmor_vars[var_id].id,
		      cmor_tables[cmor_vars[var_id].
				  ref_table_id].szTable_id );
	    cmor_handle_error( msg, CMOR_CRITICAL );
	}


/* -------------------------------------------------------------------- */
/*      Ok we need to make the associated variables                     */
/*      have been written in the case of a time varying grid            */
/* -------------------------------------------------------------------- */

	if( ( cmor_vars[var_id].grid_id > -1 )
	    && ( cmor_grids[cmor_vars[var_id].grid_id].istimevarying ==
		 1 ) ) {
	    for( i = 0; i < 4; i++ ) {
		if( cmor_grids[cmor_vars[var_id].grid_id].associated_variables[i] != -1 ) {
/* -------------------------------------------------------------------- */
/*      ok this associated coord should be stored                       */
/* -------------------------------------------------------------------- */
		    if( cmor_vars[var_id].ntimes_written !=
			cmor_vars[var_id].ntimes_written_coords[i] ) {
/* -------------------------------------------------------------------- */
/*      ok we either wrote more or less data but                        */
/*      in any case not the right amount!                               */
/* -------------------------------------------------------------------- */

			if( cmor_vars[var_id].ntimes_written == 0 ) {
			    for( j = 0; j < cmor_vars[var_id].ndims; j++ ) {
				if( cmor_axes
				    [cmor_vars[var_id].axes_ids[j]].axis ==
				    'T' ) {
				    sprintf( ctmp2, "%i",
					     cmor_axes[cmor_vars
						       [var_id].axes_ids
						       [j]].length );
				    break;
				}
			    }
			} else {
			    sprintf( ctmp2, "%i",
				     cmor_vars[var_id].ntimes_written );
			}
			if( cmor_vars[var_id].ntimes_written_coords[i] == -1 ) {
			    sprintf( ctmp, "no" );
			} else {
			    sprintf( ctmp, "%i",
				     cmor_vars
				     [var_id].ntimes_written_coords[i] );
			}
			snprintf( msg, CMOR_MAX_STRING,
				  "while closing variable %i (%s, table %s)\n! "
				  "we noticed it has a time varying grid, \n! "
			          "you wrote %s time steps for the variable,\n! "
			          "but its associated variable %i (%s) has\n! "
			          "%s times written",
				  cmor_vars[var_id].self,
				  cmor_vars[var_id].id,
				  cmor_tables[cmor_vars[var_id].ref_table_id].
				  szTable_id, ctmp2,
				  cmor_vars[cmor_grids
					    [cmor_vars[var_id].
					     grid_id].associated_variables
					    [i]].self,
				  cmor_vars[cmor_grids
					    [cmor_vars[var_id].
					     grid_id].associated_variables
					    [i]].id, ctmp );
			cmor_handle_error( msg, CMOR_CRITICAL );
		    }
		}
	    }
	}
	for( i = 0; i < 10; i++ ) {
	    if( cmor_vars[var_id].associated_ids[i] != -1 ) {
		if( cmor_vars[var_id].ntimes_written !=
		    cmor_vars[var_id].ntimes_written_associated[i] ) {
		    sprintf( ctmp2, "%i",
			     cmor_vars[var_id].ntimes_written );
		    sprintf( ctmp, "%i",
			     cmor_vars[var_id].ntimes_written_associated
			     [i] );
		    snprintf( msg, CMOR_MAX_STRING,
			      "while closing variable %i (%s, table %s) we\n! "
		              "noticed it has a time varying associated\n! "
		              "variable, you wrote %s time steps for the\n! "
		              "variable, but its associated variable %i (%s)\n! "
		              "has %s times written",
			      cmor_vars[var_id].self, cmor_vars[var_id].id,
			      cmor_tables[cmor_vars[var_id].
					  ref_table_id].szTable_id, ctmp2,
			      cmor_vars[cmor_vars[var_id].associated_ids
					[i]].self,
			      cmor_vars[cmor_vars[var_id].associated_ids
					[i]].id, ctmp );
		    cmor_handle_error( msg, CMOR_CRITICAL );
		}
	    }
	}
/* -------------------------------------------------------------------- */
/*      ok at that point we need to construct the final name!           */
/* -------------------------------------------------------------------- */

	strncpytrim( outname, cmor_vars[var_id].base_path,
		     CMOR_MAX_STRING );

	if( cmor_tables
	    [cmor_axes[cmor_vars[var_id].axes_ids[0]].
	     ref_table_id].axes[cmor_axes[cmor_vars[var_id].axes_ids[0]].
				ref_axis_id].axis == 'T' ) {
	    cmor_get_axis_attribute( cmor_vars[var_id].axes_ids[0],
				     "units", 'c', &msg );
	    cmor_get_cur_dataset_attribute( "calendar", msg2 );

	    if( cmor_calendar_c2i( msg2, &icalo ) != 0 ) {
		snprintf( msg, CMOR_MAX_STRING,
			  "Cannot convert times for calendar: %s,\n! "
		          "closing variable %s (table: %s)",
			  msg2, cmor_vars[var_id].id,
			  cmor_tables[cmor_vars[var_id].
				      ref_table_id].szTable_id );
		cmor_handle_error( msg, CMOR_CRITICAL );
		cmor_pop_traceback(  );
		return( 1 );
	    }
/* -------------------------------------------------------------------- */
/*      ok makes a comptime out of input                                */
/* -------------------------------------------------------------------- */

	    i = cmor_vars[var_id].axes_ids[0];
	    j = cmor_axes[i].ref_table_id;
	    i = cmor_axes[i].ref_axis_id;
	    if( ( cmor_tables[j].axes[i].climatology == 1 )
		&& ( cmor_vars[var_id].first_bound != 1.e20 ) ) {
		cdRel2Comp( icalo, msg, cmor_vars[var_id].first_bound,
			    &comptime );
	    } else {
		cdRel2Comp( icalo, msg, cmor_vars[var_id].first_time,
			    &comptime );
	    }
/* -------------------------------------------------------------------- */
/*      need to figure out the approximate interval                     */
/* -------------------------------------------------------------------- */
	    int nVarAxisID;
	    int nVarRefTable;
	    int nVarRefAxisID;

	    nVarAxisID = cmor_vars[var_id].axes_ids[0];
	    nVarRefTable = cmor_axes[nVarAxisID].ref_table_id;
	    nVarRefAxisID = cmor_axes[nVarAxisID].ref_axis_id;

	    interval = cmor_convert_interval_to_seconds(
	            cmor_tables[nVarRefTable].interval,
	            cmor_tables[nVarRefTable].axes[nVarRefAxisID].units );

/* -------------------------------------------------------------------- */
/*      first time point                                                */
/* -------------------------------------------------------------------- */

	   // strncat( outname, "_", CMOR_MAX_STRING - strlen( outname ) );
	    snprintf( msg2, CMOR_MAX_STRING, "%.4ld", comptime.year );
	    strncat( outname, msg2, CMOR_MAX_STRING - strlen( outname ) );
/* -------------------------------------------------------------------- */
/*      less than a year                                                */
/* -------------------------------------------------------------------- */

	    if( interval < 29.E6 ) {
		snprintf( msg2, CMOR_MAX_STRING, "%.2i", comptime.month );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
/* -------------------------------------------------------------------- */
/*      less than a month                                               */
/* -------------------------------------------------------------------- */
	    if( interval < 2.E6 ) {
		snprintf( msg2, CMOR_MAX_STRING, "%.2i", comptime.day );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
/* -------------------------------------------------------------------- */
/*      less than a day                                                 */
/* -------------------------------------------------------------------- */
	    if( interval < 86000 ) {
		snprintf( msg2, CMOR_MAX_STRING, "%.2i",
			  ( int ) comptime.hour );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
/* -------------------------------------------------------------------- */
/*      less than 6hr                                                   */
/* -------------------------------------------------------------------- */
	    if( interval < 21000 ) {
/* -------------------------------------------------------------------- */
/*      from now on add 1 more level of precision since that frequency  */
/* -------------------------------------------------------------------- */

		ierr =
		    ( int ) ( ( comptime.hour -
				( int ) ( comptime.hour ) ) * 60. );
		snprintf( msg2, CMOR_MAX_STRING, "%.2i", ierr );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
	    if( interval < 3000 ) {	/* less than an hour */
		snprintf( msg2, CMOR_MAX_STRING, "%.2i",
			  ( int ) ( ( comptime.hour -
				      ( int ) ( comptime.hour ) ) *
				    3600. ) - ierr * 60 );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }

/* -------------------------------------------------------------------- */
/*      separator between first and last time                           */
/* -------------------------------------------------------------------- */

	    strncat( outname, "-", CMOR_MAX_STRING - strlen( outname ) );

	    if( ( cmor_tables[j].axes[i].climatology == 1 )
		&& ( cmor_vars[var_id].last_bound != 1.e20 ) ) {
		cdRel2Comp( icalo, msg, cmor_vars[var_id].last_bound,
			    &comptime );
/* -------------------------------------------------------------------- */
/*      ok apparently we don't like the new time format                 */
/*      if it's ending at midnight exactly so I'm removing              */
/*      one second...                                                   */
/* -------------------------------------------------------------------- */

		if( icalo == cdMixed ) {
		    cdCompAddMixed( comptime, -1. / 3600., &comptime );
		} else {
		    cdCompAdd( comptime, -1. / 3600., icalo, &comptime );
		}
	    } else {
		cdRel2Comp( icalo, msg, cmor_vars[var_id].last_time,
			    &comptime );
	    }

/* -------------------------------------------------------------------- */
/*      last time point                                                 */
/* -------------------------------------------------------------------- */

	    snprintf( msg2, CMOR_MAX_STRING, "%.4ld", comptime.year );
	    strncat( outname, msg2, CMOR_MAX_STRING - strlen( outname ) );
/* -------------------------------------------------------------------- */
/*      less than a year                                                */
/* -------------------------------------------------------------------- */

	    if( interval < 29.E6 ) {
		snprintf( msg2, CMOR_MAX_STRING, "%.2i", comptime.month );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
/* -------------------------------------------------------------------- */
/*      less than a month                                               */
/* -------------------------------------------------------------------- */

	    if( interval < 2.E6 ) {
		snprintf( msg2, CMOR_MAX_STRING, "%.2i", comptime.day );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
/* -------------------------------------------------------------------- */
/*      less than a day                                                 */
/* -------------------------------------------------------------------- */

	    if( interval < 86000 ) {
		snprintf( msg2, CMOR_MAX_STRING, "%.2i",
			  ( int ) comptime.hour );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
/* -------------------------------------------------------------------- */
/*      less than 6hr                                                   */
/* -------------------------------------------------------------------- */

	    if( interval < 21000 ) {
/* -------------------------------------------------------------------- */
/*      from now on add 1 more level of precision since that frequency  */
/* -------------------------------------------------------------------- */

		ierr =
		    ( int ) ( ( comptime.hour -
				( int ) ( comptime.hour ) ) * 60. );
		snprintf( msg2, CMOR_MAX_STRING, "%.2i", ierr );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
/* -------------------------------------------------------------------- */
/*      less than an hour                                               */
/* -------------------------------------------------------------------- */
	    if( interval < 3000 ) {
		snprintf( msg2, CMOR_MAX_STRING, "%.2i",
			  ( int ) ( ( comptime.hour -
				      ( int ) ( comptime.hour ) ) *
				    3600. ) - ierr * 60 );
		strncat( outname, msg2,
			 CMOR_MAX_STRING - strlen( outname ) );
	    }

	    if( cmor_tables
		[cmor_axes[cmor_vars[var_id].axes_ids[0]].
		 ref_table_id].axes[cmor_axes[cmor_vars[var_id].
					      axes_ids[0]].
				    ref_axis_id].climatology == 1 ) {
		strncat( outname, "-clim",
			 CMOR_MAX_STRING - strlen( outname ) );
	    }
	}

	if( cmor_vars[var_id].suffix_has_date == 1 ) {
/* -------------------------------------------------------------------- */
/*      all right we need to pop out the date part....                  */
/* -------------------------------------------------------------------- */

	    n = strlen( cmor_vars[var_id].suffix );
	    i = 0;
	    while( cmor_vars[var_id].suffix[i] != '_' )
		i++;
	    i++;
	    while( ( cmor_vars[var_id].suffix[i] != '_' ) && i < n )
		i++;
/* -------------------------------------------------------------------- */
/*      ok now we have the length of dates                              */
/*      at this point we are either at the                              */
/*      _clim the actual _suffix or the end (==nosuffix)                */
/*      checking if _clim needs to be added                             */
/* -------------------------------------------------------------------- */
	    if( cmor_tables
		[cmor_axes[cmor_vars[var_id].axes_ids[i]].
		 ref_table_id].axes[cmor_axes[cmor_vars[var_id].
					      axes_ids[0]].
				    ref_axis_id].climatology == 1 ) {
		i += 5;
	    }
	    strcpy( msg, "" );
	    for( j = i; j < n; j++ ) {
		msg[j - i] = cmor_vars[var_id].suffix[i];
		msg[j - i + 1] = '\0';
	    }
	} else {
	    strncpy( msg, cmor_vars[var_id].suffix, CMOR_MAX_STRING );
	}

	if( strlen( msg ) > 0 ) {
	    strncat( outname, "_", CMOR_MAX_STRING - strlen( outname ) );
	    strncat( outname, msg, CMOR_MAX_STRING - strlen( outname ) );
	}

	strncat( outname, ".nc", CMOR_MAX_STRING - strlen( outname ) );


/* -------------------------------------------------------------------- */
/*      ok now we can actually move the file                            */
/*      here we need to make sure we are not in preserve mode!          */
/* -------------------------------------------------------------------- */
	if( ( CMOR_NETCDF_MODE == CMOR_PRESERVE_4 )
	    || ( CMOR_NETCDF_MODE == CMOR_PRESERVE_3 ) ) {
	    FILE *fperr;

/* -------------------------------------------------------------------- */
/*      ok first let's check if the file does exists or not             */
/* -------------------------------------------------------------------- */
	    fperr = NULL;
	    fperr = fopen( outname, "r" );
	    if( fperr != NULL ) {
		sprintf( msg, "%s.copy", outname );
		if( rename( cmor_vars[var_id].current_path, msg ) == 0 ) {
		    snprintf( msg, CMOR_MAX_STRING,
			      "Output file ( %s ) already exists,\n! "
		             "remove file or use CMOR_REPLACE or\n! "
		             "CMOR_APPEND for CMOR_NETCDF_MODE value\n! "
		             "in cmor_setup for convenience the file\n! "
		             "you were trying to write has been saved\n! "
		             "at: %s.copy",
			      outname, outname );
		} else {
		    snprintf( msg, CMOR_MAX_STRING,
			      "Output file ( %s ) already exists,\n! "
		              "remove file or use CMOR_REPLACE or\n! "
		              "CMOR_APPEND for CMOR_NETCDF_MODE value in\n! "
		              "cmor_setup.",
			      outname );
		}
		cmor_handle_error( msg, CMOR_CRITICAL );
	    }
	}
	printf("%d\n",var_id);
	printf("%s\n",cmor_vars[var_id].current_path);
	ierr = rename( cmor_vars[var_id].current_path, outname );
	printf("%d\n",ierr);
	if( ierr != 0 ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "could not rename temporary file: %s to final file\n"
	              "name: %s",
		      cmor_vars[var_id].current_path, outname );
	    // cmor_handle_error( msg, CMOR_CRITICAL );
	}
	if( file_name != NULL ) {
	    strncpy( file_name, outname, CMOR_MAX_STRING );
	}

/* -------------------------------------------------------------------- */
/*      At this point we need to check the file's size                  */
/*      and issue a warning if greater than 4Gb                         */
/* -------------------------------------------------------------------- */

	stat( outname, &buf );
	sz = buf.st_size;
	if( sz > maxsz ) {
	    sprintf( msg,
		     "Closing file: %s size is greater than 4Gb, while\n! "
	             "this is acceptable \n! ",
		     outname );
	    cmor_handle_error( msg, CMOR_WARNING );
	}

	if( preserve != NULL ) {
	    cmor_vars[var_id].initialized = -1;
	    cmor_vars[var_id].ntimes_written = 0;
	    cmor_vars[var_id].time_nc_id = -999;
	    cmor_vars[var_id].time_bnds_nc_id = -999;
	    for( i = 0; i < 10; i++ ) {
		cmor_vars[var_id].ntimes_written_coords[i] = -1;
		cmor_vars[var_id].ntimes_written_associated[i] = 0;
		cmor_vars[var_id].associated_ids[i] = -1;
		cmor_vars[var_id].nc_var_id = -999;
	    }
	    for( i = 0; i < cmor_vars[var_id].nattributes; i++ ) {
		if( strcmp( cmor_vars[var_id].attributes[i],
		        "cell_methods" ) == 0 ) {
		    cmor_set_variable_attribute_internal( var_id,
		            "cell_methods",
		            'c',
		            cmor_tables[cmor_vars[var_id].ref_table_id].
		            vars[cmor_vars[var_id].ref_var_id].cell_methods );
		}
	    }
	} else {
	    cmor_reset_variable( var_id );
	    cmor_vars[var_id].closed = 1;
	}
    }
    cleanup_varid = -1;
    cmor_pop_traceback(  );
    return( 0 );
}

/************************************************************************/
/*                             cmor_close()                             */
/************************************************************************/
int cmor_close( void ) {
    int i,  j;
    extern int cmor_nvars;
    char msg[CMOR_MAX_STRING];
    extern ut_system *ut_read;
    extern FILE *output_logfile;

    cmor_add_traceback( "cmor_close" );
    cmor_is_setup(  );
    if( output_logfile == NULL )
	output_logfile = stderr;

    for( i = 0; i < cmor_nvars + 1; i++ ) {
	if( cmor_vars[i].initialized != -1 ) {
	    if( cmor_vars[i].closed == 0 ) {
	        cmor_close_variable( i, NULL, NULL );
	    }
	} else if( ( cmor_vars[i].needsinit == 1 )
		   && ( cmor_vars[i].closed != 1 ) ) {
	    snprintf( msg, CMOR_MAX_STRING,
		      "variable %s (%i, table: %s) has been defined\n! "
	              "but never initialized",
		      cmor_vars[i].id, i,
		      cmor_tables[cmor_vars[i].ref_table_id].szTable_id );
	    cmor_handle_error( msg, CMOR_WARNING );
	}
    }
    for( i = 0; i < CMOR_MAX_TABLES; i++ ) {
	for( j = 0; j < CMOR_MAX_ELEMENTS; j++ ) {
	    if( cmor_tables[i].axes[j].requested != NULL ) {
		free( cmor_tables[i].axes[j].requested );
		cmor_tables[i].axes[j].requested = NULL;
	    }
	    if( cmor_tables[i].axes[j].requested_bounds != NULL ) {
		free( cmor_tables[i].axes[j].requested_bounds );
		cmor_tables[i].axes[j].requested_bounds = NULL;
	    }
	    if( cmor_tables[i].axes[j].crequested != NULL ) {
		free( cmor_tables[i].axes[j].crequested );
		cmor_tables[i].axes[j].crequested = NULL;
	    }
	}
	if( cmor_tables[i].nforcings > 0 ) {
	    for( j = 0; j < cmor_tables[i].nforcings; j++ ) {
		free( cmor_tables[i].forcings[j] );
		cmor_tables[i].forcings[j] = NULL;
	    }
	    free( cmor_tables[i].forcings );
	    cmor_tables[i].forcings = NULL;
	    cmor_tables[i].nforcings = 0;
	}
    }
    for( i = 0; i < CMOR_MAX_GRIDS; i++ ) {
	if( cmor_grids[i].lons != NULL ) {
	    free( cmor_grids[i].lons );
	    cmor_grids[i].lons = NULL;
	}
	if( cmor_grids[i].lats != NULL ) {
	    free( cmor_grids[i].lats );
	    cmor_grids[i].lats = NULL;
	}
	if( cmor_grids[i].blons != NULL ) {
	    free( cmor_grids[i].blons );
	    cmor_grids[i].blons = NULL;
	}
	if( cmor_grids[i].blats != NULL ) {
	    free( cmor_grids[i].blats );
	    cmor_grids[i].blats = NULL;
	}
    }
    if( cmor_nerrors != 0 || cmor_nwarnings != 0 ) {
	fprintf( output_logfile,
		 "! ------\n! CMOR is now closed.\n! ------\n! "
	         "During execution we encountered:\n! " );
#ifdef COLOREDOUTPUT
	fprintf( output_logfile, "%c[%d;%dm", 0X1B, 1, 34 );
#endif
	fprintf( output_logfile, "%3i Warning(s)", cmor_nwarnings );
#ifdef COLOREDOUTPUT
	fprintf( output_logfile, "%c[%dm", 0X1B, 0 );
#endif
	fprintf( output_logfile, "\n! " );
#ifdef COLOREDOUTPUT
	fprintf( output_logfile, "%c[%d;%dm", 0X1B, 1, 31 );
#endif
	fprintf( output_logfile, "%3i Error(s)", cmor_nerrors );
#ifdef COLOREDOUTPUT
	fprintf( output_logfile, "%c[%dm", 0X1B, 0 );
#endif
	fprintf( output_logfile,
		 "\n! ------\n! Please review them.\n! ------\n! " );
    } else {
	fprintf( output_logfile,
		 "! ------\n! CMOR is now closed.\n! ------\n! \n! "
	        "We encountered no warnings or errors during execution\n! "
	        "------\n! Congratulations!\n! ------\n! " );
    }
    if( output_logfile != stderr )
	fclose( output_logfile );
    cmor_pop_traceback(  );
    return( 0 );
}


/************************************************************************/
/*                          cmor_trim_string()                          */
/************************************************************************/
void cmor_trim_string( char *in, char *out ) {
    int n, i, j;

    if( in == NULL ) {
	out = NULL;
	return;
    }
    n = strlen( in );

    if( n == 0 ) {
	out[0] = '\0';
	return;
    }
    if( n > CMOR_MAX_STRING )
	n = CMOR_MAX_STRING;	
    j = 0;
    for( i = 0; i < n; i++ ) {
	if( in[i] != ' ' && in[i] != '\n' && in[i] != '\t' ) {
	    break;
	} else {
	    j++;
	}
    }
    for( i = j; i < n; i++ ) {
	out[i - j] = in[i];
    }
    out[i - j] = '\0';
    n = strlen( out );
    i = n;
    while( ( out[i] == '\0' || out[i] == ' ' ) ) {
	out[i] = '\0';
	i--;
    }
}
