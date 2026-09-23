#!/usr/bin/env python3
"""Build the Crimson Sentinel in Blender 5.x.
Run: blender --background --python tools/create_knight_blender.py
Editable contour meshes + subdivision, lofts and cloth surfaces; no cube assembly.
Blender coordinates: Z up, front -Y. OBJ coordinates: Y up, front -Z.
"""
import bpy, math, bmesh
from mathutils import Vector
from pathlib import Path
from math import sin, cos, pi
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets'/'knight'
TILES={'steel':(0,1),'gold':(1,1),'cloth':(0,0),'leather':(1,0),'mail':(1,0),'shadow':(1,0)}
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
for datablocks in (bpy.data.meshes,bpy.data.curves,bpy.data.materials,bpy.data.cameras,bpy.data.lights):
    for block in list(datablocks):
        if block.users==0: datablocks.remove(block)
model=bpy.data.collections.new('CRIMSON SENTINEL | authored geometry'); bpy.context.scene.collection.children.link(model)
materials={}
atlas=bpy.data.images.load(str(OUT/'knight_atlas.png'),check_existing=True)
atlas.pack()
for name,(metal,rough) in {'steel':(.82,.32),'gold':(.78,.3),'cloth':(0,.9),'leather':(0,.65),'mail':(.12,.83),'shadow':(0,.95)}.items():
    mat=bpy.data.materials.new(name); mat.use_nodes=True
    nodes=mat.node_tree.nodes; bsdf=nodes.get('Principled BSDF')
    tex=nodes.new('ShaderNodeTexImage'); tex.image=atlas; tex.interpolation='Linear'
    if name in ('mail','shadow'):
        tint=(.15,.18,.21,1) if name=='mail' else (.025,.03,.035,1)
        multiply=nodes.new('ShaderNodeMixRGB');multiply.blend_type='MULTIPLY';multiply.inputs[0].default_value=1;multiply.inputs[2].default_value=tint
        mat.node_tree.links.new(tex.outputs['Color'],multiply.inputs[1]);mat.node_tree.links.new(multiply.outputs[0],bsdf.inputs['Base Color'])
    else:mat.node_tree.links.new(tex.outputs['Color'],bsdf.inputs['Base Color'])
    bsdf.inputs['Metallic'].default_value=metal; bsdf.inputs['Roughness'].default_value=rough
    materials[name]=mat

def uv_tile(name,u,v):
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

# A connected human silhouette beneath fitted armor: seven-and-a-quarter heads.
loft('Arming doublet | tailored torso',[(0,.015,1.025,.155,.102),(0,.015,1.04,.17,.108),(0,.015,1.28,.182,.13),(0,.015,1.46,.224,.125),(0,.015,1.52,.20,.11)],'mail',20,1)
loft('Mail chausses and arming skirt',[(0,.015,.84,.182,.11),(0,.015,.90,.183,.115),(0,.015,1.08,.16,.11)],'mail',20,1)
# Shaped breastplate: waist pinch, globose lower chest and a subtle medial keel.
cuirass=[(0,.012,1.105,.155,.112),(0,.012,1.12,.161,.12),(0,.012,1.23,.187,.146),(0,.01,1.35,.214,.15),(0,.014,1.445,.224,.145),(0,.018,1.495,.208,.126),(0,.02,1.55,.143,.099),(0,.02,1.558,.137,.095)]
loft('Cuirass | raised medial keel and anatomical waist',cuirass,'steel',28,1,.012)
ring_trim('Rolled cuirass waist',cuirass[0],.0035,ridge=.012)
# Thin decorative engraving follows the curved pectoral transition.
for side in [-1,1]:
    pts=[]
    for i in range(12):
        t=i/11; x=side*(.028+.139*t); z=1.466-.052*t; y=-.132+.027*t*t
        pts.append((x,y,z))
    tube('Pectoral inlaid accent '+str(side),pts,.0018,'gold')
