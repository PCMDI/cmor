# If this example is not executed from the directory containing the
# CMOR code, please first complete the following steps:
#
#   1. In any directory, create 'Tables/', 'Test/' and 'CMIP6/' directories.
#
#   2. Download
#      https://github.com/PCMDI/cmor/blob/master/TestTables/CMIP6_Omon.json
#      and https://github.com/PCMDI/cmor/blob/master/TestTables/CMIP6_CV.json
#      to the 'Tables/' directory.
#
#   3. Download
#      https://github.com/PCMDI/cmor/blob/master/Test/<filename>.json
#      to the 'Test/' directory.

import cmor
import numpy
import unittest
import os
import sys
import tempfile

# ==============================
#  main thread
# ==============================


def run():
    unittest.main()


class TestCase(unittest.TestCase):

    def testCMIP6(self):

        # Create x and y coordinates and bounds.
        #
        coordVals = numpy.arange(-533750.0, 533750.0 +
                                 2500.0, 2500.0, numpy.float32)
        coordBnds = numpy.zeros((coordVals.shape[0], 2), numpy.float32)

        coordBnds[:, 0] = coordVals - 1250.0
        coordBnds[:, 1] = coordVals + 1250.0

        # Create longitude and latitude fields for a polar stereographic projection.
        #
        xgrid, ygrid = numpy.broadcast_arrays(
            numpy.expand_dims(
                coordVals, 0), numpy.expand_dims(
                coordVals, 1))

        rhogrid = numpy.sqrt(xgrid**2 + ygrid**2)
        cgrid = 2.0 * numpy.arctan((0.5 / 6378137.0) * rhogrid)
        latgrid = (180.0 / 3.141592654) * numpy.arcsin(numpy.cos(cgrid))
        longrid = (180.0 / 3.141592654) * numpy.arctan2(xgrid, -ygrid)

        # Set up CMOR with information from the CMOR config dictionary.
        #

        cmor.setup("Tables", netcdf_file_action=cmor.CMOR_REPLACE_4)

        # Create the output CMOR dataset using the output configuration.
        #
        cmor.dataset_json("Test/CMOR_input_example.json")

        # Load the grid table.
        #
        cmor_table_obj = cmor.load_table("CMIP6_grids.json")

        # Create ygre and xgre axes.
        #
        entry = {
            'table_entry': 'y',
            'units': 'm',
            'coord_vals': coordVals,
            'cell_bounds': coordBnds}

        axis_ygre = cmor.axis(**entry)

        entry = {
            'table_entry': 'x',
            'units': 'm',
            'coord_vals': coordVals,
            'cell_bounds': coordBnds}

        axis_xgre = cmor.axis(**entry)

        # Create the grid
        #
        grid_id = cmor.grid(
            axis_ids=[
                axis_ygre,
                axis_xgre],
            latitude=latgrid,
            longitude=longrid)

        # Set the CMOR grid mapping.
        #
        #    mapnm = 'polar_stereographic'
        param_dict = {
            'latitude_of_projection_origin': [90.0, 'degrees_north'],
            'longitude_of_projection_origin': [135.0, 'degrees_east'],
            'standard_parallel': [70.0, 'degrees_north'],
            'false_northing': [0.0, 'meters'],
            'false_easting': [0.0, 'meters']
        }
        ierr = cmor.set_grid_mapping(
            grid_id, 'polar_stereographic', param_dict)
        self.assertEqual(ierr, 0)


if __name__ == '__main__':
    run()
