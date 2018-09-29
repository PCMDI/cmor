'''
Note the memory size from repeated calls to cmor

Memory stats are taken from http://code.activestate.com/recipes/286222/
'''

import cmor
import sys
import os
import unittest
import base_test_cmor_python


class TestCase(base_test_cmor_python.BaseCmorTest):

    def _VmB(self, VmKey):
        '''Private.
        '''
        _proc_status = '/proc/%d/status' % os.getpid()

        _scale = {'kB': 1024.0, 'mB': 1024.0 * 1024.0,
                'KB': 1024.0, 'MB': 1024.0 * 1024.0}
        # get pseudo file  /proc/<pid>/status
        try:
            t = open(_proc_status)
            v = t.read()
            t.close()
        except BaseException:
            return 0.0  # non-Linux?
        # get VmKey line e.g. 'VmRSS:  9999  kB\n ...'
        i = v.index(VmKey)
        v = v[i:].split(None, 3)  # whitespace
        if len(v) < 3:
            return 0.0  # invalid format?
        # convert Vm value to bytes
        return float(v[1]) * _scale[v[2]]


    def memory(self, since=0.0):
        '''Return memory usage in bytes.
        '''
        return self._VmB('VmSize:') - since


    def resident(self, since=0.0):
        '''Return resident memory usage in bytes.
        '''
        return self._VmB('VmRSS:') - since


    def stacksize(self, since=0.0):
        '''Return stack size in bytes.
        '''
        return self._VmB('VmStk:') - since


    def memory_usage(self, fhd, mem=0, res=0, stk=0):
        nmem = self.memory()
        nres = self.resident()
        nstk = self.stacksize()
        fhd.write('memory: %s\t%i\t\t%i\t\t%i\n' %
                (nmem, self.memory(mem), self.resident(res), self.stacksize(stk)))
        return nmem, nres, nstk


    def testMemoryCheck(self):
        try:
            cmor.setup(inpath=self.tabledir, netcdf_file_action=cmor.CMOR_REPLACE, logfile=self.logfile)
            cmor.dataset_json(os.path.join(self.testdir, "common_user_input.json"))

            table = 'CMIP6_Amon.json'
            cmor.load_table(table)

            tval = []
            tbounds = []

            passtime = True
            ntimes = 1200
            for time in range(ntimes):
                tval.append(15 + time * 30)
                tbounds.append([time * 30, (time + 1) * 30])

            if passtime:
                timdef = {'table_entry': 'time',
                        'units': 'days since 2000-01-01 00:00:00',
                        }
            else:
                timdef = {'table_entry': 'time',
                        'units': 'days since 2000-01-01 00:00:00',
                        'coord_vals': tval,
                        'cell_bounds': tbounds,
                        }

            axes = [timdef,
                    {'table_entry': 'latitude',
                    'units': 'degrees_north',
                    'coord_vals': [0],
                    'cell_bounds': [[-1, 1], ],
                    },
                    {'table_entry': 'longitude',
                    'units': 'degrees_east',
                    'coord_vals': [90],
                    'cell_bounds': [89, 91]},
                    ]

            axis_ids = list()
            for axis in axes:
                axis_id = cmor.axis(**axis)
                axis_ids.append(axis_id)
            varid = cmor.variable('ts', 'K', axis_ids)

            mem = 0
            res = 0
            stk = 0
            memi, resi, stki = self.memory_usage(sys.stdout)

            for time in range(ntimes):
                a = [275]
                if passtime:
                    tval = [15 + time * 30]
                    tbounds = [time * 30, (time + 1) * 30]
                    cmor.write(varid, [275], time_vals=tval, time_bnds=tbounds)
                else:
                    cmor.write(varid, [275])
                mem, res, stk = self.memory_usage(sys.stdout, mem, res, stk)
                #mem,res,stk = self.memory_usage(sys.stdout,memi,resi,stki)
                print '---'

            cmor.close(varid)
            cmor.close()
            self.processLog()
        except BaseException:
            raise
            

if __name__ == '__main__':
    unittest.main()
