# CMIP7 CMOR Example Notebooks

These notebooks demonstrate how to use CMOR to create CMIP7 dataset files from synthetic data. Each notebook is self-contained and downloads `cmip7-cmor-tables` if the tables are not already present in the repository.

## Setup

Create the notebook environment from the repository root:

```bash
python3.14 -m venv notebook-env
notebook-env/bin/python -m pip install cmor --extra-index-url https://pcmdi.github.io/cmor
notebook-env/bin/python -m pip install jupyterlab xarray matplotlib netCDF4
```

Start JupyterLab:

```bash
notebook-env/bin/jupyter lab notebooks
```

The notebooks write generated NetCDF files under `notebooks/output/`.

## Notebooks

1. `01_basic_ocean_surface_temperature.ipynb`: writes monthly sea surface temperature (`tos_tavg-u-hxy-sea`) on a global 10 degree latitude-longitude grid.
2. `02_atmos_surface_air_temperature.ipynb`: writes 2 m air temperature (`tas_tavg-h2m-hxy-u`) and demonstrates the fixed-height coordinate.
3. `03_pressure_level_air_temperature.ipynb`: writes air temperature (`ta_tavg-p19-hxy-air`) on the CMIP7 19 pressure levels.
4. `04_hybrid_sigma_zfactors.ipynb`: writes air temperature (`ta_tavg-al-hxy-u`) on a hybrid sigma pressure coordinate with CMOR zfactors.
5. `05_curvilinear_ocean_grid.ipynb`: writes sea surface temperature (`tos_tavg-u-hxy-sea`) using `cmor.grid` with two-dimensional latitude and longitude coordinates.
