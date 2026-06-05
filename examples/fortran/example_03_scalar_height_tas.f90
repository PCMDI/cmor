program example_03_scalar_height_tas
  use cmip7_fortran_common
  implicit none

  character(len=1024) :: repo_root
  character(len=1024) :: output_dir
  character(len=2048) :: tables_path
  character(len=2048) :: input_path
  integer :: lat_id
  integer :: lon_id
  integer :: time_id
  integer :: height_id
  integer :: var_id
  integer :: ierr
  integer :: nul_pos
  real :: tas(nlon, nlat, ntimes)
  character(len=2048) :: filename
  double precision :: lat(nlat)
  double precision :: lon(nlon)
  double precision :: time(ntimes)
  double precision :: lat_bnds(2, nlat)
  double precision :: lon_bnds(2, nlon)
  double precision :: time_bnds(2, ntimes)

  call get_example_args(repo_root, output_dir)
  call prepare_cmor_paths(repo_root, output_dir, "example_03_input.json", &
       "mon", "r9", "f2", tables_path, input_path)

  ierr = cmor_setup(inpath=trim(tables_path), &
       netcdf_file_action=CMOR_REPLACE, &
       exit_control=CMOR_EXIT_ON_MAJOR)
  call check_status("cmor_setup", ierr)

  ierr = cmor_dataset_json(trim(input_path))
  call check_status("cmor_dataset_json", ierr)

  ierr = cmor_load_table("CMIP7_atmos.json")
  call check_id("cmor_load_table(CMIP7_atmos)", ierr)

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
  call check_id("cmor_axis(longitude)", lon_id)

  lat_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="latitude", &
       units="degrees_north", &
       length=nlat, &
       coord_vals=lat, &
       cell_bounds=lat_bnds)
  call check_id("cmor_axis(latitude)", lat_id)

  time_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="time", &
       units="days since 1979-01-01", &
       length=ntimes, &
       coord_vals=time, &
       cell_bounds=time_bnds)
  call check_id("cmor_axis(time)", time_id)

  height_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="height2m", &
       units="m", &
       length=1, &
       coord_vals=(/ 2.0d0 /))
  call check_id("cmor_axis(height2m)", height_id)

  tas(:, :, 1) = reshape((/ &
       254.0895, 258.4085, 250.5549, 258.7101, &
       258.6680, 258.2990, 252.1237, 255.0432, &
       253.7254, 251.2460, 254.3168, 255.4808 /), &
       (/ nlon, nlat /))
  tas(:, :, 2) = reshape((/ &
       259.7908, 252.2754, 257.1892, 253.3132, &
       253.8823, 253.4698, 253.5381, 254.9730, &
       256.1002, 251.8168, 259.3698, 250.2994 /), &
       (/ nlon, nlat /))

  var_id = cmor_variable(table="CMIP7_atmos.json", &
       table_entry="tas_tavg-h2m-hxy-u", &
       units="K", &
       axis_ids=(/ lon_id, lat_id, time_id /), &
       missing_value=missing_value)
  call check_id("cmor_variable(tas)", var_id)

  ierr = cmor_write(var_id, tas)
  call check_status("cmor_write", ierr)

  filename = ""
  ierr = cmor_close(var_id, file_name=filename)
  call check_status("cmor_close(var)", ierr)
  nul_pos = index(filename, char(0))
  if (nul_pos > 0) filename(nul_pos:) = " "
  write(*, '(a)') trim(filename)

  ierr = cmor_close()
  call check_status("cmor_close", ierr)
end program example_03_scalar_height_tas
