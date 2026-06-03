program example_04_basin_axis
  use cmip7_fortran_common
  implicit none

  integer, parameter :: nbasin = 3
  character(len=1024) :: repo_root
  character(len=1024) :: output_dir
  character(len=2048) :: tables_path
  character(len=2048) :: input_path
  character(len=21) :: basin_names(nbasin)
  integer :: lat_id
  integer :: time_id
  integer :: basin_id
  integer :: var_id
  integer :: ierr
  integer :: nul_pos
  real :: heat_transport(nlat, nbasin, ntimes)
  character(len=2048) :: filename
  double precision :: lat(nlat)
  double precision :: lat_bnds(2, nlat)
  double precision :: time(ntimes)
  double precision :: time_bnds(2, ntimes)

  call get_example_args(repo_root, output_dir)
  call prepare_cmor_paths(repo_root, output_dir, "example_04_input.json", &
       "mon", "r1", "f1", tables_path, input_path)

  ierr = cmor_setup(inpath=trim(tables_path), &
       netcdf_file_action=CMOR_REPLACE, &
       exit_control=CMOR_EXIT_ON_MAJOR)
  call check_status("cmor_setup", ierr)

  ierr = cmor_dataset_json(trim(input_path))
  call check_status("cmor_dataset_json", ierr)

  ierr = cmor_load_table("CMIP7_ocean.json")
  call check_id("cmor_load_table(CMIP7_ocean)", ierr)

  lat = (/ 10.0d0, 20.0d0, 30.0d0 /)
  lat_bnds(:, 1) = (/ 5.0d0, 15.0d0 /)
  lat_bnds(:, 2) = (/ 15.0d0, 25.0d0 /)
  lat_bnds(:, 3) = (/ 25.0d0, 35.0d0 /)

  lat_id = cmor_axis(table="CMIP7_ocean.json", &
       table_entry="latitude", &
       units="degrees_north", &
       length=nlat, &
       coord_vals=lat, &
       cell_bounds=lat_bnds)
  call check_id("cmor_axis(latitude)", lat_id)

  time = (/ 15.5d0, 45.5d0 /)
  time_bnds(:, 1) = (/ 0.0d0, 31.0d0 /)
  time_bnds(:, 2) = (/ 31.0d0, 60.0d0 /)
  time_id = cmor_axis(table="CMIP7_ocean.json", &
       table_entry="time", &
       units="days since 1979-01-01", &
       length=ntimes, &
       coord_vals=time, &
       cell_bounds=time_bnds)
  call check_id("cmor_axis(time)", time_id)

  basin_names(1) = "atlantic_arctic_ocean"
  basin_names(2) = "indian_pacific_ocean"
  basin_names(3) = "global_ocean"
  basin_id = cmor_axis(table="CMIP7_ocean.json", &
       table_entry="basin", &
       units="", &
       length=nbasin, &
       coord_vals=basin_names)
  call check_id("cmor_axis(basin)", basin_id)

  heat_transport(:, :, 1) = reshape((/ &
       -80.0, -84.0, -88.0, &
       -100.0, -104.0, -76.0, &
       -120.0, -92.0, -96.0 /), &
       (/ nlat, nbasin /))
  heat_transport(:, :, 2) = reshape((/ &
       -79.0, -83.0, -87.0, &
       -99.0, -103.0, -75.0, &
       -107.0, -111.0, -115.0 /), &
       (/ nlat, nbasin /))

  var_id = cmor_variable(table="CMIP7_ocean.json", &
       table_entry="htovgyre_tavg-u-hyb-sea", &
       units="W", &
       axis_ids=(/ lat_id, basin_id, time_id /), &
       missing_value=missing_value)
  call check_id("cmor_variable(htovgyre)", var_id)

  ierr = cmor_write(var_id, heat_transport)
  call check_status("cmor_write", ierr)

  filename = ""
  ierr = cmor_close(var_id, file_name=filename)
  call check_status("cmor_close(var)", ierr)
  nul_pos = index(filename, char(0))
  if (nul_pos > 0) filename(nul_pos:) = " "
  write(*, '(a)') trim(filename)

  ierr = cmor_close()
  call check_status("cmor_close", ierr)
end program example_04_basin_axis
