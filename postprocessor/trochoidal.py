from collections import namedtuple
import operator
import area
import math
from area import Curve

EPSILON = 1.0e-03
RESOLUTION = 0.05


def trochoidal(a, stepover, cut_mode):
    curve_list = []
    ma_list = a.MedialAxis()
    prev_ma = ma_list[0]
    first = True
    for i in xrange(1, len(ma_list)):
        ma = ma_list[i]
        curve = area.Curve()

        curve.append(area.Vertex(0, ma.center, area.Point(0,0)))
        curve_list.append(curve)
        continue

        vector = ma.center - prev_ma.center
        vector.normalize()
        perimeter = prev_ma.center + vector * prev_ma.radius
        perimeter2 = ma.center + vector * ma.radius
        dist = perimeter2.dist(perimeter)

        print "Previous Center:", ma.center_prev, " Center:", ma.center, " Radius:", ma.radius, " Point:", ma.p, " Nearest Neighbor:", ma.nn

        print "Distance:", dist
        if dist < (stepover - RESOLUTION):
            continue

#         if dist > stepover:
#             ma.center = ma.center - vector * (stepover / 2)

        print "*** CUT ***"
        if first:    # maximum inscribed circle
            first = False
            mic_center = ma_list[0].center
            mic_radius = ma_list[0].radius
            outdir = ma.nn - mic_center      # direction vector, ending where the next arc starts
            spiral_archimedes(curve, mic_center, mic_radius, outdir, math.pi / 16, stepover)

            # Round out the spiral with a circle
            outdir.normalize()
            outdir *= mic_radius
            curve.append(area.Vertex(-1, mic_center - outdir, mic_center))
            curve.append(area.Vertex(-1, mic_center + outdir, mic_center))
            loc = mic_center + outdir

        if loc.dist(ma.nn) < loc.dist(ma.p):
            v1 = area.Vertex(0, ma.nn, area.Point(0,0))
            v2 = area.Vertex(-1, ma.p, ma.center)
        else:
            v1 = area.Vertex(0, ma.p, area.Point(0,0))
            v2 = area.Vertex(1, ma.nn, ma.center)

        if loc.dist(v1.p) > EPSILON:
            curve.append(v1)

        curve.append(v2)
        curve_list.append(curve)
        loc = v2.p

        prev_ma = ma
    return curve_list


def spiral_archimedes(curve, center, r1, outdir, dtheta, step):
    out_theta = math.atan2(outdir.y, outdir.x)
    b = step / (2 * math.pi)

    # figure out the end-angle
    theta_max = r1 / step * (2 * math.pi)
    residual = theta_max % (2 * math.pi)
    theta = dtheta

    # re-clock the spiral so that the endpoint comes out where we want
    clock = math.pi - residual - out_theta

    trg_prev = center
    while theta <= theta_max: 
        r = b * theta

        # interpolate into a circular arc
        mid_trg = triangulate_circle(center, r, theta + clock - dtheta/2)
        trg = triangulate_circle(center, r, theta + clock)
        circle = area.Circle(trg_prev, mid_trg, trg)
        curve.append(area.Vertex(-1, trg, circle.center))
        theta += dtheta
        trg_prev = trg

    # one more time to finish any fraction of an angle
    if theta > theta_max:
        r = b * theta_max
        mid_trg = triangulate_circle(center, r, theta + clock - dtheta/2)
        trg = triangulate_circle(center, r, theta_max + clock)
        circle = area.Circle(trg_prev, mid_trg, trg)
        curve.append(area.Vertex(-1, trg, circle.center))

def triangulate_circle(center, r, theta):
    return center + r * area.Point( -math.cos(theta), math.sin(theta) )
