#define _XOPEN_SOURCE

#define _GNU_SOURCE
#include <string.h>
#include <time.h>
#include <regex.h>
#include "cmor.h"
#include "json.h"
#include "json_tokener.h"
#include "arraylist.h"
#include "libgen.h"
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
        pArray = json_object_get_array(joValue);
        length = array_list_length(pArray);
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
    int i;

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
    for (i = 0; i < CMOR_MAX_JSON_ARRAY; i++) {
        CV->aszValue[i][0] = '\0';
    }
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
/* -------------------------------------------------------------------- */
/* Recursively go down the tree and free branch                         */
/* -------------------------------------------------------------------- */
    if (CV->oValue != NULL) {
        for (k = 0; k < CV->nbObjects; k++) {
            cmor_CV_free(&CV->oValue[k]);
        }
    }
    if (CV->oValue != NULL) {
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
    char msg[CMOR_MAX_STRING];
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
/* Retrieve default Further URL info                                    */
/* -------------------------------------------------------------------- */
    strncpy(szFurtherInfoURLTemplate, cmor_current_dataset.furtherinfourl,
            CMOR_MAX_STRING);

/* -------------------------------------------------------------------- */
/*    If this is a string with no token we have nothing to do.          */
/* -------------------------------------------------------------------- */
    szToken = strtok(szFurtherInfoURLTemplate, "<>");
    if (strcmp(szToken, cmor_current_dataset.furtherinfourl) == 0) {
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
            strlen(szFurtherInfoFileURL));

    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_FURTHERINFOURL) == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_FURTHERINFOURL, szValue);
        if (strncmp(szFurtherInfoURL, szValue, CMOR_MAX_STRING) != 0) {
            cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
            snprintf(msg, CMOR_MAX_STRING,
                     "The further info in attribute does not match "
                     "the one found in your Control Vocabulary(CV) File. \n! "
                     "We found \"%s\" and \n! "
                     "CV requires \"%s\" \n! "
                     "Check your Control Vocabulary file \"%s\".\n! ",
                     szValue, szFurtherInfoURL, CV_Filename);

            cmor_handle_error(msg, CMOR_WARNING);
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
    char msg[CMOR_MAX_STRING];
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
            snprintf(msg, CMOR_MAX_STRING,
                     "You regular expression \"%s\" is invalid. \n! "
                     "Please refer to the CMIP6 documentations.\n! ",
                     szTokenRequired);
            regfree(&regex);
            cmor_handle_error(msg, CMOR_NORMAL);
            cmor_pop_traceback();
            return (-1);
        }
        /* -------------------------------------------------------------------- */
        /*        Execute regular expression                                    */
        /* -------------------------------------------------------------------- */
        reti = regexec(&regex, szSourceType, 0, NULL, 0);
        if (reti == REG_NOMATCH) {
            snprintf(msg, CMOR_MAX_STRING,
                     "The following source type(s) \"%s\" are required and\n! "
                     "some source type(s) could not be found in your "
                     "input file. \n! "
                     "Your file contains a source type of \"%s\".\n! "
                     "Check your Control Vocabulary file \"%s\".\n! ",
                     szReqSourceTypeCpy, szSourceType, CV_Filename);
            regfree(&regex);
            cmor_handle_error(msg, CMOR_NORMAL);
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
        snprintf(msg, CMOR_MAX_STRING,
                 "You source_type attribute contains invalid source types\n! "
                 "Your source type is set to \"%s\".  The required source types\n! "
                 "are \"%s\" and possible additional source types are \"%s\" \n! "
                 "Check your Control Vocabulary file \"%s\".\n! ",
                 szSourceType, szReqSourceTypeCpy, szAddSourceTypeCpy,
                 CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
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
    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int rc;
    int i;
    int j = 0;

    cmor_is_setup();
    cmor_add_traceback("_CV_checkSourceID");
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
/* -------------------------------------------------------------------- */
/*  Find experiment_ids dictionary in Control Vocabulary                */
/* -------------------------------------------------------------------- */
    CV_source_ids = cmor_CV_rootsearch(CV, CV_KEY_SOURCE_IDS);

    if (CV_source_ids == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your \"source_ids\" key could not be found in\n! "
                 "your Control Vocabulary file.(%s)\n! ", CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    // retrieve source_id
    rc = cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE_ID, szSource_ID);
    if (rc != 0) {
        snprintf(msg, CMOR_MAX_STRING,
                 "You \"%s\" is not defined, check your required attributes\n! "
                 "See Control Vocabulary JSON file.(%s)\n! ",
                 GLOBAL_ATT_SOURCE_ID, CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
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
                                                        CV_source_id->aszValue
                                                        [0], 1);
            }
            // Check source with experiment_id label.
            rc = cmor_get_cur_dataset_attribute(GLOBAL_ATT_SOURCE, szSource);
            for (j = 0; j < CV_source_id->nbObjects; j++) {
                if (strcmp(CV_source_id->oValue[j].key, CV_KEY_SOURCE_LABEL) ==
                    0) {
                    break;
                }
            }
            if (j == CV_source_id->nbObjects) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Could not find %s string in source_id section.\n! \n! \n! "
                         "See Control Vocabulary JSON file. (%s)\n! ",
                         CV_KEY_SOURCE_LABEL, CV_Filename);
                cmor_handle_error(msg, CMOR_WARNING);
                break;
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
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" with value \n! \"%s\" "
                         "will be replaced with "
                         "value \n! \"%s\".\n! \n! \n!  "
                         "See Control Vocabulary JSON file.(%s)\n! ",
                         GLOBAL_ATT_SOURCE, szSource,
                         CV_source_id->oValue[j].szValue, CV_Filename);
                cmor_handle_error(msg, CMOR_WARNING);
            }
            break;
        }
    }
    // We could not found the Source ID in the CV file
    if (i == CV_source_ids->nbObjects) {

        snprintf(msg, CMOR_MAX_STRING,
                 "The source_id, \"%s\",  which you specified in your \n! "
                 "input file could not be found in \n! "
                 "your Controlled Vocabulary file. (%s) \n! \n! "
                 "Please correct your input file or contact "
                 "cmor@listserv.llnl.gov to register\n! "
                 "a new source.   ", szSource_ID, CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
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
    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    cmor_add_traceback("_CV_VerifyNBElement");
//    printf("**** CV->key: %s\n", CV->key);
//    printf("**** CV->anElement: %d\n", CV->anElements);
//    printf("**** CV->asValue: %s\n", CV->aszValue[0]);
//    printf("**** CV->szValue: %s\n", CV->szValue);

    if (CV->anElements > 1) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your %s has more than 1 element\n! "
                 "only the first one will be used\n! "
                 "Check your Control Vocabulary file \"%s\".\n! ",
                 CV->key, CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (1);
    } else if (CV->anElements == -1) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your %s has more than 0 element\n! "
                 "Check your Control Vocabulary file \"%s\".\n! ",
                 CV->key, CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (1);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       CV_CompareNoParent()                           */
/************************************************************************/
int CV_CompareNoParent(char *szKey)
{
    char msg[CMOR_MAX_STRING];
    char szValue[CMOR_MAX_STRING];
    cmor_add_traceback("_CV_CompareNoParent");

    if (cmor_has_cur_dataset_attribute(szKey) == 0) {
        cmor_get_cur_dataset_attribute(szKey, szValue);
        if (strcmp(szValue, NO_PARENT) != 0) {
            snprintf(msg, CMOR_MAX_STRING,
                     "Your input attribute %s with value \"%s\" \n! "
                     "will be replaced with value \"%s\".\n! ", szKey,
                     szValue, NO_PARENT);
            cmor_set_cur_dataset_attribute_internal(szKey, NO_PARENT, 1);
            cmor_handle_error(msg, CMOR_WARNING);
            cmor_pop_traceback();
            return (-1);
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

    char msg[CMOR_MAX_STRING];

    cmor_add_traceback("_CV_checkSubExperiment");
    // Initialize variables
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENTID, szExperiment_ID);
    // Look for sub_experiment_id section
    CV_sub_experiment_id = cmor_CV_rootsearch(CV, CV_KEY_SUB_EXPERIMENT_ID);
    if (CV_sub_experiment_id == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your \"sub_experiment_id\" key could not be found in\n! "
                 "your Control Vocabulary file.(%s)\n! ", CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    // Look for experiment_id section
    CV_experiment_id = cmor_CV_rootsearch(CV, CV_KEY_EXPERIMENT_ID);
    if (CV_experiment_id == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your \"experiment_id\" key could not be found in\n! "
                 "your Control Vocabulary file.(%s)\n! ", CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    // Get specified experiment
    CV_experiment = cmor_CV_search_child_key(CV_experiment_id, szExperiment_ID);
    if (CV_experiment == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your experiment_id \"%s\" defined in your input file\n! "
                 "could not be found in your Control Vocabulary file.(%s)\n! ",
                 szExperiment_ID, CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    // sub_experiment_id
    CV_experiment_sub_exp_id = cmor_CV_search_child_key(CV_experiment,
                                                        GLOBAL_ATT_SUB_EXPT_ID);
    if (CV_experiment_sub_exp_id == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your \"%s\" defined in your input file\n! "
                 "could not be found in your Control Vocabulary file.(%s)\n! ",
                 GLOBAL_ATT_SUB_EXPT_ID, CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    // Check sub_experiment_id value
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT_ID) != 0) {
        // sub_experiment_id not found and set to "none"
        if (CV_IsStringInArray(CV_experiment_sub_exp_id, NONE)) {
            snprintf(msg, CMOR_MAX_STRING,
                     "Your input attribute \"%s\" was not defined and \n! "
                     "will be set to \"%s\"\n! "
                     "as defined in your Control Vocabulary file \"%s\".\n! ",
                     GLOBAL_ATT_SUB_EXPT_ID, NONE, CV_Filename);
            cmor_handle_error(msg, CMOR_WARNING);
            cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SUB_EXPT_ID,
                                                    NONE, 1);
        } else {
            // can't be "none".
            snprintf(msg, CMOR_MAX_STRING,
                     "Your input attribute \"%s\" is not defined properly \n! "
                     "for your experiment \"%s\" \n! \n! "
                     "See Control Vocabulary JSON file.(%s)\n! ",
                     GLOBAL_ATT_SUB_EXPT_ID, szExperiment_ID, CV_Filename);
            cmor_handle_error(msg, CMOR_NORMAL);
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
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" defined as \"%s\" "
                         "will be replaced with \n! "
                         "\"%s\" as defined in your Control Vocabulary file.\n! ",
                         GLOBAL_ATT_SUB_EXPT_ID, szSubExptID,
                         CV_experiment_sub_exp_id->aszValue[0]);
                cmor_handle_error(msg, CMOR_WARNING);
                cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SUB_EXPT_ID,
                                                        CV_experiment_sub_exp_id->aszValue
                                                        [0], 1);

            } else {
                // too many options.
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined properly \n! "
                         "for your experiment \"%s\"\n! "
                         "There is more than 1 option for this sub_experiment.\n! "
                         "See Control Vocabulary JSON file.(%s)\n! ",
                         GLOBAL_ATT_SUB_EXPT_ID, szExperiment_ID, CV_Filename);
                cmor_handle_error(msg, CMOR_NORMAL);
                cmor_pop_traceback();
                return (-1);
            }
        }
    }
    // sub_experiment has not been defined!
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT) != 0) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your input attribute \"%s\" was not defined and \n! "
                 "will be set to \"%s\" \n! "
                 "as defined in your Control Vocabulary file \"%s\".\n! ",
                 GLOBAL_ATT_SUB_EXPT, NONE, CV_Filename);
        cmor_handle_error(msg, CMOR_WARNING);
        cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_SUB_EXPT, NONE, 1);
    } else {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_SUB_EXPT, szValue);
        CV_sub_experiment_id_key =
          cmor_CV_search_child_key(CV_sub_experiment_id, szSubExptID);
        if (CV_sub_experiment_id_key == NULL) {
            snprintf(msg, CMOR_MAX_STRING,
                     "Your \"sub_experiment\" text describing  \n! "
                     "sub_experiment_id \"%s\" could not be found in \n! "
                     "your Control Vocabulary file.(%s)\n! ", szSubExptID,
                     CV_Filename);
            cmor_handle_error(msg, CMOR_NORMAL);
            cmor_pop_traceback();
            return (-1);
        }

        if (strcmp(szValue, CV_sub_experiment_id_key->szValue) != 0) {
            snprintf(msg, CMOR_MAX_STRING,
                     "Your input attribute \"%s\" defined as \"%s\" "
                     "will be replaced with \n! "
                     "\"%s\" as defined in your Control Vocabulary file.\n! ",
                     GLOBAL_ATT_SUB_EXPT, szValue,
                     CV_sub_experiment_id_key->szValue);
            cmor_handle_error(msg, CMOR_WARNING);
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
    regex_t regex;

    char CV_Filename[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    int rc;

    szParentExpValue[0] = '\0';
    cmor_add_traceback("_CV_checkParentExpID");

    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENTID, szExperiment_ID);
    // Look for experiment_id section
    CV_experiment_ids = cmor_CV_rootsearch(CV, CV_KEY_EXPERIMENT_ID);
    if (CV_experiment_ids == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your \"experiment_id\" key could not be found in\n! "
                 "your Control Vocabulary file.(%s)\n! ", CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    // Get specified experiment
    CV_experiment = cmor_CV_search_child_key(CV_experiment_ids,
                                             szExperiment_ID);
    if (CV_experiment == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your experiment_id \"%s\" defined in your input file\n! "
                 "could not be found in your Control Vocabulary file.(%s)\n! ",
                 szExperiment_ID, CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    // Do we have a parent_experiment_id?
    if (cmor_has_cur_dataset_attribute(GLOBAL_ATT_PARENT_EXPT_ID) != 0) {
        CV_parent_exp_id = cmor_CV_search_child_key(CV_experiment,
                                                    PARENT_ACTIVITY_ID);
        if (CV_IsStringInArray(CV_parent_exp_id, NO_PARENT)) {
            cmor_pop_traceback();
            return (0);
        } else {
            snprintf(msg, CMOR_MAX_STRING,
                     "Your input attribute \"%s\" is not defined properly \n! "
                     "for your experiment \"%s\"\n!\n! "
                     "See Control Vocabulary JSON file.(%s)\n! ",
                     GLOBAL_ATT_PARENT_EXPT_ID, CV_experiment->key,
                     CV_Filename);
            cmor_handle_error(msg, CMOR_CRITICAL);
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
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your input attribute branch_time_in_child \"%s\" "
                             "is not a double floating point \n! ",
                             szBranchTimeInChild);
                    cmor_handle_error(msg, CMOR_WARNING);
                }

            }
            // Do we have branch_time_in_parent?
            if (cmor_has_cur_dataset_attribute(BRANCH_TIME_IN_PARENT) == 0) {
                cmor_get_cur_dataset_attribute(BRANCH_TIME_IN_PARENT, szValue);
                if (strcmp(szValue, "0.0") != 0) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your input attribute %s %s \n! "
                             "has been replaced with 0.0 \n! ",
                             BRANCH_TIME_IN_PARENT, szValue);
                    cmor_set_cur_dataset_attribute_internal
                      (BRANCH_TIME_IN_PARENT, "0.0", 1);

                    cmor_handle_error(msg, CMOR_WARNING);
                }
            }

            cmor_pop_traceback();
            return (0);
        } else {
            // real parent case
            // Parent Activity ID
            if (cmor_has_cur_dataset_attribute(PARENT_ACTIVITY_ID) != 0) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined properly \n! "
                         "for your experiment \"%s\"\n!\n! "
                         "See Control Vocabulary JSON file.(%s)\n! ",
                         PARENT_ACTIVITY_ID, CV_experiment->key, CV_Filename);
                cmor_handle_error(msg, CMOR_CRITICAL);

            } else {
                cmor_get_cur_dataset_attribute(PARENT_ACTIVITY_ID, szValue);
                CV_parent_activity_id = cmor_CV_search_child_key(CV_experiment,
                                                                 PARENT_ACTIVITY_ID);
                if (CV_IsStringInArray(CV_parent_activity_id, szValue) == 0) {
                    if (CV_parent_activity_id->anElements == 1) {
                        snprintf(msg, CMOR_MAX_STRING,
                                 "Your input attribute parent_activity_id \"%s\" defined as \"%s\" "
                                 "will be replaced with \n! "
                                 "\"%s\" as defined in your Control Vocabulary file.\n! ",
                                 PARENT_ACTIVITY_ID, szValue,
                                 CV_parent_activity_id->aszValue[0]);
                        cmor_handle_error(msg, CMOR_WARNING);
                        cmor_set_cur_dataset_attribute_internal
                          (PARENT_ACTIVITY_ID,
                           CV_parent_activity_id->aszValue[0], 1);

                    } else {
                        snprintf(msg, CMOR_MAX_STRING,
                                 "Your input attribute \"%s\" is not defined properly \n! "
                                 "for your experiment \"%s\"\n! "
                                 "There is more than 1 option for this experiment.\n! "
                                 "See Control Vocabulary JSON file.(%s)\n! ",
                                 PARENT_ACTIVITY_ID, CV_experiment->key,
                                 CV_Filename);
                        cmor_handle_error(msg, CMOR_WARNING);
                    }
                }
            }
            // branch method
            if (cmor_has_cur_dataset_attribute(BRANCH_METHOD)) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please describe the spin-up procedure as defined \n! "
                         "in CMIP6 documentations.\n! ",
                         BRANCH_METHOD, szExperiment_ID);
                cmor_handle_error(msg, CMOR_CRITICAL);

            } else {
                cmor_get_cur_dataset_attribute(BRANCH_METHOD, szBranchMethod);
                if (strlen(szBranchMethod) == 0) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your input attribute %s is an empty string\n! "
                             "Please describe the spin-up procedure as defined \n! "
                             "in CMIP6 documentations.\n! ", BRANCH_METHOD);
                }
            }
            // branch_time_in_child
            if (cmor_has_cur_dataset_attribute(BRANCH_TIME_IN_CHILD)) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         BRANCH_TIME_IN_CHILD, szExperiment_ID);
                cmor_handle_error(msg, CMOR_CRITICAL);

            } else {
                cmor_get_cur_dataset_attribute(BRANCH_TIME_IN_CHILD,
                                               szBranchTimeInChild);
                rc = sscanf(szBranchTimeInChild, "%lf", &dBranchTimeInChild);
                if ((rc == 0) || (rc == EOF)) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your input attribute branch_time_in_child \"%s\" "
                             "is not a double floating point \n! ",
                             szBranchTimeInChild);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
            }

            // branch_time_in_parent
            if (cmor_has_cur_dataset_attribute(BRANCH_TIME_IN_PARENT)) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         BRANCH_TIME_IN_PARENT, szExperiment_ID);
                cmor_handle_error(msg, CMOR_CRITICAL);

            } else {
                cmor_get_cur_dataset_attribute(BRANCH_TIME_IN_PARENT,
                                               szBranchTimeInParent);
                rc = sscanf(szBranchTimeInParent, "%lf", &dBranchTimeInParent);
                if ((rc == 0) || (rc == EOF)) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your input attribute branch_time_in_parent \"%s\" "
                             "is not a double floating point \n! ",
                             szBranchTimeInParent);
                    cmor_handle_error(msg, CMOR_CRITICAL);
                }
            }
            // parent_time_units
            if (cmor_has_cur_dataset_attribute(PARENT_TIME_UNITS)) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         PARENT_TIME_UNITS, szExperiment_ID);
                cmor_handle_error(msg, CMOR_CRITICAL);

            } else {
                char template[CMOR_MAX_STRING];
                int reti;
                cmor_get_cur_dataset_attribute(PARENT_TIME_UNITS,
                                               szParentTimeUnits);
                strcpy(template,
                       "^days[[:space:]]since[[:space:]][[:digit:]]\\{4,4\\}-[[:digit:]]\\{1,2\\}-[[:digit:]]\\{1,2\\}");

                reti = regcomp(&regex, template, 0);
                if (reti) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "You regular expression \"%s\" is invalid. \n! "
                             "Please refer to the CMIP6 documentations.\n! ",
                             template);
                    regfree(&regex);
                    cmor_handle_error(msg, CMOR_NORMAL);
                    cmor_pop_traceback();
                    return (-1);
                }
