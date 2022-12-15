!!$pgf90 -I/work/NetCDF/5.1/include -L/work/NetCDF/5.1/lib -l netcdf -L. -l cmor Test/test_dimensionless.f90 -IModules -o cmor_test
!!$pgf90 -g -I/pcmdi/charles_work/NetCDF/include -L/pcmdi/charles_work/NetCDF/lib -lnetcdf -module Modules -IModules -L. -lcmor -I/pcmdi/charles_work/Unidata/include -L/pcmdi/charles_work/Unidata/lib -ludunits Test/test_dimensionless.f90 -o cmor_test

MODULE local_subs

USE cmor_users_functions
PRIVATE
PUBLIC read_coords, read_time, read_3d_input_files, read_2d_input_files
CONTAINS

SUBROUTINE read_coords(alats, alons, plevs, bnds_lat, bnds_lon)

  IMPLICIT NONE
   
  DOUBLE PRECISION, INTENT(OUT), DIMENSION(:) :: alats
  DOUBLE PRECISION, INTENT(OUT), DIMENSION(:) :: alons
  DOUBLE PRECISION, INTENT(OUT), DIMENSION(:) :: plevs
  DOUBLE PRECISION, INTENT(OUT), DIMENSION(:,:) :: bnds_lat
  DOUBLE PRECISION, INTENT(OUT), DIMENSION(:,:) :: bnds_lon
  
  INTEGER :: i
  
!  data passed to CMOR is stored in an array with longitude varying from -180 to 180
!  CMOR will subsequently rearrange the dimension order (and the ordering of the data),
!  consistent with the CMIP6 specs.  

  DO i = 1, SIZE(alons)
     alons(i) = (i-0.5)*360./SIZE(alons) - 180.
     bnds_lon(1,i) = (i - 1)*360./SIZE(alons) - 180.
     bnds_lon(2,i) = i*360./SIZE(alons) - 180.
  END DO
  
  DO i = 1, SIZE(alats)
     alats(i) = -90. + (i-0.5)*180./SIZE(alats)
     bnds_lat(1,i) = -90. + (i-1)*180./SIZE(alats)
     bnds_lat(2,i) = -90. + i*180./SIZE(alats)
  END DO

!  data passed to CMOR is stored in an array with plev varying from 1 to 1000
!  CMOR will subsequently reverse the order and scale dimension to store in “Pa”, 
!  consistent with the CMIP6 specs.  CMOR also rearranges the data to make
!  consistent.

  plevs = (/1., 5., 10., 20., 30., 50., 70., 100., 150., 200., &
            250., 300., 400., 500., 600., 700., 850., 925., 1000. /)

  RETURN

END SUBROUTINE read_coords

SUBROUTINE read_time(it, time, time_bnds)
  
  IMPLICIT NONE
  
  INTEGER, INTENT(IN) :: it
  DOUBLE PRECISION, INTENT(OUT) :: time
  DOUBLE PRECISION, INTENT(OUT), DIMENSION(2,1) :: time_bnds

  DOUBLE PRECISION :: time0, monthdays, yeardays

!  For this example, the calendar is specified as 360_day in the CMOR_input_example.json file
!  so time bounds are based on that.  Note: CMOR will check and correct any time values that
!  do not lie mid-way between the bounds.

  monthdays = 30.d0
  yeardays = 360.d0
  time0 = (2015. - 1850.) * yeardays
  
  time = time0 + (it-0.5)*monthdays
  time_bnds(1,1) = time0 + (it-1)*monthdays
  time_bnds(2,1) = time0 + it*monthdays

  RETURN

END SUBROUTINE read_time

SUBROUTINE read_3d_input_files(it, varname, field)

  IMPLICIT NONE
  
  INTEGER, INTENT(IN) :: it
  CHARACTER(len=*), INTENT(IN) :: varname
  REAL, INTENT(OUT), DIMENSION(:,:,:) :: field
  
  INTEGER :: i, j, k
  REAL :: factor, offset
  CHARACTER(len=LEN(varname)) :: tmp
  
  tmp = TRIM(ADJUSTL(varname))

  SELECT CASE (tmp)

  CASE ('CLOUD')  
     factor = 2.0
     offset = 0.
  CASE ('U')  
     factor = 1.0
     offset = -40.
  CASE ('T')
     factor = 1.0
     offset = 220.

  END SELECT

