program joerg

  USE cmor_users_functions
  IMPLICIT NONE

  integer axes(10),error_flag
    error_flag = cmor_setup(inpath='Test', netcdf_file_action='replace')


  error_flag = cmor_dataset(                                   &
       outpath='Test',                                         &
       experiment_id='abrupt 4XCO2',           &
       institution=                                            &
       'GICC (Generic International Climate Center, ' //       &
       'Geneva, Switzerland)',                                 &
       source='GICCM1 (2002): ' //                             &
       'atmosphere:  GICAM3 (gicam_0_brnchT_itea_2, T63L32); '// &
       'ocean: MOM (mom3_ver_3.5.2, 2x3L15); '             //  &
       'sea ice: GISIM4; land: GILSM2.5',                      &
       calendar='noleap',                                      &
       realization=1,                                          &
       history='Output from archive/giccm_03_std_2xCO2_2256.', &
       institute_id = 'PCMDI', &
       comment='Equilibrium reached after 30-year spin-up ' // &
       'after which data were output starting with nominal '// &
       'date of January 2030',                                 &
       references='Model described by Koder and Tolkien ' //   &
       '(J. Geophys. Res., 2001, 576-591).  Also '        //   &
       'see http://www.GICC.su/giccm/doc/index.html '     //   &
       ' 2XCO2 simulation described in Dorkey et al. '    //   &
       '(Clim. Dyn., 2003, 323-357.)',&
       model_id='GICCM1',forcing='TO',contact="Barry Bonds",&
       parent_experiment_id="N/A",branch_time=bt)

    call cmor_set_table(table_id=ntables(2))
     axes(1) = cmor_axis(                            &
       table_entry        = 'i_index',               &
       length             = nlon,                    &
       coord_vals         = xii,                     &
       units              = '1')

     axes(2) = cmor_axis(                            &
       table_entry        = 'j_index',               &
       length             = nlat,                    &
       coord_vals         = yii,                     &
       units              = '1')

     grid_id = cmor_grid(                            &
       axis_ids           = axes,                    &
       latitude           = olat_val,                &
       longitude          = olon_val,                &
       latitude_vertices  = bnds_olat,               &
       longitude_vertices = bnds_olon)

     call cmor_set_table(table_id=ntables(1))

      var_ids              = cmor_variable(           &
        table_entry        = vartabin(1,i),           &
        units              = vartabin(2,i),                  &
        positive           = vartabin(3,i),           &
        axis_ids           = (/ grid_id, tim_id /),   &
        missing_value      = miss_val(i) )

      error_flag = cmor_write(                        &
         var_id            = var_ids,                 &
         data              = ar5all2d(:,:,:,i),       &
         ntimes_passed     = ntim,                    &
         file_suffix       = SUFFIX,                  &
         time_vals         = time,                    &
         time_bnds         = bnds_time)
