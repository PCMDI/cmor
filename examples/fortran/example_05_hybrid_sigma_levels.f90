program example_05_hybrid_sigma_levels
  use cmip7_fortran_common
  implicit none

  integer, parameter :: nlev = 5
  character(len=1024) :: repo_root
  character(len=1024) :: output_dir
  character(len=2048) :: tables_path
  character(len=2048) :: input_path
  integer :: lat_id
  integer :: lon_id
  integer :: time_id
  integer :: lev_id
  integer :: ps_id
  integer :: var_id
  integer :: ierr
  integer :: nul_pos
  integer :: i
  integer :: j
  integer :: k
  integer :: t
  real :: cl(nlon, nlat, nlev, ntimes)
  real :: ps(nlon, nlat, ntimes)
  character(len=2048) :: filename
  double precision :: lat(nlat)
  double precision :: lon(nlon)
  double precision :: time(ntimes)
  double precision :: lat_bnds(2, nlat)
  double precision :: lon_bnds(2, nlon)
  double precision :: time_bnds(2, ntimes)
  double precision :: lev(nlev)
  double precision :: lev_bnds(nlev + 1)
  double precision :: a_coeff(nlev)
  double precision :: b_coeff(nlev)
  double precision :: a_bnds(nlev + 1)
  double precision :: b_bnds(nlev + 1)
  double precision :: p0

  call get_example_args(repo_root, output_dir)
  call prepare_cmor_paths(repo_root, output_dir, "example_05_input.json", &
       "mon", "r1", "f1", tables_path, input_path)

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

  lev = (/ 0.92d0, 0.72d0, 0.50d0, 0.30d0, 0.10d0 /)
  lev_bnds = (/ 1.00d0, 0.83d0, 0.61d0, 0.40d0, 0.20d0, 0.00d0 /)
  lev_id = cmor_axis(table="CMIP7_atmos.json", &
       table_entry="standard_hybrid_sigma", &
       units="1", &
       length=nlev, &
       coord_vals=lev, &
       cell_bounds=lev_bnds)
  call check_id("cmor_axis(standard_hybrid_sigma)", lev_id)

  a_coeff = (/ 0.12d0, 0.22d0, 0.30d0, 0.20d0, 0.10d0 /)
  b_coeff = (/ 0.80d0, 0.50d0, 0.20d0, 0.10d0, 0.00d0 /)
  a_bnds = (/ 0.06d0, 0.18d0, 0.26d0, 0.25d0, 0.15d0, 0.00d0 /)
  b_bnds = (/ 0.94d0, 0.65d0, 0.35d0, 0.15d0, 0.05d0, 0.00d0 /)
  p0 = 100000.0d0

  ierr = cmor_zfactor(zaxis_id=lev_id, &
       zfactor_name="a", &
       axis_ids=(/ lev_id /), &
       zfactor_values=a_coeff, &
       zfactor_bounds=a_bnds)
  call check_id("cmor_zfactor(a)", ierr)

  ierr = cmor_zfactor(zaxis_id=lev_id, &
       zfactor_name="b", &
       axis_ids=(/ lev_id /), &
       zfactor_values=b_coeff, &
       zfactor_bounds=b_bnds)
  call check_id("cmor_zfactor(b)", ierr)

  ierr = cmor_zfactor(zaxis_id=lev_id, &
       zfactor_name="p0", &
       units="Pa", &
       zfactor_values=p0)
  call check_id("cmor_zfactor(p0)", ierr)

  ps_id = cmor_zfactor(zaxis_id=lev_id, &
       zfactor_name="ps", &
       axis_ids=(/ lon_id, lat_id, time_id /), &
       units="Pa")
  call check_id("cmor_zfactor(ps)", ps_id)

  do t = 1, ntimes
    do j = 1, nlat
      do i = 1, nlon
        ps(i, j, t) = 97000.0 + 400.0 * real(i - 1) &
             + 1600.0 * real(j - 1) + 100.0 * real(t - 1)
      enddo
    enddo
  enddo

  do t = 1, ntimes
    do k = 1, nlev
      do j = 1, nlat
        do i = 1, nlon
          cl(i, j, k, t) = 75.0 - 5.0 * real(k) - 1.2 * real(j - 1) &
               + 0.4 * real(i - 1) + 0.1 * real(t - 1)
        enddo
      enddo
    enddo
  enddo

  var_id = cmor_variable(table="CMIP7_atmos.json", &
       table_entry="cl_tavg-al-hxy-u", &
       units="%", &
       axis_ids=(/ lon_id, lat_id, lev_id, time_id /), &
       missing_value=missing_value)
  call check_id("cmor_variable(cl)", var_id)

  ierr = cmor_write(var_id, cl)
  call check_status("cmor_write(cl)", ierr)
  ierr = cmor_write(ps_id, ps, store_with=var_id)
  call check_status("cmor_write(ps)", ierr)

  filename = ""
  ierr = cmor_close(var_id, file_name=filename)
  call check_status("cmor_close(var)", ierr)
  nul_pos = index(filename, char(0))
  if (nul_pos > 0) filename(nul_pos:) = " "
  write(*, '(a)') trim(filename)

  ierr = cmor_close()
  call check_status("cmor_close", ierr)
end program example_05_hybrid_sigma_levels