# Overlapping fauld lames wrap the waist rather than hanging as boxes.
for j in range(3):
    z=1.115-j*.044; rx=.162+j*.01
    levels=[(0,.015,z-.049,rx+.013,.123+j*.003),(0,.015,z-.039,rx+.015,.126+j*.003),(0,.015,z+.012,rx,.116+j*.003)]
    loft('Articulated fauld lame %02d'%j,levels,'steel',24,1)
    ring_trim('Fauld rolled seam %02d'%j,levels[0],.0022)
# Neck and fitted gorget.
loft('Mail collar',[(0,.015,1.526,.09,.085),(0,.015,1.60,.078,.073),(0,.015,1.677,.076,.073)],'mail',20,1)
loft('Gorget | articulated collar',[(0,.017,1.527,.13,.10),(0,.017,1.541,.13,.10),(0,.017,1.593,.095,.079),(0,.017,1.599,.093,.078)],'steel',24,1)
ring_trim('Gorget edge',(0,.017,1.599,.093,.078),.0023,'gold')

# Natural, slightly asymmetric weight-bearing stance.
legs=[('R',(-.102,.014,.981),(-.121,-.023,.575),(-.137,-.045,.135)),('L',(.100,.021,.981),(.137,.055,.575),(.153,.089,.135))]
for label,hip,knee,ankle in legs:
    h,k,a=Vector(hip),Vector(knee),Vector(ankle)
    bone(label+' leg | quilted articulation',hip,ankle,[(0,.075,.073),(.45,.058,.060),(1,.044,.044)],'mail',12,1)
    bone(label+' cuisse | tapered thigh',h.lerp(k,.06),h.lerp(k,.88),[(0,.088,.09),(.06,.092,.094),(.40,.082,.092),(.93,.064,.067),(1,.061,.065)],'steel',16,1)
    bone(label+' greave | anatomical calf',k.lerp(a,.17),a,[(0,.065,.068),(.08,.069,.071),(.30,.072,.078),(.59,.06,.065),(.94,.043,.047),(1,.044,.045)],'steel',16,1)
    # Poleyn's curved asymmetric wing follows the knee joint.
    x,y,z=k; s=-1 if label=='R' else 1
    outline=[(x-.055,z+.071),(x+.055,z+.071),(x+.081+(s*.006),z+.025),(x+.061,z-.048),(x,z-.077),(x-.057,z-.05),(x-.081+(s*.006),z+.003)]
    plate(label+' poleyn | floating kneecap',outline,y-.075,.025,'steel',1,.004)
    bolt(label+' knee articulation rivet',(x+s*.064,y-.081,z+.003),.005)
    tube(label+' greave central ridge',[(a.x,a.y-.049,.17),(a.x*.5+k.x*.5,(a.y+k.y)/2-.070,.33),(k.x,k.y-.074,.49)],.0017,'gold')
    # A long, tapered sabaton profile with a raised instep and pointed toe.
    x,y,z=a
    bone(label+' ankle | continuous mail articulation',(x,y,.09),(x,y,.24),[(0,.041,.043),(.15,.043,.045),(.9,.043,.047),(1,.042,.046)],'mail',12,1)
    loft(label+' articulated ankle cuff',[(x,y,.095,.045,.047),(x,y,.10,.048,.050),(x,y,.165,.048,.050),(x,y,.175,.045,.048)],'steel',16,1)
    levels=[(x,y,.065,.055,.077),(x,y-.024,.085,.069,.124),(x,y-.064,.101,.067,.162),(x,y-.093,.121,.051,.161),(x,y-.102,.145,.041,.123),(x,y-.050,.179,.044,.067)]
    # Cross-sections run along the foot, with longitudinal heights instead of a cube.
    verts=[];uv=[];n=12
    sections=[(y+.073,.045,.065,.055),(y+.049,.056,.097,.064),(y-.035,.068,.125,.068),(y-.11,.061,.083,.060),(y-.195,.034,.052,.038),(y-.237,.006,.038,.009)]
    for j,(yy,rx,top,bottomrad) in enumerate(sections):
        for i in range(n):
            aa=2*pi*i/n; zz=.05+(top-.05)*max(0,sin(aa)) + .019*min(0,sin(aa))
            verts.append((x+rx*cos(aa),yy,zz));uv.append((i/n,j/(len(sections)-1)))
    faces=[]
    for j in range(len(sections)-1):
        for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
    faces += [tuple(range(n)),tuple(reversed([(len(sections)-1)*n+i for i in range(n)]))]
    mesh_object(label+' sabaton | tapered articulated foot',verts,faces,'steel',uv,1)
    for j in range(4):
        yy=y-.037-j*.033; width=.063-j*.006; height=.123-j*.014
        pts=[(x+width*cos(pi*i/12),yy,.051+(height-.051)*sin(pi*i/12)) for i in range(13)]
        tube(label+' sabaton transverse lame '+str(j),pts,.0028)
