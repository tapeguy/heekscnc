import area
import math

curve_list_for_zigs = []
rightward_for_zigs = True
sin_angle_for_zigs = 0.0
cos_angle_for_zigs = 1.0
sin_minus_angle_for_zigs = 0.0
cos_minus_angle_for_zigs = 1.0
one_over_units = 1.0


def zigzag(a, stepover, zig_unidirectional, zig_angle):
    if a.num_curves() == 0:
        return

    global rightward_for_zigs
    global curve_list_for_zigs
    global sin_angle_for_zigs
    global cos_angle_for_zigs
    global sin_minus_angle_for_zigs
    global cos_minus_angle_for_zigs
    global one_over_units

    radians_angle = zig_angle * math.pi / 180
    sin_angle_for_zigs = math.sin(-radians_angle)
    cos_angle_for_zigs = math.cos(-radians_angle)
    sin_minus_angle_for_zigs = math.sin(radians_angle)
    cos_minus_angle_for_zigs = math.cos(radians_angle)

    one_over_units = 1 / area.get_units()

    a = rotated_area(a)

    b = area.Box()
    a.GetBox(b)

    x0 = b.MinX() - 1.0
    x1 = b.MaxX() + 1.0

    height = b.MaxY() - b.MinY()
    num_steps = int(height / stepover + 1)
    y = b.MinY() + 0.1 * one_over_units
    null_point = area.Point(0, 0)
    rightward_for_zigs = True
    curve_list_for_zigs = []

    for i in range(0, num_steps):
        y0 = y
        y = y + stepover
        p0 = area.Point(x0, y0)
        p1 = area.Point(x0, y)
        p2 = area.Point(x1, y)
        p3 = area.Point(x1, y0)
        c = area.Curve()
        c.append(area.Vertex(0, p0, null_point, 0))
        c.append(area.Vertex(0, p1, null_point, 0))
        c.append(area.Vertex(0, p2, null_point, 1))
        c.append(area.Vertex(0, p3, null_point, 0))
        c.append(area.Vertex(0, p0, null_point, 1))
        a2 = area.Area()
        a2.append(c)
        a2.Intersect(a)
        make_zig(a2, y0, y, zig_unidirectional)
        if zig_unidirectional == False:
            rightward_for_zigs = (rightward_for_zigs == False)

    reorder_zigs()
    return curve_list_for_zigs


def make_zig_curve(curve, y0, y, zig_unidirectional):
    if rightward_for_zigs:
        curve.Reverse()

    # find a high point to start looking from
    high_point = None
    for vertex in curve.getVertices():
        if high_point == None:
            high_point = vertex.p
        elif vertex.p.y > high_point.y:
            # use this as the new high point
            high_point = vertex.p
        elif math.fabs(vertex.p.y - high_point.y) < 0.002 * one_over_units:
            # equal high point
            if rightward_for_zigs:
                # use the furthest left point
                if vertex.p.x < high_point.x:
                    high_point = vertex.p
            else:
                 # use the furthest right point
                if vertex.p.x > high_point.x:
                    high_point = vertex.p

    zig = area.Curve()

    high_point_found = False
    zig_started = False
    zag_found = False

    for i in range(0, 2): # process the curve twice because we don't know where it will start
        prev_p = None
        for vertex in curve.getVertices():
            if zag_found: break
            if prev_p != None:
                if zig_started:
                    zig.append(unrotated_vertex(vertex))
                    if math.fabs(vertex.p.y - y) < 0.002 * one_over_units:
                        zag_found = True
                        break
                elif high_point_found:
                    if math.fabs(vertex.p.y - y0) < 0.002 * one_over_units:
                        if zig_started:
                            zig.append(unrotated_vertex(vertex))
                        elif math.fabs(prev_p.y - y0) < 0.002 * one_over_units and vertex.type == 0:
                            zig.append(area.Vertex(0, unrotated_point(prev_p), area.Point(0, 0)))
                            zig.append(unrotated_vertex(vertex))
                            zig_started = True
                elif vertex.p.x == high_point.x and vertex.p.y == high_point.y:
                    high_point_found = True
            prev_p = vertex.p

    if zig_started:

        if zig_unidirectional == True:
            # remove the last bit of zig
            if math.fabs(zig.LastVertex().p.y - y) < 0.002 * one_over_units:
                vertices = zig.getVertices()
                while len(vertices) > 0:
                    v = vertices[len(vertices)-1]
                    if math.fabs(v.p.y - y0) < 0.002 * one_over_units:
                        break
                    else:
                        vertices.pop()
                zig = area.Curve()
                for v in vertices:
                    zig.append(v)

        curve_list_for_zigs.append(zig)

def make_zig(a, y0, y, zig_unidirectional):
    for curve in a.getCurves():
        make_zig_curve(curve, y0, y, zig_unidirectional)

reorder_zig_list_list = []

def add_reorder_zig(curve):
    global reorder_zig_list_list

    # look in existing lists
    s = curve.FirstVertex().p
    for curve_list in reorder_zig_list_list:
        last_curve = curve_list[len(curve_list) - 1]
        e = last_curve.LastVertex().p
        if math.fabs(s.x - e.x) < 0.002 * one_over_units and math.fabs(s.y - e.y) < 0.002 * one_over_units:
            curve_list.append(curve)
            return

    # else add a new list
    curve_list = []
    curve_list.append(curve)
    reorder_zig_list_list.append(curve_list)

def reorder_zigs():
    global curve_list_for_zigs
    global reorder_zig_list_list
    reorder_zig_list_list = []
    for curve in curve_list_for_zigs:
        add_reorder_zig(curve)

    curve_list_for_zigs = []
    for curve_list in reorder_zig_list_list:
        for curve in curve_list:
            curve_list_for_zigs.append(curve)

def rotated_point(p):
    return area.Point(p.x * cos_angle_for_zigs - p.y * sin_angle_for_zigs, p.x * sin_angle_for_zigs + p.y * cos_angle_for_zigs)

def unrotated_point(p):
    return area.Point(p.x * cos_minus_angle_for_zigs - p.y * sin_minus_angle_for_zigs, p.x * sin_minus_angle_for_zigs + p.y * cos_minus_angle_for_zigs)

def rotated_vertex(v):
    if v.type:
        return area.Vertex(v.type, rotated_point(v.p), rotated_point(v.c))
    return area.Vertex(v.type, rotated_point(v.p), area.Point(0, 0))

def unrotated_vertex(v):
    if v.type:
        return area.Vertex(v.type, unrotated_point(v.p), unrotated_point(v.c))
    return area.Vertex(v.type, unrotated_point(v.p), area.Point(0, 0))

def rotated_area(a):
    an = area.Area()
    for curve in a.getCurves():
        curve_new = area.Curve()
        for v in curve.getVertices():
            curve_new.append(rotated_vertex(v))
        an.append(curve_new)
    return an
