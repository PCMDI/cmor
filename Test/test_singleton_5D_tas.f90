!!$pgf90 -I/work/NetCDF/5.1/include -L/work/NetCDF/5.1/lib -l netcdf -L. -l cmor Test/test_dimensionless.f90 -IModules -o cmor_test
!!$pgf90 -g -I/pcmdi/charles_work/NetCDF/include -L/pcmdi/charles_work/NetCDF/lib -lnetcdf -module Modules -IModules -L. -lcmor -I/pcmdi/charles_work/Unidata/include -L/pcmdi/charles_work/Unidata/lib -ludunits Test/test_dimensionless.f90 -o cmor_test

MODULE local_subs

  USE cmor_users_functions
  PRIVATE
  PUBLIC read_coords, read_time, read_2d_input_files
CONTAINS
  
  SUBROUTINE read_coords(alats, alons, plevs, bnds_lat, bnds_lon)

    IMPLICIT NONE
    
    DOUBLE PRECISION, INTENT(OUT), DIMENSION(:) :: alats
    REAL, INTENT(OUT), DIMENSION(:) :: alons
    DOUBLE PRECISION, INTENT(OUT), DIMENSION(:) :: plevs
    DOUBLE PRECISION, INTENT(OUT), DIMENSION(:,:) :: bnds_lat
    REAL, INTENT(OUT), DIMENSION(:,:) :: bnds_lon
    
    INTEGER :: i
    
    DO i = 1, SIZE(alons)
       alons(i) = (i-1)*360./SIZE(alons)
       bnds_lon(1,i) = (i - 1.5)*360./SIZE(alons)
       bnds_lon(2,i) = (i - 0.5)*360./SIZE(alons)
    END DO
    
    DO i = 1, SIZE(alats)
       alats(i) = (size(alats)+1-i)*10
       bnds_lat(1,i) = (size(alats)+1-i)*10 + 5.
       bnds_lat(2,i) = (size(alats)+1-i)*10 - 5.
    END DO
  
  END SUBROUTINE read_coords

  SUBROUTINE read_time(it, time, time_bnds)
    
    IMPLICIT NONE
    
    INTEGER, INTENT(IN) :: it
    DOUBLE PRECISION, INTENT(OUT) :: time
    DOUBLE PRECISION, INTENT(OUT), DIMENSION(2,1) :: time_bnds
    
    time = (it-0.5)*30.
    time_bnds(1,1) = (it-1)*30.
    time_bnds(2,1) = it*30.
    
    RETURN
  END SUBROUTINE read_time
  
include "reader_2D_3D.f90"
END MODULE local_subs


PROGRAM ipcc_test_code
!
!   Purpose:   To serve as a generic example of an application that
!       uses the "Climate Model Output Rewriter" (CMOR)