# Hip tassets: curved shells follow each thigh and overlap the fauld naturally.
for s in [-1,1]:
    cx=s*.125
    for j in range(3):
        z=1.045-j*.068; w=.084-j*.005
        outline=[(cx-w,z+.018),(cx+w,z+.018),(cx+w*.94,z-.045),(cx+w*.67,z-.075),(cx-w*.77,z-.067)]
        plate(('R' if s<0 else 'L')+' tasset lame '+str(j),outline,-.121-j*.002,.006,'steel',1,.003)
        for sign in [-1,1]:bolt('Tasset suspension rivet',(cx+sign*w*.72,-.128-j*.002,z-.012),.0035)

# Arms are fitted to the body, with elbows and wrists in a relaxed heroic pose.
arms=[('R',(-.245,.012,1.487),(-.331,-.015,1.268),(-.394,-.09,1.075)),('L',(.245,.014,1.487),(.323,.035,1.255),(.361,-.186,1.145))]
for label,shoulder,elbow,wrist in arms:
    sh,el,wr=Vector(shoulder),Vector(elbow),Vector(wrist)
    bone(label+' sleeve | upper arming joint',sh,el,[(0,.067,.065),(.5,.062,.060),(1,.053,.050)],'mail',12,1)
    bone(label+' sleeve | forearm arming joint',el,wr,[(0,.054,.052),(.5,.049,.049),(1,.034,.035)],'mail',12,1)
    # Small, close-fitting shoulder caps; three lames follow the deltoid.
    for j in range(3):
        start=sh.lerp(el,-.10+j*.135); end=sh.lerp(el,.21+j*.12)
        r=.097-j*.011
        if j==0:
            start=sh.lerp(el,-.24);end=sh.lerp(el,.27)
            profile=[(0,r*.13,r*.13),(.09,r*.40,r*.4),(.34,r*.85,r*.85),(.64,r,r*.99),(.95,r*.96,r*.95),(1,r*.94,r*.93)]
        else:profile=[(0,r*.88,r*.87),(.035,r*.98,r*.97),(.16,r,r*.99),(.82,r*.95,r*.94),(.965,r*.90,r*.89),(1,r*.88,r*.87)]
        bone(label+' pauldron lame '+str(j),start,end,profile,'steel',16,1)
    bone(label+' rerebrace | fitted upper arm',sh.lerp(el,.38),sh.lerp(el,.93),[(0,.073,.069),(.08,.076,.073),(.90,.053,.055),(1,.052,.054)],'steel',14,1)
    bone(label+' vambrace | flared forearm',el.lerp(wr,.17),wr,[(0,.068,.066),(.1,.073,.069),(.45,.061,.06),(.93,.041,.043),(1,.04,.041)],'steel',16,1)
    x,y,z=el
    outline=[(x-.057,z+.058),(x+.053,z+.058),(x+.075,z+.008),(x+.053,z-.06),(x-.054,z-.058),(x-.078,z-.005)]
    plate(label+' couter | sculpted elbow cup',outline,y-.065,.018,'steel',1,.003)
    bolt(label+' elbow pivot',(x,y-.084,z),.005)
    # A short flare at the cuff, never a box around the wrist.
    bone(label+' gauntlet flared cuff',el.lerp(wr,.88),wr.lerp(el,-.16),[(0,.054,.053),(.15,.054,.053),(.92,.041,.043),(1,.04,.041)],'steel',14,1)

