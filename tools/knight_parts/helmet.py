"""Reference-matched great helm: ruled visor, ocular recesses, crown and inlay.

Blender coordinates are Z-up and front -Y. This module only adds helmet parts;
assembly, materials, output, and rendering belong to the parent build script.
"""
from .common import *
from mathutils.bvhtree import BVHTree

_SHELL_BVH = None

# Heights are authored for an approximately two-metre armored figure. The visor
# is two separate surfaces so the medial keel retains a crisp normal break.
_FACE_ROWS = [
    # z at the medial keel, half-width, keel y, cheek setback, edge elevation
    (1.646, .087, -.180, .098, .041),
    (1.653, .091, -.188, .108, .036),
    (1.671, .099, -.194, .114, .026),
    (1.701, .108, -.199, .119, .014),
    (1.742, .116, -.201, .124, .004),
    (1.780, .123, -.199, .129, .000),
    (1.805, .127, -.192, .132, .000),
    (1.823, .128, -.185, .134, .000),
    (1.841, .128, -.179, .134, .000),
]
_SKULL = [
    (0, .025, 1.658, .083, .084),
    (0, .027, 1.669, .094, .090),
    (0, .029, 1.711, .111, .102),
    (0, .031, 1.771, .126, .112),
    (0, .028, 1.833, .130, .122),
    (0, .023, 1.878, .123, .122),
    (0, .022, 1.916, .111, .108),
    (0, .024, 1.946, .088, .088),
    (0, .027, 1.969, .053, .055),
    (0, .030, 1.980, .018, .021),
    (0, .030, 1.982, .008, .010),
]


def _ocular_lower(q):
    # Very narrow curved eye openings. The centre is hidden by the nasal inlay;
    # each opening rises to the outer hinge without a broad horizontal 'smile'.
    return 1.841 - .008 * sin(pi * q) + .012 * q ** 3


def _face_point(side, row, q):
    z, width, front, setback, lift = row
    if row is _FACE_ROWS[-1]:
        z = _ocular_lower(q)
    else:
        z += lift * q ** 1.15
    # A gently forged plane meets its mirrored half at a definite sharp keel.
    y = front + setback * (.83 * q + .17 * q * q)
    return side * width * q, y, z


def _face_y(x, z):
    """Evaluate the visor skin for raised inlay; no detached floating curves."""
    z = min(max(z, _FACE_ROWS[0][0]), _FACE_ROWS[-1][0])
    lower, upper = _FACE_ROWS[0], _FACE_ROWS[-1]
    for a, b in zip(_FACE_ROWS, _FACE_ROWS[1:]):
        if a[0] <= z <= b[0]:
            lower, upper = a, b
            break
    t = (z - lower[0]) / max(upper[0] - lower[0], 1e-6)
    width = lower[1] * (1 - t) + upper[1] * t
    front = lower[2] * (1 - t) + upper[2] * t
    setback = lower[3] * (1 - t) + upper[3] * t
    q = min(1.0, abs(x) / width)
    return front + setback * (.83 * q + .17 * q * q)


def _surface(name, rows, side, mat='steel', columns=20, thickness=.002):
    verts, faces, uv = [], [], []
    for j, row in enumerate(rows):
        for i in range(columns + 1):
            q = i / columns
            verts.append(_face_point(side, row, q))
            uv.append((.5 + side * q * .5, j / (len(rows) - 1)))
    n = columns + 1
    for j in range(len(rows) - 1):
        for i in range(columns):
            face = (j*n+i, j*n+i+1, (j+1)*n+i+1, (j+1)*n+i)
            faces.append(face if side > 0 else tuple(reversed(face)))
    return mesh_object(name, verts, faces, mat, uv, 0, thickness)


def _catmull(points, steps=7):
    """Sample flowing 2D inlay curves without adding oversized round trim."""
    result = []
    for index in range(len(points) - 1):
        a = Vector(points[max(0, index-1)])
        b = Vector(points[index])
        c = Vector(points[index+1])
        d = Vector(points[min(len(points)-1, index+2)])
        for i in range(steps):
            t = i / steps
            v = .5 * ((2*b) + (-a+c)*t + (2*a-5*b+4*c-d)*t*t + (-a+3*b-3*c+d)*t*t*t)
            result.append(tuple(v))
    result.append(points[-1])
    return result


def _trace(name, points, side=1, radius=.00165, mat='gold', offset=.0016):
    line = []
    for x, z in _catmull(points):
        x *= side
        line.append((x, _face_y(x, z) - offset, z))
    return tube(name, line, radius, mat, sides=6)


def _skull_section(z):
    for a, b in zip(_SKULL, _SKULL[1:]):
        if a[2] <= z <= b[2]:
            t = (z-a[2]) / (b[2]-a[2])
            return tuple(a[k]*(1-t)+b[k]*t for k in range(5))
    return _SKULL[-1]


