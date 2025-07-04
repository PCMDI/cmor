#define _XOPEN_SOURCE

#define _GNU_SOURCE
#include <string.h>
#include <time.h>
#include <regex.h>
#include "cmor.h"
#include <netcdf.h>
#include <json-c/json.h>
#include <json-c/json_tokener.h>
#include <json-c/arraylist.h>
#include "libgen.h"
#include "math.h"
#ifndef REG_NOERROR
#define REG_NOERROR 0
#endif
extern void cdCompAdd(cdCompTime comptime,
                      double value, cdCalenType calendar, cdCompTime * result);

extern void cdCompAddMixed(cdCompTime ct, double value, cdCompTime * result);
/************************************************************************/
/*                        cmor_CV_set_att()                             */
/************************************************************************/
void cmor_CV_set_att(cmor_CV_def_t * CV, char *szKey, json_object * joValue)
{
    json_bool bValue;
    double dValue;
    int nValue;
    array_list *pArray;
    int k, length;

    strcpy(CV->key, szKey);

    if (json_object_is_type(joValue, json_type_null)) {
        printf("Will not save NULL JSON type from CV.json\n");

    } else if (json_object_is_type(joValue, json_type_boolean)) {
        bValue = json_object_get_boolean(joValue);
        CV->nValue = (int)bValue;
        CV->type = CV_integer;

    } else if (json_object_is_type(joValue, json_type_double)) {
        dValue = json_object_get_double(joValue);
        CV->dValue = dValue;
        CV->type = CV_double;

    } else if (json_object_is_type(joValue, json_type_int)) {
        nValue = json_object_get_int(joValue);
        CV->nValue = nValue;
        CV->type = CV_integer;
/* -------------------------------------------------------------------- */
/* if value is a JSON object, recursively call in this function         */
/* -------------------------------------------------------------------- */
    } else if (json_object_is_type(joValue, json_type_object)) {

        int nCVId = -1;
        int nbObject = 0;
        cmor_CV_def_t *oValue;
        int nTableID = CV->table_id;    /* Save Table ID */

        json_object_object_foreach(joValue, key, value) {
            nbObject++;
            oValue = (cmor_CV_def_t *) realloc(CV->oValue,
                                               sizeof(cmor_CV_def_t) *
                                               nbObject);
            CV->oValue = oValue;

            nCVId++;
            cmor_CV_init(&CV->oValue[nCVId], nTableID);
            cmor_CV_set_att(&CV->oValue[nCVId], key, value);
        }
        CV->nbObjects = nbObject;
        CV->type = CV_object;

    } else if (json_object_is_type(joValue, json_type_array)) {
        int i;
        pArray = json_object_get_array(joValue);
        length = array_list_length(pArray);
        CV->aszValue = (char **)  malloc(length * sizeof(char**));
        for(i=0; i < length; i++) {
            CV->aszValue[i] = (char *) malloc(sizeof(char) * CMOR_MAX_STRING);
        }
        json_object *joItem;
        CV->anElements = length;
        for (k = 0; k < length; k++) {
            joItem = (json_object *) array_list_get_idx(pArray, k);
            strcpy(CV->aszValue[k], json_object_get_string(joItem));
        }
        CV->type = CV_stringarray;

    } else if (json_object_is_type(joValue, json_type_string)) {
        strcpy(CV->szValue, json_object_get_string(joValue));
        CV->type = CV_string;
    }

}

/************************************************************************/
/*                           cmor_CV_init()                             */
/************************************************************************/
void cmor_CV_init(cmor_CV_def_t * CV, int table_id)
{

    cmor_is_setup();
    cmor_add_traceback("_init_CV_def");
    CV->table_id = table_id;
    CV->type = CV_undef;        //undefined
    CV->nbObjects = -1;
    CV->nValue = -1;
    CV->key[0] = '\0';
    CV->szValue[0] = '\0';
    CV->dValue = -9999.9;
    CV->oValue = NULL;
    CV->aszValue = NULL;
    CV->anElements = -1;

    cmor_pop_traceback();
}

/************************************************************************/
/*                         cmor_CV_print()                              */
/************************************************************************/
void cmor_CV_print(cmor_CV_def_t * CV)
{
    int k;

    if (CV != NULL) {
        if (CV->key[0] != '\0') {
            printf("key: %s \n", CV->key);
        } else {
            return;
        }
        switch (CV->type) {

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
              for (k = 0; k < CV->nbObjects; k++) {
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
void cmor_CV_printall()
{
    int j, i;
    cmor_CV_def_t *CV;
    int nCVs;
/* -------------------------------------------------------------------- */
/* Parse tree and print key values.                                     */
/* -------------------------------------------------------------------- */
    for (i = 0; i < CMOR_MAX_TABLES; i++) {
        if (cmor_tables[i].CV != NULL) {
            printf("table %s\n", cmor_tables[i].szTable_id);
            nCVs = cmor_tables[i].CV->nbObjects;
            for (j = 0; j <= nCVs; j++) {
                CV = &cmor_tables[i].CV[j];
                cmor_CV_print(CV);
            }
        }
    }
}
/************************************************************************/
/*               cmor_CV_set_dataset_attr_from_key()                    */
/************************************************************************/
cmor_CV_def_t *cmor_CV_set_dataset_attr_from_key(cmor_CV_def_t * CV,
                                                    char *key)
{
    int i;
    cmor_CV_def_t *searchCV;
    int nbCVs = -1;
    cmor_add_traceback("_CV_search_child_key");

    nbCVs = CV->nbObjects;

    // Look at this objects
    if (strcmp(CV->key, key) == 0) {
        cmor_pop_traceback();
        return (CV);
    }
    // Look at each of object key
    for (i = 0; i < nbCVs; i++) {
        // Is there a branch on that object?
        if (&CV->oValue[i] != NULL) {
            searchCV = cmor_CV_set_dataset_attr_from_key(&CV->oValue[i], key);
            if (searchCV != NULL) {
                cmor_pop_traceback();
                return (searchCV);
            }
        }
    }
    cmor_pop_traceback();
    return (NULL);
}

/************************************************************************/
/*                  cmor_CV_search_child_key()                          */
/************************************************************************/
cmor_CV_def_t *cmor_CV_search_child_key(cmor_CV_def_t * CV, char *key)
{
    int i;
    cmor_CV_def_t *searchCV;
    int nbCVs = -1;
    cmor_add_traceback("_CV_search_child_key");

    nbCVs = CV->nbObjects;

    // Look at this objects
    if (strcmp(CV->key, key) == 0) {
        cmor_pop_traceback();
        return (CV);
    }
    // Look at each of object key
    for (i = 0; i < nbCVs; i++) {
        // Is there a branch on that object?
        if (&CV->oValue[i] != NULL) {
            searchCV = cmor_CV_search_child_key(&CV->oValue[i], key);
            if (searchCV != NULL) {
                cmor_pop_traceback();
                return (searchCV);
            }
        }
    }
    cmor_pop_traceback();
    return (NULL);
}

/************************************************************************/
/*                     cmor_CV_rootsearch()                             */
/************************************************************************/
cmor_CV_def_t *cmor_CV_rootsearch(cmor_CV_def_t * CV, char *key)
{
    int i;
    int nbCVs = -1;
    cmor_add_traceback("_CV_rootsearch");

    // Look at first objects
    if (strcmp(CV->key, key) == 0) {
        cmor_pop_traceback();
        return (CV);
    }
    // Is there more than 1 object?
    if (CV->nbObjects != -1) {
        nbCVs = CV->nbObjects;
    }
    // Look at each of object key
    for (i = 1; i < nbCVs; i++) {
        if (strcmp(CV[i].key, key) == 0) {
            cmor_pop_traceback();
            return (&CV[i]);
        }
    }
    cmor_pop_traceback();
    return (NULL);
}

/************************************************************************/
/*                      cmor_CV_get_value()                             */
/************************************************************************/
char *cmor_CV_get_value(cmor_CV_def_t * CV, char *key)
{

    switch (CV->type) {
      case CV_string:
          return (CV->szValue);
          break;

      case CV_integer:
          sprintf(CV->szValue, "%d", CV->nValue);
          break;

      case CV_stringarray:
          return (CV->aszValue[0]);
          break;

      case CV_double:
          sprintf(CV->szValue, "%lf", CV->dValue);
          break;

      case CV_object:
          return (NULL);
          break;

      case CV_undef:
          CV->szValue[0] = '\0';
          break;
    }

    return (CV->szValue);
}

/************************************************************************/
/*                         cmor_CV_free()                               */
/************************************************************************/
void cmor_CV_free(cmor_CV_def_t * CV)
{
    int k;
    int i;
    int length;
/* -------------------------------------------------------------------- */
/* Free allocated double string array                                   */
/* -------------------------------------------------------------------- */
    length = CV->anElements;
    if(length != 0) {
        for(i=0; i < length; i++) {
            free(CV->aszValue[i]);
        }
        free(CV->aszValue);
    }
/* -------------------------------------------------------------------- */
/* Recursively go down the tree and free branch                         */
/* -------------------------------------------------------------------- */
    if (CV->oValue != NULL) {
        for (k = 0; k < CV->nbObjects; k++) {
            cmor_CV_free(&CV->oValue[k]);
        }
        free(CV->oValue);
        CV->oValue = NULL;
    }
}

/************************************************************************/
/*                    cmor_CV_checkFurtherInfoURL()                     */
/************************************************************************/
int cmor_CV_checkFurtherInfoURL(int nVarRefTblID)
{
    char szFurtherInfoURLTemplate[CMOR_MAX_STRING];
    char szFurtherInfoURL[CMOR_MAX_STRING];
    char copyURL[CMOR_MAX_STRING];
    char szFurtherInfoBaseURL[CMOR_MAX_STRING];
    char szFurtherInfoFileURL[CMOR_MAX_STRING];
    char szValue[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];

    char *baseURL;
    char *fileURL;
    char *szToken;

    szFurtherInfoURL[0] = '\0';
    szFurtherInfoFileURL[0] = '\0';
    szFurtherInfoBaseURL[0] = '\0';
    cmor_is_setup();
    cmor_add_traceback("_CV_checkFurtherInfoURL");

/* -------------------------------------------------------------------- */
/* If the template is an emtpy string, then skip this check.            */
/* -------------------------------------------------------------------- */
    if (cmor_current_dataset.furtherinfourl[0] == '\0') {
        cmor_pop_traceback();
        return (0);
    }

/* -------------------------------------------------------------------- */
/* Retrieve default Further URL info                                    */
/* -------------------------------------------------------------------- */
    strncpy(szFurtherInfoURLTemplate, cmor_current_dataset.furtherinfourl,
            CMOR_MAX_STRING);

/* -------------------------------------------------------------------- */
/*    If this is a string with no token we have nothing to do.          */
/* -------------------------------------------------------------------- */
    szToken = strtok(szFurtherInfoURLTemplate, "<>");
    if (szToken == NULL) {
        cmor_handle_error_variadic(
            "The further info URL value of \"%s\" is invalid. \n! ",
            CMOR_NORMAL,
            szFurtherInfoURLTemplate);
        cmor_pop_traceback();
        return (-1);
    }

    if (strcmp(szToken, cmor_current_dataset.furtherinfourl) == 0) {
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_FURTHERINFOURL,
                                                cmor_current_dataset.furtherinfourl, 0);
        cmor_pop_traceback();
        return (0);
    }

    strncpy(szFurtherInfoURLTemplate, cmor_current_dataset.furtherinfourl,
            CMOR_MAX_STRING);
/* -------------------------------------------------------------------- */
/*    Separate path template from file template.                        */
/* -------------------------------------------------------------------- */
    strcpy(copyURL, szFurtherInfoURLTemplate);
    baseURL = dirname(copyURL);
    cmor_CreateFromTemplate(nVarRefTblID, baseURL, szFurtherInfoBaseURL, "/");

    strcpy(copyURL, szFurtherInfoURLTemplate);
    fileURL = basename(copyURL);

    cmor_CreateFromTemplate(nVarRefTblID, fileURL, szFurtherInfoFileURL, ".");

    strncpy(szFurtherInfoURL, szFurtherInfoBaseURL, CMOR_MAX_STRING);
    strcat(szFurtherInfoURL, "/");
    strncat(szFurtherInfoURL, szFurtherInfoFileURL,
            CMOR_MAX_STRING - strlen(szFurtherInfoURL));

    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_FURTHERINFOURL) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_FURTHERINFOURL, szValue);
        if (strncmp(szFurtherInfoURL, szValue, CMOR_MAX_STRING) != 0) {
            cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
            cmor_handle_error_variadic(
                     "The further info in attribute does not match "
                     "the one found in your Controlled Vocabulary(CV) File. \n! "
                     "We found \"%s\" and \n! "
                     "CV requires \"%s\" \n! "
                     "Check your Controlled Vocabulary file \"%s\".\n! ",
                     CMOR_WARNING, szValue, szFurtherInfoURL, CV_Filename);
            cmor_pop_traceback();
            return (-1);

        }

    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_FURTHERINFOURL,
                                            szFurtherInfoURL, 0);
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                            get_CV_Error()                            */
/************************************************************************/
int get_CV_Error()
{
    return (CV_ERROR);
}

/************************************************************************/
/*                      cmor_CV_checkSourceType()                       */
/************************************************************************/
int cmor_CV_checkSourceType(cmor_CV_def_t * CV_exp, char *szExptID)
{
    int nObjects;
    cmor_CV_def_t *CV_exp_attr;
    char szAddSourceType[CMOR_MAX_STRING];
    char szReqSourceType[CMOR_MAX_STRING];
    char szAddSourceTypeCpy[CMOR_MAX_STRING];
    char szReqSourceTypeCpy[CMOR_MAX_STRING];

    char szSourceType[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int i, j;
    char *szTokenRequired;
    char *szTokenAdd;
    int nbSourceType;
    char *ptr;
    int nbGoodType;
    regex_t regex;
    int reti;

    cmor_add_traceback("_CV_checkSourceType");

    szAddSourceType[0] = '\0';
    szReqSourceType[0] = '\0';
    szAddSourceTypeCpy[0] = '\0';
    szReqSourceTypeCpy[0] = '\0';
    szSourceType[0] = '\0';
    nbGoodType = 0;
    nbSourceType = -1;
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);

    szAddSourceType[0] = '\0';
    nObjects = CV_exp->nbObjects;
    // Parse all experiment attributes get source type related attr.
    for (i = 0; i < nObjects; i++) {
        CV_exp_attr = &CV_exp->oValue[i];
        if (strcmp(CV_exp_attr->key, CV_EXP_ATTR_ADDSOURCETYPE) == 0) {
            for (j = 0; j < CV_exp_attr->anElements; j++) {
                strcat(szAddSourceType, CV_exp_attr->aszValue[j]);
                strcat(szAddSourceType, " ");
                strcat(szAddSourceTypeCpy, CV_exp_attr->aszValue[j]);
                strcat(szAddSourceTypeCpy, " ");

            }
            continue;
        }
        if (strcmp(CV_exp_attr->key, CV_EXP_ATTR_REQSOURCETYPE) == 0) {
            for (j = 0; j < CV_exp_attr->anElements; j++) {
                strcat(szReqSourceType, CV_exp_attr->aszValue[j]);
                strcat(szReqSourceType, " ");
                strcat(szReqSourceTypeCpy, CV_exp_attr->aszValue[j]);
                strcat(szReqSourceTypeCpy, " ");

            }
            continue;
        }

    }
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SOURCE_TYPE) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_TYPE, szSourceType);

        // Count how many are token  we have.
        ptr = szSourceType;
        if (szSourceType[0] != '\0') {
            nbSourceType = 1;
        } else {
            cmor_pop_traceback();
            return (-1);
        }
        while ((ptr = strpbrk(ptr, " ")) != NULL) {
            nbSourceType++;
            ptr++;
        }
    }

    szTokenRequired = strtok(szReqSourceType, " ");
    while (szTokenRequired != NULL) {
        reti = regcomp(&regex, szTokenRequired, REG_EXTENDED);
        if (reti) {
            cmor_handle_error_variadic(
                     "You regular expression \"%s\" is invalid. \n! "
                     "Please refer to the CMIP6 documentations.\n! ",
                     CMOR_NORMAL, szTokenRequired);
            regfree(&regex);
            cmor_pop_traceback();
            return (-1);
        }
        /* -------------------------------------------------------------------- */
        /*        Execute regular expression                                    */
        /* -------------------------------------------------------------------- */
        reti = regexec(&regex, szSourceType, 0, NULL, 0);
        if (reti == REG_NOMATCH) {
            cmor_handle_error_variadic(
                     "The following source type(s) \"%s\" are required and\n! "
                     "some source type(s) could not be found in your "
                     "input file. \n! "
                     "Your file contains a source type of \"%s\".\n! "
                     "Check your Controlled Vocabulary file \"%s\".\n! ",
                     CMOR_NORMAL, szReqSourceTypeCpy, szSourceType, CV_Filename);
            regfree(&regex);
        } else {
            nbGoodType++;
        }
        regfree(&regex);
        szTokenRequired = strtok(NULL, " ");
    }

    szTokenAdd = strtok(szAddSourceType, " ");
    while (szTokenAdd != NULL) {
        // Is the token "CHEM"?
        if (strcmp(szTokenAdd, "CHEM") == 0) {
            reti = regcomp(&regex, szTokenAdd, REG_EXTENDED);
            if (regexec(&regex, szSourceType, 0, NULL, 0) == REG_NOERROR) {
                nbGoodType++;
            };
        } else if (strcmp(szTokenAdd, "AER") == 0) {
            regfree(&regex);
            // Is the token "AER"?
            reti = regcomp(&regex, szTokenAdd, REG_EXTENDED);
            if (regexec(&regex, szSourceType, 0, NULL, 0) == REG_NOERROR) {
                nbGoodType++;
            };
        } else {
            regfree(&regex);
            // Is the token in the list of AddSourceType?
            reti = regcomp(&regex, szTokenAdd, REG_EXTENDED);
            if (regexec(&regex, szSourceType, 0, NULL, 0) == REG_NOERROR) {
                nbGoodType++;
            };
        }

        szTokenAdd = strtok(NULL, " ");
        regfree(&regex);
    }

    if (nbGoodType != nbSourceType) {
        cmor_handle_error_variadic(
                 "You source_type attribute contains invalid source types\n! "
                 "Your source type is set to \"%s\".  The required source types\n! "
                 "are \"%s\" and possible additional source types are \"%s\" \n! "
                 "Check your Controlled Vocabulary file \"%s\".\n! ",
                 CMOR_NORMAL, szSourceType, szReqSourceTypeCpy, szAddSourceTypeCpy,
                 CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }

    cmor_pop_traceback();
    return (0);

}

