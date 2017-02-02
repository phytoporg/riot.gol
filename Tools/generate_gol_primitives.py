import sys
import numpy as np

primitive_blinker_1 = [
        (0, 0), (1, 0), (2, 0)
        ]

primitive_beacon_1 = [
        (0, 0), (1, 0), (-1, 0), (3, 3), (3, 2), (2, 3)
        ]

primitive_glider_1 = [
        (1, 0), (2, 1), (2, 2), (1, 2), (0, 2)
        ]

primitive_lightweight_spaceship_1 = [
        (1, 0), (4, 0), (0, 1), (0, 2), (0, 3), (1, 3), (2, 3), (3, 3), (4, 2)
        ]

primitive_exploder_1 = [
	(0,0), (2,0), (4,0), (0,1), (4,1), (0,2), (4,2), (0,3), (4,3), (0,4), (2,4), (4,4)
	]

primitives_dict = {
    "blinker"               : primitive_blinker_1,
    "beacon"                : primitive_beacon_1,
    "glider"                : primitive_glider_1,
    "lightweight_spaceship" : primitive_lightweight_spaceship_1,
    "exploder"              : primitive_exploder_1,
    }

def print_usage(program_name):
    print "Usage: python {0} ({1}) x y [rotations=0]".format(program_name, ",".join(primitives_dict.keys()))

def rotate_primitive(primitive, rotations):
    x = [x for x,_ in primitive]
    y = [y for _,y in primitive]

    rows = max(y) - min(y) + 1
    cols = max(x) - min(x) + 1

    m = np.zeros((rows, cols))
    for c,r in primitive:
        m[r - min(y), c - min(x)] = 1

    rot = np.rot90(m, k = rotations)
    x_where,y_where = np.where(rot > 0)
    return zip(x_where, y_where)
    return None


def main(primitive_name, x, y, rotations=0):
    global primitives_dict
    if primitive_name not in primitives_dict:
        print "Invalid primitive: {0}".format(primitive_name)
        return
    
    primitive = primitives_dict[primitive_name]
    if rotations > 0:
        primitive = rotate_primitive(primitive, rotations)

    primitive = [(offx + x, offy + y) for offx, offy in primitive]
    for t in primitive:
        print "({0},{1})".format(t[0], t[1])

if __name__=="__main__":
    if len(sys.argv) < 2:
        print_usage(sys.argv[0])
    else:
        rotations = 0 if not len(sys.argv) > 4 else int(sys.argv[4])
        main(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), rotations)