!    CMOR writes CF-compliant netCDF files.
!    Its use is strongly encouraged by the IPCC and is intended for use 
!       by those participating in many community-coordinated standard 
!       climate model experiments (e.g., AMIP, CMIP, CFMIP, PMIP, APE,
!       etc.)
!
!   Background information for this sample code:
!
!      Atmospheric standard output requested by IPCC are listed in 
!   tables available on the web.  Monthly mean output is found in
!   tables A1a and A1c.  This sample code processes only two 3-d 
!   variables listed in table A1c ("monthly mean atmosphere 3-D data" 
!   and only four 2-d variables listed in table A1a ("monthly mean 
!   atmosphere + land surface 2-D (latitude, longitude) data").  The 
!   extension to many more fields is trivial.
!
!      For this example, the user must fill in the sections of code that 
!   extract the 3-d and 2-d fields from his monthly mean "history" 
!   files (which usually contain many variables but only a single time 
!   slice).  The CMOR code will write each field in a separate file, but 
!   many monthly mean time-samples will be stored together.  These 
!   constraints partially determine the structure of the code.
!
!
!   Record of revisions:

!       Date        Programmer(s)           Description of change
!       ====        ==========              =====================
!      10/22/03     Rusty Koder              Original code
!       1/28/04     Les R. Koder             Revised to be consistent
!                                            with evolving code design

! include module that contains the user-accessible cmor functions.
  USE cmor_users_functions
  USE local_subs

  IMPLICIT NONE

  !   dimension parameters:
  ! ---------------------------------
  INTEGER, PARAMETER :: ntimes = 2    ! number of time samples to process
  INTEGER, PARAMETER :: lon = 4       ! number of longitude grid cells  
  INTEGER, PARAMETER :: lat = 3       ! number of latitude grid cells
  INTEGER, PARAMETER :: lev = 5       ! number of standard pressure levels
  INTEGER, PARAMETER :: lev2 =17       ! number of standard pressure levels
  INTEGER, PARAMETER :: n2d = 1       ! number of IPCC Table A1a fields to be
                                      !     output.

  !   Tables associating the user's variables with IPCC standard output 
  !   variables.  The user may choose to make this association in a 
  !   different way (e.g., by defining values of pointers that allow him 
  !   to directly retrieve data from a data record containing many 
  !   different variables), but in some way the user will need to map his 
  !   model output onto the Tables specifying the MIP standard output.

  ! ----------------------------------

  CHARACTER (LEN=8), DIMENSION(n2d) :: &
                          varin2d=(/ 'TSURF   ' /)

                                ! Units appropriate to my data
   CHARACTER (LEN=6), DIMENSION(n2d) :: &
                          units2d=(/ 'K     '/)

   CHARACTER (LEN=4), DIMENSION(n2d) :: &
                      positive2d= (/  '    '/)

                     ! Corresponding IPCC Table A1a entry (variable name) 
  CHARACTER (LEN=11), DIMENSION(n2d) :: &
                        entry2d = (/ 'tasforecast'/)

!  uninitialized variables used in communicating with CMOR:
!  ---------------------------------------------------------

  INTEGER :: error_flag
  INTEGER :: znondim_id, zfactor_id
  INTEGER, DIMENSION(n2d) :: var2d_ids
  REAL, DIMENSION(lon,lat) :: data2d
  REAL, DIMENSION(lon,lat,lev2) :: data3d
  DOUBLE PRECISION, DIMENSION(lat,1,1,lon) :: scramble
  DOUBLE PRECISION, DIMENSION(lat) :: alats
  REAL, DIMENSION(lon) :: alons
  DOUBLE PRECISION, DIMENSION(lev2) :: plevs
  DOUBLE PRECISION, DIMENSION(1) :: time
  DOUBLE PRECISION, DIMENSION(2,1):: bnds_time
  DOUBLE PRECISION, DIMENSION(2,lat) :: bnds_lat
  REAL, DIMENSION(2,lon) :: bnds_lon
  DOUBLE PRECISION, DIMENSION(lev) :: zlevs
  DOUBLE PRECISION, DIMENSION(lev+1) :: zlev_bnds
  REAL, DIMENSION(lev) :: a_coeff
  REAL, DIMENSION(lev) :: b_coeff
  REAL :: p0
  REAL, DIMENSION(lev+1) :: a_coeff_bnds
  REAL, DIMENSION(lev+1) :: b_coeff_bnds
  INTEGER :: ilon, ilat, ipres, ilev, itim, ilon2,ilat2,itim2
  INTEGER :: iht, itk

  !  Other variables:
  !  ---------------------
  
  INTEGER :: it, m, i, j
  double precision bt

  bt=0.
  ! ================================
  !  Execution begins here:
  ! ================================
  
  ! Read coordinate information from model into arrays that will be passed 
  !   to CMOR.
  ! Read latitude, longitude, and pressure coordinate values into 
  !   alats, alons, and plevs, respectively.  Also generate latitude and 
  !   longitude bounds, and store in bnds_lat and bnds_lon, respectively.
  !   Note that all variable names in this code can be freely chosen by
  !   the user.

  !   The user must write the subroutine that fills the coordinate arrays 
  !   and their bounds with actual data.  The following line is simply a
  !   a place-holder for the user's code, which should replace it.
  
  !  *** possible user-written call ***
  
  call read_coords(alats, alons, plevs, bnds_lat, bnds_lon)
  
  ! Specify path where tables can be found and indicate that existing 
  !    netCDF files should not be overwritten.
  
  error_flag = cmor_setup(inpath='Test', netcdf_file_action='replace')
  
  ! Define dataset as output from the GICC model (first member of an
  !   ensemble of simulations) run under IPCC 2xCO2 equilibrium
  !   experiment conditions, and provide information to be included as 
  !   attributes in all CF-netCDF files written as part of this dataset.

  error_flag = cmor_dataset_json("Test/test2.json")
  !  Define all axes that will be needed

  ilat = cmor_axis(  &
       table='Tables/CMIP6_excerpts.json',    &
       table_entry='latitude',       &
       units='degrees_north',        &  
       length=lat,                   &
       coord_vals=alats,             & 
       cell_bounds=bnds_lat)        
      
  ilon = cmor_axis(  &
       table='Tables/CMIP6_excerpts.json',    &
       table_entry='longitude',      &
       length=lon,                   &
       units='degrees_east',         &
       coord_vals=alons,             &
       cell_bounds=bnds_lon)      
        
  itk = cmor_axis(  &
       table='Tables/CMIP6_excerpts.json',    &
       table_entry='forecast',       &
       units='days since 2016-04-13',                   &
       length=1,                   &
       coord_vals=(/0.0/))


  iht = cmor_axis(  &
       table='Tables/CMIP6_excerpts.json',    &
       table_entry='height2m',       &
       units='m',                   &
       length=1,                   &
       coord_vals=(/3.0/))

  !   note that the time axis is defined next, but the time coordinate 
  !   values and bounds will be passed to cmor through function 
  !   cmor_write (later, below).

  itim = cmor_axis(  &
       table='Tables/CMIP6_excerpts.json',    &
       table_entry='time',           &
       units='days since 2030-1-1',  &
       length=ntimes,                &
       interval='1 month')
  

  !  Define variables appearing in IPCC table A1a (2-d variables)
  
  var2d_ids = cmor_variable(    &
             table='Tables/CMIP6_excerpts.json',  &
             table_entry=entry2d(1),     & 
             units=units2d(1),           & 
             axis_ids=(/ ilat, iht,  itk, ilon, itim /), &
             missing_value=bt,       &
             positive=positive2d(1),     &
             original_name=varin2d(1))   

  PRINT*, ' '
  PRINT*, 'completed everything up to writing output fields '
  PRINT*, ' '

  !  Loop through history files (each containing several different fields, 
  !       but only a single month of data, averaged over the month).  Then 
  !       extract fields of interest and write these to netCDF files (with 
  !       one field per file, but all months included in the loop).
  
  time_loop: DO it=1, ntimes
     
     ! In the following loops over the 3d and 2d fields, the user-written    
     ! subroutines (read_3d_input_files and read_2d_input_files) retrieve 
     ! the requested IPCC table A1c and table A1a fields and store them in 
     ! data3d and data2d, respectively.  In addition a user-written code 
     ! (read_time) retrieves the time and time-bounds associated with the 
     ! time sample (in units of 'days since 1970-1-1', consistent with the 
     ! axis definitions above).  The bounds are set to the beginning and 
     ! the end of the month retrieved, indicating the averaging period.
     
     ! The user must write a code to obtain the times and time-bounds for
     !   the time slice.  The following line is simply a place-holder for
     !   the user's code, which should replace it.
     
    call read_time(it, time(1), bnds_time)

    print*, 'shape(data2d)',shape(data2d),varin2d(1)
    print*, 'writing:',entry2d(1)
    ! The user must write the code that fills the arrays of data
    ! that will be passed to CMOR.  The following line is simply a
    ! a place-holder for the user's code, which should replace it.
           
    call read_2d_input_files(it, varin2d(1), data2d)                  
           
    ! append a single time sample of data for a single field to 
    ! the appropriate netCDF file.
           

    DO j=1,lat
       DO i=1,lon
          print *,i,j,data2d(i,j)
          scramble(j,1,1,i) = data2d(i,j)
       END DO
    END DO
    error_flag = cmor_write(                                  &
         var_id        = var2d_ids(1),                        &
         data          = scramble(:,1,1,:),                            &
         ntimes_passed = 1,                                   &
         time_vals     = time,                                &
         time_bnds     = bnds_time  )

    IF (error_flag < 0) THEN
       ! write diagnostic messages to standard output device
       write(*,*) ' Error encountered writing IPCC Table A1a ' &
            // 'field ', entry2d(1), ', which I call ', varin2d(1)
       write(*,*) ' Was processing time sample: ', time 
              
    END IF

  END DO time_loop
  
  !   Close all files opened by CMOR.
  
  error_flag = cmor_close()  

  print*, ' '
  print*, '******************************'
  print*, ' '
  print*, 'ipcc_test_code executed to completion '   
  print*, ' '
  print*, '******************************'
  
END PROGRAM ipcc_test_code

