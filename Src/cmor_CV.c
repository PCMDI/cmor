#include <string.h>
#include <stdio.h>
#include "cmor.h"
#include "json.h"
#include "json_tokener.h"
#include "arraylist.h"

/************************************************************************/
/*                      cmor_set_CV_def_att()                           */
/************************************************************************/
void cmor_set_CV_def_att(cmor_CV_def_t *CV,
                        char *szKey,
                        json_object *joValue) {
    json_bool bValue;
    double dValue;
    int nValue;
    char szValue[CMOR_MAX_STRING];
    array_list *pArray;
    int i, length;


    strcpy(CV->key, szKey);

    if( json_object_is_type( joValue, json_type_null) ) {
        printf("Will not save NULL JSON type from CV.json\n");

    } else if(json_object_is_type( joValue, json_type_boolean)) {
        bValue = json_object_get_boolean(joValue);
        CV->nValue = (int) bValue;

    } else if(json_object_is_type( joValue, json_type_double)) {
        dValue = json_object_get_double(joValue);
        CV->dValue = dValue;

    } else if(json_object_is_type( joValue, json_type_int)) {
        nValue = json_object_get_int(joValue);
        CV->nValue = nValue;
/* -------------------------------------------------------------------- */
/* if value is a JSON object, recursively call in this function          */
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
            cmor_init_CV_def(&CV->oValue[nCVId], nTableID);
            cmor_set_CV_def_att(&CV->oValue[nCVId], key, value  );
        }
        CV->nbObjects=nbObject;


    } else if(json_object_is_type( joValue, json_type_array)) {
        pArray = json_object_get_array(joValue);
        length = array_list_length(pArray);
        json_object *joItem;
        CV->anElements = length;
        for(int k=0; k<length; k++) {
            joItem = (json_object *) array_list_get_idx(pArray, k);
            strcpy(CV->aszValue[k], json_object_get_string(joItem));
        }

    } else if(json_object_is_type( joValue, json_type_string)) {
        strcpy(CV->cValue, json_object_get_string(joValue));
    }



}

/************************************************************************/
/*                         cmor_init_CV_def()                           */
/************************************************************************/
void cmor_init_CV_def( cmor_CV_def_t *CV, int table_id ) {
    int i;

    cmor_is_setup(  );
    cmor_add_traceback("cmor_init_CV_def");
    CV->table_id = table_id;
    CV->nbObjects = -1;
    CV->nValue = -1;
    CV->key[0] = '\0';
    CV->cValue[0] = '\0';
    CV->dValue=-9999.9;
    CV->oValue = NULL;
    for(int i=0; i< CMOR_MAX_JSON_ARRAY; i++) {
        CV->aszValue[i][0]='\0';
    }
    CV->anElements = -1;

    cmor_pop_traceback();
}

/************************************************************************/
/*                      cmor_print_CV_all()                             */
/************************************************************************/
void cmor_print_CV(cmor_CV_def_t *CV) {
    int nCVs;
    int i,j,k;

    if (CV != NULL) {
        if(CV->key[0] != '\0'){
            printf("key: %s \n", CV->key);
        }
        else {
            return;
        }

        if (CV->cValue[0] != '\0') {
            printf("value: %s\n", CV->cValue);
        }
        else if (CV->anElements != -1) {
            printf("value: [\n");
            for (k = 0; k < CV->anElements; k++) {
                printf("value: %s\n", CV->aszValue[k]);
            }
            printf("        ]\n");

        }
        else if (CV->nValue != -1) {
            printf("value: %d\n", CV->nValue);
        }
        else if (CV->oValue != NULL) {
            printf("nbObjects=%d\n", CV->nbObjects);
            for(k=0; k< CV->nbObjects; k++){
                cmor_print_CV(&CV->oValue[k]);
            }
        }

    }
}

/************************************************************************/
/*                      cmor_print_CV_all()                             */
/************************************************************************/
void cmor_print_CV_all() {
    int j,i,k;
    cmor_CV_def_t *CV;
    int nCVs;
/* -------------------------------------------------------------------- */
/* Parse tree and print key values.                                     */
/* -------------------------------------------------------------------- */

    for( i = 0; i < CMOR_MAX_TABLES; i++ ) {
        printf("table %s\n", cmor_tables[i].szTable_id);
        nCVs = cmor_tables[i].nCVs;
        for( j=0; j<= nCVs; j++ ) {
            CV = &cmor_tables[i].CV[j];
            cmor_print_CV(CV);

        }
    }
}

/************************************************************************/
/*                     cmor_CV_search_key()                             */
/************************************************************************/
cmor_CV_def_t * cmor_CV_search_key(cmor_CV_def_t *CV, char *key){
    int i;

    for(i = 0; i< CV->nbObjects; i++) {
        if(strcmp(CV[i].key, key)){
            return(CV);
        }
    }
    return(NULL);
}
/************************************************************************/
/*                      cmor_CV_get_value()                             */
/************************************************************************/
char *cmor_CV_get_value(cmor_CV_def_t *CV, char *key) {
        char *szValue;
        int i,j;
        int nCVs;

        for( i = 0; i < CMOR_MAX_TABLES; i++ ) {
            printf("table %s\n", cmor_tables[i].szTable_id);
            nCVs = cmor_tables[i].nCVs;
            for( j=0; j<= nCVs; j++ ) {
                CV = &cmor_tables[i].CV[j];
                cmor_print_CV(CV);

            }
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


