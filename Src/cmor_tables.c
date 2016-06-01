#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include "cmor.h"
#include "cmor_func_def.h"
#include <netcdf.h>
#include <udunits2.h>
#include <stdlib.h>
#include "cmor_locale.h"
#include "json.h"
#include "json_tokener.h"
#include <sys/stat.h>

/************************************************************************/
/*                               wfgetc()                               */
/************************************************************************/
int wfgetc( FILE * afile ) {
    int i = fgetc( afile );

    while( i == '\r' ) {
	i = fgetc( afile );
    }
    return(i);
}
/************************************************************************/
/*                       cmor_get_table_attr                            */
/*                                                                      */
/*  tags for template used in input config file (.json)                 */
/*                                                                      */
/************************************************************************/
int cmor_get_table_attr( char *szToken, cmor_table_t * table, char *out) {
    int i;
    t_symstruct lookuptable[]= {
            {"mip_era",     table->mip_era  },
            {"table",       table->szTable_id    },
            {"realm",       table->realm       },
            {"date",        table->date        },
            {"product",     table->product     },
            {"path",        table->path        },
            {"frequency",   table->frequency   },
            {"", ""},
            {"", ""},
            {"", ""}
    };

    int nKeys=(sizeof(lookuptable)/sizeof(t_symstruct));

    for (i=0; i < nKeys; i++) {
        t_symstruct *sym = &lookuptable[i];
        if(strcmp(szToken, sym->key) == 0) {
            strcpy(out, sym->value);
            cmor_pop_traceback(  );
            return(0);
        }
    }

    cmor_pop_traceback(  );
    return(1);

}


/************************************************************************/
/*                          cmor_init_table()                           */
/************************************************************************/
void cmor_init_table( cmor_table_t * table, int id ) {
    int i;

    cmor_add_traceback( "cmor_init_table" );
    cmor_is_setup(  );
    /* init the table */
    table->id = id;
    table->nvars = -1;
    table->naxes = -1;
    table->nexps = -1;
    table->nmappings = -1;
    table->cf_version = 1.6;
    table->cmor_version = 3.0;
    table->mip_era[0] = '\0';
    table->szTable_id[0] = '\0';
    strcpy( table->realm, "REALM" );
    table->date[0] = '\0';
    table->missing_value = 1.e20;
    table->interval = 0.;
    table->interval_warning = .1;
    table->interval_error = .2;
    table->URL[0] = '\0';
    strcpy( table->product, "output" );
    table->path[0] = '\0';
    table->frequency[0] = '\0';
    table->nforcings = 0;
    for( i = 0; i < CMOR_MAX_ELEMENTS; i++ ) {
	table->expt_ids[i][0] = '\0';
	table->sht_expt_ids[i][0] = '\0';
	table->generic_levels[i][0] = '\0';
    }
    table->CV = NULL;

    cmor_pop_traceback(  );



}


