import cmor,numpy
import unittest
import sys,os
import tempfile
import pdb

class TestInstitutionMethods(unittest.TestCase):

    def test_Institution(self):

        # ------------------------------------------------------
        # Copy stdout and stderr file descriptor for cmor output
        # ------------------------------------------------------
        newstdout = os.dup(1)
        newstderr = os.dup(2)
        # --------------
        # Create tmpfile
        # --------------
        tmpfile = tempfile.mkstemp()
        os.dup2(tmpfile[0], 1)
        os.dup2(tmpfile[0], 2)
        os.close(tmpfile[0])
        # -------------------------------------------
        # Try to call cmor with a bad institution_ID
        # -------------------------------------------
        try:
            error_flag = cmor.setup(inpath='Tables', netcdf_file_action=cmor.CMOR_REPLACE)
            error_flag = cmor.dataset_json("Test/test_python_CMIP6_CV_badinstitution.json")
  
            # ------------------------------------------
            # load Omon table and create masso variable
            # ------------------------------------------
            cmor.load_table("CMIP6_Omon.json")
            itime = cmor.axis(table_entry="time",units='months since 2010',
                              coord_vals=numpy.array([0,1,2,3,4.]),
                              cell_bounds=numpy.array([0,1,2,3,4,5.]))
            ivar = cmor.variable(table_entry="masso",axis_ids=[itime],units='kg')

            data=numpy.random.random(5)
            for i in range(0,5):
                cmor.write(ivar,data[i:i])
            error_flag = cmor.close()
        finally:
            os.dup2(newstdout,1)
            os.dup2(newstderr,2)
            sys.stdout = os.fdopen(newstdout, 'w', 0)
            sys.stderr = os.fdopen(newstderr, 'w', 0)
            f=open(tmpfile[1],'r')
            lines=f.readlines()
            for line in lines:
                if line.find('Error:') != -1:
                    self.assertIn('bad institution', line.strip())
            f.close()
            os.unlink(tmpfile[1])


if __name__ == '__main__':
    unittest.main()

