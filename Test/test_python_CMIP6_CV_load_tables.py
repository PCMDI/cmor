import cmor
import glob
import unittest



def run():
    unittest.main()

class TestLoadTables(unittest.TestCase):
    def testLoadTables(self):
        tables = glob.glob("Tables/CMIP6*json")
        for table in tables:
            if "formula_terms" in table:
                continue
            cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
            cmor.dataset_json("Test/CMOR_input_example.json")
            print("Loading table:", table)
            ierr = cmor.load_table(table)
            self.assertEqual(ierr, 0)
            cmor.close()

if __name__ == '__main__':
    run()