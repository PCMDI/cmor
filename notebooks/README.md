# CMIP7 CMOR Example Notebooks

These notebooks demonstrate how to use CMOR to create CMIP7 dataset files. They use the `cmip7-cmor-tables` checkout in this repository.

## Setup

Create the notebook environment from the repository root:

```bash
python -m venv venv
source venv/bin/activate
pip install cmor --extra-index-url https://pcmdi.github.io/cmor
pip install jupyterlab xarray matplotlib netCDF4 basemap pyproj nbstripout
```

Start JupyterLab:

```bash
jupyter lab notebooks
```

The notebooks write generated NetCDF files under `notebooks/output/`.

### Rebuilding notebook output
To rebuild checked-in example notebook output from the repository root:

```bash
tools/rebuild_notebook_output.sh notebooks/01_basic_ocean_surface_temperature.ipynb
```

Use `tools/rebuild_notebook_output.sh --all` to rebuild every example notebook.
The tool executes notebooks in place, keeps the regenerated outputs, and strips
notebook metadata with `nbstripout`.

## Notebooks

1. [Basic Ocean Surface Temperature](01_basic_ocean_surface_temperature.ipynb): writes monthly sea surface temperature (`tos_tavg-u-hxy-sea`) on a latitude-longitude grid.
2. [Atmospheric Surface Air Temperature](02_atmos_surface_air_temperature.ipynb): writes 2 m air temperature (`tas_tavg-h2m-hxy-u`) and demonstrates the fixed-height coordinate.
3. [Atmospheric Temperature On Pressure Levels](03_pressure_level_air_temperature.ipynb): writes air temperature (`ta_tavg-p19-hxy-air`) on the CMIP7 19 pressure levels.
4. [Hybrid Sigma Humidity Tendency With Z-Factors](04_hybrid_sigma_humidity_tendency.ipynb): writes humidity tendency (`tnhusscpbl_tavg-al-hxy-u`) on a hybrid sigma pressure coordinate with CMOR zfactors and surface pressure.
5. [Ocean Heat Transport By Basin](05_ocean_heat_transport_by_basin.ipynb): writes northward ocean heat transport (`htovgyre_tavg-u-hyb-sea`) using latitude and ocean basin coordinates.
6. [Land-Use Fraction](06_land_use_fraction.ipynb): writes land-use fraction (`fracLut_tpt-u-hxy-u`) using time-point, land-use, latitude, and longitude coordinates.
7. [Sea Ice Concentration On Projected Grids](07_non_lat_lon_sea_ice_concentration.ipynb): writes sea-ice area percentage (`siconc_tavg-u-hxy-u`) on projected Northern and Southern Hemisphere grids.