/************************************************************************/
/*                      cmor_CV_checkSourceID()                         */
/************************************************************************/
int cmor_CV_checkSourceID(cmor_CV_def_t * CV)
{
    cmor_CV_def_t *CV_source_ids;
    cmor_CV_def_t *CV_source_id = NULL;

    char szSource_ID[CMOR_MAX_STRING];
    char szSource[CMOR_MAX_STRING];
    char szSubstring[CMOR_MAX_STRING];
    char *pos;

    int nLen;
    char CV_Filename[CMOR_MAX_STRING];
    char CMOR_Filename[CMOR_MAX_STRING];
    int rc;
    int i;
    int j = 0;

    cmor_is_setup();
    cmor_add_traceback("_CV_checkSourceID");
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    rc = cmor_has_cur_dataset_attribute(CMOR_INPUTFILENAME);
    if (rc == 0) {
        cmor_get_cur_dataset_attribute(CMOR_INPUTFILENAME, CMOR_Filename);
    } else {
        CMOR_Filename[0] = '\0';
    }
    
/* -------------------------------------------------------------------- */
/*  Find experiment_ids dictionary in Controlled Vocabulary                */
/* -------------------------------------------------------------------- */
    CV_source_ids = cmor_CV_rootsearch(CV, CV_KEY_SOURCE_IDS);

    if (CV_source_ids == NULL) {
        cmor_handle_error_variadic(
                 "Your \"source_ids\" key could not be found in\n! "
                 "your Controlled Vocabulary file.(%s)\n! ", CMOR_NORMAL, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    // retrieve source_id
    rc = cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID, szSource_ID);
    if (rc != 0) {
        cmor_handle_error_variadic(
                 "Your \"%s\" is not defined, check your required attributes\n! "
                 "See Controlled Vocabulary JSON file.(%s)\n! ",
                 CMOR_NORMAL, GLOBAL_ATT_SOURCE_ID, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
    // Validate  source_id
    for (i = 0; i < CV_source_ids->nbObjects; i++) {
        CV_source_id = &CV_source_ids->oValue[i];
        if (strncmp(CV_source_id->key, szSource_ID, CMOR_MAX_STRING) == 0) {
            // Make sure that "source" exist.
            if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SOURCE) != 0) {
                cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SOURCE,
                                                        CV_source_id->szValue, 1);
            }
            // Check source with experiment_id label.
            rc = cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE, szSource);
            if(CV_source_id->nbObjects < 1) {
                cmor_handle_error_variadic(
                         "You did not define a %s section in your source_id %s.\n! \n! \n! "
                         "See Controlled Vocabulary JSON file. (%s)\n! ",
                         CMOR_CRITICAL, CV_KEY_SOURCE_LABEL, szSource_ID, CV_Filename);
                return(-1);
            }
            for (j = 0; j < CV_source_id->nbObjects; j++) {
                if (strcmp(CV_source_id->oValue[j].key, CV_KEY_SOURCE_LABEL) ==
                    0) {
                    break;
                }
            }
            if (j == CV_source_id->nbObjects) {
                cmor_handle_error_variadic(
                         "Could not find %s string in source_id section.\n! \n! \n! "
                         "See Controlled Vocabulary JSON file. (%s)\n! ",
                         CMOR_CRITICAL, CV_KEY_SOURCE_LABEL, CV_Filename);
                return(-1);
            }
            pos = strchr(CV_source_id->oValue[j].szValue, ')');
            strncpy(szSubstring, CV_source_id->oValue[j].szValue,
                    CMOR_MAX_STRING);
            nLen = strlen(CV_source_id->oValue[j].szValue);
            if (pos != 0) {
                nLen = pos - CV_source_id->oValue[j].szValue + 1;
            }
            szSubstring[nLen] = '\0';
            if (strncmp(szSubstring, szSource, nLen) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" with value \n! \"%s\" "
                         "will be replaced with "
                         "value \n! \"%s\".\n! \n! \n!  "
                         "See Controlled Vocabulary JSON file.(%s)\n! ",
                         CMOR_WARNING, GLOBAL_ATT_SOURCE, szSource,
                         CV_source_id->oValue[j].szValue, CV_Filename);
            }
            break;
        }
    }
    // We could not found the Source ID in the CV file
    if (i == CV_source_ids->nbObjects) {

        cmor_handle_error_variadic(
                 "The source_id, \"%s\", found in your \n! "
                 "input file (%s) could not be found in \n! "
                 "your Controlled Vocabulary file. (%s) \n! \n! "
                 "Please correct your input file by using a valid source_id listed in your MIP tables' CV file.\n! "
                 "To add a new source_id to the %s file, open a new issue in the\n! "
                 "table's Github repository. Managed project CMOR and MIP tables are listed at\n! "
                 "https://wcrp-cmip.github.io/WGCM_Infrastructure_Panel/cmor_and_mip_tables.html. \n! "
                 "Contact \"pcmdi-cmip@llnl.gov\" for additional guidance.  \n! \n! "
                 "See \"http://cmor.llnl.gov/mydoc_cmor3_CV/\" for further information about\n! "
                 "the \"source_id\" and \"source\" global attributes.  ", 
                 CMOR_NORMAL, szSource_ID, CMOR_Filename, CV_Filename, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    // Set/replace attribute.
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SOURCE_ID,
                                            CV_source_id->key, 1);
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SOURCE,
                                            CV_source_id->oValue[j].szValue, 1);

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       CV_VerifyNBElement()                           */
/************************************************************************/
int CV_VerifyNBElement(cmor_CV_def_t * CV)
{
    char CV_Filename[CMOR_MAX_STRING];
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    cmor_add_traceback("_CV_VerifyNBElement");
//    printf("**** CV->key: %s\n", CV->key);
//    printf("**** CV->anElement: %d\n", CV->anElements);
//    printf("**** CV->asValue: %s\n", CV->aszValue[0]);
//    printf("**** CV->szValue: %s\n", CV->szValue);

    if (CV->anElements > 1) {
        cmor_handle_error_variadic(
                 "Your %s has more than 1 element\n! "
                 "only the first one will be used\n! "
                 "Check your Controlled Vocabulary file \"%s\".\n! ",
                 CMOR_NORMAL, CV->key, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    } else if (CV->anElements == -1) {
        cmor_handle_error_variadic(
                 "Your %s has more than 0 element\n! "
                 "Check your Controlled Vocabulary file \"%s\".\n! ",
                 CMOR_NORMAL, CV->key, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       CV_CompareNoParent()                           */
/************************************************************************/
int CV_CompareNoParent(char *szKey)
{
    char szValue[CMOR_MAX_STRING];
    cmor_add_traceback("_CV_CompareNoParent");

    if (cmor_has_cur_dataset_attribute(szKey) == 0) {
        cmor_get_cur_dataset_attribute(szKey, szValue);
        if (strcmp(szValue, NO_PARENT) != 0) {
            cmor_handle_error_variadic(
                     "Your input attribute %s with value \"%s\" \n! "
                     "will be replaced with value \"%s\".\n! ", 
                     CMOR_WARNING, szKey, szValue, NO_PARENT);
            cmor_set_cur_dataset_attribute_internal(szKey, NO_PARENT, 1);
        }
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                            InArray()                                 */
/************************************************************************/
int CV_IsStringInArray(cmor_CV_def_t * CV, char *szValue)
{
    int nElements;
    int i;
    int found = 0;
    cmor_add_traceback("_CV_InArray");
    nElements = CV->anElements;
    for (i = 0; i < nElements; i++) {
        if (strcmp(CV->aszValue[i], szValue) == 0) {
            found = 1;
            break;
        }
    }
    cmor_pop_traceback();
    return (found);
}

/************************************************************************/
/*                     cmor_CV_checkSubExpID()                          */
/************************************************************************/
int cmor_CV_checkSubExpID(cmor_CV_def_t * CV)
{
    cmor_CV_def_t *CV_experiment_id;
    cmor_CV_def_t *CV_experiment;
    cmor_CV_def_t *CV_experiment_sub_exp_id;
    cmor_CV_def_t *CV_sub_experiment_id;
    cmor_CV_def_t *CV_sub_experiment_id_key;

    char szExperiment_ID[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    char szSubExptID[CMOR_MAX_STRING];
    char szValue[CMOR_MAX_STRING];
    char szVariant[CMOR_MAX_STRING];

    int ierr = 0;

    cmor_add_traceback("_CV_checkSubExperiment");
    // Initialize variables
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    ierr = cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENTID, szExperiment_ID);
    if (ierr != 0) {
        cmor_handle_error_variadic(
                 "Your \"%s\" is not defined, check your required attributes\n! "
                 "See Controlled Vocabulary JSON file.(%s)\n! ",
                 CMOR_NORMAL, GLOBAL_ATT_EXPERIMENTID, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
    // Look for sub_experiment_id section
    CV_sub_experiment_id = cmor_CV_rootsearch(CV, CV_KEY_SUB_EXPERIMENT_ID);
    if (CV_sub_experiment_id == NULL) {
        cmor_handle_error_variadic(
                 "Your \"sub_experiment_id\" key could not be found in\n! "
                 "your Controlled Vocabulary file.(%s)\n! ", CMOR_NORMAL, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
    // Look for experiment_id section
    CV_experiment_id = cmor_CV_rootsearch(CV, CV_KEY_EXPERIMENT_ID);
    if (CV_experiment_id == NULL) {
        cmor_handle_error_variadic(
                 "Your \"experiment_id\" key could not be found in\n! "
                 "your Controlled Vocabulary file.(%s)\n! ", CMOR_NORMAL, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
    // Get specified experiment
    CV_experiment = cmor_CV_search_child_key(CV_experiment_id, szExperiment_ID);
    if (CV_experiment == NULL) {
        cmor_handle_error_variadic(
                 "Your experiment_id \"%s\" defined in your input file\n! "
                 "could not be found in your Controlled Vocabulary file.(%s)\n! ",
                 CMOR_NORMAL, szExperiment_ID, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
    // sub_experiment_id
    CV_experiment_sub_exp_id = cmor_CV_search_child_key(CV_experiment,
                                                        GLOBAL_ATT_SUB_EXPT_ID);
    if (CV_experiment_sub_exp_id == NULL) {
        cmor_handle_error_variadic(
                 "Your \"%s\" defined in your input file\n! "
                 "could not be found in your Controlled Vocabulary file.(%s)\n! ",
                 CMOR_NORMAL, GLOBAL_ATT_SUB_EXPT_ID, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
    // Check sub_experiment_id value
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT_ID) != 0) {
        // sub_experiment_id not found and set to "none"
        if (CV_IsStringInArray(CV_experiment_sub_exp_id, NONE)) {
            cmor_handle_error_variadic(
                     "Your input attribute \"%s\" was not defined and \n! "
                     "will be set to \"%s\"\n! "
                     "as defined in your Controlled Vocabulary file \"%s\".\n! ",
                     CMOR_WARNING, GLOBAL_ATT_SUB_EXPT_ID, NONE, CV_Filename);
            cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SUB_EXPT_ID,
                                                    NONE, 1);
        } else {
            // can't be "none".
            cmor_handle_error_variadic(
                     "Your input attribute \"%s\" is not defined properly \n! "
                     "for your experiment \"%s\" \n! \n! "
                     "See Controlled Vocabulary JSON file.(%s)\n! ",
                     CMOR_NORMAL, GLOBAL_ATT_SUB_EXPT_ID, szExperiment_ID, CV_Filename);
            cmor_pop_traceback();
            return (-1);

        }

    } else {
        // sub_experiment_id has been defined!
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT_ID, szSubExptID);
        // cannot be found in CV
        if (!CV_IsStringInArray(CV_experiment_sub_exp_id, szSubExptID)) {
            // only 1 element in list set it!
            if (CV_experiment_sub_exp_id->anElements == 1) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" defined as \"%s\" "
                         "will be replaced with \n! "
                         "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                         CMOR_WARNING, GLOBAL_ATT_SUB_EXPT_ID, szSubExptID,
                         CV_experiment_sub_exp_id->aszValue[0]);
                cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SUB_EXPT_ID,
                                                        CV_experiment_sub_exp_id->aszValue
                                                        [0], 1);

            } else {
                // too many options.
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined properly \n! "
                         "for your experiment \"%s\"\n! "
                         "There is more than 1 option for this sub_experiment.\n! "
                         "See Controlled Vocabulary JSON file.(%s)\n! ",
                         CMOR_NORMAL, GLOBAL_ATT_SUB_EXPT_ID, szExperiment_ID, CV_Filename);
                cmor_pop_traceback();
                return (-1);
            }
        }
    }
    // sub_experiment has not been defined!
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT) != 0) {
        cmor_handle_error_variadic(
                 "Your input attribute \"%s\" was not defined and \n! "
                 "will be set to \"%s\" \n! "
                 "as defined in your Controlled Vocabulary file \"%s\".\n! ",
                 CMOR_WARNING, GLOBAL_ATT_SUB_EXPT, NONE, CV_Filename);
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SUB_EXPT, NONE, 1);
    } else {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT, szValue);
        CV_sub_experiment_id_key =
          cmor_CV_search_child_key(CV_sub_experiment_id, szSubExptID);
        if (CV_sub_experiment_id_key == NULL) {
            cmor_handle_error_variadic(
                     "Your \"sub_experiment\" text describing  \n! "
                     "sub_experiment_id \"%s\" could not be found in \n! "
                     "your Controlled Vocabulary file.(%s)\n! ", 
                     CMOR_NORMAL, szSubExptID, CV_Filename);
            cmor_pop_traceback();
            return (-1);
        }

        if (strcmp(szValue, CV_sub_experiment_id_key->szValue) != 0) {
            cmor_handle_error_variadic(
                     "Your input attribute \"%s\" defined as \"%s\" "
                     "will be replaced with \n! "
                     "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                     CMOR_WARNING, GLOBAL_ATT_SUB_EXPT, szValue,
                     CV_sub_experiment_id_key->szValue);
            cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SUB_EXPT,
                                                    CV_sub_experiment_id_key->szValue,
                                                    1);
        }
    }
    // append sub-experiment_id
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT_ID) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT_ID, szValue);
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_MEMBER_ID, szVariant);

        if (strcmp(szValue, NONE) != 0) {
            // not already in variant
            if (strstr(szVariant, szValue) == NULL) {
                strcat(szValue, "-");
                strcat(szValue, szVariant);
                cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_MEMBER_ID,
                                                        szValue, 1);
            }
        }
    }

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                    cmor_CV_checkParentExpID()                        */
/************************************************************************/
int cmor_CV_checkParentExpID(cmor_CV_def_t * CV)
{
    cmor_CV_def_t *CV_experiment_ids;
    cmor_CV_def_t *CV_experiment;
    cmor_CV_def_t *CV_parent_exp_id;
    cmor_CV_def_t *CV_parent_activity_id;
    cmor_CV_def_t *CV_source_id;
    cmor_CV_def_t *CV_source;

    char szValue[CMOR_MAX_STRING];
    char szParentExpValue[CMOR_MAX_STRING];
    char szExperiment_ID[CMOR_MAX_STRING];
    char szBranchMethod[CMOR_MAX_STRING];
    char szBranchTimeInChild[CMOR_MAX_STRING];
    char szBranchTimeInParent[CMOR_MAX_STRING];
    double dBranchTimeInChild;
    double dBranchTimeInParent;
    char szParentSourceId[CMOR_MAX_STRING];
    char szParentTimeUnits[CMOR_MAX_STRING];
    char szParentVariantLabel[CMOR_MAX_STRING];
    char *parent_mip_era;
    regex_t regex;

    char CV_Filename[CMOR_MAX_STRING];
    int rc;
    int ierr = 0;

    szParentExpValue[0] = '\0';
    cmor_add_traceback("_CV_checkParentExpID");

    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    ierr = cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENTID, szExperiment_ID);
    if (ierr != 0) {
        cmor_handle_error_variadic(
                 "Your \"%s\" is not defined, check your required attributes\n! "
                 "See Controlled Vocabulary JSON file.(%s)\n! ",
                 CMOR_NORMAL, GLOBAL_ATT_EXPERIMENTID, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
    // Look for experiment_id section
    CV_experiment_ids = cmor_CV_rootsearch(CV, CV_KEY_EXPERIMENT_ID);
    if (CV_experiment_ids == NULL) {
        cmor_handle_error_variadic(
                 "Your \"experiment_id\" key could not be found in\n! "
                 "your Controlled Vocabulary file.(%s)\n! ", CMOR_NORMAL, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    // Get specified experiment
    CV_experiment = cmor_CV_search_child_key(CV_experiment_ids,
                                             szExperiment_ID);
    if (CV_experiment == NULL) {
        cmor_handle_error_variadic(
                 "Your experiment_id \"%s\" defined in your input file\n! "
                 "could not be found in your Controlled Vocabulary file.(%s)\n! ",
                 CMOR_NORMAL, szExperiment_ID, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
    // Do we have a parent_experiment_id?
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_PARENT_EXPT_ID) != 0) {
        CV_parent_exp_id = cmor_CV_search_child_key(CV_experiment,
                                                    PARENT_EXPERIMENT_ID);
        if (CV_IsStringInArray(CV_parent_exp_id, NO_PARENT)) {
            cmor_handle_error_variadic(
                    "Your input attribute \"%s\" defined as \"\" "
                    "will be replaced with \n! "
                    "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                    CMOR_WARNING, PARENT_EXPERIMENT_ID, NO_PARENT);
            cmor_set_cur_dataset_attribute_internal(PARENT_EXPERIMENT_ID,
                                                    NO_PARENT, 1);
        } else {
            cmor_handle_error_variadic(
                     "Your input attribute \"%s\" is not defined properly \n! "
                     "for your experiment \"%s\"\n!\n! "
                     "See Controlled Vocabulary JSON file.(%s)\n! ",
                     CMOR_NORMAL, GLOBAL_ATT_PARENT_EXPT_ID, CV_experiment->key,
                     CV_Filename);
            cmor_pop_traceback();
            return (-1);
        }
    }
    // The provider defined a parent experiment.
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_PARENT_EXPT_ID) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_PARENT_EXPT_ID,
                                       szParentExpValue);
        // "no parent" case
        if (strcmp(szParentExpValue, NO_PARENT) == 0) {
            CV_CompareNoParent(PARENT_ACTIVITY_ID);
            CV_CompareNoParent(PARENT_MIP_ERA);
            CV_CompareNoParent(PARENT_SOURCE_ID);
            CV_CompareNoParent(PARENT_TIME_UNITS);
            CV_CompareNoParent(PARENT_VARIANT_LABEL);
            CV_CompareNoParent(BRANCH_METHOD);
            // Do we have branch_time_in_child?
            if (cmor_has_cur_dataset_attribute(BRANCH_TIME_IN_CHILD) == 0) {
                cmor_get_cur_dataset_attribute(BRANCH_TIME_IN_CHILD,
                                               szBranchTimeInChild);
                rc = sscanf(szBranchTimeInChild, "%lf", &dBranchTimeInChild);
                if ((rc == 0) || (rc == EOF)) {
                    cmor_handle_error_variadic(
                             "Your input attribute branch_time_in_child \"%s\" "
                             "is not a double floating point \n! ",
                             CMOR_WARNING, szBranchTimeInChild);
                }

            }
            // Do we have branch_time_in_parent?
            if (cmor_has_cur_dataset_attribute(BRANCH_TIME_IN_PARENT) == 0) {
                cmor_get_cur_dataset_attribute(BRANCH_TIME_IN_PARENT, szValue);
                if (strcmp(szValue, "0.0") != 0) {
                    cmor_handle_error_variadic(
                             "Your input attribute %s %s \n! "
                             "has been replaced with 0.0 \n! ",
                             CMOR_WARNING, BRANCH_TIME_IN_PARENT, szValue);
                    cmor_set_cur_dataset_attribute_internal
                      (BRANCH_TIME_IN_PARENT, "0.0", 1);

                }
            }
        } else {
            // real parent case
            // Parent Activity ID
            if (cmor_has_cur_dataset_attribute(PARENT_ACTIVITY_ID) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined properly \n! "
                         "for your experiment \"%s\"\n!\n! "
                         "See Controlled Vocabulary JSON file.(%s)\n! ",
                         CMOR_NORMAL, PARENT_ACTIVITY_ID, CV_experiment->key, CV_Filename);
                ierr = -1;
            } else {
                cmor_get_cur_dataset_attribute(PARENT_ACTIVITY_ID, szValue);
                CV_parent_activity_id = cmor_CV_search_child_key(CV_experiment,
                                                                 PARENT_ACTIVITY_ID);
                if (CV_IsStringInArray(CV_parent_activity_id, szValue) == 0) {
                    if (CV_parent_activity_id->anElements == 1) {
                        cmor_handle_error_variadic(
                                 "Your input attribute \"%s\" defined as \"%s\" "
                                 "will be replaced with \n! "
                                 "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                                 CMOR_WARNING, PARENT_ACTIVITY_ID, szValue,
                                 CV_parent_activity_id->aszValue[0]);
                        cmor_set_cur_dataset_attribute_internal
                          (PARENT_ACTIVITY_ID,
                           CV_parent_activity_id->aszValue[0], 1);

                    } else {
                        cmor_handle_error_variadic(
                                 "Your input attribute \"%s\" is not defined properly \n! "
                                 "for your experiment \"%s\"\n! "
                                 "There is more than 1 option for this experiment.\n! "
                                 "See Controlled Vocabulary JSON file.(%s)\n! ",
                                 CMOR_WARNING, PARENT_ACTIVITY_ID, CV_experiment->key,
                                 CV_Filename);
                    }
                }
            }
            // branch method
            if (cmor_has_cur_dataset_attribute(BRANCH_METHOD) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please describe the spin-up procedure as defined \n! "
                         "in CMIP6 documentations.\n! ",
                         CMOR_NORMAL, BRANCH_METHOD, szExperiment_ID);
                ierr = -1;

            } else {
                cmor_get_cur_dataset_attribute(BRANCH_METHOD, szBranchMethod);
                if (strlen(szBranchMethod) == 0) {
                    cmor_handle_error_variadic(
                             "Your input attribute %s is an empty string\n! "
                             "Please describe the spin-up procedure as defined \n! "
                             "in CMIP6 documentations.\n! ", CMOR_NORMAL, BRANCH_METHOD);
                    ierr = -1;
                }
            }
            // branch_time_in_child
            if (cmor_has_cur_dataset_attribute(BRANCH_TIME_IN_CHILD) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         CMOR_NORMAL, BRANCH_TIME_IN_CHILD, szExperiment_ID);
                ierr = -1;
            } else {
                cmor_get_cur_dataset_attribute(BRANCH_TIME_IN_CHILD,
                                               szBranchTimeInChild);
                rc = sscanf(szBranchTimeInChild, "%lf", &dBranchTimeInChild);
                if ((rc == 0) || (rc == EOF)) {
                    cmor_handle_error_variadic(
                             "Your input attribute branch_time_in_child \"%s\" "
                             "is not a double floating point \n! ",
                             CMOR_NORMAL, szBranchTimeInChild);
                    ierr = -1;
                }
            }
            // branch_time_in_parent
            if (cmor_has_cur_dataset_attribute(BRANCH_TIME_IN_PARENT) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         CMOR_NORMAL, BRANCH_TIME_IN_PARENT, szExperiment_ID);
                ierr = -1;
            } else {
                cmor_get_cur_dataset_attribute(BRANCH_TIME_IN_PARENT,
                                               szBranchTimeInParent);
                rc = sscanf(szBranchTimeInParent, "%lf", &dBranchTimeInParent);
                if ((rc == 0) || (rc == EOF)) {
                    cmor_handle_error_variadic(
                             "Your input attribute branch_time_in_parent \"%s\" "
                             "is not a double floating point \n! ",
                             CMOR_NORMAL, szBranchTimeInParent);
                    ierr = -1;
                }
            }
            // parent_time_units
            if (cmor_has_cur_dataset_attribute(PARENT_TIME_UNITS) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         CMOR_NORMAL, PARENT_TIME_UNITS, szExperiment_ID);
                ierr = -1;
            } else {
                char template[CMOR_MAX_STRING];
                int reti;
                cmor_get_cur_dataset_attribute(PARENT_TIME_UNITS,
                                               szParentTimeUnits);
                strcpy(template,
                       "^days[[:space:]]since[[:space:]][[:digit:]]\\{4,4\\}-[[:digit:]]\\{1,2\\}-[[:digit:]]\\{1,2\\}");

                reti = regcomp(&regex, template, 0);
                if (reti) {
                    cmor_handle_error_variadic(
                             "You regular expression \"%s\" is invalid. \n! "
                             "Please refer to the CMIP6 documentations.\n! ",
                             CMOR_NORMAL, template);
                    regfree(&regex);
                    ierr = -1;
                } else {
                    // Execute regular expression
                    reti = regexec(&regex, szParentTimeUnits, 0, NULL, 0);
                    if (reti == REG_NOMATCH) {
                        cmor_handle_error_variadic(
                                "Your  \"%s\" set to \"%s\" is invalid. \n! "
                                "Please refer to the CMIP6 documentations.\n! ",
                                CMOR_NORMAL, PARENT_TIME_UNITS, szParentTimeUnits);
                        ierr = -1;
                    }
                }
                regfree(&regex);
            }
            // parent_variant_label
            if (cmor_has_cur_dataset_attribute(PARENT_VARIANT_LABEL) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         CMOR_NORMAL, PARENT_VARIANT_LABEL, szExperiment_ID);
                ierr = -1;
            } else {
                char template[CMOR_MAX_STRING];
                int reti;
                cmor_get_cur_dataset_attribute(PARENT_VARIANT_LABEL,
                                               szParentVariantLabel);
                strcpy(template,
                       "^r[[:digit:]]\\{1,\\}i[[:digit:]]\\{1,\\}p[[:digit:]]\\{1,\\}f[[:digit:]]\\{1,\\}$");

                reti = regcomp(&regex, template, 0);
                if (reti) {
                    cmor_handle_error_variadic(
                             "You regular expression \"%s\" is invalid. \n! "
                             "Please refer to the CMIP6 documentations.\n! ",
                             CMOR_NORMAL, template);
                    ierr = -1;
                } else {
                    // Execute regular expression
                    reti = regexec(&regex, szParentVariantLabel, 0, NULL, 0);
                    if (reti == REG_NOMATCH) {
                        cmor_handle_error_variadic(
                                "You  \"%s\" set to \"%s\" is invalid. \n! "
                                "Please refer to the CMIP6 documentations.\n! ",
                                CMOR_NORMAL, PARENT_VARIANT_LABEL, szParentVariantLabel);
                        ierr = -1;
                    }
                }
                regfree(&regex);
            }
            // parent_source_id
            if (cmor_has_cur_dataset_attribute(PARENT_SOURCE_ID) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         CMOR_NORMAL, PARENT_SOURCE_ID, szExperiment_ID);
                ierr = -1;
            } else {
                cmor_get_cur_dataset_attribute(PARENT_SOURCE_ID,
                                               szParentSourceId);
                CV_source_id = cmor_CV_rootsearch(CV, CV_KEY_SOURCE_IDS);
                if (CV_source_id == NULL) {
                    cmor_handle_error_variadic(
                             "Your \"source_id\" key could not be found in\n! "
                             "your Controlled Vocabulary file.(%s)\n! ",
                             CMOR_NORMAL, CV_Filename);
                    ierr = -1;
                } else {
                    // Get specified experiment
                    cmor_get_cur_dataset_attribute(PARENT_SOURCE_ID,
                                                szParentSourceId);
                    CV_source = cmor_CV_search_child_key(CV_source_id,
                                                        szParentSourceId);
                    if (CV_source == NULL) {
                        cmor_handle_error_variadic(
                                "Your parent_source_id \"%s\" defined in your input file\n! "
                                "could not be found in your Controlled Vocabulary file.(%s)\n! ",
                                CMOR_NORMAL, szParentSourceId, CV_Filename);
                        ierr = -1;
                    }
                }
            }
            // parent_mip_era
            if (cmor_has_cur_dataset_attribute(PARENT_MIP_ERA) != 0) {
                cmor_handle_error_variadic(
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         CMOR_NORMAL, PARENT_MIP_ERA, szExperiment_ID);
                ierr = -1;
            } else {
                cmor_get_cur_dataset_attribute(PARENT_MIP_ERA, szValue);
                if (cmor_has_cur_dataset_attribute(GLOBAL_IS_CMIP7) == 0) {
                    parent_mip_era = CMIP7;
                } else {
                    parent_mip_era = CMIP6;
                }
                if (strcmp(parent_mip_era, szValue) != 0) {
                    cmor_handle_error_variadic(
                             "Your input attribute \"%s\" defined as \"%s\" "
                             "will be replaced with \n! "
                             "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                             CMOR_WARNING, PARENT_MIP_ERA, szValue, parent_mip_era);
                    cmor_set_cur_dataset_attribute_internal(PARENT_MIP_ERA,
                                                            szValue, 1);
                }
            }
        }
    }
    cmor_pop_traceback();
    return (ierr);
}

