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
    input_path = output_dir / "example_02_input.json"
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
    plev_id = cmor.axis(
        "plev19",
        "Pa",
        coord_vals=np.array(
            [
                100000.0,
                92500.0,
                85000.0,
                70000.0,
                60000.0,
                50000.0,
                40000.0,
                30000.0,
                25000.0,
                20000.0,
                15000.0,
                10000.0,
                7000.0,
                5000.0,
                3000.0,
                2000.0,
                1000.0,
                500.0,
                100.0,
            ],
            dtype="d",
        ),
    )
    var_id = cmor.variable(
        "ta_tavg-p19-hxy-air",
        "K",
        [time_id, plev_id, lat_id, lon_id],
        missing_value=1.0e20,
    )
    data = np.linspace(
        250.0,
        275.0,
        2 * 19 * 3 * 4,
        dtype="f4",
    ).reshape(2, 19, 3, 4)
    data[0, 0, 0, 0] = np.float32(1.0e20)
    cmor.write(var_id, data)
    path = cmor.close(var_id, file_name=True)
    cmor.close()
    return path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Write CMIP7 example 2 with CMOR."
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
