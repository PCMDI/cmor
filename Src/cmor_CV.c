#include <string.h>
#include <stdio.h>
#include "cmor.h"
#include "json.h"
#include "json_tokener.h"
#include "arraylist.h"

/************************************************************************/
/*                        cmor_CV_set_att()                             */
/************************************************************************/
void cmor_CV_set_att(cmor_CV_def_t *CV,
                        char *szKey,
                        json_object *joValue) {
    json_bool bValue;
    double dValue;
    int nValue;
    char szValue[CMOR_MAX_STRING];
    array_list *pArray;
    int i, k, length;


    strcpy(CV->key, szKey);

    if( json_object_is_type( joValue, json_type_null) ) {
        printf("Will not save NULL JSON type from CV.json\n");

    } else if(json_object_is_type( joValue, json_type_boolean)) {
        bValue = json_object_get_boolean(joValue);
        CV->nValue = (int) bValue;
        CV->type = CV_integer;

    } else if(json_object_is_type( joValue, json_type_double)) {
        dValue = json_object_get_double(joValue);
        CV->dValue = dValue;
        CV->type = CV_double;

    } else if(json_object_is_type( joValue, json_type_int)) {
        nValue = json_object_get_int(joValue);
        CV->nValue = nValue;
        CV->type = CV_integer;
/* -------------------------------------------------------------------- */
/* if value is a JSON object, recursively call in this function         */
/* -------------------------------------------------------------------- */
   } else if(json_object_is_type( joValue, json_type_object)) {

        int nCVId = -1;
        int nbObject = 0;
        cmor_CV_def_t * oValue;
        int nTableID = CV->table_id; /* Save Table ID */

        json_object_object_foreach(joValue, key, value) {
            nbObject++;
            oValue = (cmor_CV_def_t *) realloc(CV->oValue,
                   sizeof(cmor_CV_def_t) * nbObject);
            CV->oValue = oValue;

            nCVId++;
            cmor_CV_init(&CV->oValue[nCVId], nTableID);
            cmor_CV_set_att(&CV->oValue[nCVId], key, value  );
        }
        CV->nbObjects=nbObject;
        CV->type = CV_object;


    } else if(json_object_is_type( joValue, json_type_array)) {
        pArray = json_object_get_array(joValue);
        length = array_list_length(pArray);
        json_object *joItem;
        CV->anElements = length;
        for(k=0; k<length; k++) {
            joItem = (json_object *) array_list_get_idx(pArray, k);
            strcpy(CV->aszValue[k], json_object_get_string(joItem));
        }
        CV->type=CV_stringarray;

    } else if(json_object_is_type( joValue, json_type_string)) {
        strcpy(CV->szValue, json_object_get_string(joValue));
        CV->type=CV_string;
    }



}


/************************************************************************/
/*                           cmor_CV_init()                             */
/************************************************************************/
void cmor_CV_init( cmor_CV_def_t *CV, int table_id ) {
    int i;

    cmor_is_setup(  );
    cmor_add_traceback("cmor_init_CV_def");
    CV->table_id = table_id;
    CV->type = CV_undef;  //undefined
    CV->nbObjects = -1;
    CV->nValue = -1;
    CV->key[0] = '\0';
    CV->szValue[0] = '\0';
    CV->dValue=-9999.9;
    CV->oValue = NULL;
    for(i=0; i< CMOR_MAX_JSON_ARRAY; i++) {
        CV->aszValue[i][0]='\0';
    }
    CV->anElements = -1;

    cmor_pop_traceback();
}

/************************************************************************/
/*                         cmor_CV_print()                              */
/************************************************************************/
void cmor_CV_print(cmor_CV_def_t *CV) {
    int nCVs;
    int i,j,k;

    if (CV != NULL) {
        if(CV->key[0] != '\0'){
            printf("key: %s \n", CV->key);
        }
        else {
            return;
        }
        switch(CV->type) {

        case CV_string:
            printf("value: %s\n", CV->szValue);
            break;

        case CV_integer:
            printf("value: %d\n", CV->nValue);
            break;

        case CV_stringarray:
            printf("value: [\n");
            for (k = 0; k < CV->anElements; k++) {
                printf("value: %s\n", CV->aszValue[k]);
            }
            printf("        ]\n");
            break;

        case CV_double:
            printf("value: %lf\n", CV->dValue);
            break;

        case CV_object:
            printf("*** nbObjects=%d\n", CV->nbObjects);
            for(k=0; k< CV->nbObjects; k++){
                cmor_CV_print(&CV->oValue[k]);
            }
            break;

        case CV_undef:
            break;
        }

    }
}

/************************************************************************/
/*                       cmor_CV_printall()                             */
/************************************************************************/
void cmor_CV_printall() {
    int j,i,k;
    cmor_CV_def_t *CV;
    int nCVs;
/* -------------------------------------------------------------------- */
/* Parse tree and print key values.                                     */
/* -------------------------------------------------------------------- */

    for( i = 0; i < CMOR_MAX_TABLES; i++ ) {
        if(cmor_tables[i].CV != NULL) {
            printf("table %s\n", cmor_tables[i].szTable_id);
            nCVs = cmor_tables[i].CV->nbObjects;
            for( j=0; j<= nCVs; j++ ) {
                CV = &cmor_tables[i].CV[j];
                cmor_CV_print(CV);
            }
        }
    }
}

