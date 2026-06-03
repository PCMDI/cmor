program example_06_curvilinear_grid
  use cmip7_fortran_common
  implicit none

  integer, parameter :: nx = 4
  integer, parameter :: ny = 3
  integer, parameter :: nvertices = 4
  character(len=1024) :: repo_root
  character(len=1024) :: output_dir
  character(len=2048) :: tables_path
  character(len=2048) :: input_path
  integer :: x_id
  integer :: y_id
  integer :: time_id
  integer :: grid_id
  integer :: grid_table_id
  integer :: var_id
  integer :: ierr
  integer :: nul_pos
  integer :: i
  integer :: j
  integer :: t
  real :: hfls(nx, ny, ntimes)
  character(len=2048) :: filename
  double precision :: x(nx)
  double precision :: y(ny)
  double precision :: x_bnds(2, nx)
  double precision :: y_bnds(2, ny)
  double precision :: latitude(nx, ny)
  double precision :: longitude(nx, ny)
  double precision :: latitude_vertices(nvertices, nx, ny)
  double precision :: longitude_vertices(nvertices, nx, ny)
  double precision :: time(ntimes)
  double precision :: time_bnds(2, ntimes)
  character(len=30) :: parameter_names(6)
  character(len=1) :: parameter_units(6)
  double precision :: parameter_values(6)

  call get_example_args(repo_root, output_dir)
  call prepare_cmor_paths(repo_root, output_dir, "example_06_input.json", &
       "mon", "r1", "f1", tables_path, input_path)

  ierr = cmor_setup(inpath=trim(tables_path), &
       netcdf_file_action=CMOR_REPLACE, &
       exit_control=CMOR_EXIT_ON_MAJOR)
  call check_status("cmor_setup", ierr)

  ierr = cmor_dataset_json(trim(input_path))
  call check_status("cmor_dataset_json", ierr)

  grid_table_id = cmor_load_table("CMIP7_grids.json")
  call cmor_set_table(grid_table_id)

  x = (/ 0.0d0, 10000.0d0, 20000.0d0, 30000.0d0 /)
  x_bnds(:, 1) = (/ -5000.0d0, 5000.0d0 /)
  x_bnds(:, 2) = (/ 5000.0d0, 15000.0d0 /)
  x_bnds(:, 3) = (/ 15000.0d0, 25000.0d0 /)
  x_bnds(:, 4) = (/ 25000.0d0, 35000.0d0 /)
  y = (/ 0.0d0, 10000.0d0, 20000.0d0 /)
  y_bnds(:, 1) = (/ -5000.0d0, 5000.0d0 /)
  y_bnds(:, 2) = (/ 5000.0d0, 15000.0d0 /)
  y_bnds(:, 3) = (/ 15000.0d0, 25000.0d0 /)

  x_id = cmor_axis(table_entry="x", units="m", length=nx, &
       coord_vals=x, cell_bounds=x_bnds)
  y_id = cmor_axis(table_entry="y", units="m", length=ny, &
       coord_vals=y, cell_bounds=y_bnds)

  do j = 1, ny
    do i = 1, nx
      latitude(i, j) = 10.0d0 * dble(j) - 2.0d0 * dble(i - 1)
      longitude(i, j) = 280.0d0 + 10.0d0 * dble(i - 1) &
           + 2.0d0 * dble(j - 1)
      latitude_vertices(:, i, j) = (/ latitude(i, j) - 5.0d0, &
           latitude(i, j) - 4.0d0, latitude(i, j) + 5.0d0, &
           latitude(i, j) + 4.0d0 /)
      longitude_vertices(:, i, j) = (/ longitude(i, j) - 5.0d0, &
           longitude(i, j) + 5.0d0, longitude(i, j) + 5.0d0, &
           longitude(i, j) - 5.0d0 /)
    enddo
  enddo

  grid_id = cmor_grid(axis_ids=(/ x_id, y_id /), &
       latitude=latitude, &
       longitude=longitude, &
       latitude_vertices=latitude_vertices, &
       longitude_vertices=longitude_vertices)

  parameter_names = (/ "standard_parallel1           ", &
       "longitude_of_central_meridian", &
       "latitude_of_projection_origin", &
       "false_easting                ", &
       "false_northing               ", &
       "standard_parallel2           " /)
  parameter_units = (/ " ", " ", " ", " ", " ", " " /)
  parameter_values = (/ -20.0d0, 175.0d0, 13.0d0, 8.0d0, 0.0d0, 20.0d0 /)
  ierr = cmor_set_grid_mapping(grid_id, "lambert_conformal_conic", &
       parameter_names, parameter_values, parameter_units)
  call check_status("cmor_set_grid_mapping", ierr)

  ierr = cmor_load_table("CMIP7_atmos.json")
  time = (/ 15.5d0, 45.5d0 /)
  time_bnds(:, 1) = (/ 0.0d0, 31.0d0 /)
  time_bnds(:, 2) = (/ 31.0d0, 60.0d0 /)
  time_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="time", &
       units="days since 1979-01-01", &
       length=ntimes, &
       coord_vals=time, &
       cell_bounds=time_bnds)

  do t = 1, ntimes
    do j = 1, ny
      do i = 1, nx
        hfls(i, j, t) = 80.0 + 2.0 * real(i - 1) + 8.0 * real(j - 1) &
             + real(t - 1)
      enddo
    enddo
  enddo

  var_id = cmor_variable(table="CMIP7_atmos.json", &
       table_entry="hfls_tavg-u-hxy-u", &
       units="W m-2", &
       axis_ids=(/ grid_id, time_id /), &
       positive="up", &
       missing_value=missing_value)

  ierr = cmor_write(var_id, hfls)
  call check_status("cmor_write", ierr)

  filename = ""
  ierr = cmor_close(var_id, file_name=filename)
  call check_status("cmor_close(var)", ierr)
  nul_pos = index(filename, char(0))
  if (nul_pos > 0) filename(nul_pos:) = " "
  write(*, '(a)') trim(filename)

  ierr = cmor_close()
  call check_status("cmor_close", ierr)
end program example_06_curvilinear_grid
