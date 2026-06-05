#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cmor
import numpy as np

REPO_ROOT = Path(__file__).resolve().parents[1]
TABLES_PATH = REPO_ROOT / "cmip7-cmor-tables" / "tables"
CV_PATH = REPO_ROOT / "cmip7-cmor-tables" / "tables-cvs" / "cmor-cvs.json"
CMOR_CV_PATH = Path("..") / CV_PATH.relative_to(TABLES_PATH.parent)


def configure(output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    user_input = {
        "_AXIS_ENTRY_FILE": "CMIP7_coordinate.json",
        "_FORMULA_VAR_FILE": "CMIP7_formula_terms.json",
        "_cmip7_option": 1,
        "_controlled_vocabulary_file": str(CMOR_CV_PATH),
        "activity_id": "CMIP",
        "calendar": "360_day",
        "experiment_id": "amip",
        "forcing_index": "f1",
        "frequency": "mon",
        "grid_label": "g999",
        "initialization_index": "i1",
        "institution_id": "MOHC",
        "license_id": "CC-BY-4.0",
        "nominal_resolution": "100 km",
        "outpath": str(output_dir),
        "physics_index": "p1",
        "realization_index": "r1",
        "region": "glb",
        "source_id": "DUMMY-MODEL",
    }
    input_path = output_dir / "example_05_input.json"
    input_path.write_text(json.dumps(user_input, indent=2, sort_keys=True))
    cmor.setup(inpath=str(TABLES_PATH), netcdf_file_action=cmor.CMOR_REPLACE)
    cmor.dataset_json(str(input_path))


def write_example(output_dir: Path) -> str:
    configure(output_dir)
    cmor.load_table("CMIP7_atmos.json")
    lat_id = cmor.axis(
        "latitude",
        "degrees_north",
        coord_vals=np.array([10.0, 20.0, 30.0], dtype="d"),
        cell_bounds=np.array([5.0, 15.0, 25.0, 35.0], dtype="d"),
    )
    lon_id = cmor.axis(
        "longitude",
        "degrees_east",
        coord_vals=np.array([0.0, 90.0, 180.0, 270.0], dtype="d"),
        cell_bounds=np.array([-45.0, 45.0, 135.0, 225.0, 315.0], dtype="d"),
    )
    time_id = cmor.axis(
        "time",
        "days since 1979-01-01",
        coord_vals=np.array([15.5, 45.5], dtype="d"),
        cell_bounds=np.array([0.0, 31.0, 60.0], dtype="d"),
    )
    lev_id = cmor.axis(
        "standard_hybrid_sigma",
        "1",
        coord_vals=np.array([0.92, 0.72, 0.50, 0.30, 0.10], dtype="d"),
        cell_bounds=np.array([1.00, 0.83, 0.61, 0.40, 0.20, 0.00], dtype="d"),
    )
    cmor.zfactor(
        zaxis_id=lev_id,
        zfactor_name="a",
        axis_ids=[lev_id],
        zfactor_values=np.array([0.12, 0.22, 0.30, 0.20, 0.10], dtype="d"),
        zfactor_bounds=np.array(
            [0.06, 0.18, 0.26, 0.25, 0.15, 0.00],
            dtype="d",
        ),
    )
    cmor.zfactor(
        zaxis_id=lev_id,
        zfactor_name="b",
        axis_ids=[lev_id],
        zfactor_values=np.array([0.80, 0.50, 0.20, 0.10, 0.00], dtype="d"),
        zfactor_bounds=np.array(
            [0.94, 0.65, 0.35, 0.15, 0.05, 0.00],
            dtype="d",
        ),
    )
    cmor.zfactor(
        zaxis_id=lev_id,
        zfactor_name="p0",
        units="Pa",
        zfactor_values=100000.0,
    )
    ps = np.array(
        [
            97000.0,
            97400.0,
            97800.0,
            98200.0,
            98600.0,
            99000.0,
            99400.0,
            99800.0,
            100200.0,
            100600.0,
            101000.0,
            101400.0,
            97100.0,
            97500.0,
            97900.0,
            98300.0,
            98700.0,
            99100.0,
            99500.0,
            99900.0,
            100300.0,
            100700.0,
            101100.0,
            101500.0,
        ],
        dtype="f4",
    ).reshape(2, 3, 4)
    ps_id = cmor.zfactor(
        zaxis_id=lev_id,
        zfactor_name="ps",
        axis_ids=[time_id, lat_id, lon_id],
        units="Pa",
    )
    var_id = cmor.variable(
        "cl_tavg-al-hxy-u",
        "%",
        [time_id, lev_id, lat_id, lon_id],
        missing_value=1.0e20,
    )
    data = np.array(
        [
            72.8, 73.2, 73.6, 74.0,
            71.6, 72.0, 72.4, 72.4,
            70.4, 70.8, 70.8, 71.2,
            67.6, 69.2, 69.6, 70.0,
            66.0, 66.4, 66.8, 67.2,
            64.8, 65.2, 65.6, 66.0,
            63.6, 64.0, 64.4, 64.4,
            60.8, 61.2, 62.8, 63.2,
            59.6, 59.6, 60.0, 60.4,
            58.0, 58.4, 58.8, 59.2,
            56.8, 57.2, 57.6, 58.0,
            54.0, 54.4, 54.8, 56.4,
            52.8, 53.2, 53.2, 53.6,
            51.6, 51.6, 52.0, 52.4,
            50.0, 50.4, 50.8, 51.2,
            72.9, 73.3, 73.7, 74.1,
            71.7, 72.1, 72.5, 72.5,
            70.5, 70.9, 70.9, 71.3,
            67.7, 69.3, 69.7, 70.1,
            66.1, 66.5, 66.9, 67.3,
            64.9, 65.3, 65.7, 66.1,
            63.7, 64.1, 64.5, 64.5,
            60.9, 61.3, 62.9, 63.3,
            59.7, 59.7, 60.1, 60.5,
            58.1, 58.5, 58.9, 59.3,
            56.9, 57.3, 57.7, 58.1,
            54.1, 54.5, 54.9, 56.5,
            52.9, 53.3, 53.3, 53.7,
            51.7, 51.7, 52.1, 52.5,
            50.1, 50.5, 50.9, 51.3,
        ],
        dtype="f4",
    ).reshape(2, 5, 3, 4)
    cmor.write(var_id, data)
    cmor.write(ps_id, ps, store_with=var_id)
    path = cmor.close(var_id, file_name=True)
    cmor.close()
    return path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Write CMIP7 example 5 with CMOR."
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path(__file__).resolve().parent / "output",
    )
    args = parser.parse_args()
    print(write_example(args.output_dir))


if __name__ == "__main__":
    main()