/************************************************************************/
/*                  cmor_CV_search_child_key()                          */
/************************************************************************/
cmor_CV_def_t *cmor_CV_search_child_key(cmor_CV_def_t *CV, char *key){
    int i,j;
    cmor_CV_def_t *searchCV;
    int nbCVs = -1;
    cmor_add_traceback("cmor_CV_search_child_key");

    nbCVs = CV->nbObjects;

    // Look at this objects
    if(strcmp(CV->key, key)== 0){
        cmor_pop_traceback();
        return(CV);
    }

    // Look at each of object key
     for(i = 0; i< nbCVs; i++) {
         // Is there a branch on that object?
         if(&CV->oValue[i] != NULL) {
             searchCV = cmor_CV_search_key(&CV->oValue[i], key);
             if(searchCV != NULL ){
                 return(searchCV);
             }
         }
     }
     return(NULL);
}

/************************************************************************/
/*                     cmor_CV_search_key()                             */
/************************************************************************/
cmor_CV_def_t * cmor_CV_search_key(cmor_CV_def_t *CV, char *key){
    int i,j;
    cmor_CV_def_t *searchCV;
    int nbCVs = -1;
    cmor_add_traceback("cmor_CV_search_key");

    // Look at first objects
    if(strcmp(CV->key, key)== 0){
        cmor_pop_traceback();
        return(CV);
    }
    // Is there more than 1 object?
    if( CV->nbObjects != -1 ) {
        nbCVs = CV->nbObjects;
    }
    // Look at each of object key
    for(i = 1; i< nbCVs; i++) {
        if(strcmp(CV[i].key, key)== 0){
            cmor_pop_traceback();
            return(&CV[i]);
        }
        // Is there a branch on that object?
        if(CV[i].oValue != NULL) {
            for(j=0; j< CV[i].nbObjects; j++) {
                searchCV = cmor_CV_search_key(&CV[i].oValue[j], key);
                if(searchCV != NULL ){
                    return(searchCV);
                }
            }
        }
    }
    cmor_pop_traceback();
    return(NULL);
}
/************************************************************************/
/*                      cmor_CV_get_value()                             */
/************************************************************************/
char *cmor_CV_get_value(cmor_CV_def_t *CV, char *key) {
        char *szValue;
        int i,j;
        int nCVs;

        switch(CV->type) {
        case CV_string:
            return(CV->szValue);
            break;

        case CV_integer:
            sprintf(CV->szValue, "%d", CV->nValue);
            break;

        case CV_stringarray:
            return(CV->aszValue[0]);
            break;

        case CV_double:
            sprintf(CV->szValue, "%lf", CV->dValue);
            break;

        case CV_object:
            return(NULL);
            break;

        case CV_undef:
            break;
        }

        return(szValue);
}
/************************************************************************/
/*                         cmor_CV_free()                               */
/************************************************************************/
void cmor_CV_free(cmor_CV_def_t *CV) {
    int i,k;
/* -------------------------------------------------------------------- */
/* Recursively go down the tree and free branch                         */
/* -------------------------------------------------------------------- */

    if(CV->oValue != NULL) {
        for(k=0; k< CV->nbObjects; k++){
            cmor_CV_free(&CV->oValue[k]);
        }
    }
    if(CV->oValue != NULL) {
        free(CV->oValue);
        CV->oValue=NULL;
    }

}
/************************************************************************/
/*                      cmor_CV_setInstitution()                        */
/************************************************************************/
void cmor_CV_setInstitution( cmor_CV_def_t *CV){
    cmor_CV_def_t *CV_institution_ids;
    cmor_CV_def_t *CV_institution;

    char szInstitution_ID[CMOR_MAX_STRING];
    char szInstitution[CMOR_MAX_STRING];

    char msg[CMOR_MAX_STRING];
    int rc;

    cmor_add_traceback("cmor_CV_setInstitution");
    // Find current Insitution ID
    cmor_get_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION_ID, szInstitution_ID);

    // Find Institution dictionaries in Control Vocabulary
    CV_institution_ids = cmor_CV_search_key(CV, CV_KEY_INSTITUTION_IDS);
    CV_institution = cmor_CV_search_child_key( CV_institution_ids,
                                               szInstitution_ID);

    if(CV_institution == NULL){
        snprintf( msg, CMOR_MAX_STRING,
                "Your defined institution_id \"%s\" could not be found \n! "
                "in your Control Vocabulary file.\n! ",
                szInstitution_ID);
        cmor_handle_error( msg, CMOR_CRITICAL );
        cmor_pop_traceback(  );
        return;
    }

    // Did the user defined an Institution Attribute?
    rc = cmor_has_cur_dataset_attribute( GLOBAL_ATT_INSTITUTION );
    if( rc == 0 ) {
        // Retrieve institution and compare with the one we have in the
        // Control Vocabulary file.  If it does not matche tell the user
        // the we will supersede his defintion with the one in the Control
        // Vocabulary file.
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION, szInstitution);


        // Check if an institution has been defined! If not we exit.
        if(CV_institution->szValue[0] == '\0'){
            snprintf( msg, CMOR_MAX_STRING,
                    "There is no institution associated to institution_id \"%s\"\n! "
                    "in your Control Vocabulary file.\n! "
                    "Check your institution_ids dictionary!!\n! ",
                            szInstitution_ID);
            cmor_handle_error( msg, CMOR_CRITICAL );
            cmor_pop_traceback(  );
            return;
        }
        // Check if they have the same string
        if(strncmp(szInstitution, CV_institution->szValue, CMOR_MAX_STRING) != 0){
            snprintf( msg, CMOR_MAX_STRING,
                    "Your defined institution \"%s\" will be overwritten by \n! "
                    "\"%s\"\n! "
                    "as defined in your Control Vocabulary file.\n! ",
                    szInstitution, CV_institution->szValue);
            cmor_handle_error( msg, CMOR_NORMAL );
        }
    }

    // Set instituion according to the Control Vocabulary
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_INSTITUTION,
                                            CV_institution->szValue, 1);
    cmor_pop_traceback();
    return;
}

