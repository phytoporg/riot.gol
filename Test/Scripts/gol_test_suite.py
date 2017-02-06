from generate_gol_primitives import generate_primitive, primitives_dict
from gol_validator import GolValidator

import random as rd
import tempfile
import sys
import os

import shutil

DEFAULT_NUM_TESTS=10
DEFAULT_RANGE_XTARGET=(-9223372036854775807,9223372036854775807)
DEFAULT_RANGE_YTARGET=(-9223372036854775807,9223372036854775807)
DEFAULT_NUM_GENERATIONS=100

#
# Reference implementation has to process all cells, so these can't
# go too high without taking a large perf hit.
#
# It's also easier to debug failures if these are smaller; just crank
# up the number of tests to get adequate coverage.
#
DEFAULT_WIDTH=30
DEFAULT_HEIGHT=30

# 
# GolTestSuite is a quick helper class which will:
# 1) Use generate_primitives() to generate a number of random GoL primitives (i.e.
#    blinker, exploder, glider, spaceship, etc) into a config file.
# 2) Execute both the reference implementation and the test target implementation 
#    of GoL and compare text dumps of their outputs.
# 3) Repeat steps 1 and 2 for a while and report on the success or failure of each
#    test.
#
class GolTestSuite:
    #
    # ref_exe: path to reference executable
    # testtarget_exe: reference to "real" GoL implementation
    # num_tests: Number of tests to run
    #
    def __init__(self, ref_exe, testtarget_exe, num_tests):
        self.num_tests   = num_tests
        self.validator   = GolValidator(ref_exe, testtarget_exe)

    def run_test(self, test_index):
        num_primitives = rd.randrange(10, 200)
        filedesc,filepath = tempfile.mkstemp(text=True)

        with open(filepath, 'w') as fw:
            xmin = rd.randrange(*DEFAULT_RANGE_XTARGET)
            ymin = rd.randrange(*DEFAULT_RANGE_YTARGET)
            for primitive_index in range(num_primitives):
                primitive_key = rd.choice(primitives_dict.keys())

                x = rd.randrange(xmin, xmin + DEFAULT_WIDTH)
                y = rd.randrange(ymin, ymin + DEFAULT_HEIGHT)
                rotations = rd.randrange(1, 4)

                primitive = generate_primitive(primitive_key, x, y, rotations)
                for tup in primitive:
                    fw.write("({0},{1})\n".format(tup[0], tup[1]))

        if not self.validator.run_test(filepath, DEFAULT_NUM_GENERATIONS):
            print "{0}: Failed! Input file is at {1}".format(test_index, filepath)
            exit(-1)
        else:
            print "{0}: Succeeded".format(test_index)

        os.close(filedesc)
	os.remove(filepath)

    def run_tests(self):
        for test_index in range(self.num_tests):
            self.run_test(test_index)

def print_usage(program_name):
    print "Usage: python {0} <ref_exe> <testtarget_exe> [num_tests={1}]".format(program_name, DEFAULT_NUM_TESTS)

if __name__=="__main__":
    if len(sys.argv) < 2:
        print_usage(sys.argv[0])
        exit(-1)

    ref_exe = sys.argv[1]
    testtarget_exe = sys.argv[2]
    num_tests = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_NUM_TESTS

    test_suite = GolTestSuite(ref_exe, testtarget_exe, num_tests)
    test_suite.run_tests()