/************************************************************************/
/*                     cmor_CV_checkExperiment()                        */
/************************************************************************/
int cmor_CV_checkExperiment(cmor_CV_def_t * CV)
{
    cmor_CV_def_t *CV_experiment_ids;
    cmor_CV_def_t *CV_experiment;
    cmor_CV_def_t *CV_experiment_attr;

    char szExperiment_ID[CMOR_MAX_STRING];
    char szValue[CMOR_MAX_STRING];
    char szExpValue[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int rc;
    int nObjects;
    int i;
    int j;
    int bError;
    int ierr;

    szExpValue[0] = '\0';
    cmor_add_traceback("_CV_checkExperiment");
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    ierr = cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENTID, szExperiment_ID);
    if (ierr != 0) {
        cmor_handle_error_variadic(
                 "Your \"%s\" is not defined, check your required attributes\n! "
                 "See Controlled Vocabulary JSON file.(%s)\n! ",
                 CMOR_NORMAL, GLOBAL_ATT_EXPERIMENTID, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
/* -------------------------------------------------------------------- */
/*  Find experiment_ids dictionary in Controlled Vocabulary                */
/* -------------------------------------------------------------------- */
    CV_experiment_ids = cmor_CV_rootsearch(CV, CV_KEY_EXPERIMENT_ID);
    if (CV_experiment_ids == NULL) {
        cmor_handle_error_variadic(
                 "Your \"experiment_ids\" key could not be found in\n! "
                 "your Controlled Vocabulary file.(%s)\n! ", CMOR_NORMAL, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    CV_experiment = cmor_CV_search_child_key(CV_experiment_ids,
                                             szExperiment_ID);

    if (CV_experiment == NULL) {
        cmor_handle_error_variadic(
                 "Your experiment_id \"%s\" defined in your input file\n! "
                 "could not be found in your Controlled Vocabulary file.(%s)\n! ",
                 CMOR_NORMAL, szExperiment_ID, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }

    ierr = 0;
    nObjects = CV_experiment->nbObjects;
    // Parse all experiment attributes
    for (i = 0; i < nObjects; i++) {
        bError = 0;
        CV_experiment_attr = &CV_experiment->oValue[i];
        rc = cmor_has_cur_dataset_attribute(CV_experiment_attr->key);
        strcpy(szExpValue, CV_experiment_attr->szValue);
        // Validate source type first
        if (strcmp(CV_experiment_attr->key, CV_EXP_ATTR_DESCRIPTION) == 0) {
            continue;
        }
        if (strcmp(CV_experiment_attr->key, CV_EXP_ATTR_REQSOURCETYPE) == 0) {
            if(cmor_CV_checkSourceType(CV_experiment, szExperiment_ID) != 0)
                ierr = -1;
            continue;
        }
        // Warn user if experiment value from input file is different than
        // Controlled Vocabulary value.
        // experiment from Controlled Vocabulary will replace User entry value.
        if (rc == 0) {
            cmor_get_cur_dataset_attribute(CV_experiment_attr->key, szValue);
            if (CV_experiment_attr->anElements > 0) {
                for (j = 0; j < CV_experiment_attr->anElements; j++) {
                    //
                    // Find a string that match this value in the list?
                    //
                    if (strncmp(CV_experiment_attr->aszValue[j], szValue,
                                CMOR_MAX_STRING) == 0) {
                        break;
                    }
                }
                if (j == CV_experiment_attr->anElements) {
                    if (CV_experiment_attr->anElements == 1) {
                        strcpy(szExpValue, CV_experiment_attr->aszValue[0]);
                        bError = 1;
                    } else {
                        cmor_handle_error_variadic(
                                 "Your input attribute \"%s\" with value \n! \"%s\" "
                                 "is not set properly and \n! "
                                 "has multiple possible candidates \n! "
                                 "defined for experiment_id \"%s\".\n! \n!  "
                                 "See Controlled Vocabulary JSON file.(%s)\n! ",
                                 CMOR_CRITICAL,
                                 CV_experiment_attr->key, szValue,
                                 CV_experiment->key, CV_Filename);
                        cmor_pop_traceback();
                        return (-1);

                    }
                }
            } else {
                //
                // Check for string instead of list of string object!
                //
                if (CV_experiment_attr->szValue[0] != '\0') {
                    if (strncmp(CV_experiment_attr->szValue, szValue,
                    CMOR_MAX_STRING) != 0) {
                        strcpy(szExpValue, CV_experiment_attr->szValue);
                        bError = 1;
                    }
                }
            }
        }
        if (bError == 1) {
            cmor_handle_error_variadic(
                     "Your input attribute \"%s\" with value \n! \"%s\" "
                     "needs to be replaced with "
                     "value \"%s\"\n! "
                     "as defined for experiment_id \"%s\".\n! \n!  "
                     "See Controlled Vocabulary JSON file.(%s)\n! ",
                     CMOR_NORMAL,
                     CV_experiment_attr->key, szValue, szExpValue,
                     CV_experiment->key, CV_Filename);
            ierr = -1;
        }
        // Set/replace attribute.
        cmor_set_cur_dataset_attribute_internal(CV_experiment_attr->key,
                                                szExpValue, 1);
        if (cmor_has_cur_dataset_attribute(CV_experiment_attr->key) == 0) {
            cmor_get_cur_dataset_attribute(CV_experiment_attr->key, szValue);
        }
    }
    cmor_pop_traceback();

    return (ierr);
}

/************************************************************************/
/*                      cmor_CV_checkFilename()                         */
/************************************************************************/
int cmor_CV_checkFilename(cmor_CV_def_t * CV, int var_id,
        char *szInTimeCalendar, char *szInTimeUnits, char *infile) {

    cdCalenType icalo;
    char outname[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    char szTmp[CMOR_MAX_STRING];
    cdCompTime comptime;
    int i, j, n;
    int timeDim;
    int refvarid;
    cdCompTime starttime, endtime;
    int axis_id;
    refvarid = cmor_vars[var_id].ref_var_id;
    outname[0] = '\0';
    cmor_CreateFromTemplate(0, cmor_current_dataset.file_template,
            outname, "_");
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    timeDim = -1;
    for (i = 0; i < cmor_tables[0].vars[refvarid].ndims; i++) {
        int dim = cmor_tables[0].vars[refvarid].dimensions[i];
        if (cmor_tables[0].axes[dim].axis == 'T') {
            timeDim = dim;
            break;
        }
    }

    if (timeDim != -1) {
        cmor_set_cur_dataset_attribute(GLOBAL_ATT_CALENDAR, szInTimeCalendar,0);
        cmor_vars[var_id].axes_ids[0] = timeDim;
        cmor_axes[timeDim].ref_table_id = 0;
        cmor_axes[timeDim].ref_axis_id = timeDim;
        cmor_axis(&axis_id, cmor_tables[0].axes[timeDim].id,
                szInTimeUnits, 1,
                &cmor_tables[0].axes[timeDim].value,
                cmor_tables[0].axes[timeDim].type,
                cmor_tables[0].axes[timeDim].bounds_value, 0, "");

        // retrieve calendar

        if (cmor_calendar_c2i(szInTimeCalendar, &icalo) != 0) {
            cmor_handle_error_var_variadic(
                    "Cannot convert times for calendar: %s,\n! "
                    "closing variable %s (table: %s)", 
                    CMOR_CRITICAL, var_id, szInTimeCalendar, cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (-1);
        }
        //Compute timestamps

        if ((cmor_tables[0].axes[timeDim].climatology == 1)
                && (cmor_vars[0].first_bound != 1.e20)) {
            cdRel2Comp(icalo, szInTimeUnits, cmor_vars[0].first_bound,
                    &comptime);
        } else {
            cdRel2Comp(icalo, szInTimeUnits, cmor_vars[0].first_time,
                    &comptime);
        }
        /* -------------------------------------------------------------------- */
        /*      need to figure out the approximate interval                     */
        /* -------------------------------------------------------------------- */

        if (cmor_tables[0].axes[timeDim].climatology == 1) {
            starttime.year = 0;
            starttime.month = 0;
            starttime.day = 0;
            starttime.hour = 0.0;
            endtime = starttime;
            cdRel2Comp(icalo, szInTimeUnits, cmor_vars[var_id].first_bound, &starttime);
            cdRel2Comp(icalo, szInTimeUnits, cmor_vars[var_id].last_bound, &endtime);
        } else {
            cdRel2Comp(icalo, szInTimeUnits, cmor_vars[var_id].first_time, &starttime);
            cdRel2Comp(icalo, szInTimeUnits, cmor_vars[var_id].last_time, &endtime);
        }

        /* -------------------------------------------------------------------- */
        /*      We want start and end times that are greater than               */
        /*      x minutes 59.5 seconds to be set to x+1 minutes 0 seconds       */
        /*      so add a half second so that when floats are converted to       */
        /*       integers, this will round to nearest second (we know that      */
        /*       the time coordinates are positive                              */
        /*                                                                      */
        /*    note that in the following, cdCompAdd expects the increment added */
        /*      to be expressed in units of hours                               */
        /* -------------------------------------------------------------------- */

        if (icalo == cdMixed) {
            cdCompAddMixed(starttime, 0.5 / 3600., &starttime);
            cdCompAddMixed(endtime, 0.5 / 3600., &endtime);

        } else {
            cdCompAdd(starttime, 0.5 / 3600., icalo, &starttime);
            cdCompAdd(endtime, 0.5 / 3600., icalo, &endtime);
        }
        /* -------------------------------------------------------------------- */
        /*      need to figure out the frequency                                */
        /* -------------------------------------------------------------------- */
        int frequency_code;
        char frequency[CMOR_MAX_STRING];
        char start_string[CMOR_MAX_STRING];
        char end_string[CMOR_MAX_STRING];
        int start_seconds, end_seconds, start_minutes, end_minutes;

        if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_FREQUENCY) == 0) {
            cmor_get_cur_dataset_attribute(GLOBAL_ATT_FREQUENCY, frequency);
        }

        if (strstr(frequency, "yr") != NULL) {
            frequency_code = 1;
        } else if (strstr(frequency, "dec") != NULL) {
            frequency_code = 1;
        } else if (strstr(frequency, "monC") != NULL) {
            frequency_code = 6;
        } else if (strstr(frequency, "mon") != NULL) {
            frequency_code = 2;
        } else if (strstr(frequency, "day") != NULL) {
            frequency_code = 3;
        } else if (strstr(frequency, "subhr") != NULL) {
            frequency_code = 5;
        } else if (strstr(frequency, "hr") != NULL) {
            frequency_code = 4;
        } else if (strstr(frequency, "fx") != NULL) {
            frequency_code = 99;
        } else {
            frequency_code = 0;
        }

        switch (frequency_code) {
        case 1:
            /* frequency is yr, decadal */
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld", starttime.year);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld", endtime.year);
            break;
        case 2:
            /* frequency is mon */
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i", starttime.year,
                    starttime.month);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i", endtime.year,
                    endtime.month);
            break;
        case 3:
            /* frequency is day */
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i",
                    starttime.year, starttime.month, starttime.day);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i", endtime.year,
                    endtime.month, endtime.day);
            break;
        case 4:
            /* frequency is 6hr, 3hr, 1hr */
            /* round to the nearest minute */
            start_minutes = round((starttime.hour - (int) starttime.hour) * 60.);
            end_minutes = round((endtime.hour - (int) endtime.hour) * 60.);
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i%.2i%.2i",
                    starttime.year, starttime.month, starttime.day,
                    (int) starttime.hour, start_minutes);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i%.2i%.2i",
                    endtime.year, endtime.month, endtime.day,
                    (int) endtime.hour, end_minutes);
            break;
        case 5:
            /* frequency is subhr */
            /* round to the nearest second */
            start_seconds = (int) ((starttime.hour - (int) starttime.hour)
                    * 3600);
            end_seconds = (int) ((endtime.hour - (int) endtime.hour) * 3600);
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i%.2i%.2i%.2i",
                    starttime.year, starttime.month, starttime.day,
                    (int) starttime.hour, (int) (start_seconds / 60),
                    (start_seconds % 60));
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i%.2i%.2i%.2i%.2i",
                    endtime.year, endtime.month, endtime.day,
                    (int) endtime.hour, (int) (end_seconds / 60),
                    (end_seconds % 60));

            break;
        case 6:
            // frequency is monC
            // add/subtract 1 hour (this is overkill, but safe)
            // to prevent truncation errors from possibly leading you to
            // the wrong month
            //
            // note that cdCompAdd expects the increment added to be
            //      expressed in units of hours

            if (icalo == cdMixed) {
                cdCompAddMixed(starttime, 1.0, &starttime);
                cdCompAddMixed(endtime, -1.0, &endtime);
            } else {
                cdCompAdd(starttime, 1.0, icalo, &starttime);
                cdCompAdd(endtime, -1.0, icalo, &endtime);
            }
            snprintf(start_string, CMOR_MAX_STRING, "%.4ld%.2i", starttime.year,
                    starttime.month);
            snprintf(end_string, CMOR_MAX_STRING, "%.4ld%.2i", endtime.year,
                    endtime.month);
            break;
        case 99:
            /* frequency is fx */
            /* don't need to do anything, time string will ignored in next step */
            break;
        default:
            cmor_handle_error_var_variadic(
                    "Cannot find frequency %s. Closing variable %s (table: %s)",
                    CMOR_CRITICAL, var_id, frequency, cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id);
            cmor_pop_traceback();
            return (-1);
        }

        strncat(outname, "_", CMOR_MAX_STRING - strlen(outname));
        strncat(outname, start_string, CMOR_MAX_STRING - strlen(outname));
        strncat(outname, "-", CMOR_MAX_STRING - strlen(outname));
        strncat(outname, end_string, CMOR_MAX_STRING - strlen(outname));

        if (cmor_tables[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_table_id].
                axes[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_axis_id].
                climatology == 1) {
            strncat(outname, "-clim", CMOR_MAX_STRING - strlen(outname));
        }

        if (cmor_vars[0].suffix_has_date == 1) {
            /* -------------------------------------------------------------------- */
            /*      all right we need to pop out the date part....                  */
            /* -------------------------------------------------------------------- */
            n = strlen(cmor_vars[0].suffix);
            i = 0;
            while (cmor_vars[0].suffix[i] != '_')
                i++;
            i++;
            while ((cmor_vars[0].suffix[i] != '_') && i < n)
                i++;
            /* -------------------------------------------------------------------- */
            /*      ok now we have the length of dates                              */
            /*      at this point we are either at the                              */
            /*      _clim the actual _suffix or the end (==nosuffix)                */
            /*      checking if _clim needs to be added                             */
            /* -------------------------------------------------------------------- */
            if (cmor_tables[0].axes[timeDim].climatology == 1) {
                i += 5;
            }
            strcpy(szTmp, "");
            for (j = i; j < n; j++) {
                szTmp[j - i] = cmor_vars[var_id].suffix[i];
                szTmp[j - i + 1] = '\0';
            }
        } else {
            strncpy(szTmp, cmor_vars[0].suffix, CMOR_MAX_STRING);
        }

        if (strlen(szTmp) > 0) {
            strncat(outname, "_", CMOR_MAX_STRING - strlen(outname));
            strncat(outname, szTmp, CMOR_MAX_STRING - strlen(outname));
        }
    }
    strncat(outname, ".nc", CMOR_MAX_STRING - strlen(outname));
    if (strcmp(infile, outname) != 0) {
        cmor_handle_error_var_variadic(
                "Your filename \n! "
                "\"%s\" \n! "
                "does not match the CMIP6 requirement.\n! \n! "
                "Your output filename should be: \n! "
                "\"%s\"\n! \n! "
                "and should follow this template: \n!"
                "\"%s\"\n! \n! "
                "See your Controlled Vocabulary file.(%s)\n! ", 
                CMOR_NORMAL, var_id, infile, outname,
                cmor_current_dataset.file_template, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    cmor_pop_traceback();
    return (0);

}

/************************************************************************/
/*                      cmor_CV_setInstitution()                        */
/************************************************************************/
int cmor_CV_setInstitution(cmor_CV_def_t * CV)
{
    cmor_CV_def_t *CV_institution_ids;
    cmor_CV_def_t *CV_institution;

    char szInstitution_ID[CMOR_MAX_STRING];
    char szInstitution[CMOR_MAX_STRING];

    char CMOR_Filename[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int rc;

    cmor_add_traceback("_CV_setInstitution");
/* -------------------------------------------------------------------- */
/*  Find current Insitution ID                                          */
/* -------------------------------------------------------------------- */

    cmor_get_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION_ID, szInstitution_ID);
    rc = cmor_has_cur_dataset_attribute(CMOR_INPUTFILENAME);
    if (rc == 0) {
        cmor_get_cur_dataset_attribute(CMOR_INPUTFILENAME, CMOR_Filename);
    } else {
        CMOR_Filename[0] = '\0';
    }
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);

/* -------------------------------------------------------------------- */
/*  Find Institution dictionaries in Controlled Vocabulary                 */
/* -------------------------------------------------------------------- */
    CV_institution_ids = cmor_CV_rootsearch(CV, CV_KEY_INSTITUTION_ID);
    if (CV_institution_ids == NULL) {
        cmor_handle_error_variadic(
                 "Your \"%s\" key could not be found in\n! "
                 "your Controlled Vocabulary file.(%s)\n! ",
                 CMOR_NORMAL, CV_KEY_INSTITUTION_ID, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    CV_institution = cmor_CV_search_child_key(CV_institution_ids,
                                              szInstitution_ID);

    if (CV_institution == NULL) {
        cmor_handle_error_variadic(
                 "The institution_id, \"%s\", found in your \n! "
                 "input file (%s) could not be found in \n! "
                 "your Controlled Vocabulary file. (%s) \n! \n! "
                 "Please correct your input file by using a valid institution_id listed in your MIP tables' CV file.\n! "
                 "To add a new institution_id to the %s file, open a new issue in the\n! "
                 "table's Github repository. Managed project CMOR and MIP tables are listed at\n! "
                 "https://wcrp-cmip.github.io/WGCM_Infrastructure_Panel/cmor_and_mip_tables.html. \n! "
                 "Contact \"pcmdi-cmip@llnl.gov\" for additional guidance.  \n! \n! "
                 "See \"http://cmor.llnl.gov/mydoc_cmor3_CV/\" for further information about\n! "
                 "the \"institution_id\" and \"institution\" global attributes.  ",
                 CMOR_NORMAL, szInstitution_ID, CMOR_Filename, CV_Filename, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }
/* -------------------------------------------------------------------- */
/* Did the user defined an Institution Attribute?                       */
/* -------------------------------------------------------------------- */

    rc = cmor_has_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION);
    if (rc == 0) {
/* -------------------------------------------------------------------- */
/* Retrieve institution and compare with the one we have in the         */
/* Controlled Vocabulary file.  If it does not matches tell the user       */
/* the we will supersede his definition with the one in the Control     */
/* Vocabulary file.                                                     */
/* -------------------------------------------------------------------- */
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION, szInstitution);

/* -------------------------------------------------------------------- */
/*  Check if an institution has been defined! If not we exit.           */
/* -------------------------------------------------------------------- */
        if (CV_institution->szValue[0] == '\0') {
            cmor_handle_error_variadic(
                     "There is no institution associated to institution_id \"%s\"\n! "
                     "in your Controlled Vocabulary file.\n! "
                     "Check your \"%s\" dictionary!!\n! ", 
                     CMOR_NORMAL, CV_KEY_INSTITUTION_ID, szInstitution_ID);
            cmor_pop_traceback();
            return (-1);
        }
/* -------------------------------------------------------------------- */
/*  Check if they have the same string                                  */
/* -------------------------------------------------------------------- */
        if (strncmp(szInstitution, CV_institution->szValue, CMOR_MAX_STRING) !=
            0) {
            cmor_handle_error_variadic(
                     "Your input attribute institution \"%s\" will be replaced with \n! "
                     "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                     CMOR_WARNING, szInstitution, CV_institution->szValue);
        }
    }
/* -------------------------------------------------------------------- */
/*   Set institution according to the Controlled Vocabulary                */
/* -------------------------------------------------------------------- */
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_INSTITUTION,
                                            CV_institution->szValue, 1);
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                      cmor_CV_setLicense()                            */
/************************************************************************/
int cmor_CV_setLicense(cmor_CV_def_t * CV)
{
    cmor_CV_def_t *CV_license, *CV_lic_ids, *CV_lic_template;
    cmor_CV_def_t *CV_lic, *CV_lic_type, *CV_lic_url;

    char license[CMOR_MAX_STRING];
    char license_from_template[CMOR_MAX_STRING];
    char license_id[CMOR_MAX_STRING];
    char license_type[CMOR_MAX_STRING];
    char license_url[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int has_license, has_lic_type, has_lic_url;

    cmor_add_traceback("_CV_setLicense");

    // Find current license ID
    cmor_get_cur_dataset_attribute(GLOBAL_ATT_LICENSE_ID, license_id);
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);

    // Find license dictionary in Controlled Vocabulary
    CV_license = cmor_CV_rootsearch(CV, CV_KEY_LICENSE);
    if (CV_license == NULL) {
        cmor_handle_error_variadic(
                    "Your \"%s\" key could not be found in\n! "
                    "your Controlled Vocabulary file.(%s)\n! ",
                    CMOR_NORMAL, CV_KEY_LICENSE, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }

    CV_lic_ids = cmor_CV_search_child_key(CV_license, CV_KEY_LICENSE_ID);
    if (CV_lic_ids == NULL) {
        cmor_handle_error_variadic(
                    "Your \"%s\" key could not be found in\n! "
                    "your Controlled Vocabulary file.(%s)\n! ",
                    CMOR_NORMAL, CV_KEY_LICENSE_ID, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }

    CV_lic_template = cmor_CV_search_child_key(CV_license, CV_KEY_LICENSE_TEMPLATE);
    if (CV_lic_template == NULL) {
        cmor_handle_error_variadic(
                    "License attribute, \"%s\", could not be found in "
                    "your Controlled Vocabulary file. (%s)",
                    CMOR_NORMAL, CV_KEY_LICENSE_TEMPLATE, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }

    CV_lic = cmor_CV_search_child_key(CV_lic_ids, license_id);
    if (CV_lic == NULL) {
        cmor_handle_error_variadic(
                    "The license_id, \"%s\", could not be found in "
                    "your Controlled Vocabulary file. (%s)",
                    CMOR_NORMAL, license_id, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }

    CV_lic_type = cmor_CV_search_child_key(CV_lic, CV_KEY_LICENSE_TYPE);
    if (CV_lic_type == NULL) {
        cmor_handle_error_variadic(
                    "License attribute, \"%s\", could not be found in "
                    "your Controlled Vocabulary file. (%s)",
                    CMOR_NORMAL, CV_KEY_LICENSE_TYPE, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }

    CV_lic_url = cmor_CV_search_child_key(CV_lic, CV_KEY_LICENSE_URL);
    if (CV_lic_url == NULL) {
        cmor_handle_error_variadic(
                    "License attribute, \"%s\", could not be found in "
                    "your Controlled Vocabulary file. (%s)",
                    CMOR_NORMAL, CV_KEY_LICENSE_URL, CV_Filename);
        cmor_pop_traceback();
        return (-1);
    }

    // Check if user provided license type or URL
    has_lic_type = cmor_has_cur_dataset_attribute(GLOBAL_ATT_LICENSE_TYPE);
    if (has_lic_type == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_LICENSE_TYPE, license_type);

        if (strncmp(license_type, CV_lic_type->szValue, CMOR_MAX_STRING) !=
            0) {
            cmor_handle_error_variadic(
                "Your input attribute license type \"%s\" will be replaced with \n! "
                "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                CMOR_WARNING, license_type, CV_lic_type->szValue);
        }
    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_LICENSE_TYPE, CV_lic_type->szValue, 1);

    has_lic_url = cmor_has_cur_dataset_attribute(GLOBAL_ATT_LICENSE_URL);
    if (has_lic_url == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_LICENSE_URL, license_url);

        if (strncmp(license_url, CV_lic_url->szValue, CMOR_MAX_STRING) !=
            0) {
            cmor_handle_error_variadic(
                "Your input attribute license URL \"%s\" will be replaced with \n! "
                "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                CMOR_WARNING, license_url, CV_lic_url->szValue);
        }
    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_LICENSE_URL, CV_lic_url->szValue, 1);

    // Build license string
    license_from_template[0] = '\0';
    cmor_CreateFromTemplate(0, CV_lic_template->szValue, license_from_template, "");

    has_license = cmor_has_cur_dataset_attribute(GLOBAL_ATT_LICENSE);
    if (has_license == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_LICENSE, license);

        if (strncmp(license, license_from_template, CMOR_MAX_STRING) !=
            0) {
            cmor_handle_error_variadic(
                "Your input attribute license \"%s\" will be replaced with \n! "
                "\"%s\" as defined in your Controlled Vocabulary file.\n! ",
                CMOR_WARNING, license, license_from_template);
        }
    }
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_LICENSE, license_from_template, 1);

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                    cmor_CV_ValidateAttribute()                       */
/*                                                                      */
/*  Search for attribute and verify that values is within a list        */
/*                                                                      */
/*    i.e. "mip_era": [ "CMIP6", "CMIP5" ]                              */
/*                                                                      */
/*    Set block of attributes such as:                                  */
/*           key: {                                                     */
/*               value: {                                               */
/*                      key:value,                                      */
/*                      key:value,                                      */
/*                      key:value                                       */
/*                      }                                               */
/*                }                                                     */
/*                                                                      */
/************************************************************************/
int cmor_CV_ValidateAttribute(cmor_CV_def_t * CV, char *szKey)
{
    cmor_CV_def_t *attr_CV;
    cmor_CV_def_t *key_CV;
    cmor_CV_def_t *list_CV;
    cmor_CV_def_t *CV_key;
    cmor_CV_def_t *required_attrs;
    char szValue[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    char szValids[CMOR_MAX_STRING];
    char szOutput[CMOR_MAX_STRING];
    char szTmp[CMOR_MAX_STRING];
    char tableValue[CMOR_MAX_STRING];
    char *valueToken;
    int i, j;
    int nObjects;
    regex_t regex;
    int reti;
    int ierr;
    int nValueLen;
    int isRequired;

    cmor_add_traceback("_CV_ValidateAttribute");

    szValids[0] = '\0';
    szOutput[0] = '\0';
    attr_CV = cmor_CV_rootsearch(CV, szKey);
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);

/* -------------------------------------------------------------------- */
/* This attribute does not need validation.                             */
/* -------------------------------------------------------------------- */
    if (attr_CV == NULL) {
        cmor_pop_traceback();
        return (0);
    }

    ierr = cmor_get_cur_dataset_attribute(szKey, szValue);
/* -------------------------------------------------------------------- */
/* If this is a list, search the attribute                              */
/* -------------------------------------------------------------------- */
    for (i = 0; i < attr_CV->anElements; i++) {
        // Special case for source type any element may match
        strncpy(szTmp, attr_CV->aszValue[i], CMOR_MAX_STRING);
        if (strcmp(szKey, GLOBAL_ATT_SOURCE_TYPE) != 0) {
            if (attr_CV->aszValue[i][0] != '^') {
                snprintf(szTmp, CMOR_MAX_STRING, "^%s", attr_CV->aszValue[i]);
            }

            nValueLen = strlen(szTmp);
            if (szTmp[nValueLen - 1] != '$') {
                strcat(szTmp, "$");
            }
        }
        strncpy(attr_CV->aszValue[i], szTmp, CMOR_MAX_STRING);
        reti = regcomp(&regex, attr_CV->aszValue[i], 0);
        if (reti) {
            cmor_handle_error_variadic(
                     "You regular expression \"%s\" is invalid. \n! "
                     "Check your Controlled Vocabulary file \"%s\".\n! ",
                     CMOR_NORMAL, attr_CV->aszValue[i], CV_Filename);
            regfree(&regex);
            cmor_pop_traceback();
            return (-1);

        }
/* -------------------------------------------------------------------- */
/*        Execute regular expression                                    */
/* -------------------------------------------------------------------- */
        reti = regexec(&regex, szValue, 0, NULL, 0);
        if (!reti) {
            regfree(&regex);
            break;
        }
        regfree(&regex);
    }
/* -------------------------------------------------------------------- */
/*  only critical if the attribute was not found                        */
/* -------------------------------------------------------------------- */
    if (ierr) {
        cmor_pop_traceback();
        return (-1);
    }
/* -------------------------------------------------------------------- */
/* If this is a dictionary, set all attributes                          */
/*  Set block of attributes.                                            */
/* -------------------------------------------------------------------- */
    if (attr_CV->nbObjects != -1) {
        key_CV = cmor_CV_rootsearch(CV, szKey);
        // Realm and source type are attributes that can have multiple values
        // in a string separated by spaces.
        if (strcmp(szKey, GLOBAL_ATT_REALM) == 0
            || strcmp(szKey, GLOBAL_ATT_SOURCE_TYPE) == 0) {
            valueToken = strtok(szValue, " ");
            while (valueToken != NULL) {
                list_CV = cmor_CV_search_child_key(key_CV, valueToken);
                if (list_CV == NULL) {
                    cmor_handle_error_variadic(
                        "The registered CV attribute \"%s\" as defined as \"%s\" "
                        "contains values not found in the CV.",
                        CMOR_NORMAL, szKey, szValue);
                    cmor_pop_traceback();
                    return (-1);
                }
                valueToken = strtok(NULL, " ");
            }
            cmor_pop_traceback();
            return (0);
        } else {
            if (strcmp(szKey, GLOBAL_ATT_LICENSE) == 0) {
                // If the license attribute is an object in the CV,
                // then the user needs to specify a license_id that
                // is used to build the license from a template.
                if(cmor_has_cur_dataset_attribute(GLOBAL_ATT_LICENSE_ID) != 0){
                    cmor_handle_error_variadic(
                        "The CV contains license information that requires "
                        "the attribute \"%s\" to be defined. \n! "
                        "Please select a \"%s\" value from the \"%s\" section of the CV.",
                        CMOR_NORMAL,
                        GLOBAL_ATT_LICENSE_ID, GLOBAL_ATT_LICENSE_ID, GLOBAL_ATT_LICENSE);
                    cmor_pop_traceback();
                    return (-1);
                }
                cmor_pop_traceback();
                return (0);
            }
            list_CV = cmor_CV_search_child_key(key_CV, szValue);
            if (list_CV == NULL) {
                cmor_handle_error_variadic(
                    "The registered CV attribute \"%s\" as defined as \"%s\" "
                    "does not match any value in the CV.",
                    CMOR_NORMAL, szKey, szValue);
                cmor_pop_traceback();
                return (-1);
            } else if (strcmp(szKey, GLOBAL_ATT_FREQUENCY) == 0) {
                // The contents of the frequency object are not used
                // as global attributes.
                cmor_pop_traceback();
                return (0);
            }
        }
        nObjects = list_CV->nbObjects;
        required_attrs = cmor_CV_rootsearch(CV, CV_KEY_REQUIRED_GBL_ATTRS);
        for (i = 0; i < nObjects; i++) {
            CV_key = &list_CV->oValue[i];

            // Check if the attribute is required
            isRequired = 0;
            for (j = 0; j < required_attrs->anElements; j++) {
                if(strcmp(CV_key->key, required_attrs->aszValue[j]) == 0)
                    isRequired = 1;
            }

            if (CV_key->szValue[0] != '\0') {
                if(cmor_has_cur_dataset_attribute(CV_key->key) == 0){
                    cmor_get_cur_dataset_attribute(CV_key->key, szTmp);
                    if(szTmp[0] != '\0' && strcmp(CV_key->szValue, szTmp) != 0){
                        reti = cmor_get_table_attr(CV_key->key, &cmor_tables[CMOR_TABLE], tableValue);
                        if(reti == 0 && strcmp(tableValue, szTmp) == 0){
                            cmor_handle_error_variadic(
                                    "The registered CV attribute \"%s\" as defined as \"%s\" "
                                    "will be replaced with \n! "
                                    "\"%s\" as defined in the table %s\n! ",
                                    CMOR_WARNING, CV_key->key, CV_key->szValue, szTmp, cmor_tables[CMOR_TABLE].szTable_id);
                        } else {
                            cmor_handle_error_variadic(
                                    "The registered CV attribute \"%s\" as defined as \"%s\" "
                                    "will be replaced with \n! "
                                    "\"%s\" as defined in your user input file\n! ",
                                    CMOR_WARNING, CV_key->key, CV_key->szValue, szTmp);
                        }
                    } else {
                        cmor_set_cur_dataset_attribute_internal(CV_key->key,
                                CV_key->szValue, 1);
                    }
                } else {
                    cmor_set_cur_dataset_attribute_internal(CV_key->key,
                            CV_key->szValue, 1);
                }
            } else if (CV_key->anElements == 1 && isRequired == 1) {
                if(cmor_has_cur_dataset_attribute(CV_key->key) == 0){
                    cmor_get_cur_dataset_attribute(CV_key->key, szTmp);
                    if(szTmp[0] != '\0' && strcmp(CV_key->aszValue[0], szTmp) != 0){
                        reti = cmor_get_table_attr(CV_key->key, &cmor_tables[CMOR_TABLE], tableValue);
                        if(reti == 0 && strcmp(tableValue, szTmp) == 0){
                            cmor_handle_error_variadic(
                                    "The registered CV attribute \"%s\" as defined as \"%s\" "
                                    "will be replaced with \n! "
                                    "\"%s\" as defined in the table %s\n! ",
                                    CMOR_WARNING, CV_key->key, CV_key->aszValue[0], szTmp, cmor_tables[CMOR_TABLE].szTable_id);
                        } else {
                            cmor_handle_error_variadic(
                                    "The registered CV attribute \"%s\" as defined as \"%s\" "
                                    "will be replaced with \n! "
                                    "\"%s\" as defined in your user input file\n! ",
                                    CMOR_WARNING, CV_key->key, CV_key->aszValue[0], szTmp);
                        }
                    } else {
                        cmor_set_cur_dataset_attribute_internal(CV_key->key,
                                CV_key->aszValue[0], 1);
                    }
                } else {
                    cmor_set_cur_dataset_attribute_internal(CV_key->key,
                            CV_key->aszValue[0], 1);
                }
            } else if (CV_key->anElements > 1 && isRequired == 1) {
                if(cmor_has_cur_dataset_attribute(CV_key->key) != 0) {
                    cmor_handle_error_variadic(
                            "The registered CV attribute \"%s\" has multiple values \n! "
                            "defined in \"%s\"\n! "
                            "Please select one from the entry %s.%s.%s.",
                            CMOR_NORMAL, CV_key->key, CV_Filename, szKey, szValue, CV_key->key);
                    cmor_pop_traceback();
                    return (-1);
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/* We could not validate this attribute, exit.                          */
/* -------------------------------------------------------------------- */
    if (i == (attr_CV->anElements)) {
        cmor_handle_error_variadic(
                 "The attribute \"%s\" could not be validated. \n! "
                 "The current input value is "
                 "\"%s\", which is not valid. \n! \n! "
                 "Valid values must match those found in the \"%s\" "
                 "section\n! of your Controlled Vocabulary (CV) file \"%s\".\n! ",
                 CMOR_NORMAL, szKey, szValue, szKey, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                  cmor_CV_check_branding_suffix()                     */
/************************************************************************/
int cmor_CV_check_branding_suffix(cmor_CV_def_t *CV)
{
    cmor_CV_def_t *required_attrs;
    cmor_CV_def_t *branding_template;
    char *attr_name;
    char attr_value[CMOR_MAX_STRING];
    char branding_suffix[CMOR_MAX_STRING];
    int i;

    cmor_add_traceback("_CV_check_branding_suffix");

    // Do nothing if required_global_attributes is not in the CV
    // or if branding_suffix is not a required attribute
    required_attrs = cmor_CV_rootsearch(CV, CV_KEY_REQUIRED_GBL_ATTRS);
    if (required_attrs == NULL) {
        cmor_pop_traceback();
        return (0);
    }

    for (i = 0; i < required_attrs->anElements; i++) {
        attr_name = required_attrs->aszValue[i];
        if (strcmp(attr_name, GLOBAL_ATT_BRANDINGSUFFIX) == 0)
            break;
    }
    if (i == required_attrs->anElements) {
        cmor_pop_traceback();
        return (0);
    }

    // Check if the template for the branding suffix is in the CV file
    branding_template = cmor_CV_rootsearch(CV, CV_KEY_BRANDING_TEMPLATE);
    if (branding_template == NULL) {
        cmor_handle_error_variadic(
            "The branding suffix template \"%s\" "
            "was not found in your CV file. "
            "It is required for building the \"%s\" attribute.",
            CMOR_NORMAL, CV_KEY_BRANDING_TEMPLATE, GLOBAL_ATT_BRANDINGSUFFIX);
        cmor_pop_traceback();
        return (-1);
    }

    // Compare value made from the template with the global attribute value
    cmor_get_cur_dataset_attribute(GLOBAL_ATT_BRANDINGSUFFIX, attr_value);
    branding_suffix[0] = '\0';
    cmor_CreateFromTemplate(0, branding_template->szValue, branding_suffix, "-");
    if (strcmp(branding_suffix, attr_value) != 0) {
        cmor_handle_error_variadic(
            "Your branding label attribute \"%s\" "
            "has the value \"%s\", which is not valid. "
            "The value must be \"%s\"",
            CMOR_NORMAL, attr_name, attr_value, branding_suffix);
        cmor_pop_traceback();
        return (-1);
    }

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                        cmor_CV_checkGrids()                          */
/************************************************************************/
int cmor_CV_checkGrids(cmor_CV_def_t * CV)
{
    int rc;
    char szGridLabel[CMOR_MAX_STRING];
    char szGridResolution[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    char szCompare[CMOR_MAX_STRING];

    cmor_CV_def_t *CV_grid_labels;
    cmor_CV_def_t *CV_grid_resolution;
    cmor_CV_def_t *CV_grid_child;
    int i;

    cmor_add_traceback("_CV_checkGrids");

    rc = cmor_has_cur_dataset_attribute(GLOBAL_ATT_GRID_LABEL);
    if (rc == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_GRID_LABEL, szGridLabel);
    }

    rc = cmor_has_cur_dataset_attribute(GLOBAL_ATT_GRID_RESOLUTION);
    if (rc == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_GRID_RESOLUTION,
                                       szGridResolution);
    }

    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);

    CV_grid_labels = cmor_CV_rootsearch(CV, CV_KEY_GRID_LABELS);
    if (CV_grid_labels == NULL) {
        cmor_handle_error_variadic(
                 "Your \"grid_label\" key could not be found in\n! "
                 "your Controlled Vocabulary file.(%s)\n! ", CMOR_NORMAL, CV_Filename);

        cmor_pop_traceback();
        return (-1);
    }
    if (CV_grid_labels->anElements > 0) {
        for (i = 0; i < CV_grid_labels->anElements; i++) {
            // Remove regular expression to compare strings.
            strncpy(szCompare, CV_grid_labels->aszValue[i], CMOR_MAX_STRING);
            if (szCompare[0] == '^') {
                strncpy(szCompare, &CV_grid_labels->aszValue[i][1],
                        strlen(CV_grid_labels->aszValue[i]) - 2);
                szCompare[strlen(CV_grid_labels->aszValue[i]) - 2] = '\0';
            }
            rc = strcmp(szCompare, szGridLabel);
            if (rc == 0) {
                break;
            }
        }
        if (i == CV_grid_labels->anElements) {
            cmor_handle_error_variadic(
                     "Your attribute grid_label is set to \"%s\" which is invalid."
                     "\n! \n! Check your Controlled Vocabulary file \"%s\".\n! ",
                     CMOR_NORMAL, szGridLabel, CV_Filename);
            cmor_pop_traceback();
            return (-1);

        }
    } else {
        CV_grid_child = cmor_CV_search_child_key(CV_grid_labels, szGridLabel);
        if (CV_grid_child == NULL) {
            cmor_handle_error_variadic(
                    "Your attribute grid_label is set to \"%s\" which is invalid."
                            "\n! \n! Check your Controlled Vocabulary file \"%s\".\n! ",
                    CMOR_NORMAL, szGridLabel, CV_Filename);
            cmor_pop_traceback();
            return (-1);
        }
    }
    CV_grid_resolution = cmor_CV_rootsearch(CV, CV_KEY_GRID_RESOLUTION);
    if (CV_grid_resolution == NULL) {
        cmor_handle_error_variadic(
                 "Your \"nominal_resolution\" key could not be found in\n! "
                 "your Controlled Vocabulary file.(%s)\n! ", CMOR_NORMAL, CV_Filename);
        cmor_pop_traceback();
        return (-1);

    }

    if (CV_grid_resolution->anElements > 0) {
        for (i = 0; i < CV_grid_resolution->anElements; i++) {
            strncpy(szCompare, CV_grid_resolution->aszValue[i],
                    CMOR_MAX_STRING);
            // Remove regular expression to compare strings.
            if (CV_grid_resolution->aszValue[i][0] == '^') {
                strncpy(szCompare, &CV_grid_resolution->aszValue[i][1],
                        strlen(CV_grid_resolution->aszValue[i]) - 2);
                szCompare[strlen(CV_grid_resolution->aszValue[i]) - 2] = '\0';

            }
            rc = strcmp(szCompare, szGridResolution);
            if (rc == 0) {
                break;
            }
        }
        if (i == CV_grid_resolution->anElements) {
            cmor_handle_error_variadic(
                     "Your attribute grid_resolution is set to \"%s\" which is invalid."
                     "\n! \n! Check your Controlled Vocabulary file \"%s\".\n! ",
                     CMOR_NORMAL, szGridResolution, CV_Filename);
            cmor_pop_traceback();
            return (-1);

        }
    }

    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                    cmor_CV_CheckGblAttributes()                      */
/************************************************************************/
int cmor_CV_checkGblAttributes(cmor_CV_def_t * CV)
{
    cmor_CV_def_t *required_attrs;
    int i;
    int rc;
    int bCriticalError = 0;
    int ierr = 0;
    cmor_add_traceback("_CV_checkGblAttributes");
    required_attrs = cmor_CV_rootsearch(CV, CV_KEY_REQUIRED_GBL_ATTRS);
    if (required_attrs != NULL) {
        // First, validate required attributes that are set by the user.
        // This will also set any attributes in the controlled vocabulary
        // that are dependent on user input and only have one value.
        for (i = 0; i < required_attrs->anElements; i++) {
            if (cmor_has_cur_dataset_attribute(required_attrs->aszValue[i]) == 0) {
                rc = cmor_CV_ValidateAttribute(CV, required_attrs->aszValue[i]);
                if (rc != 0) {
                    bCriticalError = 1;
                    ierr += -1;
                }
            }
        }
        // Check that all required attributes are set, including those set by 
        // cmor_CV_ValidateAttribute in the above loop.
        for (i = 0; i < required_attrs->anElements; i++) {
            rc = cmor_has_cur_dataset_attribute(required_attrs->aszValue[i]);
            if (rc != 0) {
                cmor_handle_error_variadic(
                         "Your Controlled Vocabulary file specifies one or more\n! "
                         "required attributes.  The following\n! "
                         "attribute was not properly set.\n! \n! "
                         "Please set attribute: \"%s\" in your input file.",
                         CMOR_NORMAL, required_attrs->aszValue[i]);
                bCriticalError = 1;
                ierr += -1;
            }
            rc = cmor_CV_ValidateAttribute(CV, required_attrs->aszValue[i]);
            if (rc != 0) {
                bCriticalError = 1;
                ierr += -1;
            }
        }
    }
    if (bCriticalError) {
        cmor_handle_error_variadic("Please fix required attributes mentioned in\n! "
                          "the warnings/error above and rerun. (aborting!)\n! ",
                          CMOR_NORMAL);
    }
    cmor_pop_traceback();
    return (ierr);
}

/************************************************************************/
/*                       cmor_CV_set_entry()                            */
/************************************************************************/
int cmor_CV_set_entry(cmor_table_t * table, json_object * value)
{
    extern int cmor_ntables;
    int nCVId;
    int nbObjects = 0;
    cmor_CV_def_t *CV;
    cmor_CV_def_t *newCV;
    cmor_table_t *cmor_table;
    cmor_table = &cmor_tables[cmor_ntables];

    cmor_is_setup();

    cmor_add_traceback("_CV_set_entry");
/* -------------------------------------------------------------------- */
/* CV 0 contains number of objects                                      */
/* -------------------------------------------------------------------- */
    nbObjects++;
    newCV = (cmor_CV_def_t *) realloc(cmor_table->CV, sizeof(cmor_CV_def_t));

    cmor_table->CV = newCV;
    CV = newCV;
    cmor_CV_init(CV, cmor_ntables);
    cmor_table->CV->nbObjects = nbObjects;

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

        if (CVName[0] == '#') {
            continue;
        }
        cmor_CV_set_att(CV, CVName, CVValue);
    }
    CV = &cmor_table->CV[0];
    CV->nbObjects = nbObjects;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_CV_checkTime()                            */
/************************************************************************/
int cmor_CV_checkISOTime(char *szAttribute)
{
    struct tm tm;
    int rc;
    char *szReturn;
    char szDate[CMOR_MAX_STRING];

    rc = cmor_has_cur_dataset_attribute(szAttribute);
    if (rc == 0) {
/* -------------------------------------------------------------------- */
/*   Retrieve Date                                                      */
/* -------------------------------------------------------------------- */
        cmor_get_cur_dataset_attribute(szAttribute, szDate);
    }
/* -------------------------------------------------------------------- */
/*    Check data format                                                 */
/* -------------------------------------------------------------------- */
    memset(&tm, 0, sizeof(struct tm));
    szReturn = strptime(szDate, "%FT%H:%M:%SZ", &tm);
    if (szReturn == NULL) {
        cmor_handle_error_variadic(
                 "Your global attribute "
                 "\"%s\" set to \"%s\" is not a valid date.\n! "
                 "ISO 8601 date format \"YYYY-MM-DDTHH:MM:SSZ\" is required."
                 "\n! ", CMOR_NORMAL, szAttribute, szDate);
        cmor_pop_traceback();
        return (-1);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                         cmor_CV_variable()                           */
/************************************************************************/
int cmor_CV_variable(int *var_id, char *name, char *units, 
                     float *missing, int *imissing,
                     double startime, double endtime,
                     double startimebnds, double endtimebnds)
{

    int vrid = -1;
    int i;
    int iref;
    char ctmp[CMOR_MAX_STRING];
    cmor_var_def_t refvar;

    cmor_is_setup();

    cmor_add_traceback("cmor_CV_variable");

    if (CMOR_TABLE == -1) {
        cmor_handle_error_variadic("You did not define a table yet!", CMOR_CRITICAL);
        cmor_pop_traceback();
        return (-1);
    }

/* -------------------------------------------------------------------- */
/*      ok now look which variable is corresponding in table if not     */
/*      found then error                                                */
/* -------------------------------------------------------------------- */
    iref = -1;
    cmor_trim_string(name, ctmp);
    for (i = 0; i < cmor_tables[CMOR_TABLE].nvars + 1; i++) {
        if (strcmp(cmor_tables[CMOR_TABLE].vars[i].id, ctmp) == 0) {
            iref = i;
            break;
        }
    }
/* -------------------------------------------------------------------- */
/*      ok now look for out_name to see if we can it.                   */
/* -------------------------------------------------------------------- */

    if (iref == -1) {
        for (i = 0; i < cmor_tables[CMOR_TABLE].nvars + 1; i++) {
            if (strcmp(cmor_tables[CMOR_TABLE].vars[i].out_name, ctmp) == 0) {
                iref = i;
                break;
            }
        }
    }

    if (iref == -1) {
        cmor_handle_error_variadic(
                 "Could not find a matching variable for name: '%s'", CMOR_CRITICAL, ctmp);
        cmor_pop_traceback();
        return (-1);
    }

    refvar = cmor_tables[CMOR_TABLE].vars[iref];
    for (i = 0; i < CMOR_MAX_VARIABLES; i++) {
        if(cmor_vars[i].ref_table_id == CMOR_TABLE) {
            if (refvar.out_name[0] == '\0') {
                if(strncmp(cmor_vars[i].id, name, CMOR_MAX_STRING) == 0) {
                    *var_id = i;
                    return (0);
                }
            } else {
                if(strncmp(cmor_vars[i].id, refvar.out_name, CMOR_MAX_STRING) == 0 ) {
                    *var_id = i;
                    return (0);
                }
            }
        }
    }


    for (i = 0; i < CMOR_MAX_VARIABLES; i++) {
        if (cmor_vars[i].self == -1) {
            vrid = i;
            break;
        }
    }

    cmor_vars[vrid].ref_table_id = CMOR_TABLE;
    cmor_vars[vrid].ref_var_id = iref;

/* -------------------------------------------------------------------- */
/*      init some things                                                */
/* -------------------------------------------------------------------- */

    strcpy(cmor_vars[vrid].suffix, "");
    strcpy(cmor_vars[vrid].base_path, "");
    strcpy(cmor_vars[vrid].current_path, "");
    cmor_vars[vrid].suffix_has_date = 0;

/* -------------------------------------------------------------------- */
/*      output missing value                                            */
/* -------------------------------------------------------------------- */

    cmor_vars[vrid].omissing = (double)cmor_tables[CMOR_TABLE].missing_value;

/* -------------------------------------------------------------------- */
/*      copying over values from ref var                                */
/* -------------------------------------------------------------------- */

    cmor_vars[vrid].valid_min = refvar.valid_min;
    cmor_vars[vrid].valid_max = refvar.valid_max;
    cmor_vars[vrid].ok_min_mean_abs = refvar.ok_min_mean_abs;
    cmor_vars[vrid].ok_max_mean_abs = refvar.ok_max_mean_abs;
    cmor_vars[vrid].shuffle = refvar.shuffle;
    cmor_vars[vrid].deflate = refvar.deflate;
    cmor_vars[vrid].deflate_level = refvar.deflate_level;
    cmor_vars[vrid].zstandard_level = refvar.zstandard_level;
    cmor_vars[vrid].first_bound = startimebnds;
    cmor_vars[vrid].last_bound = endtimebnds;
    cmor_vars[vrid].first_time = startime;
    cmor_vars[vrid].last_time = endtime;

    cmor_vars[vrid].quantize_mode = NC_NOQUANTIZE;
    cmor_vars[vrid].quantize_nsd = 1;

    if (refvar.out_name[0] == '\0') {
        strncpy(cmor_vars[vrid].id, name, CMOR_MAX_STRING);
    } else {
        strncpy(cmor_vars[vrid].id, refvar.out_name, CMOR_MAX_STRING);
    }

    cmor_set_variable_attribute_internal(vrid,
                                         VARIABLE_ATT_STANDARDNAME, 'c',
                                         refvar.standard_name);

    cmor_set_variable_attribute_internal(vrid,
                                         VARIABLE_ATT_LONGNAME, 'c',
                                         refvar.long_name);

    if (refvar.flag_values[0] != '\0') {
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FLAGVALUES, 'c',
                                             refvar.flag_values);
    }
    if (refvar.flag_meanings[0] != '\0') {

        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FLAGMEANINGS,
                                             'c', refvar.flag_meanings);
    }

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_COMMENT, 'c',
                                         refvar.comment);

    if (strcmp(refvar.units, "?") == 0) {
        strncpy(cmor_vars[vrid].ounits, units, CMOR_MAX_STRING);
    } else {
        strncpy(cmor_vars[vrid].ounits, refvar.units, CMOR_MAX_STRING);
    }

    if (refvar.type != 'c') {
        cmor_set_variable_attribute_internal(vrid,
                                             VARIABLE_ATT_UNITS,
                                             'c', cmor_vars[vrid].ounits);
    }

    strncpy(cmor_vars[vrid].iunits, units, CMOR_MAX_STRING);

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_CELLMETHODS,
                                         'c', refvar.cell_methods);

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_CELLMEASURES, 'c',
                                         refvar.cell_measures);

    if (refvar.positive == 'u') {
        if (cmor_is_required_variable_attribute(refvar, VARIABLE_ATT_POSITIVE)
            == 0) {

            cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_POSITIVE,
                                                 'c', "up");

        }

    } else if (refvar.positive == 'd') {
        if (cmor_is_required_variable_attribute(refvar, VARIABLE_ATT_POSITIVE)
            == 0) {

            cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_POSITIVE,
                                                 'c', "down");

        }
    }

    if (refvar.type == 'i') {
        cmor_vars[vrid].type = 'i';
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_MISSINGVALUES,
                                         'i', imissing);
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FILLVAL, 'i',
                                             imissing);
    } else {
        if (refvar.type == '\0') {
            cmor_vars[vrid].type = 'f';
        }
        else {
            cmor_vars[vrid].type = refvar.type;
        }
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_MISSINGVALUES,
                                         'f', missing);
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FILLVAL, 'f',
                                             missing);
    }


    cmor_vars[vrid].self = vrid;
    *var_id = vrid;
    cmor_pop_traceback();
    return (0);
};
