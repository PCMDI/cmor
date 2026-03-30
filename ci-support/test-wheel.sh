#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
WHEEL_DIR=${WHEEL_DIR:-"${ROOT_DIR}/wheelhouse"}
TEST_VENV_DIR=${TEST_VENV_DIR:-"${ROOT_DIR}/build/wheel-test-venv"}
PYTHON_BIN=${PYTHON_BIN:-python}

cd "${ROOT_DIR}"

if [ "${CMOR_WHEEL_ALREADY_INSTALLED:-0}" != "1" ]; then
    rm -rf "${TEST_VENV_DIR}"
    "${PYTHON_BIN}" -m venv "${TEST_VENV_DIR}"
    source "${TEST_VENV_DIR}/bin/activate"
fi

if [ "${CMOR_TEST_DEPS_ALREADY_INSTALLED:-0}" != "1" ]; then
    python -m pip install --upgrade pip
    python -m pip install \
        typing-extensions \
        netcdf4 \
        pyfive \
        hdf5plugin
fi

if [ "${CMOR_WHEEL_ALREADY_INSTALLED:-0}" != "1" ]; then
    python -m pip install --no-deps "${WHEEL_DIR}"/*.whl
fi

export HDF5_PLUGIN_PATH
HDF5_PLUGIN_PATH="$(python -c 'import hdf5plugin; print(hdf5plugin.PLUGINS_PATH)')"

CMOR_REPO_ROOT="${ROOT_DIR}" python - <<'PY'
import os
import pathlib
import tempfile

import cmor
import hdf5plugin
import netCDF4
import pyfive

cmor_path = pathlib.Path(cmor.__file__).resolve()
assert "site-packages" in str(cmor_path), cmor_path

xml_path = os.environ.get("UDUNITS2_XML_PATH")
assert xml_path, "UDUNITS2_XML_PATH was not set"
assert pathlib.Path(xml_path).is_file(), xml_path
assert os.environ.get("HDF5_PLUGIN_PATH") == hdf5plugin.PLUGINS_PATH

repo_root = pathlib.Path(os.environ["CMOR_REPO_ROOT"])
tables_path = repo_root / "cmip6-cmor-tables" / "Tables"
assert tables_path.is_dir(), tables_path

with tempfile.NamedTemporaryFile(prefix="cmor-wheel-", suffix=".log") as logfile:
    status = cmor.setup(
        inpath=str(tables_path),
        netcdf_file_action=cmor.CMOR_REPLACE,
        logfile=logfile.name,
    )
    assert status == 0, status
    cmor.close()
PY

wheel_python_tests=(
    "Test/test_non_monotonic_climo_bounds.py"
    "Test/test_python_history.py"
    "Test/test_python_missing_values.py"
    "Test/test_python_sos_psu_units.py"
    "Test/test_python_CMIP6_projections.py"
    "Test/test_python_toomany_tables.py"
    "Test/test_python_direct_calls.py"
    "Test/test_python_user_interface_00.py"
    "Test/test_python_user_interface_01.py"
    "Test/test_python_user_interface_03.py"
    "Test/test_python_common.py"
    "Test/cmor_speed_and_compression.py"
    "Test/cmor_speed_and_compression_01.py"
    "Test/test_compression.py"
    "Test/test_python_appending.py"
    "Test/test_python_bounds_request.py"
    "Test/test_python_new_tables.py"
    "Test/test_python_jamie.py"
    "Test/test_python_jamie_2.py"
    "Test/test_python_jamie_3.py"
    "Test/test_python_jamie_4.py"
    "Test/test_python_jamie_6.py"
    "Test/test_python_jamie_7.py"
    "Test/test_python_jamie_8.py"
    "Test/test_python_memory_check.py"
    "Test/test_python_open_close_cmor_multiple.py"
    "Test/test_python_joerg_2.py"
    "Test/test_python_joerg_3.py"
    "Test/test_python_joerg_5.py"
    "Test/test_python_joerg_6.py"
    "Test/test_python_joerg_7.py"
    "Test/test_python_joerg_8.py"
    "Test/test_python_joerg_11.py"
    "Test/test_python_joerg_12.py"
    "Test/test_python_YYYMMDDHH_exp_fmt.py"
    "Test/test_python_region.py"
    "Test/jamie_hybrid_height.py"
    "Test/jamie_positive.py"
    "Test/test_python_1D_var.py"
    "Test/test_python_cfmip_site_axis_test.py"
    "Test/test_python_grid_and_ocn_sigma.py"
    "Test/test_python_jamie_site_surface.py"
    "Test/test_python_reverted_lats.py"
    "Test/test_lon_gt_360.py"
    "Test/test_lon_thro_360.py"
    "Test/test_python_jamie_11.py"
    "Test/test_python_joerg_tim2_clim_02.py"
    "Test/test_site_ts.py"
    "Test/test_python_free_wrapping_issue.py"
    "Test/test_python_filename_time_range.py"
    "Test/test_cmor_half_levels.py"
    "Test/test_cmor_half_levels_wrong_generic_level.py"
    "Test/test_cmor_python_not_enough_data.py"
    "Test/test_cmor_python_not_enough_times_written.py"
    "Test/test_python_forecast_coordinates.py"
    "Test/test_cmor_CMIP6Plus.py"
    "Test/test_cmor_zstandard_and_quantize.py"
    "Test/test_python_branded_variable.py"
    "Test/test_cmor_CMIP7.py"
    "Test/test_cmor_crs.py"
    "Test/test_cmor_nan_check.py"
    "Test/test_cmor_unsupported_calendar.py"
    "Test/test_cmor_time_interval_check.py"
    "Test/test_cmor_license_attributes.py"
    "Test/test_cmor_nested_cv_attribute.py"
    "Test/test_cmor_path_and_file_templates.py"
    "Test/test_cmor_check_cv_structure.py"
    "Test/test_cmor_time_value_and_bounds_mismatch.py"
    "Test/test_cmor_chunking.py"
    "Test/test_cmor_associated_variable.py"
    "Test/test_cmor_frequency_required.py"
    "Test/test_python_CMIP6_CV_sub_experimentnotset.py"
    "Test/test_python_CMIP6_CV_sub_experimentbad.py"
    "Test/test_python_CMIP6_CV_furtherinfourl.py"
    "Test/test_python_CMIP6_CV_badfurtherinfourl.py"
    "Test/test_python_CMIP6_CV_fxtable.py"
    "Test/test_python_CMIP6_CV_unicode.py"
    "Test/test_python_CMIP6_CV_forcemultipleparent.py"
    "Test/test_python_CMIP6_CV_badsource.py"
    "Test/test_python_CMIP6_CV_parentvariantlabel.py"
    "Test/test_python_CMIP6_CV_nomipera.py"
    "Test/test_python_CMIP6_CV_badgridgr.py"
    "Test/test_python_CMIP6_CV_badinstitution.py"
    "Test/test_python_CMIP6_CV_badsourcetype.py"
    "Test/test_python_CMIP6_CV_badsourceid.py"
    "Test/test_python_CMIP6_CV_badgridresolution.py"
    "Test/test_python_CMIP6_CV_terminate_signal.py"
    "Test/test_python_CMIP6_CV_trackingprefix.py"
    "Test/test_python_CMIP6_CV_parentmipera.py"
    "Test/test_python_CMIP6_CV_bad_data_specs.py"
    "Test/test_python_CMIP6_CV_longrealizationindex.py"
    "Test/test_python_CMIP6_CV_sub_experimentIDbad.py"
    "Test/test_python_CMIP6_CV_badsourcetypeCHEMAER.py"
    "Test/test_python_CMIP6_CV_parentsourceid.py"
    "Test/test_python_CMIP6_CV_hierarchicalattr.py"
    "Test/test_python_CMIP6_CV_badsourcetypeRequired.py"
    "Test/test_python_CMIP6_CV_forcenoparent.py"
    "Test/test_python_CMIP6_CV_load_tables.py"
    "Test/test_python_CMIP6_CV_sub_experiment_id.py"
    "Test/test_python_CMIP6_CV_badvariant.py"
    "Test/test_python_CMIP6_CV_baddirectory.py"
    "Test/test_python_CMIP6_CV_trackingNoprefix.py"
    "Test/test_python_CMIP6_CV_HISTORY.py"
    "Test/test_python_CMIP6_CV_parenttimeunits.py"
    "Test/test_python_CMIP6_CV_invalidsourceid.py"
    "Test/test_python_CMIP6_CV_forceparent.py"
    "Test/test_python_CMIP6_CV_externalvariables.py"
    "Test/test_python_CMIP6_CV_badinstitutionID.py"
    "Test/test_python_CMIP6_CV_badgridlabel.py"
)

for test_name in "${wheel_python_tests[@]}"; do
    echo "Running ${test_name}"
    CMOR_REPO_ROOT="${ROOT_DIR}" python "${ROOT_DIR}/${test_name}"
done