# Closed gauntlets with individual curved fingers, knuckles and a thumb.
def gauntlet(label,center):
    x,y,z=center
    bone(label+' glove palm',(x,y,z+.049),(x,y,z-.045),[(0,.032,.028),(.15,.044,.039),(.70,.043,.037),(1,.032,.03)],'leather',12,1)
    plate(label+' gauntlet metacarpal plate',[(x-.043,z+.047),(x+.042,z+.047),(x+.046,z-.015),(x+.029,z-.043),(x-.034,z-.044)],y-.035,.006,'steel',1,.003)
    for i in range(4):
        xx=x-.029+i*.0195; zz=z-.027-(abs(i-1.5)*.003)
        points=[(xx,y-.027,zz),(xx,y-.041,zz-.017),(xx,y-.034,zz-.035),(xx,y-.006,zz-.038),(xx,y+.011,zz-.021)]
        for j in range(3):
            bone(label+' finger %d phalanx %d'%(i,j),points[j],points[j+1],[(0,.010,.011),(.16,.011,.011),(.86,.009,.010),(1,.008,.009)],'steel',8,0)
        bolt(label+' knuckle '+str(i),(xx,y-.043,zz+.007),.007,'steel')
    thumb=[(x+.038,y+.004,z+.025),(x+.056,y-.015,z+.006),(x+.034,y-.034,z-.010)]
    for j in range(2):bone(label+' thumb plate '+str(j),thumb[j],thumb[j+1],[(0,.014,.016),(.35,.015,.016),(1,.012,.013)],'steel',8,1)
gauntlet('R',(-.404,-.125,1.026)); gauntlet('L',(.366,-.255,1.12))

# Close-fitting pointed bascinet with a separate visor and genuine recessed ocular.
helmet=[(0,.014,1.649,.077,.081),(0,.015,1.675,.103,.105),(0,.023,1.747,.122,.121),(0,.027,1.815,.119,.124),(0,.034,1.862,.099,.109),(0,.042,1.900,.063,.079),(0,.048,1.923,.016,.027)]
loft('Bascinet | hand-shaped pointed skull',helmet,'steel',28,1)
# Visor contours bow around the face; a long low eye opening is recessed.
def visor_y(x,z):return -.119 + .22*x*x
plate('Visor | recessed ocular shadow',[(-.098,1.809),(-.050,1.810),(0,1.808),(.050,1.810),(.098,1.809),(.094,1.798),(0,1.796),(-.094,1.798)],lambda x,z:visor_y(x,z)-.015,.001,'shadow',0,.001)
# Lower visor uses gridded contours so the nose is a gently forged central point.
verts=[];uv=[]; rows=[(1.798,.104,-.137),(1.775,.102,-.165),(1.731,.09,-.198),(1.698,.079,-.164),(1.67,.062,-.111)]
for j,(z,w,front) in enumerate(rows):
    for i in range(13):
        u=i/12; x=(2*u-1)*w; y=front+(.061 if j<3 else .034)*(abs(2*u-1)**1.15)
        verts.append((x,y,z));uv.append((u,1-j/4))
faces=[]
for j in range(4):
    for i in range(12):faces.append((j*13+i,j*13+i+1,(j+1)*13+i+1,(j+1)*13+i))