/************************************************************************/
/*                    cmor_CV_CheckGblAttributes()                      */
/************************************************************************/
void cmor_CV_checkGblAttributes( cmor_CV_def_t *CV ) {
    cmor_CV_def_t *required_attrs;
    int i;
    int rc;
    char msg[CMOR_MAX_STRING];

    cmor_add_traceback( "cmor_CV_checkGblAttributes" );
    required_attrs = cmor_CV_search_key(CV, CV_KEY_REQUIRED_GBL_ATTRS);
    if( required_attrs != NULL) {
        for(i=0; i< required_attrs->anElements; i++) {
            rc = cmor_has_cur_dataset_attribute( required_attrs->aszValue[i] );
            if( rc != 0) {
                snprintf( msg, CMOR_MAX_STRING,
                          "Your Control Vocabulary file specifies one or more\n! "
                          "required attributes.  CMOR found that the following\n! "
                          "attribute was not properly set.\n! \n! "
                          "Please set attribute: \"%s\" in your input JSON file.",
                          required_attrs->aszValue[i] );
                cmor_handle_error( msg, CMOR_CRITICAL );
                cmor_pop_traceback(  );
                return;
            }
        }
    }
    cmor_pop_traceback(  );
    return;
}

/************************************************************************/
/*                 cmor_CV_ValidateGblAttributes()                  */
/************************************************************************/
int cmor_CV_ValidateGblAttributes( char *name) {


    cmor_add_traceback( "cmor_CV_ValidateGblAttributes" );
    cmor_pop_traceback(  );
    return(0);
}

/************************************************************************/
/*                       cmor_CV_set_entry()                            */
/************************************************************************/
int cmor_CV_set_entry(cmor_table_t* table,
                      json_object *value) {
    extern int cmor_ntables;
    char msg[CMOR_MAX_STRING];
    int nCVId;
    int nbObjects = 0;
    cmor_CV_def_t *CV;
    cmor_CV_def_t *newCV;
    cmor_table_t *cmor_table;
    cmor_table = &cmor_tables[cmor_ntables];
    char szName[CMOR_MAX_STRING];

    cmor_add_traceback("cmor_CV_set_entry");
    cmor_is_setup();
/* -------------------------------------------------------------------- */
/* CV 0 contains number of objects                                      */
/* -------------------------------------------------------------------- */
    nbObjects++;
    newCV = (cmor_CV_def_t *) realloc(cmor_table->CV, sizeof(cmor_CV_def_t));

    cmor_table->CV = newCV;
    CV=newCV;
    cmor_CV_init(CV, cmor_ntables);
    cmor_table->CV->nbObjects=nbObjects;


/* -------------------------------------------------------------------- */
/*  Add all values and dictionaries to the M-tree                       */
/*                                                                      */
/*  {  { "a":[] }, {"a":""}, { "a":1 }, "a":"3", "b":"value" }....      */
/*                                                                      */
/*   Cv->objects->objects->list                                         */
/*              ->objects->string                                       */
/*              ->objects->integer                                      */
/*              ->integer                                               */
/*              ->string                                                */
/*     ->list                                                           */
/* -------------------------------------------------------------------- */

    json_object_object_foreach(value, CVName, CVValue) {
        nbObjects++;
        newCV = (cmor_CV_def_t *) realloc(cmor_table->CV,
                nbObjects * sizeof(cmor_CV_def_t));
        cmor_table->CV = newCV;
        nCVId = cmor_table->CV->nbObjects;

        CV = &cmor_table->CV[nCVId];

        cmor_CV_init(CV, cmor_ntables);
        cmor_table->CV->nbObjects++;

        if( CVName[0] == '#') {
            continue;
        }
        cmor_CV_set_att(CV, CVName, CVValue);
    }
    CV = &cmor_table->CV[0];
    CV->nbObjects = nbObjects;
    cmor_pop_traceback();
    return (0);
}
