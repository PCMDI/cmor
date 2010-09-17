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
import cmor,numpy

error_flag = cmor.setup(inpath='Test', netcdf_file_action=cmor.CMOR_REPLACE)
  
error_flag = cmor.dataset(                                   
       outpath='Test',                                         
       experiment_id='noVolc2000',
       institution= 'GICC (Generic International Climate Center, Geneva, Switzerland)',                                 
       source='GICCM1 (2002): ',
       calendar='360_day',                                      
       realization=1,                                          
       contact = 'Rusty Koder (koder@middle_earth.net) ',      
       history='Output from archivcl_A1.nce/giccm_03_std_2xCO2_2256.', 
       comment='Equilibrium reached after 30-year spin-up ',                                 
       references='Model described by Koder and Tolkien ',
       model_id="GICCM1", 
       institute_id="PCMDI",
       forcing="Nat, SO",
       parent_experiment_id="lgm",branch_time=3.14159)
  

cmor.load_table("/git/cmip5-cmor-tables/Tables/CMIP5_Omon")
itime = cmor.axis(table_entry="time",units='months since 2010',coord_vals=numpy.array([0,1,2,3,4.]),cell_bounds=numpy.array([0,1,2,3,4,5.]))
ivar = cmor.variable(table_entry="masso",axis_ids=[itime],units='kg')

data=numpy.random.random(5)
for i in range(0,5):
    cmor.write(ivar,data[i:i])#,time_vals=numpy.array([i,]),time_bnds=numpy.array([i,i+1]))
error_flag = cmor.close()  

