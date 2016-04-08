import area
from nc.nc import *
import math
import kurve_funcs
from postprocessor import *

# some globals, to save passing variables as parameters too much
area_for_feed_possible = None
tool_radius_for_pocket = None

def cut_curve(curve, raise_cutter, first, prev_p, rapid_safety_space, current_start_depth, final_depth, clearance_height):

    slot_ratio = 1.0 if first else 0.0

    for vertex in curve.getVertices():
        if first:
            if raise_cutter:
                rapid(z = clearance_height)
                rapid(vertex.p.x, vertex.p.y)
                rapid(z = current_start_depth + rapid_safety_space)
    
            else:
                rapid(z = current_start_depth + rapid_safety_space)
                rapid(vertex.p.x, vertex.p.y)

             #feed down
            feed(z = final_depth)
            first = False

        if vertex.type == 1:
            arc_ccw(slot_ratio, vertex.p.x, vertex.p.y, i = vertex.c.x, j = vertex.c.y)
        elif vertex.type == -1:
            arc_cw(slot_ratio, vertex.p.x, vertex.p.y, i = vertex.c.x, j = vertex.c.y)
        else:
            feed(slot_ratio, vertex.p.x, vertex.p.y)
            
        prev_p = vertex.p
    return prev_p


def make_obround(p0, p1, radius):
    dir = p1 - p0
    d = dir.length()
    dir.normalize()
    right = area.Point(dir.y, -dir.x)
    obround = area.Area()
    c = area.Curve()
    vt0 = p0 + right * radius
    vt1 = p1 + right * radius
    vt2 = p1 - right * radius
    vt3 = p0 - right * radius
    c.append(area.Vertex(0, vt0, area.Point(0, 0)))
    c.append(area.Vertex(0, vt1, area.Point(0, 0)))
    c.append(area.Vertex(1, vt2, p1))
    c.append(area.Vertex(0, vt3, area.Point(0, 0)))
    c.append(area.Vertex(1, vt0, p0))
    obround.append(c)
    return obround

def feed_possible(p0, p1):
    if p0 == p1:
        return True
    obround = make_obround(p0, p1, tool_radius_for_pocket)
    a = area.Area(area_for_feed_possible)
    obround.Subtract(a)
    if obround.num_curves() > 0:
        return False
    return True

def cut_curvelist(prev_p, curve_list, rapid_safety_space, current_start_depth, depth, clearance_height):
    first = True
    for curve in curve_list:
        raise_cutter = True
        if prev_p != None:
            s = curve.FirstVertex().p
            if (s.x == prev_p.x and s.y == prev_p.y) or feed_possible(prev_p, s):
                raise_cutter = False

        prev_p = cut_curve(curve, raise_cutter, first, prev_p, rapid_safety_space, current_start_depth, depth, clearance_height)
        first = False
    return prev_p

def cut_curvelist_with_start(prev_p, curve_list, rapid_safety_space, current_start_depth, depth, clearance_height, start_point):
    start_x,start_y = start_point
    first = True
    for curve in curve_list:
        raise_cutter = True
        if first == True:
            direction = "on"
            radius = 0.0
            offset_extra = 0.0
            roll_radius = 0.0
            roll_on = 0.0
            roll_off = 0.0
            step_down = math.fabs(depth)
            extend_at_start = 0.0;
            extend_at_end = 0.0
            kurve_funcs.make_smaller(curve, start = area.Point(start_x,start_y))
            kurve_funcs.profile(curve, direction, radius, offset_extra, roll_radius, roll_on, roll_off, rapid_safety_space, clearance_height, current_start_depth, step_down, depth, extend_at_start, extend_at_end)
        else:
            s = curve.FirstVertex().p
            if (s.x == prev_p.x and s.y == prev_p.y) or feed_possible(prev_p, s):
                raise_cutter = False

        prev_p = cut_curve(curve, raise_cutter, first, prev_p, rapid_safety_space, current_start_depth, depth, clearance_height)
        first = False

    return prev_p

def pocket(a, tool_radius, extra_offset, stepover, depthparams, from_center, post_processor, zig_angle, start_point = None, cut_mode = 'conventional'):
    global tool_radius_for_pocket
    global area_for_feed_possible

    tool_radius_for_pocket = tool_radius

    area_for_feed_possible = area.Area(a)
    area_for_feed_possible.Offset(extra_offset - 0.01)

    a_offset = area.Area(a)
    current_offset = tool_radius + extra_offset
    a_offset.Offset(current_offset)
    a_offset.Reorder()

    if post_processor == 'zigzag':
        curve_list = zigzag(a_offset, stepover, False, zig_angle)
    elif post_processor == 'zigzag-unidirectional':
        curve_list = zigzag(a_offset, stepover, True, zig_angle)
    elif post_processor == 'offsets':
        curve_list = offsets(a_offset, stepover, from_center, cut_mode)
    elif post_processor == 'trochoidal':
        curve_list = trochoidal(a_offset, stepover, cut_mode)

    depths = depthparams.get_depths()

    current_start_depth = depthparams.start_depth

    if start_point == None:
        prev_p = None
        for depth in depths:
            prev_p = cut_curvelist(prev_p, curve_list, depthparams.rapid_safety_space, current_start_depth, depth, depthparams.clearance_height)
            current_start_depth = depth

    else:
        for depth in depths:
            cut_curvelist_with_start(curve_list, depthparams.rapid_safety_space, current_start_depth, depth, depthparams.clearance_height, start_point)
            rapid(z = depthparams.clearance_height)
            current_start_depth = depth
