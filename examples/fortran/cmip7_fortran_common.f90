module cmip7_fortran_common
  use cmor_users_functions
  implicit none

  integer, parameter :: nlon = 4
  integer, parameter :: nlat = 3
  integer, parameter :: ntimes = 2
  real, parameter :: missing_value = 1.0e20

contains

  subroutine get_example_args(repo_root, output_dir)
    character(len=*), intent(out) :: repo_root
    character(len=*), intent(out) :: output_dir

    call get_command_argument(1, repo_root)
    call get_command_argument(2, output_dir)

    if (len_trim(repo_root) == 0) repo_root = "."
    if (len_trim(output_dir) == 0) output_dir = "examples/fortran/output"
  end subroutine get_example_args

  subroutine prepare_cmor_paths(repo_root, output_dir, input_name, frequency, &
       realization_index, forcing_index, tables_path, input_path)
    character(len=*), intent(in) :: repo_root
    character(len=*), intent(in) :: output_dir
    character(len=*), intent(in) :: input_name
    character(len=*), intent(in) :: frequency
    character(len=*), intent(in) :: realization_index
    character(len=*), intent(in) :: forcing_index
    character(len=*), intent(out) :: tables_path
    character(len=*), intent(out) :: input_path
    integer :: unit

    tables_path = trim(repo_root)//"/cmip7-cmor-tables/tables"
    input_path = trim(output_dir)//"/"//trim(input_name)

    open(newunit=unit, file=trim(input_path), status="replace", &
         action="write")
    write(unit, '(a)') "{"
    call json_string(unit, "_AXIS_ENTRY_FILE", "CMIP7_coordinate.json", .true.)
    call json_string(unit, "_FORMULA_VAR_FILE", "CMIP7_formula_terms.json", .true.)
    call json_integer(unit, "_cmip7_option", 1, .true.)
    call json_string(unit, "_controlled_vocabulary_file", &
         "../tables-cvs/cmor-cvs.json", .true.)
    call json_string(unit, "activity_id", "CMIP", .true.)
    call json_string(unit, "archive_id", "WCRP", .true.)
    call json_string(unit, "calendar", "360_day", .true.)
    call json_string(unit, "experiment_id", "amip", .true.)
    call json_string(unit, "forcing_index", trim(forcing_index), .true.)
    call json_string(unit, "frequency", trim(frequency), .true.)
    call json_string(unit, "grid_label", "g999", .true.)
    call json_string(unit, "host_collection", "CMIP7", .true.)
    call json_string(unit, "initialization_index", "i1", .true.)
    call json_string(unit, "institution_id", "MOHC", .true.)
    call json_string(unit, "license_id", "CC-BY-4.0", .true.)
    call json_string(unit, "nominal_resolution", "100 km", .true.)
    call json_string(unit, "outpath", trim(output_dir), .true.)
    call json_string(unit, "physics_index", "p1", .true.)
    call json_string(unit, "realization_index", trim(realization_index), .true.)
    call json_string(unit, "region", "glb", .true.)
    call json_string(unit, "source_id", "ACCESS-ESM1-6", .false.)
    write(unit, '(a)') "}"
    close(unit)
  end subroutine prepare_cmor_paths

  subroutine json_string(unit, key, value, comma)
    integer, intent(in) :: unit
    character(len=*), intent(in) :: key
    character(len=*), intent(in) :: value
    logical, intent(in) :: comma

    if (comma) then
      write(unit, '(a)') '  "'//trim(key)//'": "'//trim(value)//'",'
    else
      write(unit, '(a)') '  "'//trim(key)//'": "'//trim(value)//'"'
    endif
  end subroutine json_string

  subroutine json_integer(unit, key, value, comma)
    integer, intent(in) :: unit
    character(len=*), intent(in) :: key
    integer, intent(in) :: value
    logical, intent(in) :: comma

    if (comma) then
      write(unit, '(a, i0, a)') '  "'//trim(key)//'": ', value, ","
    else
      write(unit, '(a, i0)') '  "'//trim(key)//'": ', value
    endif
  end subroutine json_integer

  subroutine check_status(call_name, ierr)
    character(len=*), intent(in) :: call_name
    integer, intent(in) :: ierr

    if (ierr /= 0) then
      write(*, '(a, i0)') trim(call_name)//" failed with status ", ierr
      stop 1
    endif
  end subroutine check_status

  subroutine check_id(call_name, cmor_id)
    character(len=*), intent(in) :: call_name
    integer, intent(in) :: cmor_id

    if (cmor_id < 0) then
      write(*, '(a, i0)') trim(call_name)//" failed with id ", cmor_id
      stop 1
    endif
  end subroutine check_id

  subroutine check_grid_id(call_name, cmor_id)
    character(len=*), intent(in) :: call_name
    integer, intent(in) :: cmor_id

    if (cmor_id > -CMOR_MAX_GRIDS) then
      write(*, '(a, i0)') trim(call_name)//" failed with grid id ", cmor_id
      stop 1
    endif
  end subroutine check_grid_id

  subroutine apply_cmip7_variable_metadata(var_id, tables_path, realm, &
       table_entry, frequency, region)
    integer, intent(in) :: var_id
    character(len=*), intent(in) :: tables_path
    character(len=*), intent(in) :: realm
    character(len=*), intent(in) :: table_entry
    character(len=*), intent(in) :: frequency
    character(len=*), intent(in) :: region
    character(len=2048) :: compound_name
    character(len=2048) :: cell_measures
    character(len=2048) :: long_name
    integer :: ierr
    logical :: found

    call cmip7_compound_name(realm, table_entry, frequency, region, &
         compound_name)

    cell_measures = ""
    call lookup_json_string(trim(tables_path)//"/CMIP7_cell_measures.json", &
         trim(compound_name), cell_measures, found)
    ierr = cmor_set_variable_attribute(var_id, "cell_measures", &
         trim(cell_measures))
    call check_status("cmor_set_variable_attribute(cell_measures)", ierr)

    long_name = ""
    call lookup_json_string(trim(tables_path)//"/CMIP7_long_name_overrides.json", &
         trim(compound_name), long_name, found)
    if (found) then
      ierr = cmor_set_variable_attribute(var_id, "long_name", trim(long_name))
      call check_status("cmor_set_variable_attribute(long_name)", ierr)
    endif
  end subroutine apply_cmip7_variable_metadata

  subroutine cmip7_compound_name(realm, table_entry, frequency, region, &
       compound_name)
    character(len=*), intent(in) :: realm
    character(len=*), intent(in) :: table_entry
    character(len=*), intent(in) :: frequency
    character(len=*), intent(in) :: region
    character(len=*), intent(out) :: compound_name
    character(len=1024) :: normalized_entry
    integer :: i

    normalized_entry = table_entry
    do i = 1, len_trim(normalized_entry)
      if (normalized_entry(i:i) == "_") normalized_entry(i:i) = "."
    enddo

    compound_name = trim(realm)//"."//trim(normalized_entry)//"."// &
         trim(frequency)//"."//trim(region)
  end subroutine cmip7_compound_name

  subroutine lookup_json_string(path, key, value, found)
    character(len=*), intent(in) :: path
    character(len=*), intent(in) :: key
    character(len=*), intent(out) :: value
    logical, intent(out) :: found
    character(len=8192) :: line
    character(len=2050) :: search_key
    integer :: colon_pos
    integer :: ierr
    integer :: quote_1
    integer :: quote_2
    integer :: start_pos
    integer :: unit

    value = ""
    found = .false.
    search_key = '"'//trim(key)//'"'

    open(newunit=unit, file=trim(path), status="old", action="read", &
         iostat=ierr)
    if (ierr /= 0) then
      write(*, '(a)') "Could not open CMIP7 metadata table: "//trim(path)
      stop 1
    endif

    do
      read(unit, '(a)', iostat=ierr) line
      if (ierr /= 0) exit
      if (index(line, trim(search_key)) == 0) cycle

      colon_pos = index(line, ":")
      if (colon_pos == 0) exit

      quote_1 = index(line(colon_pos + 1:), '"')
      if (quote_1 == 0) exit

      start_pos = colon_pos + quote_1
      quote_2 = index(line(start_pos + 1:), '"')
      if (quote_2 == 0) exit

      value = line(start_pos + 1:start_pos + quote_2 - 1)
      found = .true.
      exit
    enddo

    close(unit)
  end subroutine lookup_json_string

end module cmip7_fortran_common