def _prepare_surface_projection():
    """Use the evaluated/subdivided shells, not their undeformed control cage."""
    global _SHELL_BVH
    bpy.context.view_layer.update()
    graph = bpy.context.evaluated_depsgraph_get()
    vertices, faces = [], []
    prefixes = ('Helm | domed segmented skull', 'Helm | forged brow plate ',
                'Helm | ruled pointed visor ')
    for obj in model.objects:
        if obj.type != 'MESH' or not obj.name.startswith(prefixes):
            continue
        evaluated = obj.evaluated_get(graph)
        mesh = evaluated.to_mesh()
        base = len(vertices)
        vertices.extend(evaluated.matrix_world @ v.co for v in mesh.vertices)
        faces.extend(tuple(base+i for i in polygon.vertices) for polygon in mesh.polygons)
        evaluated.to_mesh_clear()
    _SHELL_BVH = BVHTree.FromPolygons(vertices, faces)


def _seat_crown(point):
    # Ray toward the helmet axis so an overlapping brow is included in the
    # exterior surface. Closest-point projection can pick the hidden skull.
    axis = Vector((0, .03, point[2]))
    outward = Vector((point[0], point[1]-.03, 0)).normalized()
    hit, normal, _, _ = _SHELL_BVH.ray_cast(axis+outward, -outward, 2.0)
    if hit is None:
        hit, normal, _, _ = _SHELL_BVH.find_nearest(Vector(point))
    if hit is None:
        return point
    return tuple(hit+normal*.0003)


def _front_surface(x, z):
    hit, _, _, _ = _SHELL_BVH.ray_cast(Vector((x,-1,z)), Vector((0,1,0)), 2.0)
    return hit.y if hit is not None else _face_y(x,z)


def _crown_rib(name, angle):
    points = []
    for i in range(38):
        z = 1.878 + .094*i/37
        _, cy, _, rx, ry = _skull_section(z)
        points.append(_seat_crown((rx*cos(angle), cy+ry*sin(angle), z)))
    tube(name, points, .00135, 'gold', sides=6)


def _brow(side):
    # Loft the forward brow back into the dome. The top contour is low enough
    # to merge with the skull, while the lower lip stands above the ocular.
    rows = [
        (1.855, .128, -.180, .134, 0),
        (1.863, .129, -.177, .135, 0),
        (1.878, .123, -.165, .130, 0),
        (1.900, .111, -.141, .121, 0),
        (1.923, .094, -.113, .105, 0),
        (1.940, .076, -.087, .085, 0),
    ]
    columns = 24
    verts, faces, uv = [], [], []
    for j, (z, width, front, setback, _) in enumerate(rows):
        for i in range(columns+1):
            q = i/columns
            zz = z
            if j == 0: zz = _ocular_lower(q) + .010
            if j == 1: zz = _ocular_lower(q) + .018
            verts.append((side*width*q, front+setback*(.83*q+.17*q*q), zz))
            uv.append((.5+side*.5*q, .45+.5*j/(len(rows)-1)))
    n=columns+1
    for j in range(len(rows)-1):
        for i in range(columns):
            f=(j*n+i,j*n+i+1,(j+1)*n+i+1,(j+1)*n+i)
            faces.append(f if side>0 else tuple(reversed(f)))
    mesh_object('Helm | forged brow plate '+str(side), verts, faces, 'steel', uv, 0, .002)
    # Ocular shadow is physically behind the upper/lower lips.
    black, faces = [], []
    for row in range(2):
        for i in range(25):
            q = .04+.94*i/24
            x = side*.128*q
            black.append((x, -.179+.134*(.83*q+.17*q*q)+.0018,
                          _ocular_lower(q)+.001+row*.009))
    for i in range(24):
        faces.append((i,i+1,26+i,25+i))
    mesh_object('Helm | recessed ocular '+str(side),black,faces,'shadow',sub=0)
    upper, lower = [], []
    for i in range(27):
        q = i/26
        x = side*.128*q
        y = -.179+.134*(.83*q+.17*q*q)-.001
        upper.append((x,y-.001,_ocular_lower(q)+.011))
        lower.append((x,y,_ocular_lower(q)-.001))
    tube('Helm | gilded brow lip '+str(side),upper,.00225,'gold',sides=6)
    tube('Helm | fine ocular lower inlay '+str(side),lower,.00145,'gold',sides=6)
    # A secondary band follows the brow's sloping forehead, not the eye gap.
    band=[]
    for i in range(25):
        q=i/24
        band.append((side*.121*q,-.163+.128*(.83*q+.17*q*q)-.002,
                     1.882-.004*sin(pi*q)))
    tube('Helm | brow crown band '+str(side),band,.0016,'gold',sides=6)


