import unittest
import Test.all_tests
testSuite = Test.all_tests.create_test_suite()
text_runner = unittest.TextTestRunner(verbosity=2).run(testSuite)
