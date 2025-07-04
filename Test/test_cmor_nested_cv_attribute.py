import json
import cmor
import unittest
import os

from netCDF4 import Dataset

CV_PATH = "TestTables/CMIP6_CV_nested_attribute.json"

DATASET_INFO = {
    "_AXIS_ENTRY_FILE": "Tables/CMIP6_coordinate.json",
    "_FORMULA_VAR_FILE": "Tables/CMIP6_formula_terms.json",
    "_controlled_vocabulary_file": CV_PATH,
    "activity_id": "CMIP",
    "branch_method": "standard",
    "branch_time_in_child": 30.0,
    "branch_time_in_parent": 10800.0,
    "calendar": "360_day",
    "cv_version": "6.2.19.0",
    "domain_id": "EUR-50",
    "experiment": "AMIP",
    "experiment_id": "amip",
    "forcing_index": "3",
    "further_info_url": "https://furtherinfo.es-doc.org/CMIP6.MOHC.HadGEM3-GC31-LL.amip.none.r1i1p1f3",
    "grid": "N96",
    "grid_label": "gn",
    "initialization_index": "1",
    "institution": "Met Office Hadley Centre, Fitzroy Road, Exeter, Devon, EX1 3PB, UK",
    "institution_id": "MOHC",
    "license": "CMIP6 model data produced by the Met Office Hadley Centre is licensed under a Creative Commons Attribution-ShareAlike 4.0 International License (https://creativecommons.org/licenses). Consult https://pcmdi.llnl.gov/CMIP6/TermsOfUse for terms of use governing CMIP6 output, including citation requirements and proper acknowledgment. Further information about this data, including some limitations, can be found via the further_info_url (recorded as a global attribute in this file) and at https://ukesm.ac.uk/cmip6. The data producers and data providers make no warranty, either express or implied, including, but not limited to, warranties of merchantability and fitness for a particular purpose. All liabilities arising from the supply of the information (including any liability arising in negligence) are excluded to the fullest extent permitted by law.",
    "mip_era": "CMIP6",
    "nominal_resolution": "250 km",
    "outpath": ".",
    "parent_activity_id": "no parent",
    "physics_index": "1",
    "realization_index": "1",
    "source": "HadGEM3-GC31-LL (2016): \naerosol: UKCA-GLOMAP-mode\natmos: MetUM-HadGEM3-GA7.1 (N96; 192 x 144 longitude/latitude; 85 levels; top level 85 km)\natmosChem: none\nland: JULES-HadGEM3-GL7.1\nlandIce: none\nocean: NEMO-HadGEM3-GO6.0 (eORCA1 tripolar primarily 1 deg with meridional refinement down to 1/3 degree in the tropics; 360 x 330 longitude/latitude; 75 levels; top grid cell 0-1 m)\nocnBgchem: none\nseaIce: CICE-HadGEM3-GSI8 (eORCA1 tripolar primarily 1 deg; 360 x 330 longitude/latitude)",
    "source_id": "HadGEM3-GC31-LL",
    "source_type": "AGCM",
    "sub_experiment": "none",
    "sub_experiment_id": "none",
    "tracking_prefix": "hdl:21.14100",
    "variant_label": "r1i1p1f3"
}


class TestNestedCVAttribute(unittest.TestCase):
    def setUp(self):
        """
        Write out a simple file using CMOR
        """
        # Set up CMOR
        cmor.setup(inpath="Tables", netcdf_file_action=cmor.CMOR_REPLACE,
                   logfile="cmor.log", create_subdirectories=0)

        # Add 'domain' and 'domain_id' to required attributes of CV
        # and 'domain_id' nested attribute
        with open("Tables/CMIP6_CV.json", "r") as cv_infile:
            cv = json.load(cv_infile)
            cv["CV"]["required_global_attributes"].append("domain")
            cv["CV"]["required_global_attributes"].append("domain_id")
            cv["CV"]["required_global_attributes"].sort()
            domain_id = {
                "EUR-50": {
                    "domain": "Europe",
                    "domain_id": "EUR-50"
                }
            }
            cv["CV"]["domain_id"] = domain_id
            with open(CV_PATH, "w") as cv_outfile:
                json.dump(cv, cv_outfile, sort_keys=True, indent=4)

        # Define dataset using DATASET_INFO
        with open("Test/input_nested_attribute.json", "w") as input_file:
            json.dump(DATASET_INFO, input_file, sort_keys=True, indent=4)

        # read dataset info
        error_flag = cmor.dataset_json("Test/input_nested_attribute.json")
        if error_flag:
            raise RuntimeError("CMOR dataset_json call failed")

    def test_nested_cv_attribute(self):
        mip_table = "CMIP6_Omon.json"
        _ = cmor.load_table(mip_table)

        itim = cmor.axis(
            table_entry='time',
            units='months since 2010-1-1',
            coord_vals=[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
            cell_bounds=[0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12])
        ivar = cmor.variable('thetaoga', units='deg_C', axis_ids=[itim, ])

        data = [280., ] * 12
        cmor.write(ivar, data)
        filename = cmor.close(ivar, file_name=True)

        ds = Dataset(filename)
        attrs = ds.ncattrs()
        self.assertTrue('domain' in attrs)
        self.assertTrue('domain_id' in attrs)
        self.assertEqual('Europe', ds.getncattr('domain'))
        self.assertEqual('EUR-50', ds.getncattr('domain_id'))
        ds.close()
        os.remove(filename)


if __name__ == '__main__':
    unittest.main()