/************************************************************************/
/*                    cmor_set_variable_entry()                         */
/************************************************************************/
int cmor_set_variable_entry(cmor_table_t* table,
                            char *variable_entry,
                            json_object *json) {
    extern int cmor_ntables;
    char szValue[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    int nVarId;
    char *szTableId;
    cmor_var_def_t *variable;
    cmor_table_t *cmor_table;
    cmor_table = &cmor_tables[cmor_ntables];

    szTableId = cmor_table->szTable_id;

    cmor_add_traceback("cmor_set_variable_entry");
    cmor_is_setup();

    /* -------------------------------------------------------------------- */
    /*      Check number of variables                                       */
    /* -------------------------------------------------------------------- */
    cmor_table->nvars++;
    nVarId = cmor_table->nvars;
    variable = &cmor_table->vars[nVarId];

    if (nVarId >= CMOR_MAX_ELEMENTS) {
        snprintf(msg, CMOR_MAX_STRING,
                "Too many variables defined for table: %s", szTableId);
        cmor_handle_error(msg, CMOR_CRITICAL);
        cmor_ntables--;
        cmor_pop_traceback();
        return (1);
    }

    cmor_init_var_def(variable, cmor_ntables);
    cmor_set_var_def_att(variable, "id", variable_entry);

    json_object_object_foreach(json, attr, value) {
/* -------------------------------------------------------------------- */
/*  Attribute keys starting with "#" are seen as comments or examples   */
/*  and they are skipped!                                               */
/* -------------------------------------------------------------------- */

        if( attr[0] == '#')
            continue;

        strcpy(szValue, json_object_get_string(value));
        cmor_set_var_def_att(variable, attr, szValue);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                        cmor_set_axis_entry()                         */
/************************************************************************/
int cmor_set_axis_entry( cmor_table_t* table,
                         char *axis_entry,
                         json_object *json ){
    extern int cmor_ntables;
    char szValue[CMOR_MAX_STRING*4];
    char msg[CMOR_MAX_STRING];
    int nAxisId;
    char *szTableId;
    cmor_axis_def_t *axis;
    cmor_table_t *cmor_table;
    cmor_table = &cmor_tables[cmor_ntables];

    szTableId = cmor_table->szTable_id;

    cmor_add_traceback("cmor_set_axis_entry");
    cmor_is_setup();

    /* -------------------------------------------------------------------- */
    /*      Check number of axes                                            */
    /* -------------------------------------------------------------------- */
    cmor_table->naxes++;
    nAxisId = cmor_table->naxes;
    axis = &cmor_table->axes[nAxisId];


    if (nAxisId >= CMOR_MAX_ELEMENTS) {
      snprintf(msg, CMOR_MAX_STRING, "Too many axes defined for table: %s",
              szTableId);
      cmor_handle_error(msg,CMOR_CRITICAL);
      cmor_ntables--;
      cmor_pop_traceback();
      return(1);
    }
    axis = &cmor_table->axes[nAxisId];

    /* -------------------------------------------------------------------- */
    /*      Define Axis                                                     */
    /* -------------------------------------------------------------------- */
    cmor_init_axis_def(axis, cmor_ntables);
    cmor_set_axis_def_att(axis, "id", axis_entry);

    /* -------------------------------------------------------------------- */
    /*      Add axis value                                                  */
    /* -------------------------------------------------------------------- */
    json_object_object_foreach(json, attr, value) {
        if( attr[0] == '#') {
            continue;
        }
        strcpy(szValue, json_object_get_string(value));
        cmor_set_axis_def_att(axis, attr, szValue);
    }
    cmor_pop_traceback();
    return (0);
}
/************************************************************************/
/*                        cmor_set_experiments()                        */
/************************************************************************/
int cmor_set_experiments(cmor_table_t * table, char att[CMOR_MAX_STRING],
                char val[CMOR_MAX_STRING]) {
    extern int cmor_ntables;
    char szError[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_set_experiments");
    cmor_is_setup();
    table->nexps++;
    /* -------------------------------------------------------------------- */
    /*      Check number of experiments                                     */
    /* -------------------------------------------------------------------- */
    if( table->nexps > CMOR_MAX_ELEMENTS ) {
        snprintf( szError, CMOR_MAX_STRING,
                  "Table %s: Too many experiments defined",
                  table->szTable_id );
        cmor_handle_error( szError, CMOR_CRITICAL );
        cmor_ntables--;
        cmor_pop_traceback(  );
        return(1);
    }

    /* -------------------------------------------------------------------- */
    /*      Insert experiment to table                                      */
    /* -------------------------------------------------------------------- */

    strncpy( table->sht_expt_ids[table->nexps], att,
            CMOR_MAX_STRING );
    strncpy( table->expt_ids[table->nexps], val,
            CMOR_MAX_STRING );

    cmor_pop_traceback();
    return(0);
}

/************************************************************************/
/*                        cmor_set_dataset_att()                        */
/************************************************************************/
int cmor_set_dataset_att(cmor_table_t * table, char att[CMOR_MAX_STRING],
		char val[CMOR_MAX_STRING]) {
	int n, i, j;
	float d, d2;
	char value[CMOR_MAX_STRING];
	char value2[CMOR_MAX_STRING];
	extern int cmor_ntables;

	cmor_add_traceback("cmor_set_dataset_att");
	cmor_is_setup();

	strncpy(value, val, CMOR_MAX_STRING);

/* -------------------------------------------------------------------- */
/*      Read non-block metadata.                                        */
/* -------------------------------------------------------------------- */
	if (strcmp(att, TABLE_HEADER_VERSION) == 0) {
		d2 = CMOR_VERSION_MAJOR;
		d = CMOR_VERSION_MINOR;
		while (d > 1.)
			d /= 10.;
		d2 += d;
		sscanf(value, "%f", &d);
		if (d > d2) {
			snprintf(value2, CMOR_MAX_STRING,
					"Table %s is defined for cmor_version %f, "
					"this library verson is: %i.%i.%i, %f",
					table->szTable_id, d,
					CMOR_VERSION_MAJOR, CMOR_VERSION_MINOR,
					CMOR_VERSION_PATCH, d2);
			cmor_handle_error(value2, CMOR_CRITICAL);
			cmor_ntables--;
			cmor_pop_traceback();
			return(1);
		}
		table->cmor_version = d;

	} else if (strcmp(att, TABLE_HEADER_GENERIC_LEVS) == 0) {
		n = 0;
		i = 0;
		while (i < (strlen(value)) ) {
			while (value[i] == ' ')
				i++;
			j = 0;
			while (i < (strlen(value)) && value[i] != ' ') {
				table->generic_levels[n][j] = value[i];
				j++;
				i++;
			}
			table->generic_levels[n][j] = '\0';
			n += 1;
		}

	} else if (strcmp(att, TABLE_HEADER_CONVENTIONS) == 0) {
                strncpy(table->Conventions, val, CMOR_MAX_STRING);

        } else if (strcmp(att, TABLE_HEADER_DATASPECSVERSION) == 0) {
                strncpy(table->data_specs_version, val, CMOR_MAX_STRING);

	} else if (strcmp(att, TABLE_HEADER_MIP_ERA) == 0) {
		strncpy(table->mip_era, value, CMOR_MAX_STRING);

	} else if (strcmp(att, TABLE_HEADER_REALM) == 0) {
		strncpy(table->realm, value, CMOR_MAX_STRING);

	} else if (strcmp(att, TABLE_HEADER_TABLE_DATE) == 0) {
		strncpy(table->date, value, CMOR_MAX_STRING);

	} else if (strcmp(att, TABLE_HEADER_BASEURL) == 0) {
		strncpy(table->URL, value, CMOR_MAX_STRING);

	} else if (strcmp(att, TABLE_HEADER_FORCINGS) == 0) {
		cmor_convert_string_to_list(value,
		                            'c',
		                            (void **) &table->forcings,
		                            &table->nforcings);

	} else if (strcmp(att, TABLE_HEADER_PRODUCT) == 0) {
		strncpy(table->product, value, CMOR_MAX_STRING);

	} else if (strcmp(att, TABLE_HEADER_FREQUENCY) == 0) {
		strncpy(table->frequency, value, CMOR_MAX_STRING);

	} else if (strcmp(att, TABLE_HEADER_TABLE_ID) == 0) {
		for (n = 0; n == cmor_ntables; n++) {
			if (strcmp(cmor_tables[n].szTable_id, value) == 0) {
				snprintf(value2, CMOR_MAX_STRING,
				        "Table %s is already defined",
				        table->szTable_id);
				cmor_handle_error(value2, CMOR_CRITICAL);
				cmor_ntables--;
				cmor_pop_traceback();
				return(1);
			}
		}
	n = strlen( value );
	for( i = n - 1; i > 0; i-- ) {
	    if( value[i] == ' ' )
		break;
	}
	if( value[i] == ' ' )
	    i++;

	for( j = i; j < n; j++ )
	    value2[j - i] = value[j];
	value2[n - i] = '\0';
	strcpy( table->szTable_id, value2 );

/* -------------------------------------------------------------------- */
/*      Save all experiment id                                          */
/* -------------------------------------------------------------------- */
    } else if( strcmp( att, TABLE_EXPIDS ) == 0 ) {
	table->nexps++;
	if( table->nexps > CMOR_MAX_ELEMENTS ) {
	    snprintf( value2, CMOR_MAX_STRING,
		      "Table %s: Too many experiments defined",
		      table->szTable_id );
	    cmor_handle_error( value2, CMOR_CRITICAL );
	    cmor_ntables--;
	    cmor_pop_traceback(  );
	    return(1);
	}

	if( value[0] == '\'' )
	    for( n = 0; n < strlen( value ) - 1; n++ )
            value[n] = value[n + 1];	/* removes leading "'" */
	n = strlen( value );

	if( value[n - 2] == '\'' )
	    value[n - 2] = '\0';	/*removes trailing "'" */
/* -------------------------------------------------------------------- */
/*      ok here we look for a ' which means there is                    */
/*      a short name associated with it                                 */
/* -------------------------------------------------------------------- */
	n = -1;
	for( j = 0; j < strlen( value ); j++ ) {
	    if( value[j] == '\'' ) {
            n = j;
            break;
	    }
	}
	if( n == -1 ) {
	    strncpy( table->expt_ids[table->nexps], value, CMOR_MAX_STRING );
	    strcpy( table->sht_expt_ids[table->nexps], "" );
	} else {
/* -------------------------------------------------------------------- */
/*      ok looks like we have a short name let clook for the next '     */
/* -------------------------------------------------------------------- */
	    i = -1;
	    for( j = n + 1; j < strlen( value ); j++ ) {
		if( value[j] == '\'' )
		    i = j;
	    }
/* -------------------------------------------------------------------- */
/*      ok we must have a ' in our exp_id_ok                            */
/* -------------------------------------------------------------------- */
	    if( i == -1 ) {	
		strncpy( table->expt_ids[table->nexps], value,
			 CMOR_MAX_STRING );
		strcpy( table->sht_expt_ids[table->nexps], "" );
	    } else {
		for( j = i + 1; j < strlen( value ); j++ ) {
		    value2[j - i - 1] = value[j];
		    value2[j - i] = '\0';
		}
		strncpy( table->sht_expt_ids[table->nexps], value2,
			 CMOR_MAX_STRING );
		value[n] = '\0';
		strncpy( table->expt_ids[table->nexps], value,
			 CMOR_MAX_STRING );
	    }
	}
    } else if( strcmp( att, TABLE_HEADER_APRX_INTRVL ) == 0 ) {
	sscanf( value, "%lf", &table->interval );
    } else if( strcmp( att, TABLE_HEADER_APRX_INTRVL_ERR ) == 0 ) {
	sscanf( value, "%f", &table->interval_error );
    } else if( strcmp( att, TABLE_HEADER_APRX_INTRVL_WRN ) == 0 ) {
	sscanf( value, "%f", &table->interval_warning );
    } else if( strcmp( att, TABLE_HEADER_MISSING_VALUE ) == 0 ) {
	sscanf( value, "%f", &table->missing_value );
    } else if( strcmp( att, TABLE_HEADER_MAGIC_NUMBER) == 0 ) {
	    
    } else {
	snprintf( value, CMOR_MAX_STRING,
		  "table: %s, unknown keyword for dataset: %s (%s)",
		  table->szTable_id, att, value );
	cmor_handle_error( value, CMOR_WARNING );
    }
    cmor_pop_traceback(  );
    return(0);
}

/************************************************************************/
/*                           cmor_set_table()                           */
/************************************************************************/

int cmor_set_table( int table ) {
    extern int CMOR_TABLE;
    char msg[CMOR_MAX_STRING];

    cmor_add_traceback( "cmor_set_table" );
    cmor_is_setup(  );
    if( table > cmor_ntables ) {
	snprintf( msg, CMOR_MAX_STRING, "Invalid table number: %i",
		  table );
	cmor_handle_error( msg, CMOR_CRITICAL );
    }
    if( cmor_tables[table].szTable_id == '\0' ) {
	snprintf( msg, CMOR_MAX_STRING,
		  "Invalid table: %i , not loaded yet!", table );
	cmor_handle_error( msg, CMOR_CRITICAL );
    }
    CMOR_TABLE = table;
    cmor_pop_traceback(  );
    return(0);
}

/************************************************************************/
/*                       cmor_load_table()                              */
/************************************************************************/
int cmor_load_table( char szTable[CMOR_MAX_STRING], int *table_id ) {
    int rc;
    char *szPath;
    char *szTableName;
    char szControlFilenameJSON[CMOR_MAX_STRING];
    char szCV[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    struct stat st;

    rc = cmor_get_cur_dataset_attribute(GLOBAL_CV_FILENAME, szCV);

/* -------------------------------------------------------------------- */
/*  build string "path/<CV>.json"                                */
/* -------------------------------------------------------------------- */
    szTableName = strdup(szTable);
    szPath = dirname(szTableName);
/* -------------------------------------------------------------------- */
/*  build string "path/CV.json"                                         */
/* -------------------------------------------------------------------- */
    strcpy(szControlFilenameJSON, szPath);
    strcat(szControlFilenameJSON, "/");
    strcat(szControlFilenameJSON, szCV);

/* -------------------------------------------------------------------- */
/*  try to load table from directory where table is found or from the   */
/*  cmor_input_path                                                     */
/* -------------------------------------------------------------------- */
    rc = stat(szControlFilenameJSON, &st);
    if(rc != 0 ) {
        strcpy(szControlFilenameJSON, cmor_input_path);
        strcat(szControlFilenameJSON, "/");
        strcat(szControlFilenameJSON, szCV);
    }

    rc= cmor_load_table_internal( szTable, table_id, TRUE);

    if((rc != TABLE_SUCCESS) && (rc != TABLE_FOUND)){
        snprintf( msg, CMOR_MAX_STRING, "Can't open table %s", szTable);
        cmor_handle_error( msg, CMOR_WARNING );
    }
    if(rc == TABLE_SUCCESS) {
        cmor_set_cur_dataset_attribute_internal(CV_INPUTFILENAME,
                                                szControlFilenameJSON, 1);
        rc= cmor_load_table_internal( szControlFilenameJSON, table_id, FALSE);
        if(rc != TABLE_SUCCESS){
            snprintf( msg, CMOR_MAX_STRING, "Can't open table %s",
                    szControlFilenameJSON);
            cmor_handle_error( msg, CMOR_CRITICAL );
        }
    } else if (rc == TABLE_FOUND) {
        rc = TABLE_SUCCESS;
    }

    free(szTableName);
    return(rc);
}
/************************************************************************/
/*                   cmor_load_table_internal()                         */
/************************************************************************/
int cmor_load_table_internal( char table[CMOR_MAX_STRING], int *table_id,
                              int bNewTable) {
    FILE *table_file;
    char word[CMOR_MAX_STRING];
    int i, n;
    int done=0;
    extern int CMOR_TABLE, cmor_ntables;
    extern char cmor_input_path[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char szVal[1024000];
    char *buffer= NULL;
    int nTableSize, read_size;
    json_object *json_obj;

    cmor_add_traceback( "cmor_load_table_internal" );
    cmor_is_setup(  );

    if( bNewTable ) {

/* -------------------------------------------------------------------- */
/*      Is the table already loaded?                                    */
/* -------------------------------------------------------------------- */
        for (i = 0; i < cmor_ntables + 1; i++) {

            if (strcmp(cmor_tables[i].path, table) == 0) {
                CMOR_TABLE = i;
                *table_id = i;
                cmor_pop_traceback();
                return (TABLE_FOUND);
            }
        }
/* -------------------------------------------------------------------- */
/*      Try to open file                                                */
/*      $path/table                                                     */
/*      /usr/local/cmor/share/table                                     */
/* -------------------------------------------------------------------- */
        cmor_ntables += 1;
        cmor_init_table( &cmor_tables[cmor_ntables], cmor_ntables );
    }

    table_file = fopen( table, "r" );
    
    if( table_file == NULL ) {
	if( table[0] != '/' ) {
	    snprintf( word, CMOR_MAX_STRING, "%s/%s", cmor_input_path,
		      table );
	    table_file = fopen( word, "r" );
	}
	if( table_file == NULL ) {
	    snprintf( word, CMOR_MAX_STRING, "%s/share/%s", CMOR_PREFIX,
		      table );
	    table_file = fopen( word, "r" );
	}
	if( table_file == NULL ) {
	    snprintf( word, CMOR_MAX_STRING, "Could not find file: %s",
		      table );
	    cmor_handle_error( word, CMOR_NORMAL );
	    cmor_ntables -= 1;
	    cmor_pop_traceback(  );
	    return(TABLE_ERROR);
	}
    }


/* -------------------------------------------------------------------- */
/*      ok now we need to store the md5                                 */
/* -------------------------------------------------------------------- */
    cmor_md5( table_file, cmor_tables[cmor_ntables].md5 );
    
/* -------------------------------------------------------------------- */
/*      Read the entire table in memory                                 */
/* -------------------------------------------------------------------- */
    fseek(table_file,0,SEEK_END);
    nTableSize = ftell( table_file );
    rewind( table_file );
    buffer = (char *) malloc( sizeof(char) * (nTableSize + 1) );
    read_size = fread( buffer, sizeof(char), nTableSize, table_file ); 
    buffer[nTableSize] = '\0';

/* -------------------------------------------------------------------- */
/*      print errror and exist if not a JSON file                       */
/* -------------------------------------------------------------------- */

    if(buffer[0]!= '{') {
        free(buffer);
        buffer=NULL;
        snprintf( msg, CMOR_MAX_STRING,
                  "Could not understand file \"%s\" Is this a JSON CMOR table?",
                   table  );
            cmor_handle_error( msg, CMOR_CRITICAL );
            cmor_ntables--;
            cmor_pop_traceback(  );
        return(TABLE_ERROR);
    }
/* -------------------------------------------------------------------- */
/*      print errro and exit if file was not completly read             */
/* -------------------------------------------------------------------- */
    if( nTableSize != read_size ) {
        free(buffer);
        buffer=NULL;
        snprintf( msg, CMOR_MAX_STRING,
                  "Could not read file %s check file permission",
                   word  );
            cmor_handle_error( msg, CMOR_CRITICAL );
            cmor_ntables--;
            cmor_pop_traceback(  );
        return(TABLE_ERROR);
    }

/* -------------------------------------------------------------------- */
/*     parse buffer into json object                                    */ 
/* -------------------------------------------------------------------- */
    json_obj = json_tokener_parse(buffer);
    if( json_obj == NULL ){
        snprintf( msg, CMOR_MAX_STRING,
                "Please validate JSON File!\n"
                "USE: http://jsonlint.com/\n"
                "Syntax Error in table: %s\n "
                "%s",table, buffer );
        cmor_handle_error( msg, CMOR_CRITICAL );
        cmor_pop_traceback();
        return(TABLE_ERROR);
    }

    json_object_object_foreach(json_obj, key, value) {
        if( key[0] == '#') {
            continue;
        }
        strcpy(szVal, json_object_get_string(value));
/* -------------------------------------------------------------------- */
/*      Now let's see what we found                                     */
/* -------------------------------------------------------------------- */
	if( strcmp( key, JSON_KEY_HEADER ) == 0 ) {
/* -------------------------------------------------------------------- */
/*      Fill up all global attributer found in header section           */
/* -------------------------------------------------------------------- */
            json_object_object_foreach(value, key, globalAttr) {
                if( key[0] == '#') {
                    continue;
                }
                strcpy(szVal, json_object_get_string(globalAttr));
                if( cmor_set_dataset_att( &cmor_tables[cmor_ntables], key, 
                                           szVal ) == 1 ) {
                    cmor_pop_traceback();
                    return(TABLE_ERROR);
                }
            }
            done=1;

        } else  if( strcmp( key, JSON_KEY_EXPERIMENT ) == 0 ){
            json_object_object_foreach(value, shortname, description) {
                if( shortname[0] == '#') {
                    continue;
                }
                strcpy(szVal, json_object_get_string(description));
                if( cmor_set_experiments( &cmor_tables[cmor_ntables],
                                          shortname,
                                          szVal ) == 1 ) {
                    cmor_pop_traceback(  );
                    return(TABLE_ERROR);
                }
            }
            done=1;
        } else if (strcmp(key, JSON_KEY_AXIS_ENTRY) == 0) {
            json_object_object_foreach(value, axisname, attributes) {
                if( axisname[0] == '#') {
                    continue;
                }
                if( cmor_set_axis_entry(&cmor_tables[cmor_ntables],
                        axisname,
                        attributes) == 1) {
                    cmor_pop_traceback();
                    return (TABLE_ERROR);
                }
            }
            done=1;
	} else if( strcmp( key, JSON_KEY_VARIABLE_ENTRY ) == 0 ) {
            json_object_object_foreach(value, varname, attributes) {
                if( varname[0] == '#') {
                    continue;
                }
                if( cmor_set_variable_entry(&cmor_tables[cmor_ntables],
                        varname,
                        attributes) == 1) {
                    cmor_pop_traceback();
                    return (TABLE_ERROR);
                }
            }
            done=1;
	} else if( strncmp( key, JSON_KEY_CV_ENTRY,2) == 0 ) {

	    if( cmor_CV_set_entry(&cmor_tables[cmor_ntables], value) == 1 ) {
                cmor_pop_traceback();
                return (TABLE_ERROR);
	    }
	    done=1;

	} else if( strcmp( key, JSON_KEY_MAPPING_ENTRY ) == 0 ) {
/* -------------------------------------------------------------------- */
/*      Work on mapping entries                                         */
/* -------------------------------------------------------------------- */
	    cmor_tables[cmor_ntables].nmappings++;
            if (cmor_tables[cmor_ntables].nmappings >= CMOR_MAX_ELEMENTS) {
                snprintf(msg, CMOR_MAX_STRING,
                        "Too many mappings defined for table: %s",
                        cmor_tables[cmor_ntables].szTable_id);
                cmor_handle_error(msg, CMOR_CRITICAL);
                cmor_ntables--;
                cmor_pop_traceback();
                return (TABLE_ERROR);
            }
            json_object_object_foreach(value, mapname, jsonValue) {
                if( mapname[0] == '#') {
                    continue;
                }
                char szLastMapID[CMOR_MAX_STRING];
                char szCurrMapID[CMOR_MAX_STRING];
                cmor_table_t *psCurrCmorTable;

                psCurrCmorTable = &cmor_tables[cmor_ntables];

                int nMap;
                nMap = psCurrCmorTable->nmappings;

                for (n = 0; n < nMap - 1; n++) {

                    strcpy(szLastMapID, psCurrCmorTable->mappings[nMap].id);
                    strcpy(szCurrMapID, psCurrCmorTable->mappings[n].id);

                    if (strcmp(szLastMapID, szCurrMapID) == 0) {
                        snprintf(msg, CMOR_MAX_STRING,
                                "mapping: %s already defined within this table (%s)",
                                cmor_tables[cmor_ntables].mappings[n].id,
                                cmor_tables[cmor_ntables].szTable_id);
                        cmor_handle_error(msg, CMOR_CRITICAL);
                    };
                }
/* -------------------------------------------------------------------- */
/*      init the variable def                                           */
/* -------------------------------------------------------------------- */
                cmor_init_grid_mapping(&psCurrCmorTable->mappings[nMap], mapname);
                json_object_object_foreach(jsonValue, key, mappar)
                {
                    if( key[0] == '#') {
                        continue;
                    }
                    char param[CMOR_MAX_STRING];

                    strcpy(param, json_object_get_string(mappar));

                    cmor_set_mapping_attribute(
                            &psCurrCmorTable->mappings[psCurrCmorTable->nmappings],
                            key, param);

                }

            }
            done = 1;

	} else {
/* -------------------------------------------------------------------- */
/*      nothing knwon we will not be setting any attributes!            */
/* -------------------------------------------------------------------- */
		    
		    snprintf( msg, CMOR_MAX_STRING,
			      "unknown section: %s, for table: %s", key,
			      cmor_tables[cmor_ntables].szTable_id );
		    cmor_handle_error( msg, CMOR_WARNING );
	}

/* -------------------------------------------------------------------- */
/*      First check for table/dataset mode values                       */
/* -------------------------------------------------------------------- */

        if ( done == 1 ){
            done = 0;
        } else {
            snprintf( msg, CMOR_MAX_STRING,
                  "attribute for unknown section: %s,%s (table: %s)",
                  key, szVal, cmor_tables[cmor_ntables].szTable_id );
            cmor_handle_error( msg, CMOR_WARNING );
            /*printf("attribute for unknown section\n"); */
        }
    }
    *table_id = cmor_ntables;
    if( bNewTable ) {
        strcpy( cmor_tables[cmor_ntables].path, table );
    }
    CMOR_TABLE = cmor_ntables;
    cmor_pop_traceback(  );
    free(buffer);
    json_object_put(json_obj);
    return(TABLE_SUCCESS);
}
