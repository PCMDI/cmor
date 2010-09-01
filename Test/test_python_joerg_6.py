 ntables(2) = CMIP5_grid,
! ntables(1) = CMIP5_Omon,
! the index i runs through a number of variables

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
          table_entry        = vartabin(1,i),           & ! epc100
          units              = vartabin(2,i),           & ! mol m-2 s-1
          positive           = vartabin(3,i),           & ! down
          axis_ids           = (/ tim_id, grid_id /),   &
          missing_value      = miss_val(i) )

        error_flag = cmor_write(                        &
           var_id            = var_ids,                 &
           data              = ar5all2d(:,:,:,i),       &
           ntimes_passed     = ntim,                    &
           file_suffix       = SUFFIX,                  &
           time_vals         = time,                    &
           time_bnds         = bnds_time)