!    it: time (1-2) k: lev (1-19 or 1-5) j: latitude (1-3) i: longitude (1-4)
!  before scaling and offset:
!         values of longitude differ by 4 or 3
!         values of latitude differ by 16
!         values for successive times differ by 1.
!         smallest value = 0.0
!         largest value = 91. (or 49. for cloud)
  
  DO k=1,SIZE(field, 3)
     DO j=1,SIZE(field, 2)
        field(1,j,k) = ((k-1)*3 + (j-1)*16 + it-1)*factor + offset + 0.225
        field(2,j,k) = ((k-1)*3 + (j-1)*16 + it+3)*factor + offset + 0.315
        field(3,j,k) = ((k-1)*3 + (j-1)*16 + it+7)*factor + offset + 0.045
        field(4,j,k) = ((k-1)*3 + (j-1)*16 + it+10)*factor + offset + 0.135
     END DO
  END DO
  
END SUBROUTINE read_3d_input_files

SUBROUTINE read_2d_input_files(it, varname, field)

  IMPLICIT NONE
  
  INTEGER, INTENT(IN) :: it
  CHARACTER(len=*), INTENT(IN) :: varname
  REAL, INTENT(OUT), DIMENSION(:,:) :: field
  
  INTEGER :: i, j
  REAL :: factor, offset, scale
  CHARACTER(len=LEN(varname)) :: tmp
  
  tmp = TRIM(ADJUSTL(varname))

  SELECT CASE (tmp)

    CASE ('LATENT')  
      factor = 4.
      offset = 0.
    CASE ('TSURF')
      factor = 1.0
      offset = -15.
    CASE ('SOIL_WET')
      factor = 2.
      offset = 0.
    CASE ('PSURF')
      factor = 3.
      offset = 900.

  END SELECT

!  For it: time (1-2)    j: latitude (1-3)     i: longitude (1-4)
!  before scaling and offset:
!         values of longitude differ by 4 
!         values of latitude differ by 16
!         values for successive times differ by 1.
!         smallest value = 0.0
!         largest value = 45.0

  DO j=1,SIZE(field, 2)
     field(1,j) =((j-1)*16 + it-1)*factor + offset + 0.225
     field(2,j) =((j-1)*16 + it+3)*factor + offset + 0.315
     field(3,j) =((j-1)*16 + it+7)*factor + offset + 0.045
     field(4,j) =((j-1)*16 + it+11)*factor + offset + 0.135
  END DO

END SUBROUTINE read_2d_input_files

END MODULE local_subs


PROGRAM CMIP6_sample_CMOR_driver
!
!   Purpose:   To serve as a generic example of an application that
!       uses the "Climate Model Output Rewriter" (CMOR)

!    CMOR writes CF-compliant netCDF files.
!    Its use is strongly encouraged, as it facilitates satisfying 
!       model output specifications for CMIP6.
!
!   Background information for this sample code:
!
!      This file should execute as a demonstration.
!   The user will need to adapt this example, by replacing the sections that 
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
!      10/22/03   Rusty Koder              Original code
!      3/1/18     Les R. Koder             Revised to be consistent
!                                            with CMIP6

! include module that contains the user-accessible cmor functions.
USE cmor_users_functions
USE local_subs

IMPLICIT NONE

!   dimension parameters:
! ---------------------------------
INTEGER, PARAMETER :: ntimes = 2    ! number of time samples to process
INTEGER, PARAMETER :: lon = 4       ! number of longitude grid cells  
INTEGER, PARAMETER :: lat = 3       ! number of latitude grid cells
INTEGER, PARAMETER :: plev = 19     ! number of standard pressure levels
INTEGER, PARAMETER :: lev = 5       ! number of model levels
INTEGER, PARAMETER :: n2d = 4       ! number of 2-d fields to be treated
INTEGER, PARAMETER :: n3d = 3       ! number of 3-d fields to be treated

!   Tables associating the user's variables with CMIP6 standard output 
!   variables.  The user may choose to make this association in a 
!   different way (e.g., by defining values of pointers that allow him 
!   to directly retrieve data from a data record containing many 
!   different variables), but in some way the user will need to map his 
!   model output onto the Tables specifying the MIP standard output.

! ----------------------------------

