#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmor.h"
#include <udunits2.h>

/* ==================================================================== */
/*      functions prototyping                                           */
/* ==================================================================== */

extern int CMOR_TABLE;
extern int cmor_set_variable_attribute_internal(int id,
                                                char *attribute_name,
                                                char type, void *value);

/************************************************************************/
/*                       cmor_init_grid_mapping()                       */
/************************************************************************/
void cmor_init_grid_mapping(cmor_mappings_t * mapping, char *id)
{
    int n;

    cmor_add_traceback("cmor_init_grid_mapping");
    cmor_is_setup();

    mapping->nattributes = 0;
    for (n = 0; n < CMOR_MAX_GRID_ATTRIBUTES; n++) {
        mapping->attributes_names[n][0] = '\0';
    }

    strcpy(mapping->coordinates, "");
    strncpy(mapping->id, id, CMOR_MAX_STRING);
    cmor_pop_traceback();

    return;

}

/************************************************************************/
/*                           cmor_copy_data()                           */
/************************************************************************/
int cmor_copy_data(double **dest1, void *data, char type, int nelts)
{
    int i;
    double *dest;

    cmor_add_traceback("cmor_copy_data");
    dest = *dest1;
/* -------------------------------------------------------------------- */
/*      First free the dest if already allocated                        */
/* -------------------------------------------------------------------- */
    if (dest != NULL) {
        free(dest);
    }
    dest = malloc(nelts * sizeof(double));
    for (i = 0; i < nelts; i++) {
        if (type == 'f')
            dest[i] = (double)((float *)data)[i];
        else if (type == 'i')
            dest[i] = (double)((int *)data)[i];
        else if (type == 'l')
            dest[i] = (double)((long *)data)[i];
        else if (type == 'd')
            dest[i] = (double)((double *)data)[i];
        else {
            cmor_handle_error_variadic(
                "wrong data type: %c", CMOR_CRITICAL, type);
        }
    }
    *dest1 = dest;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                      cmor_has_grid_attribute()                       */
/************************************************************************/

int cmor_has_grid_attribute(int gid, char *name)
{
    int i;
    int grid_id;

    grid_id = -gid - CMOR_MAX_GRIDS;
    for (i = 0; i < cmor_grids[grid_id].nattributes; i++) {
        if (strcmp(name, cmor_grids[grid_id].attributes_names[i]) == 0)
            return (0);
    }
    return (1);
}

/************************************************************************/
/*                      cmor_get_grid_attribute()                       */
/************************************************************************/

int cmor_get_grid_attribute(int gid, char *name, double *value)
{
    int i, j;
    int grid_id;

    grid_id = -gid - CMOR_MAX_GRIDS;
    j = -1;
    for (i = 0; i < cmor_grids[grid_id].nattributes; i++) {
        if (strcmp(name, cmor_grids[grid_id].attributes_names[i]) == 0)
            j = i;
    }
    if (j != -1) {
        *value = cmor_grids[grid_id].attributes_values[j];
        return (0);
    }
    return (1);
}

/************************************************************************/
/*                         cmor_convert_value()                         */
/************************************************************************/

void cmor_convert_value(char *units, char *ctmp, double *tmp)
{
/* -------------------------------------------------------------------- */
/*      Local variables                                                 */
/* -------------------------------------------------------------------- */
    ut_unit *user_units = NULL, *cmor_units = NULL;
    cv_converter *ut_cmor_converter = NULL;
    double value;

    cmor_add_traceback("cmor_convert_value");

    value = *tmp;
    if (units[0] != '\0') {
        cmor_prep_units(ctmp, units, &cmor_units, &user_units,
                        &ut_cmor_converter);
        *tmp = cv_convert_double(ut_cmor_converter, value);
        if (ut_get_status() != UT_SUCCESS) {
            cmor_handle_error_variadic(
                "Udunits: Error converting units from %s to %s",
                CMOR_CRITICAL,
                units, ctmp);
        }

        cv_free(ut_cmor_converter);
        if (ut_get_status() != UT_SUCCESS) {
            cmor_handle_error_variadic(
                "Udunits: Error freeing converter", CMOR_CRITICAL);
        }
        ut_free(cmor_units);
        if (ut_get_status() != UT_SUCCESS) {
            cmor_handle_error_variadic(
                "Udunits: Error freeing units", CMOR_CRITICAL);
        }
        ut_free(user_units);
        if (ut_get_status() != UT_SUCCESS) {
            cmor_handle_error_variadic(
                "Udunits: Error freeing units", CMOR_CRITICAL);
        }
    } else
        *tmp = value;
    cmor_pop_traceback();
    return;
}

/************************************************************************/
/*                      cmor_set_grid_attribute()                       */
/************************************************************************/

int cmor_set_grid_attribute(int gid, char *name, double *value, char *units)
{
    int i, j, iatt;
    int grid_id;
    char ctmp[CMOR_MAX_STRING];
    double tmp;

    cmor_add_traceback("cmor_set_grid_attribute");
    grid_id = -gid - CMOR_MAX_GRIDS;
    iatt = cmor_grids[grid_id].nattributes;
    tmp = *value;

/* -------------------------------------------------------------------- */
/*      locate attribute index                                          */
/* -------------------------------------------------------------------- */

    for (i = 0; i < cmor_grids[grid_id].nattributes; i++) {
        if (strcmp(name, cmor_grids[grid_id].attributes_names[i]) == 0)
            iatt = i;
    }

    if (iatt == cmor_grids[grid_id].nattributes)
        cmor_grids[grid_id].nattributes++;

/* -------------------------------------------------------------------- */
/*      loop thru attributes                                            */
/* -------------------------------------------------------------------- */

    if (strcmp(name, "false_easting") == 0) {
        j = -1;
        for (i = 0; i < cmor_grids[grid_id].ndims; i++) {
            cmor_get_axis_attribute(cmor_grids[grid_id].axes_ids[i],
                                    "standard_name", 'c', &ctmp[0]);
            if (strcmp(ctmp, "projection_x_coordinate") == 0)
                j = i;
        }
        if (j == -1) {
            cmor_handle_error_variadic(
                "grid mapping attribute: 'false easting' "
                "must be set in conjunction with ut_cmor_a "
                "'projection_x_coordinate' axis, I could not find "
                "such an axis on your grid, we will not set this attribute",
                CMOR_NORMAL);
            cmor_pop_traceback();
            return (1);
        }
        cmor_get_axis_attribute(cmor_grids[grid_id].axes_ids[j], "units",
                                'c', &ctmp[0]);
        cmor_convert_value(units, ctmp, &tmp);
    } else if (strcmp(name, "false_northing") == 0) {
        j = -1;
        for (i = 0; i < cmor_grids[grid_id].ndims; i++) {
            cmor_get_axis_attribute(cmor_grids[grid_id].axes_ids[i],
                                    "standard_name", 'c', &ctmp[0]);
            if (strcmp(ctmp, "projection_y_coordinate") == 0)
                j = i;
        }
        if (j == -1) {
            cmor_handle_error_variadic(
                "grid mapping attribute: 'false northing' "
                "must be set in conjunction with a 'projection_y_coordinate' "
                "axis, I could not find such an axis on your grid, "
                "we will not set this attribute",
                CMOR_NORMAL);
            cmor_pop_traceback();
            return (1);
        }
        cmor_get_axis_attribute(cmor_grids[grid_id].axes_ids[j], "units",
                                'c', &ctmp[0]);
        cmor_convert_value(units, ctmp, &tmp);
    } else if (strcmp(name, "grid_north_pole_latitude") == 0
               || strcmp(name, "latitude_of_projection_origin") == 0
               || strcmp(name, "standard_parallel") == 0
               || strcmp(name, "standard_parallel1") == 0
               || strcmp(name, "standard_parallel2") == 0) {
        strcpy(ctmp, "degrees_north");
        cmor_convert_value(units, ctmp, &tmp);
        if ((tmp < -90) || (tmp > 90.)) {
            cmor_handle_error_variadic(
                "%s parameter must be between -90 and 90 %s, will not be set",
                CMOR_NORMAL,
                name, ctmp);
            cmor_pop_traceback();
            return (1);
        }
    } else if (strcmp(name, "grid_north_pole_longitude") == 0
               || strcmp(name, "longitude_of_prime_meridian") == 0
               || strcmp(name, "longitude_of_central_meridian") == 0
               || strcmp(name, "longitude_of_projection_origin") == 0
               || strcmp(name, "north_pole_grid_longitude") == 0
               || strcmp(name, "straight_vertical_longitude_from_pole") == 0) {
        strcpy(ctmp, "degrees_east");
        cmor_convert_value(units, ctmp, &tmp);
        if ((tmp < -180) || (tmp > 180.)) {
            cmor_handle_error_variadic(
                "%s parameter must be between -180 and 180 %s, will not be set",
                CMOR_NORMAL,
                name, ctmp);
            cmor_pop_traceback();
            return (1);
        }
    } else if (strcmp(name, "perspective_point_height") == 0
               || strcmp(name, "semi_major_axis") == 0
               || strcmp(name, "semi_minor_axis") == 0) {
        strcpy(ctmp, "m");
        cmor_convert_value(units, ctmp, &tmp);
    } else if (strcmp(name, "scale_factor_at_central_meridian") == 0
               || strcmp(name, "scale_factor_at_projection_origin") == 0) {
        strcpy(ctmp, "m");
        cmor_convert_value(units, ctmp, &tmp);
        if (tmp < 0) {
            cmor_handle_error_variadic(
                "%s parameter must be between positive, will not be set",
                CMOR_NORMAL,
                name);
            cmor_pop_traceback();
            return (1);
        }
    }
    /*printf("setting: %s to %lf (orig: %lf)\n",name,tmp,value); */
    strncpy(cmor_grids[grid_id].attributes_names[iatt], name, CMOR_MAX_STRING);
    cmor_grids[grid_id].attributes_values[iatt] = tmp;
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_attribute_in_list()                       */
/************************************************************************/

int cmor_attribute_in_list(char *name, int n, char (*atts)[CMOR_MAX_STRING])
{
    int i, found = 1;

    for (i = 0; i < n; i++) {
        if (strcmp(name, atts[i]) == 0)
            found = 0;
    }
    return (found);
}

/************************************************************************/
/*              cmor_grid_valid_mapping_attribute_names()               */
/************************************************************************/

int cmor_grid_valid_mapping_attribute_names(char *name, int *natt, char (*att)
                                            [CMOR_MAX_STRING], int *ndims,
                                            char (*dims)
                                            [CMOR_MAX_STRING])
{
    int i, j;

    *natt = -1;                 /* -1 means mapping name not found */
    *ndims = 0;

    if (strcmp(name, "albers_conical_equal_area") == 0) {
        *natt = 5;
        strcpy(att[0], "standard_parallel");
        strcpy(att[1], "longitude_of_central_meridian");
        strcpy(att[2], "latitude_of_projection_origin");
        strcpy(att[3], "false_easting");
        strcpy(att[4], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "azimuthal_equidistant") == 0) {
        *natt = 4;
        strcpy(att[0], "longitude_of_projection_origin");
        strcpy(att[1], "latitude_of_projection_origin");
        strcpy(att[2], "false_easting");
        strcpy(att[3], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "lambert_azimuthal_equal_area") == 0) {
        *natt = 4;
        strcpy(att[0], "longitude_of_projection_origin");
        strcpy(att[1], "latitude_of_projection_origin");
        strcpy(att[2], "false_easting");
        strcpy(att[3], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "lambert_conformal_conic") == 0) {
        *natt = 5;
        strcpy(att[0], "standard_parallel");
        strcpy(att[1], "longitude_of_central_meridian");
        strcpy(att[2], "latitude_of_projection_origin");
        strcpy(att[3], "false_easting");
        strcpy(att[4], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "lambert_cylindrical_equal_area") == 0) {
        *natt = 5;
        strcpy(att[0], "standard_parallel");
        strcpy(att[1], "longitude_of_central_meridian");
        strcpy(att[2], "scale_factor_at_projection_origin");
        strcpy(att[3], "false_easting");
        strcpy(att[4], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "latitude_longitude") == 0) {
        *natt = 0;
        *ndims = 0;
    } else if (strcmp(name, "mercator") == 0) {
        *natt = 5;
        strcpy(att[0], "standard_parallel");
        strcpy(att[1], "longitude_of_projection_origin");
        strcpy(att[2], "scale_factor_at_projection_origin");
        strcpy(att[3], "false_easting");
        strcpy(att[4], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "orthographic") == 0) {
        *natt = 4;
        strcpy(att[0], "longitude_of_projection_origin");
        strcpy(att[1], "latitude_of_projection_origin");
        strcpy(att[2], "scale_factor_at_projection_origin");
        strcpy(att[3], "false_easting");
        strcpy(att[4], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_x_coordinate");
        strcpy(dims[1], "projection_y_coordinate");
    } else if (strcmp(name, "polar_stereographic") == 0) {
        *natt = 6;
        strcpy(att[0], "straight_vertical_longitude_from_pole");
        strcpy(att[1], "latitude_of_projection_origin");
        strcpy(att[2], "standard_parallel");
        strcpy(att[3], "scale_factor_at_projection_origin");
        strcpy(att[4], "false_easting");
        strcpy(att[5], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "rotated_latitude_longitude") == 0) {
        *natt = 3;
        strcpy(att[0], "grid_north_pole_latitude");
        strcpy(att[1], "grid_north_pole_longitude");
        strcpy(att[2], "north_pole_grid_longitude");
        *ndims = 2;
        strcpy(dims[0], "grid_latitude");
        strcpy(dims[1], "grid_longitude");
    } else if (strcmp(name, "stereographic") == 0) {
        *natt = 5;
        strcpy(att[0], "longitude_of_projection_origin");
        strcpy(att[1], "latitude_of_projection_origin");
        strcpy(att[2], "scale_factor_at_projection_origin");
        strcpy(att[3], "false_easting");
        strcpy(att[4], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "transverse_mercator") == 0) {
        *natt = 5;
        strcpy(att[0], "scale_factor_at_central_meridian");
        strcpy(att[1], "longitude_of_central_meridian");
        strcpy(att[2], "latitude_of_projection_origin");
        strcpy(att[3], "false_easting");
        strcpy(att[4], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    } else if (strcmp(name, "vertical_perspective") == 0) {
        *natt = 5;
        strcpy(att[0], "longitude_of_projection_origin");
        strcpy(att[1], "latitude_of_projection_origin");
        strcpy(att[2], "perspective_height_point");
        strcpy(att[3], "false_easting");
        strcpy(att[4], "false_northing");
        *ndims = 2;
        strcpy(dims[0], "projection_y_coordinate");
        strcpy(dims[1], "projection_x_coordinate");
    }

    /* Now looks up in table */
    for (i = 0; i < cmor_tables[CMOR_TABLE].nmappings; i++) {

        if (strcmp(name, cmor_tables[CMOR_TABLE].mappings[i].id) == 0) {
            /* ok that is the mapping */
            *natt = cmor_tables[CMOR_TABLE].mappings[i].nattributes;
            for (j = 0;
                 j < cmor_tables[CMOR_TABLE].mappings[i].nattributes; j++) {
                strcpy(att[j],
                       cmor_tables[CMOR_TABLE].mappings[i].attributes_names[j]);
            }
        }
    }

    if (*natt != -1) {
        strcpy(att[*natt + 0], "earth_radius");
        strcpy(att[*natt + 1], "inverse_flattening");
        strcpy(att[*natt + 2], "longitude_of_prime_meridian");
        strcpy(att[*natt + 3], "perspective_point_height");
        strcpy(att[*natt + 4], "semi_major_axis");
        strcpy(att[*natt + 5], "semi_minor_axis");
        *natt = *natt + 6;
    }
    return (0);
}

void cmor_set_mapping_attribute(cmor_mappings_t * mapping,
                                char att[CMOR_MAX_STRING],
                                char val[CMOR_MAX_STRING])
{
    int i, n;

    cmor_add_traceback("cmor_set_mapping_attribute");

    if (strcmp(att, "coordinates") == 0) {

        strncpy(mapping->coordinates, val, CMOR_MAX_STRING);

    } else if (strncmp(att, "parameter", 9) == 0) {

        n = -1;

        for (i = 0; i < mapping->nattributes; i++) {

            if (strcmp(mapping->attributes_names[i], val) == 0) {
                n = i;
                break;
            }
        }

        if (n == -1) {
            n = mapping->nattributes;
            mapping->nattributes++;
        }

        strncpy(mapping->attributes_names[n], val, CMOR_MAX_STRING);

    } else {

        cmor_handle_error_variadic(
            "Unknown attribute: '%s' for mapping '%s' (value was: '%s')",
            CMOR_WARNING,
            att, mapping->id, val);

    }
    cmor_pop_traceback();
}

/************************************************************************/
/*                       cmor_set_grid_mapping()                        */
/************************************************************************/

int cmor_set_grid_mapping(int gid, char *name, int nparam,
                          char *attributes_names, int lparams,
                          double
                          attributes_values[CMOR_MAX_GRID_ATTRIBUTES],
                          char *units, int lnunits)
{
    int grid_id, nattributes, ndims, msg_len;
    int i, j, k, l;
    char *achar, *bchar, *axes_msg;
    char lattributes_names[CMOR_MAX_GRID_ATTRIBUTES][CMOR_MAX_STRING];
    char lunits[CMOR_MAX_GRID_ATTRIBUTES][CMOR_MAX_STRING];
    char grid_attributes[CMOR_MAX_GRID_ATTRIBUTES][CMOR_MAX_STRING];
    char msg[CMOR_MAX_STRING];
    char grid_dimensions[CMOR_MAX_DIMENSIONS][CMOR_MAX_STRING];

    cmor_add_traceback("cmor_set_grid_mapping");
    if (nparam >= CMOR_MAX_GRID_ATTRIBUTES) {
        cmor_handle_error_variadic(
            "CMOR allows only %i grid parameters too be defined, "
            "you are trying to define %i parameters, if you really "
            "need that many recompile cmor changing the value of "
            "parameter: CMOR_MAX_GRID_ATTRIBUTES",
            CMOR_CRITICAL,
            CMOR_MAX_GRID_ATTRIBUTES, nparam);
    }
    achar = (char *)attributes_names;
    bchar = (char *)units;
    for (i = 0; i < nparam; i++) {
        strncpy(lattributes_names[i], achar, CMOR_MAX_STRING);
        strncpy(lunits[i], bchar, CMOR_MAX_STRING);
        achar += lparams;
        bchar += lnunits;
    }
    grid_id = -gid - CMOR_MAX_GRIDS;

/* -------------------------------------------------------------------- */
/*      reads in grid definitions                                       */
/* -------------------------------------------------------------------- */

    cmor_grid_valid_mapping_attribute_names(name, &nattributes, grid_attributes,
                                            &ndims, grid_dimensions);

    if (ndims != cmor_grids[grid_id].ndims) {
        cmor_handle_error_variadic(
            "you defined your grid with %i axes but grid_mapping "
            "'%s' requires exactly %i axes",
            CMOR_CRITICAL,
            cmor_grids[grid_id].ndims, name, ndims);
        cmor_pop_traceback();
        return (-1);

    }

/* -------------------------------------------------------------------- */
/*      First of all depending on some grid names we need to make       */
/*      sure we have the right axes set in the right order              */
/* -------------------------------------------------------------------- */

    k = 0;
    for (i = 0; i < ndims; i++) {
        for (j = 0; j < cmor_grids[grid_id].ndims; j++) {
            cmor_get_axis_attribute(cmor_grids[grid_id].original_axes_ids[j],
                                    "standard_name", 'c', &msg);
            if (strcmp(grid_dimensions[i], msg) == 0) {
                cmor_grids[grid_id].axes_ids[i] =
                  cmor_grids[grid_id].original_axes_ids[j];
/* -------------------------------------------------------------------- */
/*      Now we probably need to alter the lat/lon,etc.. associated      */
/*      variables as well !                                             */
/* -------------------------------------------------------------------- */

                for (l = 0; l < 4; l++) {
                    if (cmor_vars
                        [cmor_grids[cmor_ngrids].
                         associated_variables[l]].ndims != 0) {
                        cmor_vars[cmor_grids[cmor_ngrids].associated_variables
                                  [l]].axes_ids[i] =
                          cmor_grids[grid_id].original_axes_ids[j];
                    }
                }
                k++;
            }
        }
    }

    if (k != ndims) {
        msg_len = 0;
        for (i = 0; i < ndims; i++) {
            msg_len += snprintf(NULL, 0, " %s", grid_dimensions[i]);
        }
        msg_len += 1;
        axes_msg = (char *)malloc(msg_len * sizeof(char));
        msg_len = 0;
        for (i = 0; i < ndims; i++) {
            msg_len += sprintf(&axes_msg[msg_len], " %s", grid_dimensions[i]);
        }
        cmor_handle_error_variadic(
            "setting grid mapping to '%s' we could not find all "
            "the required axes, required axes are:%s",
            CMOR_CRITICAL,
            name, axes_msg);
        free(axes_msg);
        cmor_pop_traceback();
        return (-1);

    }

    for (i = 0; i < nparam; i++) {
        if (cmor_attribute_in_list(lattributes_names[i], nattributes,
                                   &grid_attributes[0]) == 1) {
            if ((strcmp(lattributes_names[i], "standard_parallel1") == 0
                 || strcmp(lattributes_names[i], "standard_parallel2") == 0)
                && (strcmp(name, "lambert_conformal_conic") == 0)) {

/* -------------------------------------------------------------------- */
/*      ok do nothing it is just that we need 2 values for this         */
/*      parameter                                                       */
/* -------------------------------------------------------------------- */

                cmor_set_grid_attribute(gid, lattributes_names[i],
                                        &attributes_values[i], lunits[i]);
            } else {
                cmor_handle_error_variadic(
                    "in grid_mapping, attribute '%s' (with value: %lf) "
                    "is not a known attribute for grid mapping: '%s'",
                    CMOR_WARNING,
                    lattributes_names[i], attributes_values[i], name);
                cmor_pop_traceback();
                return (-1);
            }
        } else {
            cmor_set_grid_attribute(gid, lattributes_names[i],
                                    &attributes_values[i], lunits[i]);
        }
    }
/* -------------------------------------------------------------------- */
/*      checks all parameter (but last 6 which are optional) have       */
/*      been set                                                        */
/* -------------------------------------------------------------------- */

    for (i = 0; i < nattributes - 6; i++) {
        if (cmor_has_grid_attribute(gid, grid_attributes[i]) == 1) {
            cmor_handle_error_variadic(
                "Grid mapping attribute %s has not been set, "
                "you should consider setting it",
                CMOR_WARNING,
                grid_attributes[i]);
        }
    }

/* -------------------------------------------------------------------- */
/*      Ok finally we need to copy the name to the grid struct          */
/* -------------------------------------------------------------------- */

    strncpy(cmor_grids[grid_id].mapping, name, CMOR_MAX_STRING);
    strncpy(cmor_grids[grid_id].name, name, CMOR_MAX_STRING);
    cmor_pop_traceback();
    return (0);
}

/************************************************************************/
/*                       cmor_set_crs()                                 */
/************************************************************************/

int cmor_set_crs(int gid, char *grid_mapping, int nparam,
    char *attributes_names, int lparams,
    double attributes_values[CMOR_MAX_GRID_ATTRIBUTES],
    char *units, int lnunits, char *crs_wkt)
{
    int ierr, grid_id, str_size;

    cmor_add_traceback("cmor_set_grid_mapping");

    ierr = cmor_set_grid_mapping(gid, grid_mapping, nparam,
        attributes_names, lparams, attributes_values,
        units, lnunits);

    grid_id = -gid - CMOR_MAX_GRIDS;
    strncpy(cmor_grids[grid_id].name, "crs", CMOR_MAX_STRING);

    if (cmor_grids[grid_id].crs_wkt != NULL) {
        free(cmor_grids[grid_id].crs_wkt);
        cmor_grids[grid_id].crs_wkt = NULL;
    }

    if (crs_wkt != NULL) {
        str_size = strlen(crs_wkt);
        cmor_grids[grid_id].crs_wkt = malloc(str_size * sizeof(char));
        strncpy(cmor_grids[grid_id].crs_wkt, crs_wkt, str_size);
    }

    cmor_pop_traceback();
    return (ierr);
}

/************************************************************************/
/*                 cmor_time_varying_grid_coordinate()                  */
/************************************************************************/

int cmor_time_varying_grid_coordinate(int *coord_grid_id, int grid_id,
                                      char *table_entry, char *units, char type,
                                      void *missing, int *coordinate_type)
{
    int ierr = 0, j;
    int axes[2];
    char msg[CMOR_MAX_STRING];
    int ctype = -1;
    double *dummy_values;
    cmor_grid_t *current_cmor_grids;

    int nvertices;
    int table_id;

    axes[0] = grid_id;
    current_cmor_grids = &cmor_grids[-grid_id - CMOR_MAX_GRIDS];
    nvertices = current_cmor_grids->nvertices;
    //table_id = cmor_axes[current_cmor_grids->axes_ids[0]].ref_table_id;
    table_id = CMOR_TABLE;
    cmor_add_traceback("cmor_time_varying_grid_coordinate");
    cmor_is_setup();

    strcpy(msg, "not found");
    if (coordinate_type == NULL) {
        for (j = 0; j < cmor_tables[table_id].nvars; j++) {
            if (strcmp(cmor_tables[table_id].vars[j].id, table_entry) == 0) {
                strncpy(msg, cmor_tables[table_id].vars[j].standard_name,
                        CMOR_MAX_STRING);
                break;
            }
        }
        if (strcmp(msg, "latitude") == 0)
            ctype = 0;
        if (strcmp(msg, "longitude") == 0)
            ctype = 1;
        if (strcmp(msg, "vertices_latitude") == 0)
            ctype = 2;
        if (strcmp(msg, "vertices_longitude") == 0)
            ctype = 3;
    } else {
        ctype = *coordinate_type;
    }
    switch (ctype) {
      case (0):
          ierr = cmor_variable(coord_grid_id, table_entry, units, 1, axes, type,
                               missing, NULL, NULL, NULL, NULL, NULL);
          cmor_grids[cmor_vars[*coord_grid_id].
                     grid_id].associated_variables[0] = *coord_grid_id;
          break;
      case (1):
          ierr = cmor_variable(coord_grid_id, table_entry, units, 1, axes, type,
                               missing, NULL, NULL, NULL, NULL, NULL);
          cmor_grids[cmor_vars[*coord_grid_id].
                     grid_id].associated_variables[1] = *coord_grid_id;

          break;
      case (2):
          if (nvertices == 0) {
              cmor_handle_error_variadic(
                "your defining a vertices dependent variable (%s) "
                "associated with grid %i, but you declared this grid "
                "as having 0 vertices",
                CMOR_CRITICAL,
                table_entry, grid_id);
          }
          if (cmor_grids[cmor_vars[*coord_grid_id].grid_id].associated_variables
              [3]
              == -1) {
              dummy_values = malloc(sizeof(double) * nvertices);

              for (j = 0; j < nvertices; j++) {
                  dummy_values[j] = (double)j;
              }

              cmor_axis(&axes[1], "vertices", "1", nvertices, dummy_values, 'd',
                        NULL, 0, NULL);

              free(dummy_values);

              cmor_grids[-grid_id - CMOR_MAX_GRIDS].nvertices = axes[1];

          } else {

              axes[1] = cmor_grids[-grid_id - CMOR_MAX_GRIDS].nvertices;

          }
          ierr = cmor_variable(coord_grid_id, table_entry, units, 2, axes, type,
                               missing, NULL, NULL, NULL, NULL, NULL);

          cmor_grids[cmor_vars[*coord_grid_id].
                     grid_id].associated_variables[2] = *coord_grid_id;

/* -------------------------------------------------------------------- */
/*      adds the bounds attribute                                       */
/* -------------------------------------------------------------------- */

          if (cmor_has_variable_attribute
              (cmor_grids
               [cmor_vars[*coord_grid_id].grid_id].associated_variables[0],
               "bounds") == 0) {

              cmor_get_variable_attribute(cmor_grids
                                          [cmor_vars[*coord_grid_id].
                                           grid_id].associated_variables[0],
                                          "bounds", &msg);

              strncat(msg, " ", CMOR_MAX_STRING - strlen(msg));
              strncat(msg, cmor_vars[*coord_grid_id].id,
                      CMOR_MAX_STRING - strlen(msg));

          } else {

              strncpy(msg, cmor_vars[*coord_grid_id].id, CMOR_MAX_STRING);

          }

          cmor_set_variable_attribute_internal(cmor_grids
                                               [cmor_vars
                                                [*coord_grid_id].grid_id].associated_variables
                                               [0], "bounds", 'c', msg);

          break;
      case (3):
          if (nvertices == 0) {

              cmor_handle_error_variadic(
                "your defining a vertices dependent "
                "variable (%s) associated with grid %i, "
                "but you declared this grid as having "
                "0 vertices",
                CMOR_CRITICAL,
                table_entry, grid_id);

          }

          if (cmor_grids[cmor_vars[*coord_grid_id].grid_id].associated_variables
              [2]
              == -1) {

              dummy_values = malloc(sizeof(double) * nvertices);

              for (j = 0; j < nvertices; j++) {
                  dummy_values[j] = (double)j;
              }
              cmor_axis(&axes[1], "vertices", "1", nvertices, dummy_values, 'd',
                        NULL, 0, NULL);

              free(dummy_values);

              cmor_grids[-grid_id - CMOR_MAX_GRIDS].nvertices = axes[1];

          } else {

              axes[1] = cmor_grids[-grid_id - CMOR_MAX_GRIDS].nvertices;

          }
          ierr = cmor_variable(coord_grid_id, table_entry, units, 2, axes, type,
                               missing, NULL, NULL, NULL, NULL, NULL);

          cmor_grids[cmor_vars[*coord_grid_id].
                     grid_id].associated_variables[3] = *coord_grid_id;

          /* adds the bounds attribute */
          if (cmor_has_variable_attribute
              (cmor_grids
               [cmor_vars[*coord_grid_id].grid_id].associated_variables[1],
               "bounds") == 0) {

              cmor_get_variable_attribute(cmor_grids
                                          [cmor_vars[*coord_grid_id].
                                           grid_id].associated_variables[1],
                                          "bounds", &msg);

              strncat(msg, " ", CMOR_MAX_STRING - strlen(msg));

              strncat(msg, cmor_vars[*coord_grid_id].id,
                      CMOR_MAX_STRING - strlen(msg));

          } else {

              strncpy(msg, cmor_vars[*coord_grid_id].id, CMOR_MAX_STRING);

          }
          cmor_set_variable_attribute_internal(cmor_grids
                                               [cmor_vars
                                                [*coord_grid_id].grid_id].associated_variables
                                               [1], "bounds", 'c', msg);
          break;

      default:
          cmor_handle_error_variadic(
            "unknown coord type: %i", CMOR_CRITICAL, ctype);
          cmor_pop_traceback();
          return (-1);

    }
    cmor_vars[*coord_grid_id].needsinit = 0;

    cmor_pop_traceback();
    return (ierr);
}

/************************************************************************/
/*                             cmor_grid()                              */
/************************************************************************/

int cmor_grid(int *grid_id, int ndims, int *axes_ids, char type,
              void *lat, void *lon, int nvertices, void *blat, void *blon)
{

    int i, j, n, did_vertices = 0;
    char msg[CMOR_MAX_STRING];
    int axes[2];
    double *dummy_values;

    cmor_add_traceback("cmor_grid");
    if ((axes_ids == NULL) || (ndims == 0)) {
        cmor_handle_error_variadic(
            "You need to define the grid axes first", CMOR_CRITICAL);
    }

    cmor_ngrids += 1;

    if (cmor_ngrids >= CMOR_MAX_GRIDS) {
        cmor_handle_error_variadic(
            "Too many grids defined, maximum possible "
            "grids is currently set to %i",
            CMOR_CRITICAL,
            CMOR_MAX_GRIDS);
    }

    n = 1;

    for (i = 0; i < ndims; i++) {

        if (axes_ids[i] > cmor_naxes) {
            cmor_handle_error_variadic(
                "Defining grid, Axis %i not defined yet",
                CMOR_CRITICAL,
                axes_ids[i]);
        }

        if (cmor_tables[cmor_axes[axes_ids[i]].ref_table_id].axes
            [cmor_axes[axes_ids[i]].ref_axis_id].axis == 'T') {
            cmor_grids[cmor_ngrids].istimevarying = 1;
        }

        cmor_grids[cmor_ngrids].original_axes_ids[i] = axes_ids[i];
        cmor_grids[cmor_ngrids].axes_ids[i] = axes_ids[i];
        cmor_axes[axes_ids[i]].isgridaxis = 1;
        n *= cmor_axes[axes_ids[i]].length;
    }

    cmor_grids[cmor_ngrids].ndims = ndims;
    cmor_grids[cmor_ngrids].nvertices = nvertices;

    if (lat == NULL) {

        if (cmor_grids[cmor_ngrids].istimevarying != 1) {
            cmor_handle_error_variadic(
                "you need to pass the latitude values when defining a grid",
                CMOR_CRITICAL);
        }

    } else {
        axes[0] = -cmor_ngrids - CMOR_MAX_GRIDS;
        if (cmor_grids[cmor_ngrids].istimevarying != 1) {
            cmor_copy_data(&cmor_grids[cmor_ngrids].lats, lat, type, n);
            cmor_variable(&cmor_grids[cmor_ngrids].associated_variables[0],
                          "latitude", "degrees_north", 1, &axes[0], 'd', NULL,
                          NULL, NULL, NULL, NULL, NULL);
            cmor_vars[cmor_grids[cmor_ngrids].associated_variables[0]].
              needsinit = 0;
        }
    }

    if (lon == NULL) {

        if (cmor_grids[cmor_ngrids].istimevarying != 1) {
            cmor_handle_error_variadic(
                "you need to pass the longitude values when "
                "defining a grid", CMOR_CRITICAL);
        }

    } else {
        cmor_copy_data(&cmor_grids[cmor_ngrids].lons, lon, type, n);
        axes[0] = -cmor_ngrids - CMOR_MAX_GRIDS;

        cmor_variable(&cmor_grids[cmor_ngrids].associated_variables[1],
                      "longitude", "degrees_east", 1, &axes[0], 'd', NULL,
                      NULL, NULL, NULL, NULL, NULL);
        cmor_vars[cmor_grids[cmor_ngrids].associated_variables[1]].needsinit =
          0;
    }
    if (blat == NULL) {
        if (cmor_grids[cmor_ngrids].istimevarying != 1) {
            cmor_handle_error_variadic(
                "it is recommended you pass the latitude bounds "
                "values when defining a grid", CMOR_WARNING);
        }
    } else {
        cmor_copy_data(&cmor_grids[cmor_ngrids].blats, blat, type,
                       n * nvertices);

        dummy_values = malloc(sizeof(double) * nvertices);

        for (j = 0; j < nvertices; j++)
            dummy_values[j] = (double)j;

        cmor_axis(&axes[1], "vertices", "1", nvertices, dummy_values, 'd',
                  NULL, 0, NULL);

        free(dummy_values);

        did_vertices = 1;

        cmor_variable(&cmor_grids[cmor_ngrids].associated_variables[2],
                      "vertices_latitude", "degrees_north", 2, &axes[0],
                      'd', NULL, NULL, NULL, NULL, NULL, NULL);

        cmor_vars[cmor_grids[cmor_ngrids].associated_variables[2]].needsinit =
          0;

        if (cmor_has_variable_attribute
            (cmor_grids[cmor_ngrids].associated_variables[0], "bounds") == 0) {

            cmor_get_variable_attribute(cmor_grids
                                        [cmor_ngrids].associated_variables[0],
                                        "bounds", &msg);
            strncat(msg, " ", CMOR_MAX_STRING - strlen(msg));
            strncat(msg,
                    cmor_vars[cmor_grids[cmor_ngrids].
                              associated_variables[2]].id,
                    CMOR_MAX_STRING - strlen(msg));

        } else {
            strncpy(msg,
                    cmor_vars[cmor_grids[cmor_ngrids].
                              associated_variables[2]].id, CMOR_MAX_STRING);
        }
        cmor_set_variable_attribute_internal(cmor_grids
                                             [cmor_ngrids].associated_variables
                                             [0], "bounds", 'c', msg);
    }
    if (blon == NULL) {

        if (cmor_grids[cmor_ngrids].istimevarying != 1) {
            cmor_handle_error_variadic(
                "it is recommended you pass the longitude bounds values when defining a grid",
                CMOR_WARNING);
        }

    } else {

        cmor_copy_data(&cmor_grids[cmor_ngrids].blons, blon, type,
                       n * nvertices);

        if (did_vertices == 0) {

            dummy_values = malloc(sizeof(double) * nvertices);

            for (j = 0; j < nvertices; j++) {
                dummy_values[j] = (double)j;
            }
            cmor_axis(&axes[1], "vertices", "1", nvertices, dummy_values,
                      'd', NULL, 0, NULL);
            free(dummy_values);
        }

        cmor_variable(&cmor_grids[cmor_ngrids].associated_variables[3],
                      "vertices_longitude", "degrees_east", 2, &axes[0],
                      'd', NULL, NULL, NULL, NULL, NULL, NULL);

        cmor_vars[cmor_grids[cmor_ngrids].associated_variables[3]].needsinit =
          0;

        if (cmor_has_variable_attribute
            (cmor_grids[cmor_ngrids].associated_variables[1], "bounds") == 0) {

            cmor_get_variable_attribute(cmor_grids
                                        [cmor_ngrids].associated_variables[1],
                                        "bounds", &msg);

            strncat(msg, " ", CMOR_MAX_STRING - strlen(msg));
            strncat(msg,
                    cmor_vars[cmor_grids[cmor_ngrids].
                              associated_variables[3]].id,
                    CMOR_MAX_STRING - strlen(msg));

        } else {
            strncpy(msg,
                    cmor_vars[cmor_grids[cmor_ngrids].
                              associated_variables[3]].id, CMOR_MAX_STRING);
        }
        cmor_set_variable_attribute_internal(cmor_grids
                                             [cmor_ngrids].associated_variables
                                             [1], "bounds", 'c', msg);
    }

    *grid_id = -cmor_ngrids - CMOR_MAX_GRIDS;
    cmor_pop_traceback();
    return (0);
}