/* -------------------------------------------------------------------- */
/*        Execute regular expression                                    */
/* -------------------------------------------------------------------- */
                reti = regexec(&regex, szParentTimeUnits, 0, NULL, 0);
                if (reti == REG_NOMATCH) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your  \"%s\" set to \"%s\" is invalid. \n! "
                             "Please refer to the CMIP6 documentations.\n! ",
                             PARENT_TIME_UNITS, szParentTimeUnits);
                    regfree(&regex);
                    cmor_handle_error(msg, CMOR_NORMAL);
                }
                regfree(&regex);
            }
            // parent_variant_label
            if (cmor_has_cur_dataset_attribute(PARENT_VARIANT_LABEL)) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         PARENT_VARIANT_LABEL, szExperiment_ID);
                cmor_handle_error(msg, CMOR_CRITICAL);

            } else {
                char template[CMOR_MAX_STRING];
                int reti;
                cmor_get_cur_dataset_attribute(PARENT_VARIANT_LABEL,
                                               szParentVariantLabel);
                strcpy(template,
                       "^r[[:digit:]]\\{1,\\}i[[:digit:]]\\{1,\\}p[[:digit:]]\\{1,\\}f[[:digit:]]\\{1,\\}$");

                reti = regcomp(&regex, template, 0);
                if (reti) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "You regular expression \"%s\" is invalid. \n! "
                             "Please refer to the CMIP6 documentations.\n! ",
                             template);
                    regfree(&regex);
                    cmor_handle_error(msg, CMOR_NORMAL);
                }
