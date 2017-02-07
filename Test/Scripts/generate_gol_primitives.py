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

bowling = [
(-4,-4),(-3,-4),(-2,-4),(-4,4),(-3,4),(-2,4),(-4,0),(-3,0),(-2,0),(-8,0),(-7,0),(-6,0),(-8,-8),(-7,-8),(-6,-8),(-8,8),(-7,8),(-6,8),(1,0),(2,0),(3,0),(21,0),(24,0),(20,1),(20,2),(20,3),(21,3),(22,3),(23,3),(24,2),(-30,0),(-30,2),(-29,3),(-28,3),(-27,0),(-27,3),(-26,1),(-26,2),(-26,3),(-50,0),(-50,2),(-49,3),(-48,3),(-47,0),(-47,3),(-46,1),(-46,2),(-46,3),(51,0),(54,0),(50,1),(50,2),(50,3),(51,3),(52,3),(53,3),(54,2)
]

primitives_dict = {
    "blinker"               : primitive_blinker_1,
    "beacon"                : primitive_beacon_1,
    "glider"                : primitive_glider_1,
    "lightweight_spaceship" : primitive_lightweight_spaceship_1,
    "exploder"              : primitive_exploder_1,
    "bowling"               : bowling,
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


def generate_primitive(primitive_name, x, y, rotations=0):
    global primitives_dict
    if primitive_name not in primitives_dict:
        print "Invalid primitive: {0}".format(primitive_name)
        return
    
    primitive = primitives_dict[primitive_name]
    if rotations > 0:
        primitive = rotate_primitive(primitive, rotations)

    primitive = [(offx + x, offy + y) for offx, offy in primitive]
    return primitive

if __name__=="__main__":
    if len(sys.argv) < 2:
        print_usage(sys.argv[0])
    else:
        rotations = 0 if not len(sys.argv) > 4 else int(sys.argv[4])
        primitive = generate_primitive(sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), rotations)
        for t in primitive:
            print "({0},{1})".format(t[0], t[1])