mesh_object('Hounskull visor | modeled medial prow',verts,faces,'steel',uv,1,.003)
# Brow is a narrow curved steel ledge, not a heavy bar over a block helmet.
tube('Rolled visor brow',[(-.104+i*.208/20,-.123+.033*abs(-1+i*.1),1.809-.006*(1-abs(-1+i*.1))) for i in range(21)],.004)
for s in [-1,1]:
    bolt('Visor hinge '+str(s),(s*.112,-.036,1.79),.010,'gold')
    for j in range(2):
        x=s*(.056+j*.017); z=1.741+j*.002; y=-.184+.061*abs(x/.092)
        plate('Visor fine breath aperture %d %d'%(s,j),[(x-.004,z-.0015),(x+.004,z-.0015),(x+.004,z+.0015),(x-.004,z+.0015)],y-.004,0,'shadow',0,0)
# Restrained brass brow inlay and crest line; no oversized fantasy crest.
tube('Bascinet sagittal inlay',[(0,.136,1.80),(0,.141,1.849),(0,.100,1.899),(0,.05,1.925),(0,-.019,1.901),(0,-.071,1.866),(0,-.099,1.833)],.0025,'gold')

# Cloth is authored as curved, hanging surfaces with coherent woven UVs.
verts=[];uv=[];cols=20;rowsn=12
for j in range(rowsn):
    t=j/(rowsn-1); z=1.526-1.18*t; w=.18+.12*sin(t*pi*.55)
    for i in range(cols+1):
        u=i/cols; x=(u*2-1)*w + .017*t*t
        yy=.142+.075*t+.035*sin(pi*t)+(.007+.023*t)*cos(u*8*pi+.3*t)
        zz=z+.024*(t**7)*cos(u*5*pi)+.014*sin(u*pi)*sin(t*pi)
        verts.append((x,yy,zz));uv.append((u,1-t))
faces=[]
for j in range(rowsn-1):
    for i in range(cols):faces.append((j*(cols+1)+i,j*(cols+1)+i+1,(j+1)*(cols+1)+i+1,(j+1)*(cols+1)+i))
mesh_object('Crimson cape | gravity drape and curved folds',verts,faces,'cloth',uv,1,.004)
for side in [0,cols]:tube('Cape stitched edge '+str(side),[verts[j*(cols+1)+side] for j in range(rowsn)],.002,'gold')
tube('Cape scalloped hem',verts[-(cols+1):],.002,'gold')
for s in [-1,1]:bolt('Cloak brooch '+str(s),(s*.151,-.076,1.503),.013,'gold')
# Two parted tabard skirts expose the anatomical leg armor and have a soft hem.
for s in [-1,1]:
    verts=[];uv=[];cols=5;nr=9
    for j in range(nr):
        t=j/(nr-1)
        for i in range(cols+1):
            u=i/cols; xx=s*(.010+u*(.092+.018*t)); zz=1.045-.39*t+.017*(t**5)*(u-.5)
            yy=-.137-.016*sin(pi*t)+.008*sin(u*pi*2+t*.6)+.019*t
            verts.append((xx,yy,zz));uv.append((u,1-t))
    faces=[]
    for j in range(nr-1):
        for i in range(cols):faces.append((j*(cols+1)+i,j*(cols+1)+i+1,(j+1)*(cols+1)+i+1,(j+1)*(cols+1)+i))
    mesh_object('Split tabard skirt '+str(s),verts,faces,'cloth',uv,1,.003)
    tube('Tabard hem '+str(s),verts[-(cols+1):],.0018,'gold')
# Fine leather sword belt and brass buckle.
loft('Waist belt',[(0,.012,1.099,.172,.130),(0,.012,1.127,.172,.130)],'leather',32,0)
plate('Belt buckle',[(-.028,1.13),(.028,1.13),(.028,1.095),(-.028,1.095)],-.124,0,'gold',0,.004)
plate('Buckle inset',[(-.019,1.124),(.019,1.124),(.019,1.102),(-.019,1.102)],-.128,0,'leather',0,.001)

