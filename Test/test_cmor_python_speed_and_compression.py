import cmor
import cdms2
import numpy
import os
import unittest
import base_test_cmor_python
import sys
from time import localtime, strftime
import cdtime
import time
import MV2

class TestCase(base_test_cmor_python.BaseCmorTest):

    def testSpeedAndCompression(self):
        try:
            level = int(os.environ.get("DEFLATE_LEVEL", 0))
            shuffle = int(os.environ.get("SHUFFLE", 0))

            if level == 0:
                deflate = 0
            else:
                deflate = 1
                
            cmor.setup(
                inpath=self.tabledir,
                set_verbosity=cmor.CMOR_NORMAL,
                netcdf_file_action=cmor.CMOR_REPLACE_4,
                exit_control=cmor.CMOR_EXIT_ON_MAJOR, 
                logfile=self.logfile)
            cmor.dataset_json(os.path.join(self.testdir, "common_user_input.json"))

            cdms2.setNetcdfShuffleFlag(0)
            cdms2.setNetcdfDeflateFlag(0)
            cdms2.setNetcdfDeflateLevelFlag(0)

            f = open(os.path.join(self.testdir, "speed_test_table_A"))
            s = f.read()
            f.close()
            s = s.replace("${DEFLATE_LEVEL}", str(level))
            s = s.replace("${DEFLATE}", str(deflate))
            s = s.replace("${SHUFFLE}", str(shuffle))
            f = open("mytable", "w")
            f.write(s)
            f.close()

            tables = []
            tables.append(cmor.load_table("mytable"))
            print 'Tables ids:', tables


            # read in data, just one slice
            f = cdms2.open(os.path.join(self.curdir, 'data', 'tas_ccsr-95a.xml'))


            s = f("tas", time=slice(0, 1), squeeze=1)

            ntimes = 12

            myaxes = numpy.arange(10)
            myvars = numpy.arange(10)
            myaxes[0] = cmor.axis(table_entry='latitude',
                                units='degrees_north',
                                coord_vals=s.getLatitude()[:], cell_bounds=s.getLatitude().getBounds())
            myaxes[1] = cmor.axis(table_entry='longitude',
                                units='degrees_north',
                                coord_vals=s.getLongitude()[:], cell_bounds=s.getLongitude().getBounds())


            myaxes[3] = cmor.axis(table_entry='time',
                                units='months since 1980',
                                coord_vals=numpy.arange(ntimes), cell_bounds=numpy.arange(ntimes + 1))

            pass_axes = [myaxes[2], myaxes[0], myaxes[1]]

            myvars[0] = cmor.variable(table_entry='tas',
                                    units='K',
                                    axis_ids=pass_axes,
                                    original_name='CLT',
                                    history='no history',
                                    comment='testing speed'
                                    )

            st = time.time()
            totcmor = 0
            totcdms = 0
            maxcmor = 0
            mincmor = 1000
            maxcdms = 0
            mincdms = 1000
            c0 = st
            s2 = s * 1
            sh = list(s.shape)
            print 'shape:', sh
            sh.insert(0, 1)
            s2 = MV2.reshape(s2, sh)
            s2.setAxis(1, s.getLatitude())
            s2.setAxis(2, s.getLongitude())
            f = cdms2.open(os.path.join(self.testdir, "crap.nc"), "w")
            for i in range(ntimes):
                # print 'Time:',i
                cmor.write(myvars[0], s.filled(), 1)
                c = time.time()
                # print 'cmor write time:',c-c0
                totcmor += c - c0
                if maxcmor < c - c0:
                    maxcmor = c - c0
                if mincmor > c - c0:
                    mincmor = c - c0
                c0 = c
                t = cdms2.createAxis([i])
                t.id = 'time'
                t.units = 'months since 1980'
                t.designateTime()
                s2.setAxis(0, t)
                f.write(s2, id='tas')
                c = time.time()
                # print 'cdms time:',c-c0
                totcdms += c - c0
                if maxcdms < c - c0:
                    maxcdms = c - c0
                if mincdms > c - c0:
                    mincdms = c - c0
                c0 = c
            f.close()
            cmor.close()
            self.processLog()

            today = strftime("%Y%m%d", localtime())
            ltime = cdtime.reltime(ntimes - 1, 'month since 1980').tocomp()
            #lcmor = os.stat("CMIP6/CMIP/CSIRO-BOM/NICAM/piControl/r1i1p1f1/Amon/tas/gn/v%s/tas_Amon_piControl_NICAM_r1i1p1f1_gn_198001-%i%.2i.nc" % (today,ltime.year,ltime.month))[6]
            lcmor = os.stat(
                "CMIP6/CMIP6/ISMIP6/PCMDI/PCMDI-test-1-0/piControl-withism/r11i1p1f1/Amon/tas/gr/v%s/tas_Amon_PCMDI-test-1-0_piControl-withism_r11i1p1f1_gr_198001-%i%.2i.nc" %
                (today, ltime.year, ltime.month))[6]
            print 'level:', level, "shuffle:", shuffle
            print 'total cmor:', totcmor, mincmor, totcmor / ntimes, maxcmor, lcmor
            lcdms = os.stat(os.path.join(self.testdir, "crap.nc"))[6]
            print 'total cdms:', totcdms, mincdms, totcdms / ntimes, maxcdms, lcdms
            print 'Size diff:', float(lcmor) / float(lcdms)
            print 'speed diff:', totcmor / totcdms

            if os.path.exists("summary.txt"):
                f = open("summary.txt")
                s = f.read()
                f.close()
                dic = eval(s)
            else:
                dic = {}

            dic[(level, shuffle)] = (float(lcmor) / float(lcdms), totcmor / totcdms)

            for i in range(10):
                a = dic.get((i, 0), "N/A")
                b = dic.get((i, 1), "N/A")
                print 'Level: ', i, "no suffle:", a, "shuffle", b
            f = open("summary.txt", "w")
            f.write(repr(dic))
            f.close()
        except BaseException:
            raise


if __name__ == '__main__':
    unittest.main()
