from collections import namedtuple
import operator
import area
import math
from area import Curve

EPSILON = 1.0e-05
ITERATION_LIMIT = 30

class ma_point(object):
    def __init__(self, p, normal, span1, span2):
        self.p = p
        self.normal = normal
        self.span1 = span1
        self.span2 = span2
        self.center = None
        self.radius = None


def trochoidal(a, stepover, cut_mode):
    curve_list = []
    ma_list = build_ma_list(a)
    first = True
    for ma in ma_list:
        curve = area.Curve()

        if first:    # maximum inscribed circle
            first = False
            outdir = ma.p - ma.center      # direction vector
            spiral_archimedes(curve, ma.center, ma.radius, outdir, math.pi / 4, stepover)

            # Round out the spiral with a circle
            outdir.normalize()
            outdir *= ma.radius
            curve.append(area.Vertex(-1, ma.center - outdir, ma.center))
            curve.append(area.Vertex(-1, ma.center + outdir, ma.center))

#        else:
#            linear_march(curve, ma.center - prevcenter, prevcenter, stepover, min(prevradius, ma.radius))

        vect = ma.p - ma.center
        vect.normalize()
        start = ma.center + vect * ma.radius
        vect = ma.p - start
        if vect.length() > EPSILON:
            taper_to_point(curve, vect, start, stepover, ma)

        curve_list.append(curve)
        prevcenter = ma.center
        prevradius = ma.radius
    return curve_list


def march_to_point(vect, p, step):
    point_list = []
    distance = vect.length()
    vect.normalize()
    while distance > EPSILON:
        step = min(step, distance)
        distance -= step
        p += vect * step
        point_list.append(p)
    return point_list


def taper_to_point(curve, vect, p, step, ma):
    left = ~vect
    left.normalize()
    distance = vect.length()
    vect.normalize()

    spanvect = ma.span1.v.p - ma.span1.p
    spanvect.normalize()
    opp = math.tan(math.acos(spanvect * vect))
    center = ma.center
    while distance > EPSILON:
        len = opp * distance
        curve.append(area.Vertex(0, p - left * len, area.Point(0,0)))
        curve.append(area.Vertex(1, p + left * len, center))
        step = min(step, distance)
        distance -= step
        p += vect * step
        center += vect * step
    curve.append(area.Vertex(0, p, area.Point(0,0)))


def linear_march(curve, vect, start, step, radius):
    left = ~vect
    left.normalize()
    point_list = march_to_point(vect, start, step)
    for p in point_list:
        curve.append(area.Vertex(0, p - left * radius, area.Point(0,0)))
        curve.append(area.Vertex(1, p + left * radius, p))


def spiral_archimedes(curve, center, r1, outdir, dtheta, step):
    
    out_theta = math.atan2(outdir.y, outdir.x)

    b = step / (2 * math.pi)

    # figure out the end-angle
    theta_max = r1 / step * (2 * math.pi)
    residual = theta_max % (2 * math.pi)
    theta = 0
    clock = math.pi - residual - out_theta
    while theta <= theta_max: 
        r = b * theta
        trg = center + r * area.Point( -math.cos(theta + clock), math.sin(theta + clock) )
        curve.append(area.Vertex(-1, trg, center))
        theta += dtheta
    if theta > theta_max:
        r = b * theta_max
        trg = center + r * area.Point( -math.cos(theta_max + clock), math.sin(theta_max + clock) )
        curve.append(area.Vertex(-1, trg, center))


def build_ma_list(a):
    ma_list = []

    for curve in a.getCurves():
        curve.UnFitArcs()

        span_list = curve.GetSpans()
        for index, span in enumerate(span_list):
            v = span.GetVector(0.0)
            prev = index - 1 if index > 0 else len(span_list) - 1
            vprev = span_list[prev].GetVector(0.0)
            v.normalize()
            vprev.normalize()
            bisect = (v + vprev)
            bisect.Rotate(math.pi / 2)
            bisect.normalize()
            ma_list.append(ma_point(span.p, -bisect, span_list[prev], span))

    pointlist = [o.p for o in ma_list]
    tree = kdtree(pointlist)
    for ma in ma_list:
        ma.center = medial_axis(ma.p, ma.normal, tree, 1000)
        ma.radius = inscribed_circle(ma.center, span_list)

    if len(ma_list) <= 0:
        raise "Unable to inscribe medial axis circles in shape."

    # Get index of ma with max radius in ma_list
    mic = max(enumerate(ma_list), key=lambda e: e[1].radius)[0]

    # Reorder list to make it first
    mic_ma_list = ma_list[mic:] + ma_list[:mic]
    return mic_ma_list