# Longsword is held point-down; diamond section catches a long metal highlight.
sx,sy=-.405,-.102
bone('Sword | leather-bound grip',(sx,sy,.928),(sx,sy,1.122),[(0,.017,.018),(.1,.020,.020),(.88,.017,.018),(1,.016,.017)],'leather',12,1)
for j in range(10):
    z=.944+j*.016
    ring_trim('Grip wire '+str(j),(sx,sy,z,.020,.021),.001,'gold')
loft('Sword pommel | pear form',[(sx,sy,1.119,.017,.015),(sx,sy,1.135,.030,.025),(sx,sy,1.163,.022,.019),(sx,sy,1.175,.008,.009)],'steel',16,1)
# Gently down-swept crossguard, hand-forged with tapered quillons.
pts=[(sx-.143,sy,.895),(sx-.12,sy,.911),(sx-.065,sy,.925),(sx,sy,.929),(sx+.065,sy,.925),(sx+.12,sy,.911),(sx+.143,sy,.895)]
tube('Sword crossguard | swept quillons',pts,.010,'steel',False,8)
for s in [-1,1]:bolt('Sword guard finial '+str(s),(sx+s*.14,sy-.003,.899),.008,'gold')
verts=[];uv=[]
for j,(z,w,d) in enumerate([(.926,.029,.006),(.882,.031,.007),(.22,.023,.004),(.064,.001,.001)]):
    for i,(xx,yy) in enumerate([(-w,0),(0,-d),(w,0),(0,d)]):verts.append((sx+xx,sy+yy,z));uv.append((i/3,j/3))
faces=[]
for j in range(3):
    for i in range(4):faces.append((j*4+i,j*4+(i+1)%4,(j+1)*4+(i+1)%4,(j+1)*4+i))
faces += [(3,2,1,0),(12,13,14,15)]
blade=mesh_object('Sword blade | tapered diamond cross-section',verts,faces,'steel',uv)
for poly in blade.data.polygons:poly.use_smooth=False

# Heater shield is a convex curved plate, strapped to the raised left forearm.
cx=.392
shieldrows=[(1.485,.173),(1.472,.185),(1.37,.195),(1.19,.167),(1.015,.105),(.878,.007)]
verts=[];uv=[];cols=16
for j,(z,w) in enumerate(shieldrows):
    for i in range(cols+1):
        u=i/cols; xx=(u*2-1)*w; yy=-.334-.050*(1-(xx/.20)**2)+.015*(1.2-z)
        verts.append((cx+xx,yy,z));uv.append((u,1-j/(len(shieldrows)-1)))
faces=[]
for j in range(len(shieldrows)-1):
    for i in range(cols):faces.append((j*(cols+1)+i,j*(cols+1)+i+1,(j+1)*(cols+1)+i+1,(j+1)*(cols+1)+i))