! notes:
!   The units specified below apply to the user’s data.  If these units are
!   not the same as required by the CMIP6 data request, CMOR will convert
!   the fields to correct units (as in the case of tas (TSURF), which will get
!   converted from Celsius to kelvins, and ps (PSURF), which will get converted
!   from hPa to Pa). 
!
!   For hfs (Latent), the “down” entry blow for positive2d , indicates that downward 
!   fluxes are positive for the users input to CMOR.  The data request
!   adopts the opposite convention, so CMOR will change the sign of these fluxes.
!
!   Both was (TSURF) and mrsos (SOIL_WET) are listed as 2-d fields even though
!   each of these has a singleton vertical coordinate.  CMOR automatically adds
!   this coordinate and will automatically assign it a default value (2 m for was
!   and 0.05 m for mrsos, with bounds specified as 0 to 0.1 m). In this example
!   the default is overridden below by defining in a call to cmor_axis height2m
!   and assigning it a value of 1.5 m.

                        ! My 2-d variable names
CHARACTER (LEN=8), DIMENSION(n2d) :: &
                varin2d=(/ 'LATENT  ', 'TSURF   ', 'SOIL_WET', 'PSURF   ' /)

                        ! Units of 2-d data provided to CMOR
CHARACTER (LEN=7), DIMENSION(n2d) :: &
                  units2d=(/ 'W m-2  ', 'Celsius', 'kg m-2 ', 'hPa    ' /)

                        ! positive direction convention for input data
CHARACTER (LEN=4), DIMENSION(n2d) :: &
              positive2d= (/  'down',  '    ', '    ', '    '  /)

       ! CMOR table entry labels for 2-d variables
         !  note that in CMIP, the height of the surface air temperature 
         !  measurement (was) is defined is defined using a scalar vertical
         !  coordinate specified in the CMIP_coordinates.json file.

CHARACTER (LEN=5), DIMENSION(n2d) :: &
                      entry2d = (/ 'hfls ', 'tas  ', 'mrsos', 'ps   ' /)

                        ! My 3-d variable names 
CHARACTER (LEN=5), DIMENSION(n3d) :: &
                         varin3d=(/'CLOUD', 'U    ', 'T    '/)

                              ! Units of 3-d data provided to CMOR
CHARACTER (LEN=5), DIMENSION(n3d) :: &
                          units3d=(/ '%    ', 'm s-1',   'K    '  /)

         ! CMOR table entry labels for 3-d variables
         !  note that cl is cloud fraction, which is reported on model levels,
         !  while the other variables are reported on standard pressure levels

CHARACTER (LEN=2), DIMENSION(n3d) :: entry3d = (/ 'cl', 'ua', 'ta' /)

         ! define tables where 2-d variables can be found
CHARACTER (LEN=4), DIMENSION(n2d) :: tables2d = (/ 'Amon', 'Amon', 'Lmon', 'Amon' /)

!  uninitialized variables used in interactions with CMOR:
!  ---------------------------------------------------------

INTEGER :: error_flag
INTEGER :: zfactor_id
INTEGER, DIMENSION(n2d) :: var2d_ids
INTEGER, DIMENSION(n3d) :: var3d_ids
REAL, DIMENSION(lon,lat) :: data2d
REAL, DIMENSION(lon,lat,plev) :: data3dplev
REAL, DIMENSION(lon,lat,lev) :: data3dlev
DOUBLE PRECISION, DIMENSION(lat) :: alats
DOUBLE PRECISION, DIMENSION(lon) :: alons
DOUBLE PRECISION, DIMENSION(plev) :: plevs
DOUBLE PRECISION, DIMENSION(1) :: time
DOUBLE PRECISION, DIMENSION(2,1):: bnds_time
DOUBLE PRECISION, DIMENSION(2,lat) :: bnds_lat
DOUBLE PRECISION, DIMENSION(2,lon) :: bnds_lon
DOUBLE PRECISION, DIMENSION(lev) :: zlevs
DOUBLE PRECISION, DIMENSION(lev+1) :: zlev_bnds
REAL, DIMENSION(lev) :: a_coeff
REAL, DIMENSION(lev) :: b_coeff
REAL :: p0
REAL, DIMENSION(lev+1) :: a_coeff_bnds
REAL, DIMENSION(lev+1) :: b_coeff_bnds
INTEGER :: ilonA, ilonL, ilatA, ilatL, ipres, ilev, itimA, itimL, iz

!  Other local variables:
!  ---------------------

INTEGER :: it, m  

! ================================
!  Execution begins here:
! ================================

! Read coordinate information from model into arrays that will be passed 
!   to CMOR.
! Read latitude, longitude, and pressure coordinate values into 
!   alats, alons, and plevs, respectively.  Also generate latitude and 
!   longitude bounds, and store in bnds_lat and bnds_lon, respectively.