def inscribed_circle(p, span_list):
    # Simple brute force for now, but i'm sure there is a better way
    first = True
    radius = 0
    for span in span_list:
        d = segment_distance(p, span.p, span.v.p)
        if first:
            radius = d
            first = False
        else:
            radius = min(radius, d)
    return radius


# Use the shrinking ball algorithm
# https://vimeo.com/84859998
def medial_axis(p, norm, tree, initial_radius):
    j = 0
    rprev = 0
    center = p - norm * initial_radius
    while True:
        nnlist = k_nearest_neighbours(tree, 2, center)
        nn = nnlist[0]
        if nn == p:
            if rprev == initial_radius:     # infinite radius
                center = None
                r = initial_radius
                break
            else:
                nn = nnlist[1]              # use 2nd closest point

        radius = compute_radius(p, norm, nn)

        # if radius < 0 closest point was on the wrong side of plane with normal, start over
        if radius < 0:
            radius = initial_radius
        elif radius > initial_radius:       # infinite loop
            center = None
            radius = initial_radius
            break

        # compute next ball center
        cnext = p - norm * radius

        if abs(rprev - radius) < EPSILON or j > ITERATION_LIMIT:
            break

        rprev = radius
        center = cnext
        j += 1

    return center


def compute_radius(p, norm, q):
    pq = p - q
    d = pq.length()
    cos_theta = (norm * pq) / d
    return d / (2 * cos_theta)


# kd-tree spatial index and nearest neighbour search
# http://en.wikipedia.org/wiki/Kd-tree
kdnode = namedtuple('kdnode', ['p', 'left', 'right'])

def kdtree(point_list, _depth = 0):
    if len(point_list) == 0:
        return None

    # Select axis based on depth so that axis cycles through all valid values
    axis = 'x' if _depth % 2 == 0 else 'y'

    sorted = list(point_list)
    # Sort point list and choose median as pivot element
    sorted.sort(key = operator.attrgetter(axis))
    median = len(sorted) // 2 # choose median

    # Create node and construct subtrees
    return kdnode(
        p = sorted[median],
        left = kdtree(sorted[:median], _depth + 1),
        right = kdtree(sorted[median + 1:], _depth + 1)
    )

def k_nearest_neighbours(node, k, p, _depth = 0, _bestlist = None):
    if _bestlist is None:
        _bestlist = []

    if node is None:
        return _bestlist

    added = False
    for index, best in enumerate(_bestlist):
        if distance2(node.p, p) < distance2(best, p):
            _bestlist.insert(index, node.p)
            _bestlist = _bestlist[:k]
            added = True
            break

    if not added and len(_bestlist) < k:
        _bestlist.append(node.p)
    
    axis = 'x' if _depth % 2 == 0 else 'y'

    if getattr(p, axis) < getattr(node.p, axis):
        near, far = (node.left, node.right)
    else:
        near, far = (node.right, node.left)

    _bestlist = k_nearest_neighbours(near, k, p, _depth + 1, _bestlist)

    # check the far side as well
    if far is not None:
        for best in _bestlist:
             if axis_distance2(node.p, p, axis) < distance2(best, p):
                 _bestlist = k_nearest_neighbours(far, k, p, _depth + 1, _bestlist)
                 break

    return _bestlist

# Return minimum distance between point p and line segment vw 
def segment_distance(p, v, w):
    l2 = distance2(v, w)
    if math.fabs(l2) < EPSILON:       # v == w case
        return math.sqrt(distance2(p, v))
    pv = p - v
    wv = w - v
    t = max(0, min(1, (pv * wv) / l2))
    projection = v + t * (w - v)
    return math.sqrt(distance2(p, projection))

# Distance squared
def distance2(a, b):
    return (a.x - b.x)**2 + (a.y - b.y)**2

# single axis distance (also squared)
def axis_distance2(a, b, axis):
    d = getattr(a, axis) - getattr(b, axis)
    return d * d