mesh_object('Heater shield | curved lacquered wooden core',verts,faces,'cloth',uv,1,.015)
border=[verts[i] for i in range(cols+1)] + [verts[j*(cols+1)+cols] for j in range(1,len(shieldrows))] + [verts[(len(shieldrows)-1)*(cols+1)+i] for i in range(cols-1,-1,-1)] + [verts[j*(cols+1)] for j in range(len(shieldrows)-2,0,-1)]
tube('Heater shield steel binding',border,.008,'steel',True,8)
tube('Heater shield thin gilt edge',[(x,y-.006,z) for x,y,z in border],.0022,'gold',True)
# A raised heraldic fleur-de-lis: three sculpted petals and a narrow binding.
def shield_depth(x,z):return -.393+.050*((x-cx)/.2)**2+.015*(1.2-z)
petals=[[(cx,1.400),(cx+.030,1.353),(cx+.022,1.271),(cx,1.218),(cx-.022,1.271),(cx-.030,1.353)],
[(cx-.006,1.244),(cx-.030,1.310),(cx-.070,1.327),(cx-.094,1.31),(cx-.095,1.279),(cx-.074,1.26),(cx-.065,1.285),(cx-.04,1.267),(cx-.034,1.223)],
[(cx+.006,1.244),(cx+.030,1.310),(cx+.070,1.327),(cx+.094,1.31),(cx+.095,1.279),(cx+.074,1.26),(cx+.065,1.285),(cx+.04,1.267),(cx+.034,1.223)],
[(cx-.046,1.242),(cx+.046,1.242),(cx+.046,1.223),(cx-.046,1.223)],
[(cx,1.225),(cx+.019,1.187),(cx,1.141),(cx-.019,1.187)]]
for i,outline in enumerate(petals):plate('Shield fleur-de-lis relief %d'%i,outline,shield_depth,.004,'gold',1,.002)
for j,(z,w) in enumerate(shieldrows[:5]):
    for s in [-1,1]:
        x=cx+s*w*.9;bolt('Shield perimeter rivet %d %d'%(j,s),(x,shield_depth(x,z)+.005,z-.011),.004,'gold')
# Rear arm straps are visible from behind and connect shield to the forearm.
for z in [1.11,1.28]:
    tube('Shield leather arm strap '+str(z),[(cx-.11,-.321,z),(cx-.10,-.245,z),(cx-.03,-.208,z),(cx+.06,-.25,z),(cx+.09,-.329,z)],.014,'leather',False,8)

# Per-component silhouette-preserving collapse after smoothing; fine narrow trim
# and the diamond blade remain untouched. Modifier remains editable in source.
for obj in model.objects:
    if any(mod.type=='SUBSURF' for mod in obj.modifiers):
        dec=obj.modifiers.new('Export budget | preserve contour','DECIMATE')
        dec.ratio=.375;dec.use_collapse_triangulate=True

# Studio scene: this source is immediately editable and renderable in Blender.
scene=bpy.context.scene
world=bpy.data.worlds.new('Charcoal studio');scene.world=world;world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.035,.046,.065,1)
world.node_tree.nodes['Background'].inputs[1].default_value=.45

def look_at(obj,target):obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
def area(name,location,power,color,size):
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.color=color;data.shape='DISK';data.size=size
    obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);obj.location=location;look_at(obj,(0,0,1.05))
area('Key | broad coolbox',(-3,-4,4),700,(.78,.88,1),3)
area('Fill | warmbox',(3,-2,2.5),450,(1,.84,.65),2.5)
area('Rim | silver edge',(1,2.7,3.3),850,(.7,.83,1),2)
camdata=bpy.data.cameras.new('Portrait camera');cam=bpy.data.objects.new('Portrait camera',camdata);scene.collection.objects.link(cam)
cam.location=(-2.65,-6.3,2.7);look_at(cam,(0,-.03,.98));camdata.type='ORTHO';camdata.ortho_scale=2.30;scene.camera=cam
# A ground plane is excluded from the exported model collection.
mesh=bpy.data.meshes.new('Studio ground');mesh.from_pydata([(-200,-200,.015),(200,-200,.015),(200,200,.015),(-200,200,.015)],[],[(0,1,2,3)])
ground=bpy.data.objects.new('Studio ground | not exported',mesh);scene.collection.objects.link(ground)
mat=bpy.data.materials.new('Studio matte');mat.use_nodes=True;mat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.018,.024,.035,1);mat.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.95;ground.data.materials.append(mat)
scene.render.engine='CYCLES';scene.cycles.samples=32
scene.cycles.use_denoising=True
scene.render.resolution_x=900;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(ROOT/'build'/'knight-blender.png')
scene['asset']='Crimson Sentinel | sculpted plate harness';scene['export_axes']='OBJ Y up, front -Z'
scene['construction_notes']='Editable contour lofts and panel surfaces, thickness modifiers, anatomical proportions, coherent atlas UVs.'
# Set a useful saved viewport view, no selected outlines.
bpy.ops.object.select_all(action='DESELECT')
for screen in bpy.data.screens:
    for area_ in screen.areas:
        if area_.type=='VIEW_3D':
            area_.spaces.active.region_3d.view_distance=3.1
            area_.spaces.active.region_3d.view_location=(0,0,1)
            area_.spaces.active.region_3d.view_rotation=cam.rotation_euler.to_quaternion()
            area_.spaces.active.shading.type='MATERIAL'
