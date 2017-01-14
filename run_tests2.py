testmodules = [
"Test.test_python_CMIP6_CV_baddirectory",
"Test.test_python_CMIP6_CV_badgridgr",
"Test.test_python_CMIP6_CV_badgridlabel",
"Test.test_python_CMIP6_CV_badgridresolution",
"Test.test_python_CMIP6_CV_badinstitutionIDNotSet",
"Test.test_python_CMIP6_CV_badinstitutionID",
"Test.test_python_CMIP6_CV_badinstitution",
"Test.test_python_CMIP6_CV_badsourceid",
"Test.test_python_CMIP6_CV_badsource",
"Test.test_python_CMIP6_CV_badsourcetypeCHEMAER",
"Test.test_python_CMIP6_CV_badsourcetype",
"Test.test_python_CMIP6_CV_badsourcetypeRequired",
"Test.test_python_CMIP6_CV_badvariant",
"Test.test_python_CMIP6_CV_externalvariables",
"Test.test_python_CMIP6_CV_furtherinfourl",
"Test.test_python_CMIP6_CV_fxtable",
"Test.test_python_CMIP6_CV_HISTORY",
"Test.test_python_CMIP6_CV_longrealizationindex",
"Test.test_python_CMIP6_CV_nomipera",
"Test.test_python_CMIP6_CV_trackingNoprefix",
"Test.test_python_CMIP6_CV_trackingprefix",
"Test.test_python_has_cur_dataset_attribute",
"Test.test_python_has_variable_attribute.TestHasVariableAttribute"
]
import unittest
suite = unittest.TestSuite()

for t in testmodules:
    try:
        # If the module defines a suite() function, call it to get the suite.
        mod = __import__(t, globals(), locals(), ['suite'])
        suitefn = getattr(mod, 'suite')
        suite.addTest(suitefn())
    except (ImportError, AttributeError):
        # else, just load all the test cases from the module.
        suite.addTest(unittest.defaultTestLoader.loadTestsFromName(t))

unittest.TextTestRunner(verbosity=2).run(suite)

