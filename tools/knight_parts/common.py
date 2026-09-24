"""Shared Blender surface modeling helpers for the reference-inspired knight."""
import bpy, math, bmesh
from mathutils import Vector
from pathlib import Path
from math import sin, cos, pi
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'assets'/'knight'
TILES={'steel':(0,1),'gold':(1,1),'cloth':(0,0),'leather':(1,0),'mail':(0,1),'shadow':(1,0)}
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for datablocks in (bpy.data.meshes,bpy.data.curves,bpy.data.materials,bpy.data.cameras,bpy.data.lights):
    for block in list(datablocks):
        if block.users==0: datablocks.remove(block)
model=bpy.data.collections.new('GILDED WARDEN | crafted armor and garments')
bpy.context.scene.collection.children.link(model)
atlas=bpy.data.images.load(str(OUT/'knight_atlas.png'),check_existing=True); atlas.pack()
mail_image=bpy.data.images.load(str(OUT/'knight_mail.png'),check_existing=True); mail_image.pack()
# Diffuse tint, highlight color, exponent and physical preview parameters.
MATERIAL_SPECS={
    'steel':((1,1,1),(.70,.69,.66),72,.83,.34),
    'gold':((1,1,1),(.72,.59,.35),65,.75,.39),
    'cloth':((1,1,1),(.025,.02,.015),8,0,.88),
    'leather':((.75,.72,.68),(.09,.075,.055),20,0,.68),
    'mail':((.8,.82,.84),(.30,.30,.30),36,.70,.47),
    'shadow':((.018,.018,.018),(0,0,0),4,0,.98),
}
materials={}
for name,(tint,ks,ns,metal,rough) in MATERIAL_SPECS.items():
    mat=bpy.data.materials.new(name);mat.use_nodes=True
    nodes=mat.node_tree.nodes;links=mat.node_tree.links
    bsdf=nodes.get('Principled BSDF')
    tex=nodes.new('ShaderNodeTexImage');tex.image=mail_image if name=='mail' else atlas
    tex.interpolation='Linear'
    multiply=nodes.new('ShaderNodeMixRGB');multiply.blend_type='MULTIPLY'
    multiply.inputs[0].default_value=1;multiply.inputs[2].default_value=(*tint,1)
    links.new(tex.outputs['Color'],multiply.inputs[1]);links.new(multiply.outputs[0],bsdf.inputs['Base Color'])
    bsdf.inputs['Metallic'].default_value=metal;bsdf.inputs['Roughness'].default_value=rough
    # Very shallow physical relief reads at portrait scale without inflating surfaces.
    bump=nodes.new('ShaderNodeBump');bump.inputs['Strength'].default_value=.06 if name in ('steel','gold') else .13 if name!='mail' else .32
    bump.inputs['Distance'].default_value=.00025 if name in ('steel','gold') else .00065 if name!='mail' else .0015
    links.new(tex.outputs['Color'],bump.inputs['Height']);links.new(bump.outputs['Normal'],bsdf.inputs['Normal'])
    materials[name]=mat

def uv_tile(name,u,v):
    if name=='mail': return .02+1.92*u,.02+1.92*v
    c,r=TILES[name]
    return c*.5+.055+.39*u,r*.5+.055+.39*v

def mesh_object(name,verts,faces,mat='steel',uv=None,sub=0,solid=0,bevel=0):
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces); mesh.update()
    bm=bmesh.new(); bm.from_mesh(mesh); bmesh.ops.recalc_face_normals(bm,faces=bm.faces); bm.to_mesh(mesh); bm.free()
    obj=bpy.data.objects.new(name,mesh); model.objects.link(obj); obj.data.materials.append(materials[mat])
    layer=mesh.uv_layers.new(name='Atlas UV')
    if uv is None:
        lo=[min(v[k] for v in verts) for k in range(3)]; hi=[max(v[k] for v in verts) for k in range(3)]
        axes=sorted(range(3),key=lambda k:hi[k]-lo[k],reverse=True)[:2]
        uv=[tuple((v[k]-lo[k])/max(hi[k]-lo[k],1e-6) for k in axes) for v in verts]
    for poly in mesh.polygons:
        poly.use_smooth=True
        for li in poly.loop_indices:
            vi=mesh.loops[li].vertex_index; layer.data[li].uv=uv_tile(mat,*uv[vi])
    if sub:
        mod=obj.modifiers.new('Silhouette subdivision','SUBSURF'); mod.levels=sub; mod.render_levels=sub
    if solid:
        mod=obj.modifiers.new('Forged / woven thickness','SOLIDIFY'); mod.thickness=solid; mod.offset=0
    if bevel:
        mod=obj.modifiers.new('Rolled plate edges','BEVEL'); mod.width=bevel; mod.segments=2
    obj['construction']='Authored contour surface'; obj['finish']=mat
    return obj

