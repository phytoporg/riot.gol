import sys
from os.path import exists, isfile, join, isdir
import numpy as np
import subprocess
import tempfile
import time

class GolValidator:
    def __init__(self, ref_exe, test_exe):
        assert(isfile(ref_exe))
        self.ref_exe = ref_exe
        assert(isfile(test_exe))
        self.test_exe = test_exe

    def run_test(self, input_file, generations):
        print input_file
        assert(isfile(input_file))

        temp_dir = tempfile.mkdtemp()
    
        ref_out = join(temp_dir, "ref_output.txt")
        ref_args = [self.ref_exe, input_file, generations, ref_out]
        ref_p = subprocess.Popen(ref_args)
        print "Reference output to {0}".format(ref_out)

        test_out = join(temp_dir, "test_output.txt")
        test_args = [self.test_exe, input_file, generations, test_out] 
        test_p = subprocess.Popen(test_args)
        print "Test target output to {0}".format(test_out)

        ref_p.wait()
        test_p.wait()

        failed = False
        if not isfile(ref_out):
            print "Failed to generate {0}".format(ref_out)
            failed = True
        if not isfile(test_out):
            print "Failed to generate {0}".format(test_out)
            failed = True

        if failed:
            return False

        return self.__compare_files(ref_out, test_out)
        
    def __read_ref_state(self, handle):
        first_line = handle.readline()
        if not first_line:
            return None

        # Expected format: (xmin,ymin,width,height)
        xmin,ymin,width,height = [int(d) for d in first_line.translate(None, '()').split(',')]
        generation = int(handle.readline())

        state_matrix = np.zeros((height,width), np.int8)
        for y in range(height):
            state_matrix[y,:] = [int(cell) for cell in handle.readline().split(',')]

        return {
                "generation"   : generation,
                "state_matrix" : state_matrix, 
                "xmin"         : xmin,
                "ymin"         : ymin,
                "width"        : width,
                "height"       : height
                }

    def __read_testtarget_state(self, handle):
        first_line = handle.readline()
        if not first_line:
            return None

        # Read in state dimensions: (xmin,ymin,width,height)
        xmin,ymin,width,height = [int(d) for d in first_line.translate(None, "()").split(',')]
        generation = int(handle.readline())

        # Create appropriately sized buffer of zeros
        state_matrix = np.zeros((height, width), np.int8)
        
        # Next line should contain the # of subgrids
        num_subgrids = int(handle.readline())
        for i in range(num_subgrids):
            s_xmin,s_ymin,s_width,s_height = [int(d) for d in handle.readline().translate(None, "()").split(',')]
            for j in range(s_height):
                slice_y       = s_ymin - ymin + j
                slice_x_start = s_xmin - xmin
                slice_x_end   = slice_x_start + s_width
                state_matrix[slice_y, slice_x_start:slice_x_end] = [int(cell) for cell in handle.readline().split(',')]

        return {
                "generation"   : generation,
                "state_matrix" : state_matrix, 
                "xmin"         : xmin,
                "ymin"         : ymin,
                "width"        : width,
                "height"       : height
               }

    def __compare_states(self, ref_state, testtarget_state):
        keys = ref_state.keys()
        for key in keys:
            if key == "state_matrix":
                for pair in zip(ref_state[key].flatten(),testtarget_state[key].flatten()):
                    if pair[0] != pair[1]:
                        print "Mismatch in state representation. Check generation {0}".format(ref_state["generation"])

                        bad_test_outfile = "bad_test_gen_{0}.txt".format(ref_state["generation"])
                        print "Writing mismatched test target state to {0}".format(bad_test_outfile)

                        state_matrix = testtarget_state["state_matrix"]
                        height, width = state_matrix.shape
                        print str(state_matrix[0,0])
                        with open(bad_test_outfile, 'w') as fw:
                            for row in range(height):
                                for col in range(width):
                                    fw.write(str(state_matrix[row,col]))
                                    if col < width - 1:
                                        fw.write(',')
                                fw.write('\n')
                        
                        return False

            elif ref_state[key] != testtarget_state[key]:
                print "Mismatch at {0}!".format(key)
                print "ref_state = {0}".format(ref_state)
                print "testtarget_state_state = {0}".format(testtarget_state)
                return False

        return True

    def __compare_files(self, reference_output_file, testtarget_output_file):
        with open(reference_output_file) as ref_handle:
            with open(testtarget_output_file) as test_handle:

                compare_valid = True
                while(compare_valid):
                    ref_state = self.__read_ref_state(ref_handle)
                    testtarget_state = self.__read_testtarget_state(test_handle)

                    if not ref_state and not testtarget_state:
                        break
                    elif not ref_state:
                        print testtarget_state
                        print "Ref state ran out of lines. Exiting"
                        exit(-1)
                    elif not testtarget_state:
                        print "Test target state ran out of lines. Exiting"
                        exit(-1)

                    compare_valid = self.__compare_states(ref_state, testtarget_state)
        
        if not compare_valid:
            print "Test failed."
            return False

        return True

if __name__=="__main__":
    if len(sys.argv) < 5:
        print "Usage: {0} ref_exec test_exec input_file generations".format(sys.argv[0])
    else:
        reference_exec = sys.argv[1]
        test_exec = sys.argv[2]
        input_file = sys.argv[3]
        generations = sys.argv[4]
        
        validator = GolValidator(reference_exec, test_exec)
        print "Success" if validator.run_test(input_file, generations) else "Failed"