(ROOT/'source').mkdir(exist_ok=True);(ROOT/'build').mkdir(exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'source'/'knight.blend'))

# Custom deterministic export includes evaluated modifier geometry and corner
# normals/UVs. Swapping Y/Z is a reflection, so reverse every triangle corner.
depsgraph=bpy.context.evaluated_depsgraph_get()
positions=[];texcoords=[];normals=[];position_ids={};uv_ids={};normal_ids={};faces=[];triangles=0

def indexed(values,table,records):
    key=tuple(round(float(v),7) for v in values)
    if key not in table:
        table[key]=len(records)+1;records.append(key)
    return table[key]

for obj in model.objects:
    evaluated=obj.evaluated_get(depsgraph); mesh=evaluated.to_mesh(preserve_all_data_layers=True,depsgraph=depsgraph)
    mesh.calc_loop_triangles(); uv=mesh.uv_layers.active.data
    faces.append('o '+obj.name.replace(' ','_').replace('|',''))
    name=obj.data.materials[0].name;faces.append('usemtl '+name)
    for tri in mesh.loop_triangles:
        indices=[]
        for li in reversed(tri.loops):
            vi=mesh.loops[li].vertex_index; p=obj.matrix_world@mesh.vertices[vi].co
            normal=obj.matrix_world.to_3x3()@mesh.corner_normals[li].vector; normal.normalize()
            t=uv[li].uv
            a=indexed((p.x,p.z,p.y),position_ids,positions)
            b=indexed((t.x,t.y),uv_ids,texcoords)
            c=indexed((normal.x,normal.z,normal.y),normal_ids,normals)
            indices.append('%d/%d/%d'%(a,b,c))
        faces.append('f '+' '.join(indices));triangles+=1
    evaluated.to_mesh_clear()
lines=['# Crimson Sentinel | authored in Blender','mtllib knight.mtl']
lines+=['v %.7f %.7f %.7f'%p for p in positions]
lines+=['vt %.7f %.7f'%p for p in texcoords]
lines+=['vn %.7f %.7f %.7f'%p for p in normals]
lines+=faces
(OUT/'knight.obj').write_text('\n'.join(lines)+'\n',encoding='utf-8')
mtl=[]
for name,ks,ns in [('steel',(.62,.66,.72),88),('gold',(.72,.55,.27),100),('cloth',(.015,.015,.015),8),('leather',(.08,.07,.06),20),('mail',(.08,.09,.10),20),('shadow',(0,0,0),8)]:
    kd='0.15 0.18 0.21' if name=='mail' else '0.025 0.03 0.035' if name=='shadow' else '1.0 1.0 1.0'
    mtl+=['newmtl '+name,'Ka 0.15 0.15 0.15','Kd '+kd,'Ks %s %s %s'%ks,'Ns '+str(ns),'illum 2','map_Kd knight_atlas.png','']
(OUT/'knight.mtl').write_text('\n'.join(mtl),encoding='utf-8')
print('EXPORTED %d vertices / %d triangles in %d crafted components'%(len(positions),triangles,len(model.objects)),flush=True)
bpy.ops.render.render(write_still=True)
print('PREVIEW '+scene.render.filepath,flush=True)
