import cmor
import numpy
import unittest
from base_CMIP6_CV import BaseCVsTest


def run():
    unittest.main()


class TestNaNCheck(BaseCVsTest):

    def test_nan_check(self):
        cmor.setup(inpath='Tables',
                   netcdf_file_action=cmor.CMOR_REPLACE_4,
                   logfile=self.tmpfile)

        cmor.dataset_json("Test/CMOR_input_example.json")    
        cmor.load_table("CMIP6_Amon.json")

        itime = cmor.axis(table_entry= 'time',
                        units= 'days since 2000-01-01 00:00:00',
                        coord_vals= [15, 45, 75],
                        cell_bounds= [0, 30, 60, 90])
        ilat = cmor.axis(table_entry= 'latitude',
                        units= 'degrees_north',
                        coord_vals= [0],
                        cell_bounds= [-1, 1])
        ilon = cmor.axis(table_entry= 'longitude',
                        units= 'degrees_east',
                        coord_vals= [90],
                        cell_bounds= [89, 91])

        axis_ids = [itime,ilat,ilon]
                    
        varid = cmor.variable('ts', 'K', axis_ids)

        with self.assertRaises(cmor.CMORError):
            _ = cmor.write(varid, [273, numpy.nan, 273])
        
        self.assertCV(
            "Invalid value(s) detected for variable 'ts' "
            "(table: Amon): 1 values were NaNs."
        )
        self.assertCV(
            "time: 1/45 lat: 0/0 lon: 0/90",
            line_trigger="First encountered NaN was at (axis: index/value):"
        )


if __name__ == '__main__':
    run()
