import sys
from os.path import exists, isfile
import numpy as np

def read_ref_state(handle):
    first_line = handle.readline()
    if not first_line:
        return None

    # Expected format: (xmin,ymin,width,height)
    xmin,ymin,width,height = [int(d) for d in first_line.translate(None, '()').split(',')]
    generation = int(handle.readline())

    flattened_buffer = []
    for x in range(height):
        flattened_buffer.extend([int(cell) for cell in handle.readline().split(',')])

    return {
            "generation" : generation,
            "buffer"     : flattened_buffer, 
            "xmin"       : xmin,
            "ymin"       : ymin,
            "width"      : width,
            "height"     : height
            }

def read_testtarget_state(handle):
    first_line = handle.readline()
    if not first_line:
        return None

    # Read in state dimensions: (xmin,ymin,width,height)
    xmin,ymin,width,height = [int(d) for d in first_line.translate(None, "()").split(',')]
    generation = int(handle.readline())

    # Create appropriately sized buffer of zeros
    buffer_2d = np.zeros((height, width))
    
    # Next line should contain the # of subgrids
    for i in range(int(handle.readline())):
        s_xmin,s_ymin,s_width,s_height = [int(d) for d in handle.readline().translate(None, "()").split(',')]
        for j in range(s_height):
            slice_x_start = s_xmin - xmin
            slice_y       = s_ymin - ymin + j
            slice_x_end   = slice_x_start + s_width
            buffer_2d[slice_y, slice_x_start:slice_x_end] = [int(cell) for cell in handle.readline().split(',')]

    return {
            "generation" : generation,
            "buffer"     : buffer_2d.flatten().tolist(), 
            "xmin"       : xmin,
            "ymin"       : ymin,
            "width"      : width,
            "height"     : height
           }

def compare_states(ref_state, testtarget_state):
    keys = ref_state.keys()
    for key in keys:
        if key == "buffer":
            for pair in zip(ref_state[key],testtarget_state[key]):
                if pair[0] != pair[1]:
                    print "Mismatch in state representation. Check generation {0}".format(ref_state["generation"])
                    return False
        elif ref_state[key] != testtarget_state[key]:
            print "Mismatch at key {0}!".format(key)
            print "ref_state = {0}".format(ref_state)
            print "testtarget_state_state = {0}".format(testtarget_state)
            return False

    return True

def main(reference_output_file, testtarget_output_file):
    with open(reference_output_file) as ref_handle:
        with open(testtarget_output_file) as test_handle:

            compare_valid = True
            while(compare_valid):
                ref_state = read_ref_state(ref_handle)
                testtarget_state = read_testtarget_state(test_handle)

                if not ref_state and not testtarget_state:
                    break
                elif not ref_state:
                    print "Ref state ran out of lines. Exiting"
                    exit(-1)
                elif not testtarget_state:
                    print "Test target state ran out of lines. Exiting"
                    exit(-1)

                compare_valid = compare_states(ref_state, testtarget_state)
    
    if not compare_valid:
        print "Test failed."
        exit(-1)

if __name__=="__main__":
    reference_output_file = sys.argv[1]
    testtarget_output_file = sys.argv[2]

    assert(exists(reference_output_file) and isfile(reference_output_file))
    assert(exists(testtarget_output_file) and isfile(testtarget_output_file))

    main(reference_output_file, testtarget_output_file)
