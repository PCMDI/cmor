program example_02_pressure_levels
  use cmip7_fortran_common
  implicit none

  integer, parameter :: nplev = 19
  character(len=1024) :: repo_root
  character(len=1024) :: output_dir
  integer :: lat_id
  integer :: lon_id
  integer :: time_id
  integer :: plev_id
  integer :: var_id
  integer :: ierr
  integer :: i
  integer :: j
  integer :: k
  integer :: t
  real :: ta(nlon, nlat, nplev, ntimes)
  double precision :: lat(nlat)
  double precision :: lon(nlon)
  double precision :: time(ntimes)
  double precision :: lat_bnds(2, nlat)
  double precision :: lon_bnds(2, nlon)
  double precision :: time_bnds(2, ntimes)
  double precision :: plev(nplev)

  call get_example_args(repo_root, output_dir)
  call configure_cmor(repo_root, output_dir, "example_02_input.json", &
       "mon", "r1", "f1")

  ierr = cmor_load_table("CMIP7_atmos.json")

  lat = (/ 10.0d0, 20.0d0, 30.0d0 /)
  lat_bnds(:, 1) = (/ 5.0d0, 15.0d0 /)
  lat_bnds(:, 2) = (/ 15.0d0, 25.0d0 /)
  lat_bnds(:, 3) = (/ 25.0d0, 35.0d0 /)

  lon = (/ 0.0d0, 90.0d0, 180.0d0, 270.0d0 /)
  lon_bnds(:, 1) = (/ -45.0d0, 45.0d0 /)
  lon_bnds(:, 2) = (/ 45.0d0, 135.0d0 /)
  lon_bnds(:, 3) = (/ 135.0d0, 225.0d0 /)
  lon_bnds(:, 4) = (/ 225.0d0, 315.0d0 /)

  time = (/ 15.5d0, 45.5d0 /)
  time_bnds(:, 1) = (/ 0.0d0, 31.0d0 /)
  time_bnds(:, 2) = (/ 31.0d0, 60.0d0 /)

  lon_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="longitude", &
       units="degrees_east", &
       length=nlon, &
       coord_vals=lon, &
       cell_bounds=lon_bnds)

  lat_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="latitude", &
       units="degrees_north", &
       length=nlat, &
       coord_vals=lat, &
       cell_bounds=lat_bnds)

  time_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="time", &
       units="days since 1979-01-01", &
       length=ntimes, &
       coord_vals=time, &
       cell_bounds=time_bnds)

  plev = (/ 100000.0d0, 92500.0d0, 85000.0d0, 70000.0d0, 60000.0d0, &
       50000.0d0, 40000.0d0, 30000.0d0, 25000.0d0, 20000.0d0, &
       15000.0d0, 10000.0d0, 7000.0d0, 5000.0d0, 3000.0d0, &
       2000.0d0, 1000.0d0, 500.0d0, 100.0d0 /)

  plev_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="plev19", &
       units="Pa", &
       length=nplev, &
       coord_vals=plev)

  do t = 1, ntimes
    do k = 1, nplev
      do j = 1, nlat
        do i = 1, nlon
          ta(i, j, k, t) = 250.0 + 25.0 * real(i + 4*j + 12*k + 228*(t-1)) &
               / real(nlon*nlat*nplev*ntimes)
        enddo
      enddo
    enddo
  enddo
  ta(1, 1, 1, 1) = missing_value

  var_id = cmor_variable(table="CMIP7_atmos.json", &
       table_entry="ta_tavg-p19-hxy-air", &
       units="K", &
       axis_ids=(/ lon_id, lat_id, plev_id, time_id /), &
       missing_value=missing_value)

  ierr = cmor_write(var_id, ta)
  call check_status("cmor_write", ierr)
  call close_example(var_id)
end program example_02_pressure_levels
