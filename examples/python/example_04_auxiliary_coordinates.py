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
    input_path = output_dir / "example_04_input.json"
    input_path.write_text(json.dumps(user_input, indent=2, sort_keys=True))
    cmor.setup(inpath=str(TABLES_PATH), netcdf_file_action=cmor.CMOR_REPLACE)
    cmor.dataset_json(str(input_path))


def write_example(output_dir: Path) -> str:
    configure(output_dir)
    cmor.load_table("CMIP7_ocean.json")
    time_id = cmor.axis(
        "time",
        "days since 1979-01-01",
        coord_vals=np.array([15.5, 45.5], dtype="d"),
        cell_bounds=np.array([0.0, 31.0, 60.0], dtype="d"),
    )
    lat_id = cmor.axis(
        "latitude",
        "degrees_north",
        coord_vals=np.array([10.0, 20.0, 30.0], dtype="d"),
        cell_bounds=np.array([5.0, 15.0, 25.0, 35.0], dtype="d"),
    )
    basin_id = cmor.axis(
        "basin",
        "",
        coord_vals=np.array(
            [
                "atlantic_arctic_ocean",
                "indian_pacific_ocean",
                "global_ocean",
            ],
            dtype="U21",
        ),
    )
    var_id = cmor.variable(
        "htovgyre_tavg-u-hyb-sea",
        "W",
        [time_id, basin_id, lat_id],
        missing_value=1.0e20,
    )
    data = np.array(
        [
            -80.0,
            -84.0,
            -88.0,
            -100.0,
            -104.0,
            -76.0,
            -120.0,
            -92.0,
            -96.0,
            -79.0,
            -83.0,
            -87.0,
            -99.0,
            -103.0,
            -75.0,
            -107.0,
            -111.0,
            -115.0,
        ],
        dtype="f4",
    ).reshape(2, 3, 3)
    cmor.write(var_id, data)
    path = cmor.close(var_id, file_name=True)
    cmor.close()
    return path


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Write CMIP7 example 4 with CMOR."
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
