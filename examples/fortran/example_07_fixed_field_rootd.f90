program example_07_fixed_field_rootd
  use cmip7_fortran_common
  implicit none

  character(len=1024) :: repo_root
  character(len=1024) :: output_dir
  integer :: lat_id
  integer :: lon_id
  integer :: var_id
  integer :: ierr
  real :: rootd(nlon, nlat)
  double precision :: lat(nlat)
  double precision :: lon(nlon)
  double precision :: lat_bnds(2, nlat)
  double precision :: lon_bnds(2, nlon)

  call get_example_args(repo_root, output_dir)
  call configure_cmor(repo_root, output_dir, "example_07_input.json", &
       "fx", "r1", "f1")

  ierr = cmor_load_table("CMIP7_land.json")

  lat = (/ 10.0d0, 20.0d0, 30.0d0 /)
  lat_bnds(:, 1) = (/ 5.0d0, 15.0d0 /)
  lat_bnds(:, 2) = (/ 15.0d0, 25.0d0 /)
  lat_bnds(:, 3) = (/ 25.0d0, 35.0d0 /)

  lon = (/ 0.0d0, 90.0d0, 180.0d0, 270.0d0 /)
  lon_bnds(:, 1) = (/ -45.0d0, 45.0d0 /)
  lon_bnds(:, 2) = (/ 45.0d0, 135.0d0 /)
  lon_bnds(:, 3) = (/ 135.0d0, 225.0d0 /)
  lon_bnds(:, 4) = (/ 225.0d0, 315.0d0 /)

  lon_id = cmor_axis(table="CMIP7_land.json", &
       table_entry="longitude", &
       units="degrees_east", &
       length=nlon, &
       coord_vals=lon, &
       cell_bounds=lon_bnds)

  lat_id = cmor_axis(table="CMIP7_land.json", &
       table_entry="latitude", &
       units="degrees_north", &
       length=nlat, &
       coord_vals=lat, &
       cell_bounds=lat_bnds)

  rootd = reshape((/ &
       0.50, 0.45, missing_value, 0.55, &
       0.60, 0.60, missing_value, 0.55, &
       missing_value, 0.45, 0.50, 0.50 /), &
       (/ nlon, nlat /))

  var_id = cmor_variable(table="CMIP7_land.json", &
       table_entry="rootd_ti-u-hxy-lnd", &
       units="m", &
       axis_ids=(/ lon_id, lat_id /), &
       missing_value=missing_value)

  ierr = cmor_write(var_id, rootd)
  call check_status("cmor_write", ierr)
  call close_example(var_id)
end program example_07_fixed_field_rootd