/* -------------------------------------------------------------------- */
/*        Execute regular expression                                    */
/* -------------------------------------------------------------------- */
                reti = regexec(&regex, szParentVariantLabel, 0, NULL, 0);
                if (reti == REG_NOMATCH) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "You  \"%s\" set to \"%s\" is invalid. \n! "
                             "Please refer to the CMIP6 documentations.\n! ",
                             PARENT_VARIANT_LABEL, szParentVariantLabel);
                    regfree(&regex);
                    cmor_handle_error(msg, CMOR_NORMAL);
                }
                regfree(&regex);
            }
            // parent_source_id
            if (cmor_has_cur_dataset_attribute(PARENT_SOURCE_ID) != 0) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         PARENT_SOURCE_ID, szExperiment_ID);
                cmor_handle_error(msg, CMOR_NORMAL);

            } else {
                cmor_get_cur_dataset_attribute(PARENT_SOURCE_ID,
                                               szParentSourceId);
                CV_source_id = cmor_CV_rootsearch(CV, CV_KEY_SOURCE_IDS);
                if (CV_source_id == NULL) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your \"source_id\" key could not be found in\n! "
                             "your Control Vocabulary file.(%s)\n! ",
                             CV_Filename);

                    cmor_handle_error(msg, CMOR_NORMAL);
                    cmor_pop_traceback();
                    return (-1);
                }
                // Get specified experiment
                cmor_get_cur_dataset_attribute(PARENT_SOURCE_ID,
                                               szParentSourceId);
                CV_source = cmor_CV_search_child_key(CV_source_id,
                                                     szParentSourceId);
                if (CV_source == NULL) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your parent_source_id \"%s\" defined in your input file\n! "
                             "could not be found in your Control Vocabulary file.(%s)\n! ",
                             szParentSourceId, CV_Filename);
                    cmor_handle_error(msg, CMOR_NORMAL);
                    cmor_pop_traceback();
                    return (-1);
                }
            }
            // parent_mip_era
            if (cmor_has_cur_dataset_attribute(PARENT_MIP_ERA) != 0) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your input attribute \"%s\" is not defined \n! "
                         "properly for %s \n! "
                         "Please refer to the CMIP6 documentations.\n! ",
                         PARENT_MIP_ERA, szExperiment_ID);
                cmor_handle_error(msg, CMOR_CRITICAL);

            } else {
                cmor_get_cur_dataset_attribute(PARENT_MIP_ERA, szValue);
                if (strcmp(CMIP6, szValue) != 0) {
                    snprintf(msg, CMOR_MAX_STRING,
                             "Your input attribute \"%s\" defined as \"%s\" "
                             "will be replaced with \n! "
                             "\"%s\" as defined in your Control Vocabulary file.\n! ",
                             PARENT_MIP_ERA, szValue, CMIP6);
                    cmor_handle_error(msg, CMOR_WARNING);
                    cmor_set_cur_dataset_attribute_internal(PARENT_MIP_ERA,
                                                            szValue, 1);
                }
            }
        }
    }
    cmor_pop_traceback();
    return (0);
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
    char msg[CMOR_MAX_STRING];
    char szValue[CMOR_MAX_STRING];
    char szExpValue[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    int rc;
    int nObjects;
    int i;
    int j;
    int bWarning;

    szExpValue[0] = '\0';
    cmor_add_traceback("_CV_checkExperiment");
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    cmor_get_cur_dataset_attribute(GLOBAL_ATT_EXPERIMENTID, szExperiment_ID);
/* -------------------------------------------------------------------- */
/*  Find experiment_ids dictionary in Control Vocabulary                */
/* -------------------------------------------------------------------- */
    CV_experiment_ids = cmor_CV_rootsearch(CV, CV_KEY_EXPERIMENT_ID);
    if (CV_experiment_ids == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your \"experiment_ids\" key could not be found in\n! "
                 "your Control Vocabulary file.(%s)\n! ", CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    CV_experiment = cmor_CV_search_child_key(CV_experiment_ids,
                                             szExperiment_ID);

    if (CV_experiment == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your experiment_id \"%s\" defined in your input file\n! "
                 "could not be found in your Control Vocabulary file.(%s)\n! ",
                 szExperiment_ID, CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }

    nObjects = CV_experiment->nbObjects;
    // Parse all experiment attributes
    for (i = 0; i < nObjects; i++) {
        bWarning = FALSE;
        CV_experiment_attr = &CV_experiment->oValue[i];
        rc = cmor_has_cur_dataset_attribute(CV_experiment_attr->key);
        strcpy(szExpValue, CV_experiment_attr->szValue);
        // Validate source type first
        if (strcmp(CV_experiment_attr->key, CV_EXP_ATTR_DESCRIPTION) == 0) {
            continue;
        }
        if (strcmp(CV_experiment_attr->key, CV_EXP_ATTR_REQSOURCETYPE) == 0) {
            cmor_CV_checkSourceType(CV_experiment, szExperiment_ID);
            continue;
        }
        // Warn user if experiment value from input file is different than
        // Control Vocabulary value.
        // experiment from Control Vocabulary will replace User entry value.
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
                        bWarning = TRUE;
                    } else {
                        snprintf(msg, CMOR_MAX_STRING,
                                 "Your input attribute \"%s\" with value \n! \"%s\" "
                                 "is not set properly and \n! "
                                 "has multiple possible candidates \n! "
                                 "defined for experiment_id \"%s\".\n! \n!  "
                                 "See Control Vocabulary JSON file.(%s)\n! ",
                                 CV_experiment_attr->key, szValue,
                                 CV_experiment->key, CV_Filename);
                        cmor_handle_error(msg, CMOR_CRITICAL);

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
                        bWarning = TRUE;
                    }
                }
            }
        }
        if (bWarning == TRUE) {
            snprintf(msg, CMOR_MAX_STRING,
                     "Your input attribute \"%s\" with value \n! \"%s\" "
                     "will be replaced with "
                     "value \"%s\"\n! "
                     "as defined for experiment_id \"%s\".\n! \n!  "
                     "See Control Vocabulary JSON file.(%s)\n! ",
                     CV_experiment_attr->key, szValue, szExpValue,
                     CV_experiment->key, CV_Filename);
            cmor_handle_error(msg, CMOR_WARNING);
        }
        // Set/replace attribute.
        cmor_set_cur_dataset_attribute_internal(CV_experiment_attr->key,
                                                szExpValue, 1);
        if (cmor_has_cur_dataset_attribute(CV_experiment_attr->key) == 0) {
            cmor_get_cur_dataset_attribute(CV_experiment_attr->key, szValue);
        }
    }
    cmor_pop_traceback();

    return (0);
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
    char msg[CMOR_MAX_STRING];
    cdCompTime comptime;
    int i, j, n;
    int ierr;
    int timeDim;
    cdCompTime starttime, endtime;
    int axis_id;

    outname[0] = '\0';
    ierr = cmor_CreateFromTemplate(0, cmor_current_dataset.file_template,
            outname, "_");
    cmor_get_cur_dataset_attribute(CV_INPUTFILENAME, CV_Filename);
    timeDim = -1;
    for (i = 0; i < cmor_tables[0].vars[0].ndims; i++) {
        int dim = cmor_tables[0].vars[0].dimensions[i];
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
            snprintf(szInTimeUnits, CMOR_MAX_STRING,
                    "Cannot convert times for calendar: %s,\n! "
                            "closing variable %s (table: %s)", szInTimeCalendar,
                    cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id);
            cmor_handle_error_var(szInTimeCalendar, CMOR_CRITICAL, var_id);
            cmor_pop_traceback();
            return (1);
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
        } else if (strstr(frequency, "monClim") != NULL) {
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
            start_minutes = round(
                    (starttime.hour - (int) starttime.hour) * 60.);
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
            // frequency is monClim
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
            snprintf(msg, CMOR_MAX_STRING,
                    "Cannot find frequency %s. Closing variable %s (table: %s)",
                    frequency, cmor_vars[var_id].id,
                    cmor_tables[cmor_vars[var_id].ref_table_id].szTable_id);
            cmor_handle_error_var(msg, CMOR_CRITICAL, var_id);
            cmor_pop_traceback();
            return (1);
        }

        strncat(outname, "_", CMOR_MAX_STRING - strlen(outname));
        strncat(outname, start_string, CMOR_MAX_STRING - strlen(outname));
        strncat(outname, "-", CMOR_MAX_STRING - strlen(outname));
        strncat(outname, end_string, CMOR_MAX_STRING - strlen(outname));

        if (cmor_tables[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_table_id].axes[cmor_axes[cmor_vars[var_id].axes_ids[0]].ref_axis_id].climatology
                == 1) {
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
        snprintf(szTmp, CMOR_MAX_STRING, "Your filename \n! "
                "\"%s\" \n! "
                "does not match the CMIP6 requirement.\n! \n! "
                "Your output filename should be: \n! "
                "\"%s\"\n! \n! "
                "and should follow this template: \n!"
                "\"%s\"\n! \n! "
                "See your Control Vocabulary file.(%s)\n! ", infile, outname,
                cmor_current_dataset.file_template, CV_Filename);

        cmor_handle_error_var(szTmp, CMOR_NORMAL, var_id);
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

    char msg[CMOR_MAX_STRING];
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
/*  Find Institution dictionaries in Control Vocabulary                 */
/* -------------------------------------------------------------------- */
    CV_institution_ids = cmor_CV_rootsearch(CV, CV_KEY_INSTITUTION_ID);
    if (CV_institution_ids == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your \"%s\" key could not be found in\n! "
                 "your Control Vocabulary file.(%s)\n! ",
                 CV_KEY_INSTITUTION_ID, CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    CV_institution = cmor_CV_search_child_key(CV_institution_ids,
                                              szInstitution_ID);

    if (CV_institution == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "The institution_id, \"%s\",  found in your \n! "
                 "input file (%s) could not be found in \n! "
                 "your Controlled Vocabulary file. (%s) \n! \n! "
                 "Please correct your input file or contact "
                 "\"cmor@listserv.llnl.gov\" to register\n! "
                 "a new institution_id.  \n! \n! "
                 "See \"http://cmor.llnl.gov/mydoc_cmor3_CV/\" for further information about\n! "
                 "the \"institution_id\" and \"institution\" global attributes.  ",
                 szInstitution_ID, CMOR_Filename, CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
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
/* Control Vocabulary file.  If it does not matches tell the user       */
/* the we will supersede his definition with the one in the Control     */
/* Vocabulary file.                                                     */
/* -------------------------------------------------------------------- */
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_INSTITUTION, szInstitution);

/* -------------------------------------------------------------------- */
/*  Check if an institution has been defined! If not we exit.           */
/* -------------------------------------------------------------------- */
        if (CV_institution->szValue[0] == '\0') {
            snprintf(msg, CMOR_MAX_STRING,
                     "There is no institution associated to institution_id \"%s\"\n! "
                     "in your Control Vocabulary file.\n! "
                     "Check your \"%s\" dictionary!!\n! ",
                     CV_KEY_INSTITUTION_ID, szInstitution_ID);
            cmor_handle_error(msg, CMOR_NORMAL);
            cmor_pop_traceback();
            return (-1);
        }
/* -------------------------------------------------------------------- */
/*  Check if they have the same string                                  */
/* -------------------------------------------------------------------- */
        if (strncmp(szInstitution, CV_institution->szValue, CMOR_MAX_STRING) !=
            0) {
            snprintf(msg, CMOR_MAX_STRING,
                     "Your input attribute institution \"%s\" will be replaced with \n! "
                     "\"%s\" as defined in your Control Vocabulary file.\n! ",
                     szInstitution, CV_institution->szValue);
            cmor_handle_error(msg, CMOR_WARNING);
        }
    }
/* -------------------------------------------------------------------- */
/*   Set institution according to the Control Vocabulary                */
/* -------------------------------------------------------------------- */
    cmor_set_cur_dataset_attribute_internal(GLOBAL_ATT_INSTITUTION,
                                            CV_institution->szValue, 1);
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                    cmor_CV_ValidateAttribute()                       */
/*                                                                      */
/*  Search for attribute and verify that values is within a list        */
/*                                                                      */
/*    i.e. "mip_era": [ "CMIP6", "CMIP5" ]                              */
/************************************************************************/
int cmor_CV_ValidateAttribute(cmor_CV_def_t * CV, char *szKey)
{
    cmor_CV_def_t *attr_CV;
    char szValue[CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    char szValids[CMOR_MAX_STRING * 2];
    char szOutput[CMOR_MAX_STRING * 2];
    char szTmp[CMOR_MAX_STRING];
    int i;
    regex_t regex;
    int reti;
    int ierr;
    int nValueLen;

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
            snprintf(msg, CMOR_MAX_STRING,
                     "You regular expression \"%s\" is invalid. \n! "
                     "Check your Control Vocabulary file \"%s\".\n! ",
                     attr_CV->aszValue[i], CV_Filename);
            regfree(&regex);
            cmor_handle_error(msg, CMOR_NORMAL);
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
/* We could not validate this attribute, exit.                          */
/* -------------------------------------------------------------------- */
    if (i == (attr_CV->anElements)) {
        for (i = 0; i < attr_CV->anElements; i++) {
            strcat(szValids, "\"");
            strncat(szValids, attr_CV->aszValue[i], CMOR_MAX_STRING);
            strcat(szValids, "\" ");
        }
        snprintf(szOutput, 132, "%s ...", szValids);

        snprintf(msg, CMOR_MAX_STRING,
                 "The attribute \"%s\" could not be validated. \n! "
                 "The current input value is "
                 "\"%s\" which is not valid \n! "
                 "Valid values must match the regular expression:"
                 "\n! \t[%s] \n! \n! "
                 "Check your Control Vocabulary file \"%s\".\n! ",
                 szKey, szValue, szOutput, CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
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
    char msg[CMOR_MAX_STRING];
    char CV_Filename[CMOR_MAX_STRING];
    char szCompare[CMOR_MAX_STRING];

    cmor_CV_def_t *CV_grid_labels;
    cmor_CV_def_t *CV_grid_resolution;
    int i;

    cmor_add_traceback("_CV_checkGrids");

    rc = cmor_has_cur_dataset_attribute(GLOBAL_ATT_GRID_LABEL);
    if (rc == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_GRID_LABEL, szGridLabel);
    }
/* -------------------------------------------------------------------- */
/*  "gr followed by a digit is a valid grid (regrid)                    */
/* -------------------------------------------------------------------- */
    if ((strcmp(szGridLabel, CV_KEY_GRIDLABEL_GR) >= '0') &&
        (strcmp(szGridLabel, CV_KEY_GRIDLABEL_GR) <= '9')) {
        strcpy(szGridLabel, CV_KEY_GRIDLABEL_GR);
    }

    rc = cmor_has_cur_dataset_attribute(GLOBAL_ATT_GRID_RESOLUTION);
    if (rc == 0) {
        cmor_get_cur_dataset_attribute(GLOBAL_ATT_GRID_RESOLUTION,
                                       szGridResolution);
    }

    CV_grid_labels = cmor_CV_rootsearch(CV, CV_KEY_GRID_LABELS);
    if (CV_grid_labels == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your \"grid_labels\" key could not be found in\n! "
                 "your Control Vocabulary file.(%s)\n! ", CV_Filename);

        cmor_handle_error(msg, CMOR_NORMAL);
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
            snprintf(msg, CMOR_MAX_STRING,
                     "Your attribute grid_label is set to \"%s\" which is invalid."
                     "\n! \n! Check your Control Vocabulary file \"%s\".\n! ",
                     szGridLabel, CV_Filename);
            cmor_handle_error(msg, CMOR_NORMAL);
            cmor_pop_traceback();
            return (-1);

        }
    }
    CV_grid_resolution = cmor_CV_rootsearch(CV, CV_KEY_GRID_RESOLUTION);
    if (CV_grid_resolution == NULL) {
        snprintf(msg, CMOR_MAX_STRING,
                 "Your attribute grid_label is set to \"%s\" which is invalid."
                 "\n! \n! Check your Control Vocabulary file \"%s\".\n! ",
                 szGridLabel, CV_Filename);
        cmor_handle_error(msg, CMOR_NORMAL);
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
            snprintf(msg, CMOR_MAX_STRING,
                     "Your attribute grid_resolution is set to \"%s\" which is invalid."
                     "\n! \n! Check your Control Vocabulary file \"%s\".\n! ",
                     szGridResolution, CV_Filename);
            cmor_handle_error(msg, CMOR_NORMAL);
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
    char msg[CMOR_MAX_STRING];
    int bCriticalError = FALSE;
    int ierr = 0;
    cmor_add_traceback("_CV_checkGblAttributes");
    required_attrs = cmor_CV_rootsearch(CV, CV_KEY_REQUIRED_GBL_ATTRS);
    if (required_attrs != NULL) {
        for (i = 0; i < required_attrs->anElements; i++) {
            rc = cmor_has_cur_dataset_attribute(required_attrs->aszValue[i]);
            if (rc != 0) {
                snprintf(msg, CMOR_MAX_STRING,
                         "Your Control Vocabulary file specifies one or more\n! "
                         "required attributes.  The following\n! "
                         "attribute was not properly set.\n! \n! "
                         "Please set attribute: \"%s\" in your input file.",
                         required_attrs->aszValue[i]);
                cmor_handle_error(msg, CMOR_NORMAL);
                bCriticalError = TRUE;
                ierr += -1;
            }
            rc = cmor_CV_ValidateAttribute(CV, required_attrs->aszValue[i]);
            if (rc != 0) {
                bCriticalError = TRUE;
                ierr += -1;
            }
        }
    }
    if (bCriticalError) {
        cmor_handle_error("Please fix required attributes mentioned in\n! "
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
    char msg[CMOR_MAX_STRING];

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
        snprintf(msg, CMOR_MAX_STRING,
                 "Your global attribute "
                 "\"%s\" set to \"%s\" is not a valid date.\n! "
                 "ISO 8601 date format \"YYYY-MM-DDTHH:MM:SSZ\" is required."
                 "\n! ", szAttribute, szDate);
        cmor_handle_error(msg, CMOR_NORMAL);
        cmor_pop_traceback();
        return (-1);
    }
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                         cmor_CV_variable()                           */
/************************************************************************/
int cmor_CV_variable(int *var_id, char *name, char *units, double *missing,
                     double startime, double endtime,
                     double startimebnds, double endtimebnds)
{

    int vrid = -1;
    int i;
    int iref;
    char msg[CMOR_MAX_STRING];
    char ctmp[CMOR_MAX_STRING];
    cmor_var_def_t refvar;

    cmor_is_setup();

    cmor_add_traceback("cmor_CV_variable");

    if (CMOR_TABLE == -1) {
        cmor_handle_error("You did not define a table yet!", CMOR_CRITICAL);
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
        snprintf(msg, CMOR_MAX_STRING,
                 "Could not find a matching variable for name: '%s'", ctmp);
        cmor_handle_error(msg, CMOR_CRITICAL);
    }

    refvar = cmor_tables[CMOR_TABLE].vars[iref];
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
    cmor_vars[vrid].first_bound = startimebnds;
    cmor_vars[vrid].last_bound = endtimebnds;
    cmor_vars[vrid].first_time = startime;
    cmor_vars[vrid].last_time = endtime;

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

    if ((refvar.flag_values != NULL) && (refvar.flag_values[0] != '\0')) {
        cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FLAGVALUES, 'c',
                                             refvar.flag_values);
    }
    if ((refvar.flag_meanings != NULL) && (refvar.flag_meanings[0] != '\0')) {

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

    if (refvar.type == '\0') {
        cmor_vars[vrid].type = 'f';
    } else {
        cmor_vars[vrid].type = refvar.type;
    }

    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_MISSINGVALUES,
                                         'f', missing);
    cmor_set_variable_attribute_internal(vrid, VARIABLE_ATT_FILLVAL, 'f',
                                         missing);

    cmor_vars[vrid].self = vrid;
    *var_id = vrid;
    cmor_pop_traceback();
    return (0);
};
