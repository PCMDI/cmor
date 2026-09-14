program example_08_site_dimension
  use cmip7_fortran_common
  implicit none

  integer, parameter :: nsites = 126
  integer, parameter :: nvertices = 2
  character(len=1024) :: repo_root
  character(len=1024) :: output_dir
  character(len=2048) :: tables_path
  character(len=2048) :: input_path
  character(len=2048) :: filename
  integer :: time_id
  integer :: site_id
  integer :: height_id
  integer :: grid_id
  integer :: grid_table_id
  integer :: var_id
  integer :: ierr
  integer :: nul_pos
  integer :: site
  integer :: time_index
  real :: tas(nsites, ntimes)
  double precision :: time(ntimes)
  double precision :: time_bnds(2, ntimes)
  double precision :: site_values(nsites)
  double precision :: latitude(nsites)
  double precision :: longitude(nsites)
  double precision :: latitude_vertices(nvertices, nsites)
  double precision :: longitude_vertices(nvertices, nsites)

  call get_example_args(repo_root, output_dir)
  call prepare_cmor_paths(repo_root, output_dir, "example_08_input.json", &
       "mon", "r1", "f1", tables_path, input_path)

  ierr = cmor_setup(inpath=trim(tables_path), &
       netcdf_file_action=CMOR_REPLACE, &
       exit_control=CMOR_EXIT_ON_MAJOR)
  call check_status("cmor_setup", ierr)
  ierr = cmor_dataset_json(trim(input_path))
  call check_status("cmor_dataset_json", ierr)

  ierr = cmor_load_table("CMIP7_atmos.json")
  call check_id("cmor_load_table(CMIP7_atmos)", ierr)
  time = (/ 15.5d0, 45.5d0 /)
  time_bnds(:, 1) = (/ 0.0d0, 31.0d0 /)
  time_bnds(:, 2) = (/ 31.0d0, 60.0d0 /)
  time_id = cmor_axis(table_entry="time1", &
       units="days since 1979-01-01", &
       length=ntimes, coord_vals=time, cell_bounds=time_bnds)
  call check_id("cmor_axis(time1)", time_id)
  height_id = cmor_axis(table_entry="height2m", units="m", length=1, &
       coord_vals=(/ 2.0d0 /))
  call check_id("cmor_axis(height2m)", height_id)

  do site = 1, nsites
    site_values(site) = dble(site)
    latitude(site) = -60.0d0 + 120.0d0 * dble(site - 1) / dble(nsites - 1)
    longitude(site) = 0.5d0 + 359.0d0 * dble(site - 1) / dble(nsites - 1)
    latitude_vertices(:, site) = (/ latitude(site) - 0.25d0, &
         latitude(site) + 0.25d0 /)
    longitude_vertices(:, site) = (/ longitude(site) - 0.25d0, &
         longitude(site) + 0.25d0 /)
  enddo
  site_id = cmor_axis(table_entry="site", units="1", length=nsites, &
       coord_vals=site_values)
  call check_id("cmor_axis(site)", site_id)
  grid_table_id = cmor_load_table("CMIP7_grids.json")
  call check_id("cmor_load_table(CMIP7_grids)", grid_table_id)
  call cmor_set_table(grid_table_id)
  grid_id = cmor_grid(axis_ids=(/ site_id /), latitude=latitude, &
       longitude=longitude, latitude_vertices=latitude_vertices, &
       longitude_vertices=longitude_vertices)
  call check_grid_id("cmor_grid", grid_id)

  ierr = cmor_load_table("CMIP7_atmos.json")
  call check_id("cmor_load_table(CMIP7_atmos)", ierr)
  var_id = cmor_variable(table_entry="tas_tpt-h2m-hs-u", units="K", &
       axis_ids=(/ grid_id, time_id /), missing_value=missing_value)
  call check_id("cmor_variable(tas)", var_id)
  call apply_cmip7_variable_metadata(var_id, tables_path, "atmos", &
       "tas_tpt-h2m-hs-u", "mon", "glb")

  do time_index = 1, ntimes
    do site = 1, nsites
      tas(site, time_index) = 275.0 + 15.0 * &
           real((time_index - 1) * nsites + site - 1) / real(ntimes * nsites - 1)
    enddo
  enddo
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
end program example_08_site_dimension
