import area


def offsets(a, stepover, from_center, cut_mode):
    arealist = list()
    recur(arealist, a, stepover, from_center)
    return get_curve_list(arealist, cut_mode == 'climb')

def recur(arealist, a1, stepover, from_center):
    # this makes arealist by recursively offsetting a1 inwards

    if a1.num_curves() == 0:
        return

    if from_center:
        arealist.insert(0, a1)
    else:
        arealist.append(a1)

    a_offset = area.Area(a1)
    a_offset.Offset(stepover)

    # split curves into new areas
    if area.holes_linked():
        for curve in a_offset.getCurves():
            a2 = area.Area()
            a2.append(curve)
            recur(arealist, a2, stepover, from_center)

    else:
        # split curves into new areas
        a_offset.Reorder()
        a2 = None

        for curve in a_offset.getCurves():
            if curve.IsClockwise():
                if a2 != None:
                    a2.append(curve)
            else:
                if a2 != None:
                    recur(arealist, a2, stepover, from_center)
                a2 = area.Area()
                a2.append(curve)

        if a2 != None:
            recur(arealist, a2, stepover, from_center)

def get_curve_list(arealist, reverse_curves = False):
    curve_list = list()
    for a in arealist:
        for curve in a.getCurves():
            if reverse_curves == True:
                curve.Reverse()
            curve_list.append(curve)
    return curve_list
