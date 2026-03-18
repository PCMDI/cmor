#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
WHEEL_DIR=${WHEEL_DIR:-"${ROOT_DIR}/wheelhouse"}
TEST_VENV_DIR=${TEST_VENV_DIR:-"${ROOT_DIR}/build/wheel-test-venv"}
PYTHON_BIN=${PYTHON_BIN:-python}

rm -rf "${TEST_VENV_DIR}"
"${PYTHON_BIN}" -m venv --system-site-packages "${TEST_VENV_DIR}"
source "${TEST_VENV_DIR}/bin/activate"

python -m pip install --no-deps "${WHEEL_DIR}"/*.whl

CMOR_REPO_ROOT="${ROOT_DIR}" python - <<'PY'
import os
import pathlib
import tempfile

import cmor

cmor_path = pathlib.Path(cmor.__file__).resolve()
assert "site-packages" in str(cmor_path), cmor_path

xml_path = os.environ.get("UDUNITS2_XML_PATH")
assert xml_path, "UDUNITS2_XML_PATH was not set"
assert pathlib.Path(xml_path).is_file(), xml_path

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

CMOR_REPO_ROOT="${ROOT_DIR}" python "${ROOT_DIR}/Test/test_cmor_CMIP6Plus.py"
