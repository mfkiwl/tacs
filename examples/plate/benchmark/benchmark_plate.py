'''
This script is used to regression test the example against historical values.
'''

import numpy as np
import unittest
import sys
import os

# Set the path to the example script we're testing
example_path = os.path.join(os.path.dirname(__file__), '..')
sys.path.append(example_path)

# Reference values for eval functions
FUNC_REF = {'point_force_ks_vmfailure': 1.4546105780086274,
            'point_force_mass': 12.500000000000535,
            'pressure_ks_vmfailure': 0.3522022461334997,
            'pressure_mass': 125.00000000026239}

class ExampleBenchmark(unittest.TestCase):

    N_PROCS = 8  # this is how many MPI processes to use for this TestCase.

    def setUp(self):
        # Import the example to automatically run the script
        import analysis
        self.example = analysis

    def benchmark_funcs(self):
        """
        Test the example eval functions against reference values
        """
        func_dict = self.example.funcs

        # Test functions values against historical values
        for func_name in func_dict:
            with self.subTest(function=func_name):
                np.testing.assert_allclose(func_dict[func_name], FUNC_REF[func_name], rtol=1e-6, atol=1e-6)