def _crest():
    # Thin sagittal metal blade; no plume, oversized horn, or motorcycle ridge.
    outline=[(-.103,1.928),(-.106,1.968),(-.095,2.034),(-.077,2.073),
             (-.063,2.081),(-.048,2.063),(-.023,2.018),(.026,1.975),
             (.086,1.940),(.111,1.911),(.074,1.924),(.006,1.953),(-.052,1.945)]
    # Compress only the raised crest; its fitted base stays against the dome.
    outline=[(y, 1.96+(z-1.96)*.70 if z>1.96 else z) for y,z in outline]
    verts=[]
    for side in (-1,1):
        for y,z in outline:
            width=.0045 if z>2.02 else .007
            verts.append((side*width,y,z))
    n=len(outline)
    faces=[tuple(reversed(range(n))),tuple(n+i for i in range(n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    obj=mesh_object('Helm | narrow sagittal crown blade',verts,faces,'steel',sub=0,bevel=.0007)
    for polygon in obj.data.polygons: polygon.use_smooth=False
    for side in (-1,1):
        points=[]
        for y,z in outline[:10]:
            points.append((side*(.0045 if z>2.02 else .007),y,z))
        tube('Helm | crest engraved edge '+str(side),points,.001,'gold',sides=5)


def _forehead_emblem():
    # A shallow flush lancet inlay follows the actual outer brow and visor.
    # It bridges the ocular instead of dropping backward into its dark recess.
    def depth(x,z):
        if 1.837 < z < 1.862:
            t=(z-1.837)/.025
            return _front_surface(x,1.837)*(1-t)+_front_surface(x,1.862)*t-.0005
        return _front_surface(x,z)-.0005
    outline=[(0,1.918),(.019,1.884),(.009,1.855),(0,1.823),(-.009,1.855),(-.019,1.884)]
    plate('Helm | lancet forehead escutcheon',outline,depth,0,'gold',sub=0,thickness=.0005)
    inner=[(0,1.910),(.012,1.884),(.0055,1.856),(0,1.838),(-.0055,1.856),(-.012,1.884)]
    plate('Helm | inset steel diamond',inner,lambda x,z:depth(x,z)-.0004,0,'steel',sub=0,thickness=.0004)
    for side in (-1,1):
        points=[]
        for x,z in _catmull([(0,1.905),(.0075,1.884),(.003,1.861),(0,1.847)],steps=6):
            points.append((side*x,depth(side*x,z)-.0007,z))
        tube('Helm | diamond nested engraving '+str(side),points,.0008,'gold',sides=5)


def build():
    loft('Helm | domed segmented skull',_SKULL,'steel',n=40,sub=1)
    for side in (-1,1):
        _surface('Helm | ruled pointed visor '+str(side),_FACE_ROWS,side)
        _brow(side)
        # The gold side boundary descends from the temple to the V-shaped jaw.
        _trace('Helm | long cheek scroll '+str(side),
               [(.120,1.840),(.116,1.817),(.099,1.797),(.083,1.770),(.080,1.725),(.081,1.692)],side,.00185)
        _trace('Helm | inner cheek scroll '+str(side),
               [(.106,1.827),(.095,1.810),(.073,1.798),(.060,1.780)],side,.0016)
        _trace('Helm | short cheek ray '+str(side),
               [(.079,1.819),(.071,1.800),(.066,1.784)],side,.00145)
        # Slender dark vertical vents between gilded ray borders.
        for index,(x,top,bottom) in enumerate([(.022,1.823,1.770),(.039,1.816,1.780),(.056,1.809,1.790)]):
            curve=[(x,top),(x-.003,(top+bottom)/2),(x-.002,bottom)]
            _trace('Helm | recessed breathing slot %s %s'%(side,index),curve,side,.0018,'shadow',.0008)
            _trace('Helm | vent gold edge %s %s'%(side,index),
                   [(u+.0027,z) for u,z in curve],side,.0012,'gold',.0015)
        # Thin center seams accentuate the vertical blade, terminating in a
        # pointed chin instead of a curved mouth-like horizontal strip.
        _trace('Helm | medial visor seam '+str(side),
               [(.004,1.833),(.0045,1.788),(.004,1.719),(.0015,1.650)],side,.0015)
        jaw=[]
        for i in range(25):
            q=i/24
            x,y,z=_face_point(side,_FACE_ROWS[0],q)
            jaw.append((x,y-.0018,z-.0005))
        tube('Helm | pointed jaw rolled rim '+str(side),jaw,.0021,'gold',sides=6)
        # Small side pivots read as hardware rather than decorative buttons.
        bolt('Helm | visor hinge '+str(side),(side*.126,-.018,1.834),.006,'gold')
    _prepare_surface_projection()
    for index,angle in enumerate([-pi/2,-pi/6,pi/6,pi/2,5*pi/6,7*pi/6]):
        _crown_rib('Helm | crown panel rib '+str(index),angle)
    # Fine circumferential crown seam is most visible over the rear panels.
    points=[]
    for i in range(96):
        a=2*pi*i/96
        _,cy,z,rx,ry=_skull_section(1.904)
        points.append(_seat_crown((rx*cos(a),cy+ry*sin(a),z)))
    tube('Helm | encircling crown seam',points,.0012,'gold',closed=True,sides=6)
    _forehead_emblem()
    _crest()
