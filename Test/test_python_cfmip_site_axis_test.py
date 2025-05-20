#!/usr/bin/env python
'''
Test program for creating a sample CFMIP site axis.

This program assumes that we want the output netcdf header to look something like this (in CDL):

dimensions:
   site = 119;
   lev = 38;
   time = UNLIMITED;
   bnds = 2;
variables:
   float cl(time, site, lev);
      cl:coordinates = "lat lon";
      ...
   double time(time);
      ...
   int site(site);    // integer id values for sites
      ...
   float lat(site);   // latitude values for sites
      ...
   float lon(site);   // longitude values for sites
      ...
   float orog(site);  // orography values for sites
      ...
   float lev(lev);    // hybrid_height
      lev:formula = "z(k,m) = a(k) + b(k)*orog(m)";   // m = site index
      lev:formula_terms = "a: lev b: b orog: orog";
      ...
'''
from __future__ import print_function
import cmor
import numpy

MIP_TABLE_DIR = 'Tables'   # set according to your MIP table location

#-------------------------------------------------------------------------


def setup_cmor():
    #-------------------------------------------------------------------------
    # Initialise CMOR library
    cmor.setup(inpath=MIP_TABLE_DIR, netcdf_file_action=cmor.CMOR_REPLACE_3,
               set_verbosity=cmor.CMOR_NORMAL, create_subdirectories=0)

    # Create CMOR dataset
    cmor.dataset_json("Test/CMOR_input_example.json")


#-------------------------------------------------------------------------
if __name__ == '__main__':
    #-------------------------------------------------------------------------

    # Initialise CMOR dataset and table
    setup_cmor()

    # Set dummy site lats and longs.
    site_lats = numpy.array([-90.0, 0.0, 90.0], dtype=numpy.float32)
    site_lons = numpy.array([0.0, 0.0, 0.0], dtype=numpy.float32)

    # Create CMOR axes and grids
    table_id = cmor.load_table('CMIP6_CFsubhr.json')
    # , length=1, interval='30 minutes')
    taxis_id = cmor.axis('time1', units='days since 2000-01-01 00:00:00')
    print('ok: created time axis')

    saxis_id = cmor.axis('site', units='1', coord_vals=[1, 2, 3])
    print('ok: created site axis', saxis_id)

    zaxis_id = cmor.axis(
        'hybrid_height',
        units='m',
        coord_vals=[1.0],
        cell_bounds=[
            0.0,
            2.0])
    print('ok: created height axis', zaxis_id)

    # Create zfactors for b and orog for hybrid height axis.
    # Where do these get used, if anywhere?
    bfact_id = cmor.zfactor(zaxis_id, 'b', '1', [zaxis_id], 'd', zfactor_values=[1.0],
                            zfactor_bounds=[0.0, 2.0])
    print('ok: created b zfactors')

    # Create grid object to link site-dimensioned variables to (lat,long).
    # Need to make CMIP6_grids the current MIP table for this to work.
    table_id = cmor.load_table('CMIP6_grids.json')
    gaxis_id = cmor.grid([saxis_id], site_lats, site_lons)
    print('ok: created site grid')

    # Create CMOR variable for cloud area fraction: MIP name = 'cl', STASH =
    # m01s02i261*100
    table_id = cmor.load_table('CMIP6_CFsubhr.json')
    var_id = cmor.variable('cl', '%', [taxis_id, gaxis_id, zaxis_id], data_type='f',
                           missing_value=-99.0, original_name='STASH m01s02i261*100')
    print('ok: created variable for "cl"')

    ofact_id = cmor.zfactor(zaxis_id, 'orog', 'm', [gaxis_id], 'd',
                            zfactor_values=[123.0, 123.0, 123.0])
    print('ok: created orog zfactors')
    # Write some data to this variable. First convert raw data to numpy arrays.
    shape = (1, 3, 1)
    data = numpy.array([10, 20, 30], dtype=numpy.float32)
    data = data.reshape(shape)
    cmor.write(var_id, data, time_vals=[1.0])
    print('ok: wrote variable data')

    # Close CMOR.
    cmor.close()
