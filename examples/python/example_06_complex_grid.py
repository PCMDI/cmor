#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cmor
import numpy as np

REPO_ROOT = Path(__file__).resolve().parents[2]
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
    input_path = output_dir / "example_06_input.json"
    input_path.write_text(json.dumps(user_input, indent=2, sort_keys=True))
    cmor.setup(inpath=str(TABLES_PATH), netcdf_file_action=cmor.CMOR_REPLACE)
    cmor.dataset_json(str(input_path))


def write_example(output_dir: Path) -> str:
    configure(output_dir)
    grid_table = cmor.load_table("CMIP7_grids.json")
    cmor.set_table(grid_table)
    y_id = cmor.axis(
        "y",
        "m",
        coord_vals=np.array([0.0, 10000.0, 20000.0], dtype="d"),
        cell_bounds=np.array(
            [-5000.0, 5000.0, 15000.0, 25000.0],
            dtype="d",
        ),
    )
    x_id = cmor.axis(
        "x",
        "m",
        coord_vals=np.array([0.0, 10000.0, 20000.0, 30000.0], dtype="d"),
        cell_bounds=np.array(
            [-5000.0, 5000.0, 15000.0, 25000.0, 35000.0],
            dtype="d",
        ),
    )
    latitude = np.array(
        [
            [10.0, 8.0, 6.0, 4.0],
            [20.0, 18.0, 16.0, 14.0],
            [30.0, 28.0, 26.0, 24.0],
        ],
        dtype="d",
    )
    longitude = np.array(
        [
            [280.0, 290.0, 300.0, 310.0],
            [282.0, 292.0, 302.0, 312.0],
            [284.0, 294.0, 304.0, 314.0],
        ],
        dtype="d",
    )
    latitude_vertices = np.empty((3, 4, 4), dtype="d")
    longitude_vertices = np.empty((3, 4, 4), dtype="d")
    for j in range(3):
        for i in range(4):
            latitude_vertices[j, i, :] = [
                latitude[j, i] - 5.0,
                latitude[j, i] - 4.0,
                latitude[j, i] + 5.0,
                latitude[j, i] + 4.0,
            ]
            longitude_vertices[j, i, :] = [
                longitude[j, i] - 5.0,
                longitude[j, i] + 5.0,
                longitude[j, i] + 5.0,
                longitude[j, i] - 5.0,
            ]
    grid_id = cmor.grid(
        axis_ids=[y_id, x_id],
        latitude=latitude,
        longitude=longitude,
        latitude_vertices=latitude_vertices,
        longitude_vertices=longitude_vertices,
    )
    cmor.set_grid_mapping(
        grid_id=grid_id,
        mapping_name="lambert_conformal_conic",
        parameter_names=[
            "standard_parallel1",
            "longitude_of_central_meridian",
            "latitude_of_projection_origin",
            "false_easting",
            "false_northing",
            "standard_parallel2",
        ],
        parameter_values=[-20.0, 175.0, 13.0, 8.0, 0.0, 20.0],
        parameter_units=["", "", "", "", "", ""],
    )
    cmor.load_table("CMIP7_atmos.json")
    time_id = cmor.axis(
        "time",
        "days since 1979-01-01",
        coord_vals=np.array([15.5, 45.5], dtype="d"),
        cell_bounds=np.array([0.0, 31.0, 60.0], dtype="d"),
    )
    var_id = cmor.variable(
        "hfls_tavg-u-hxy-u",
        "W m-2",
        [time_id, grid_id],
        positive="up",
        missing_value=1.0e20,
    )
    data = np.array(
        [
            80.0,
            82.0,
            84.0,
            86.0,
            88.0,
            90.0,
            92.0,
            94.0,
            96.0,
            98.0,
            100.0,
            102.0,
            81.0,
            83.0,
            85.0,
            87.0,
            89.0,
            91.0,
            93.0,
            95.0,
            97.0,
            99.0,
            101.0,
            103.0,
        ],
        dtype="f4",
    ).reshape(2, 3, 4)
    cmor.write(var_id, data)
    path = cmor.close(var_id, file_name=True)
    cmor.close()
    return path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Write CMIP7 example 6 with CMOR."
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