def loft(name,levels,mat='steel',n=20,sub=1,ridge=0):
    # Each ring: center x,y,z; width x; depth y. A narrow anterior ridge gives
    # shaped breastplates and greaves the silhouette of hammered armor.
    verts=[]; uv=[]
    for j,(x,y,z,rx,ry) in enumerate(levels):
        for i in range(n):
            a=2*pi*i/n; xx=rx*cos(a); yy=ry*sin(a)
            yy-=ridge*max(0,-sin(a))**14
            verts.append((x+xx,y+yy,z)); uv.append((i/n,j/(len(levels)-1)))
    faces=[]
    for j in range(len(levels)-1):
        for i in range(n): faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
    faces.extend([tuple(reversed(range(n))),tuple((len(levels)-1)*n+i for i in range(n))])
    return mesh_object(name,verts,faces,mat,uv,sub)

def bone(name,a,b,profiles,mat='steel',n=12,sub=1):
    a,b=Vector(a),Vector(b); axis=(b-a).normalized()
    u=axis.cross(Vector((0,-1,0))).normalized(); v=axis.cross(u).normalized()
    verts=[]; uv=[]
    for j,(t,ru,rv) in enumerate(profiles):
        c=a.lerp(b,t)
        for i in range(n):
            p=c+u*(ru*cos(2*pi*i/n))+v*(rv*sin(2*pi*i/n))
            verts.append(tuple(p)); uv.append((i/n,t))
    faces=[]
    for j in range(len(profiles)-1):
        for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
    faces += [tuple(reversed(range(n))),tuple((len(profiles)-1)*n+i for i in range(n))]
    return mesh_object(name,verts,faces,mat,uv,sub)

def tube(name,points,radius=.003,mat='steel',closed=False,sides=6):
    # Small rolled rims and seams retain a clean contour at the OBJ budget.
    verts=[]; uv=[]
    for j,p in enumerate(points):
        p=Vector(p)
        before=Vector(points[(j-1)%len(points)] if closed or j else points[0])
        after=Vector(points[(j+1)%len(points)] if closed or j+1<len(points) else points[-1])
        tangent=(after-before).normalized(); ref=Vector((0,1,0))
        if abs(tangent.dot(ref))>.95:ref=Vector((1,0,0))
        u=tangent.cross(ref).normalized(); v=tangent.cross(u)
        for i in range(sides):
            q=p+radius*(u*cos(2*pi*i/sides)+v*sin(2*pi*i/sides)); verts.append(tuple(q));uv.append((i/sides,j/max(1,len(points)-1)))
    faces=[]
    for j in range(len(points) if closed else len(points)-1):
        for i in range(sides):faces.append((j*sides+i,j*sides+(i+1)%sides,((j+1)%len(points))*sides+(i+1)%sides,((j+1)%len(points))*sides+i))
    return mesh_object(name,verts,faces,mat,uv)

def ring_trim(name,level,radius=.003,mat='steel',ridge=0):
    x,y,z,rx,ry=level
    points=[]
    for i in range(32):
        a=2*pi*i/32; points.append((x+rx*cos(a),y+ry*sin(a)-ridge*max(0,-sin(a))**14,z))
    return tube(name,points,radius,mat,True)

def plate(name,outline,depth,bulge=.018,mat='steel',sub=1,thickness=.004):
    # outline supplied as x/z coordinates; use concentric sculpted contours.
    cx=sum(p[0] for p in outline)/len(outline); cz=sum(p[1] for p in outline)/len(outline)
    n=len(outline); verts=[]; uv=[]
    xmin=min(p[0] for p in outline); xmax=max(p[0] for p in outline); zmin=min(p[1] for p in outline); zmax=max(p[1] for p in outline)
    for s in [.08,.52,.92,1.0]:
        for x,z in outline:
            xx=cx+(x-cx)*s; zz=cz+(z-cz)*s
            yy=depth(xx,zz) if callable(depth) else depth
            verts.append((xx,yy-bulge*(1-s*s),zz));uv.append(((xx-xmin)/max(xmax-xmin,.001),(zz-zmin)/max(zmax-zmin,.001)))
    faces=[tuple(reversed(range(n)))]
    for j in range(3):
        for i in range(n): faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
    return mesh_object(name,verts,faces,mat,uv,sub,thickness)

def bolt(name,p,r=.006,mat='gold'):
    x,y,z=p
    return plate(name,[(x+r*cos(i*2*pi/8),z+r*sin(i*2*pi/8)) for i in range(8)],y,.002,mat,0,.002)