!   When adapting this code for actual application, the user must 
!   replace the subroutine that fills the coordinate arrays 
!   and their bounds with data obtained from their model history files.  

!  *** possible user-written call ***

CALL read_coords(alats, alons, plevs, bnds_lat, bnds_lon)

! Specify path where tables can be found and indicate that existing 
!    netCDF files should not be overwritten.

error_flag = cmor_setup(inpath='Test', netcdf_file_action='replace')!,logfile='CMIP6_sample_CMOR_driver.log')

! Define dataset and populate with global attributes (which will be written
!    to all files created by this code.  The input file contains
!    much of the metadata needed to satisfy CMIP6 requirements.

!  The path provided in the next statement can be relative or full

error_flag = cmor_dataset_json('CMOR_input_example.json')

! =====================================================
!  Define all axes that will be needed:
! =====================================================

! note that hfls, tas, and ps from CMIP6_Amon and mrsos from CMIP6_Lmon
! need latitude, longitude, and time axes using their respective tables.

ilatA = cmor_axis(                    &
     table='Tables/CMIP6_Amon.json',  &
     table_entry='latitude',          &  
     units='degrees_north',           &  
     length=lat,                      &
     coord_vals=alats,                & 
     cell_bounds=bnds_lat)        
    

ilatL = cmor_axis(                    &
     table='Tables/CMIP6_Lmon.json',  &
     table_entry='latitude',          &  
     units='degrees_north',           &  
     length=lat,                      &
     coord_vals=alats,                & 
     cell_bounds=bnds_lat)     

!  note that in this example, the users input fields are stored such
!  that longitudes are in the range -180 to 180.  The longitude axis
!  contains the longitude values (and bounds) ordered the same as the 
!  user’s data.  CMOR will reorder these and also reorder the variable
!  fields to be consistent with the CMIP6 output requirements in which
!  longitudes are in the range 0 to 360.
ilonA = cmor_axis(  &
     table='Tables/CMIP6_Amon.json', &
     table_entry='longitude',        &
     length=lon,                     &
     units='degrees_east',           &
     coord_vals=alons,               &
     cell_bounds=bnds_lon) 

ilonL = cmor_axis(  &
     table='Tables/CMIP6_Lmon.json', &
     table_entry='longitude',        &
     length=lon,                     &
     units='degrees_east',           &
     coord_vals=alons,               &
     cell_bounds=bnds_lon) 
 
!   note that in this example, the users input fields are stored with
!   pressure levels ordered from the top of the atmosphere to the surface.
!   The plev axis defined here there fore stores values from 1. to 1000. hPa
!   for consistency with the data.  CMOR will reorder these and also reorder
!   the variable fields to be consistent with the CMIP6 requirements in which
!   the vertical coordinate is stored from the surface to the top of the atmosphere.
!   Note also that the pressure level units will be converted from hPa to Pa 
!   to make them consistent with the data request. 
ipres = cmor_axis(  &
     table='Tables/CMIP6_Amon.json', &
     table_entry='plev19',           &
     units='hPa',                    &
     length=plev,                    &
     coord_vals=plevs)

!   note that the time axis is defined next, but the time coordinate 
!   values and bounds will be passed to cmor through function 
!   cmor_write (later, below).

itimA = cmor_axis(  &
     table='Tables/CMIP6_Amon.json', &
     table_entry='time',             &
     units='days since 1850-1-1',    &
     length=ntimes)

itimL = cmor_axis(  &
    table='Tables/CMIP6_Lmon.json', &
    table_entry='time',             &
    units='days since 1850-1-1',    &
    length=ntimes)
 
!  define model eta levels (although these must be provided, CMOR will
!    replace them with a+b before writing the netCDF file; this is done
!    for consistency with the CF conventions)
zlevs = (/ 0.1, 0.3, 0.55, 0.7, 0.9 /)
zlev_bnds=(/ 0.,.2, .42, .62, .8, 1. /)

ilev = cmor_axis(                           &
     table='Tables/CMIP6_Amon.json',        &
     table_entry='standard_hybrid_sigma',   &
     length=lev,                            &
     units = '1',                           &
     coord_vals=zlevs,                      &
     cell_bounds=zlev_bnds)

!   The height2m single-valued coordinate axis only needs to be defined if you
!      want to override the default coordinate value of 2.0 m (as we do in  
!      this example).  This axis does not need to be explicitly associated with
!      any (via function cmor_variable) because the information is automatically
!      provided by CMOR based on table information.
!   Note:  each of the external CMOR tables, when imported, gets amended with
!      the addition of the coordinate information included in 
!      CMIP6_coordinate.json and CMIP6_formula_terms.json, which is why
!      the coord_vals can be overridden via this call accessing
!      CMIP6_Amon.json.

iz =  cmor_axis(  &
     table='Tables/CMIP6_Amon.json',  &
     table_entry='height2m',          &
     units='m',                       &  
     length=1,                        &
     coord_vals=(/ 1.5 /) )        

! =====================================================
!  Define all z-factors needed to transform from model level to 
!       pressure (except surface pressure, ps):
! =====================================================

p0 = 1.e3
a_coeff = (/ 0.1, 0.2, 0.3, 0.22, 0.1 /)
b_coeff = (/ 0.0, 0.1, 0.2, 0.5, 0.8 /)

a_coeff_bnds=(/0.,.15, .25, .25, .16, 0./)
b_coeff_bnds=(/0.,.05, .15, .35, .65, 1./)

error_flag = cmor_zfactor(                &
     zaxis_id=ilev,                       &
     zfactor_name='p0',                   &
     units='hPa',                         &
     zfactor_values = p0)

error_flag = cmor_zfactor(                &
     zaxis_id=ilev,                       & 
     zfactor_name='b',                    &
     axis_ids= (/ ilev /),                &
     zfactor_values = b_coeff,            &
     zfactor_bounds = b_coeff_bnds  )

error_flag = cmor_zfactor(                &
     zaxis_id=ilev,                       &
     zfactor_name='a',                    &
     axis_ids= (/ ilev /),                &
     zfactor_values = a_coeff,            &
     zfactor_bounds = a_coeff_bnds )

zfactor_id = cmor_zfactor(                &
     zaxis_id=ilev,                       &
     zfactor_name='ps',                   &
     axis_ids=(/ ilonA, ilatA, itimA /),  &
     units='hPa' )


! =====================================================
!  Define 2-d variables (including one which has an additional
!         scalar coordinate) 
! =====================================================

DO m=1,n2d

  ! Use the Lmon axes for mrsos (m==3)
  ! Otherwise, use Amon axes
  IF (m==3) THEN
    var2d_ids(m) = cmor_variable(                    &
      table='Tables/CMIP6_'//tables2d(m)//'.json',   &
      table_entry=entry2d(m),                        & 
      units=units2d(m),                              & 
      axis_ids=(/ ilonL, ilatL, itimL /),            &
      missing_value=1.0e28,                          &
      positive=positive2d(m),                        &
      original_name=varin2d(m))   
  ELSE
    var2d_ids(m) = cmor_variable(                    &
      table='Tables/CMIP6_'//tables2d(m)//'.json',   &
      table_entry=entry2d(m),                        & 
      units=units2d(m),                              & 
      axis_ids=(/ ilonA, ilatA, itimA /),            &
      missing_value=1.0e28,                          &
      positive=positive2d(m),                        &
      original_name=varin2d(m))   
  END IF

ENDDO

! =====================================================
!  Define 3-d variables 
! =====================================================

!  Define the only field to be written by this code that is a function 
!     of model level

var3d_ids(1) = cmor_variable(                   &
     table='Tables/CMIP6_Amon.json',            &
     table_entry=entry3d(1),                    &
     units=units3d(1),                          &
     axis_ids=(/ ilonA, ilatA, ilev, itimA /),  &
     missing_value=1.0e28, &
     original_name=varin3d(1))

!  Define variables that are a function of pressure
!         (3-d variables)

DO m=2,n3d
   var3d_ids(m) = cmor_variable(                   &
        table='Tables/CMIP6_Amon.json',            &
        table_entry=entry3d(m),                    &
        units=units3d(m),                          &
        axis_ids=(/ ilonA, ilatA, ipres, itimA /), &
        missing_value=1.0e28,                      &
        original_name=varin3d(m))
ENDDO

PRINT*, ' '
PRINT*, 'completed everything up to writing output fields '
PRINT*, ' '

! =====================================================
!  Loop through model’s history files (each containing several different fields, 
!       but only a single month of data, averaged over the month).  Then 
!       extract fields of interest and write these to netCDF files (with 
!       one field per file, but all months included in the loop).
! =====================================================


time_loop: DO it=1, ntimes
   
   ! In the following code to write out 3d and 2d fields, the     
   ! subroutines (read_3d_input_files and read_2d_input_files) retrieve 
   ! the requested fields and store them in 
   ! data3d and data2d, respectively.  In addition  
   ! (read_time) retrieves the time and time-bounds associated with the 
   ! time sample (in units of 'days since 1850-1-1’, consistent with the 
   ! axis definitions above).  The bounds are set to the beginning and 
   ! the end of the month retrieved, indicating the averaging period.
   
   ! For application to real model output, user must write a code 
   !   to obtain the times and time-bounds for the time slice.  
   !   The following line is simply a place-holder for
   !   the user's code, which should replace it.
 
   CALL read_time(it, time(1), bnds_time)

   ! Cycle through the 2-d fields, retrieve the requested variable and 
   ! append each to the appropriate netCDF file.
   
   ! =====================================================
   ! Write 1 time-slice of each 2-d field
   ! =====================================================

   DO m=1,n2d
      
      ! The user must write the code that fills the arrays of data
      ! that will be passed to CMOR.  The following line is simply a
      ! a place-holder for the user's code, which should replace it.
      
      call read_2d_input_files(it, varin2d(m), data2d)                  

         ! append a single time sample of data for a single field to 
         ! the appropriate netCDF file.

      error_flag = cmor_write(                                     &
              var_id        = var2d_ids(m),                        &
              data          = data2d,                              &
              ntimes_passed = 1,                                   &
              time_vals     = time,                                &
              time_bnds     = bnds_time  )
      
      IF (error_flag < 0) THEN
         ! write diagnostic messages to standard output device
         write(*,*) ' Error encountered writing field ', entry2d(m), &
              ' which was originally named ', varin2d(m)
         write(*,*) ' Was processing time sample: ', time 
      END IF
      
   END DO

   ! =====================================================
   ! Write 1 time-slice of each 3-d field
   ! =====================================================
  
 ! Now treat a single 3-d field that is a reported on model levels. 
 ! Then loop through other 3-d fields (reported on pressure levels) 

      ! The user must write the code that fills the arrays of data
      ! that will be passed to CMOR.  The following line is simply a
      ! a place-holder for the user's code, which should replace it.

   CALL read_3d_input_files(it, varin3d(1), data3dlev)

   !  The following writes cloud fraction (cl), which is the only 
   !   3-d field that is on model levels (rather than pressure levels)

   error_flag = cmor_write(                                 &
       var_id        = var3d_ids(1),                        &
       data          = data3dlev,                           &
       ntimes_passed = 1,                                   &
       time_vals     = time,                                &
       time_bnds     = bnds_time   )

   !  This model uses eta-coordinates, which needs surface pressure
   !  to generate pressure levels, so retrieve and write ps now:

   CALL read_2d_input_files(it, varin2d(4), data2d)                  

   error_flag = cmor_write(                                 &
       var_id        = zfactor_id,                          &
       data          = data2d,                              &
       ntimes_passed = 1,                                   &
       time_vals     = time,                                &
       time_bnds     = bnds_time,                           &
       store_with    = var3d_ids(1) )

  ! Cycle through the other 3-d fields (i.e., those stored on pressure  
  ! levels), and retrieve the requested variable and append each to the 
  ! appropriate netCDF file.

     DO m=2,n3d
      
      call read_3d_input_files(it, varin3d(m), data3dplev)
     
      ! append a single time sample of data for a single field to 
      ! the appropriate netCDF file.

      error_flag = cmor_write(                                  &
           var_id        = var3d_ids(m),                        &
           data          = data3dplev,                          &
           ntimes_passed = 1,                                   &
           time_vals     = time,                                &
           time_bnds     = bnds_time  )
      
      IF (error_flag < 0) THEN
         ! write diagnostic messages to standard output device
         write(*,*) ' Error encountered writing field ', entry3d(m), &
              ' which was originally named ', varin3d(m)
         write(*,*) ' Was processing time sample: ', time 
      END IF

   END DO
   
   
ENDDO time_loop

! =====================================================  
!   Close all files opened by CMOR.
! =====================================================

error_flag = cmor_close()  

print*, ' '
print*, '******************************'
print*, ' '
print*, 'CMIP6_sample_CMOR_driver code executed to completion '   
print*, ' '
print*, '******************************'

END PROGRAM CMIP6_sample_CMOR_driver

! Need to CHECK “CMOR_input_example.json” file:

! 4) parent_mip_era = “CMIP6” or “no parent”?  not “N/A”  why PrePARE missed this?
! 5) parent_experiment wrong  Why not caught by PrePARE?  not even in the CV
! 8) check “source”  isn’t this gotten from CV?