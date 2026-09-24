"""Reference-matched harness: pointed cuirass, traced gorget, wing pauldrons.
Native Blender coordinates: Z up, front -Y. Component meshes remain editable.
"""
from .common import *
from mathutils import Vector
from math import sin,cos,pi,sqrt


def _cuirass_point(a,z,rx,ry,drop=0,keel=.012):
    # A flattened anterior arc and a gentle keel replace the round barrel shape.
    front=max(0,-sin(a)); rear=max(0,sin(a))
    x=rx*cos(a)
    y=.017 + (ry*(rear**.78) if sin(a)>=0 else -ry*(front**.61))-keel*front**20
    zz=z-drop*front**5
    return (x,y,zz)


def _cuirass():
    profiles=[(1.116,.165,.127,.067),(1.125,.169,.131,.067),
              (1.202,.182,.140,.023),(1.320,.211,.155,0),
              (1.412,.235,.153,0),(1.457,.231,.140,.006),
              (1.477,.209,.123,.005),(1.484,.201,.117,.004)]
    n=48;verts=[];uv=[]
    for j,(z,rx,ry,drop) in enumerate(profiles):
        for i in range(n):
            a=2*pi*i/n;verts.append(_cuirass_point(a,z,rx,ry,drop));uv.append((i/n,j/(len(profiles)-1)))
    faces=[]
    for j in range(len(profiles)-1):
        for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
    # Open neck/waist boundaries use real plate thickness, never a puffy cap.
    obj=mesh_object('Cuirass | flattened ribcage and pointed abdominal keel',verts,faces,'steel',uv,1,.006)
    # One subtle hard medial ridge is retained through subdivision.
    crease=obj.data.attributes.new(name='crease_edge',type='FLOAT',domain='EDGE')
    for e in obj.data.edges:
        a,b=(obj.data.vertices[i].co for i in e.vertices)
        if abs(a.x)<.00001 and abs(b.x)<.00001 and a.y<0 and b.y<0:crease.data[e.index].value=.35
    # Narrow lower steel border mirrors the pointed belly and carries a fine gilt line.
    p=profiles[0]
    lower=[_cuirass_point(2*pi*i/64,*p) for i in range(64)]
    tube('Breastplate lower edge | worn bronze inlay',[(x,y-.001 if y<0 else y+.001,z) for x,y,z in lower],.0021,'gold',True,6)
    # Rivets belong at the arm openings, away from the broad main chest surface.
    for s in [-1,1]:
        bolt('Breastplate strap pin '+str(s),(s*.189,-.070,1.462),.0045,'gold')


def _gorget_contour(a,rx,ry,z,drop):
    front=max(0,-sin(a));rear=max(0,sin(a))
    return (rx*cos(a),.013+ry*sin(a),z-drop*front*(1-.74*abs(cos(a)))+.01*rear)


def _gorget_panel(name,profiles):
    n=48;verts=[];uv=[]
    for j,(rx,ry,z,drop) in enumerate(profiles):
        for i in range(n):
            verts.append(_gorget_contour(i*2*pi/n,rx,ry,z,drop));uv.append((i/n,j/(len(profiles)-1)))
    faces=[]
    for j in range(len(profiles)-1):
        for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
    mesh_object(name,verts,faces,'steel',uv,1,.003)
    p=profiles[-1]
    pts=[_gorget_contour(i*2*pi/64,*p) for i in range(64)]
    tube(name+' | fine gilt perimeter',[(x,y-.002 if y<0 else y+.002,z-.001) for x,y,z in pts],.0028,'gold',True,6)
    # Paired inlay line follows the same V, one narrow band above the rolled edge.
    tube(name+' | paired inner bronze line',[(x*.975,y-.003 if y<0 else y+.003,z+.008) for x,y,z in pts],.0017,'gold',True,5)


def _shoulder_point(s,t,u):
    # Truncated ellipsoidal shell: compound curvature in BOTH directions.
    # The medial cut is a raised retaining wing, the frontal edge follows the
    # ellipse in plan and has a small scallop rather than a rectangular hem.
    x=.196+.263*t
    normalized=(x-.290)/.170
    ring=sqrt(max(.002,1-normalized*normalized))
    y=.005+.160*ring*sin(u*pi/2)
    lip=1.408+.012*(sin(pi*t)**2)
    z=lip+.202*ring*cos(u*pi/2)
    return s*x,y,z


def _pauldron(s):
    label='R' if s<0 else 'L'
    ts=[0,.025,.085,.18,.33,.50,.69,.86,.97,1]
    us=[-1,-.97,-.87,-.68,-.43,-.2,0,.2,.43,.68,.87,.97,1]
    verts=[];uv=[]
    for t in ts:
        for u in us:verts.append(_shoulder_point(s,t,u));uv.append(((u+1)/2,t))
    n=len(us);faces=[]
    for j in range(len(ts)-1):
        for i in range(n-1):faces.append((j*n+i,j*n+i+1,(j+1)*n+i+1,(j+1)*n+i))
    mesh_object(label+' pauldron | broad forged shoulder wing',verts,faces,'steel',uv,1,.005)
    # Fine borders run around the high wing and along the drooping lower lip.
    boundary=[_shoulder_point(s,t,-1) for t in ts]
    boundary += [_shoulder_point(s,1,u) for u in us[1:]]
    boundary += [_shoulder_point(s,t,1) for t in reversed(ts[:-1])]
    boundary += [_shoulder_point(s,0,u) for u in reversed(us[1:-1])]
    tube(label+' pauldron | bronze boundary',boundary,.0025,'gold',True,6)
    # Raised inner flange stops just below the helmet, a distinctive reference feature.
    spine=[_shoulder_point(s,.008,u) for u in us]
    tube(label+' shoulder wing | narrow retaining edge',[(x,y,z+.005) for x,y,z in spine],.0030,'steel',False,6)
    # One sharply overlapping lower lame. It wraps only the outside of the arm.
    shoulder=Vector((s*.275,0,1.515));elbow=Vector((s*.38,-.01,1.18))
    axis=(elbow-shoulder).normalized();out=Vector((s,0,0));out=(out-axis*out.dot(axis)).normalized();front=axis.cross(out).normalized()
    verts=[];uv=[];angles=19
    for j,t in enumerate([.37,.385,.51,.525]):
        center=shoulder.lerp(elbow,t)
        radius=.112-(t-.37)*.06
        for i in range(angles):
            a=(-1.66+3.32*i/(angles-1));q=center+out*(radius*cos(a))+front*(radius*sin(a))
            verts.append(tuple(q));uv.append((i/(angles-1),j/3))
    faces=[]
    for j in range(3):
        for i in range(angles-1):faces.append((j*angles+i,j*angles+i+1,(j+1)*angles+i+1,(j+1)*angles+i))
    mesh_object(label+' pauldron | articulated lower lame',verts,faces,'steel',uv,1,.004)
    tube(label+' shoulder lame gilt edge',verts[-angles:],.002,'gold',False,6)
    # Tan padded shoulder sleeve, with a narrow chain cuff exposed below armor.
    bone(label+' arming doublet upper sleeve',shoulder.lerp(elbow,.14),shoulder.lerp(elbow,.64),[(0,.070,.074),(.1,.074,.078),(.66,.078,.079),(1,.071,.073)],'cloth',16,1)
    bone(label+' shoulder mail cuff',shoulder.lerp(elbow,.49),shoulder.lerp(elbow,.58),[(0,.093,.09),(.12,.095,.092),(.88,.087,.085),(1,.083,.082)],'mail',20,0)
    # The reference has a leather fastening on the underside of each shell.
    bone(label+' pauldron underside leather fastening',shoulder.lerp(elbow,.58),shoulder.lerp(elbow,.635),[(0,.079,.078),(.1,.081,.080),(.9,.079,.077),(1,.077,.075)],'leather',16,0)
    return shoulder,elbow


def _chain_mesh(name,links):
    # Selective real chain relief complements the atlas texture in exposed areas.
    verts=[];faces=[];uv=[];n=6;m=3
    for center,right,up,scale,angle in links:
        center=Vector(center);r=Vector(right).normalized();u=Vector(up).normalized();normal=r.cross(u).normalized()
        rr=r*cos(angle)+u*sin(angle);uu=-r*sin(angle)+u*cos(angle)
        base=len(verts)
        for i in range(n):
            a=2*pi*i/n;point=center+rr*(scale*cos(a))+uu*(scale*1.30*sin(a))
            radial=(rr*cos(a)+uu*sin(a)).normalized()
            for j in range(m):
                q=point+(radial*cos(2*pi*j/m)+normal*sin(2*pi*j/m))*(scale*.23)
                verts.append(tuple(q));uv.append((i/n,j/m))
        for i in range(n):
            for j in range(m):faces.append((base+i*m+j,base+((i+1)%n)*m+j,base+((i+1)%n)*m+(j+1)%m,base+i*m+(j+1)%m))
    if verts:mesh_object(name,verts,faces,'mail',uv)


def _tasset(s):
    label='R' if s<0 else 'L'
    outline=[(.152,1.063),(.194,1.079),(.228,1.051),(.269,.960),(.271,.923),(.248,.897),(.216,.897),(.181,.926),(.163,.995)]
    outline=[(s*x,z) for x,z in outline]
    def depth(x,z):return -.118+.42*(abs(x)-.17)
    plate(label+' tasset | swept leaf-shaped hip plate',outline,depth,.012,'steel',1,.004)
    cx=sum(x for x,z in outline)/len(outline);cz=sum(z for x,z in outline)/len(outline)
    rim=[]
    for i,(x,z) in enumerate(outline):
        nx,nz=outline[(i+1)%len(outline)]
        for j in range(4):
            t=j/4;xx=x*(1-t)+nx*t;zz=z*(1-t)+nz*t
            xx=cx+(xx-cx)*.972;zz=cz+(zz-cz)*.972
            rim.append((xx,depth(xx,zz)-.004,zz))
    tube(label+' tasset | antique bronze outline',rim,.0020,'gold',True,6)
    # Shallow branching tracery follows the leaf without large raised ornaments.
    paths=[[(.188,1.047),(.196,1.018),(.219,.969),(.240,.925)],
           [(.196,1.018),(.222,1.021),(.238,.995)],
           [(.219,.969),(.198,.972),(.190,.996)],
           [(.210,.988),(.229,.983),(.241,.961)]]
    for j,path in enumerate(paths):
        pts=[]
        for k in range(len(path)-1):
            for l in range(5):
                t=l/5;x=s*(path[k][0]*(1-t)+path[k+1][0]*t);z=path[k][1]*(1-t)+path[k+1][1]*t
                pts.append((x,depth(x,z)-.013,z))
        x=s*path[-1][0];z=path[-1][1];pts.append((x,depth(x,z)-.013,z))
        tube(label+' tasset | chased leaf engraving '+str(j),pts,.0017,'gold',False,5)
    for x,z in [(s*.169,1.047),(s*.206,1.048)]:bolt(label+' tasset suspension rivet',(x,depth(x,z)-.005,z),.0036,'gold')


def build():
    # Neutral taupe arming garment, hidden beneath a complete anatomical harness.
    loft('Arming doublet | fitted taupe torso',[(0,.015,.987,.165,.113),(0,.015,1.075,.160,.112),(0,.015,1.24,.188,.136),(0,.015,1.43,.227,.13),(0,.015,1.51,.185,.11)],'cloth',28,1)
    _cuirass()
    # Collar mail closes the neck space below the ornate closed helmet.
    loft('Mail coif | exposed neck collar',[(0,.012,1.541,.094,.088),(0,.012,1.56,.096,.088),(0,.012,1.649,.085,.078),(0,.012,1.669,.083,.075)],'mail',28,1)
    _gorget_panel('Gorget | lower V-shaped mantle',[(.105,.084,1.583,.011),(.111,.09,1.578,.013),(.178,.137,1.555,.066),(.230,.158,1.513,.072),(.232,.159,1.508,.071)])
    _gorget_panel('Gorget | middle overlapping V plate',[(.114,.101,1.575,.010),(.130,.119,1.569,.016),(.172,.158,1.539,.064),(.181,.167,1.531,.065),(.182,.168,1.526,.065)])
    _gorget_panel('Gorget | upper overlapping V collar',[(.096,.081,1.605,.005),(.099,.084,1.598,.005),(.123,.107,1.579,.014),(.151,.144,1.561,.047),(.154,.149,1.557,.049)])
    _gorget_panel('Gorget | close upper neck band',[(.094,.080,1.627,.003),(.097,.083,1.623,.003),(.105,.091,1.603,.004),(.106,.093,1.599,.004)])
    # A second line follows the lower V to make the double bronze trim in reference.
    p=(.218,.151,1.521,.071)
    tube('Gorget | inset V-tracery',[_gorget_contour(i*2*pi/64,*p) for i in range(64)],.0016,'gold',True,5)
    # Rear continuation of the girdle; front belt remains clean, broad dark leather.
    loft('Leather girdle | fitted waist belt',[(0,.014,1.025,.186,.139),(0,.014,1.031,.19,.142),(0,.014,1.079,.184,.137),(0,.014,1.085,.179,.132)],'leather',40,0)
    # Buckle offset to the left as on functional suspension belts.
    plate('Waist belt | bronze buckle',[(-.107,1.081),(-.061,1.081),(-.061,1.041),(-.107,1.041)],-.111,0,'gold',0,.003)
    plate('Waist belt | open buckle center',[(-.099,1.074),(-.069,1.074),(-.069,1.048),(-.099,1.048)],-.116,0,'leather',0,.001)
    tube('Waist buckle | tongue',[(-.100,-.119,1.06),(-.071,-.119,1.06)],.0016,'gold',False,5)
    # Cloth is tan, short, and fitted around the hips; there is no cape or tabard.
    loft('Taupe quilted hip garment',[(0,.019,.80,.191,.12),(0,.019,.84,.20,.127),(0,.019,.97,.18,.125),(0,.019,1.035,.172,.12)],'cloth',32,1)
    # Shallow split at the chain skirt center gives a practical fitted hem.
    n=40;verts=[];uv=[]
    for j,(z,rx,ry) in enumerate([(1.035,.183,.134),(.996,.192,.137),(.91,.21,.145),(.813,.216,.143)]):
        for i in range(n):
            a=i*2*pi/n;front=max(0,-sin(a));notch=(.057*front**24 if j==3 else 0)
            verts.append((rx*cos(a),.02+ry*sin(a),z+notch));uv.append((i/n,j/3))
    faces=[]
    for j in range(3):
        for i in range(n):faces.append((j*n+i,j*n+(i+1)%n,(j+1)*n+(i+1)%n,(j+1)*n+i))
    mesh_object('Chainmail skirt | flared split hem',verts,faces,'mail',uv,1,.004)
    for s in [-1,1]:_tasset(s)
    shoulders=[_pauldron(s) for s in [-1,1]]
    # Actual interlinked relief where it most clearly reads in the full figure.
    links=[]
    for row in range(22):
        z=.851+row*.007
        for col in range(45):
            if (row+col)%2:continue
            x=-.136+col*.0062+(row%2)*.002
            y=.02-.145*sqrt(max(.01,1-(x/.216)**2))-.002
            links.append(((x,y,z),(1,x*.60,0),(0,0,1),.0028,(.34 if row%2 else -.34)))
    _chain_mesh('Mail skirt | real interlinked front relief',links)
    for index,(shoulder,elbow) in enumerate(shoulders):
        links=[];s=-1 if index==0 else 1
        for row in range(3):
            center=shoulder.lerp(elbow,.50+row*.023)
            for col in range(12):
                a=-1.50+col*3.0/11
                # Exposed front-facing rows curve around the outside upper arm.
                x=center.x+s*.09*cos(a);y=center.y-.089*sin(a);z=center.z+.009*cos(a)
                links.append(((x,y,z),(cos(a)*0+1,.3*sin(a),0),(0,0,1),.0032,(.30 if row%2 else -.30)))
        _chain_mesh(('R' if s<0 else 'L')+' shoulder | mail cuff relief',links)
    # Fine collar links remain visible in the opening behind the gold-edged gorget.
    links=[]
    for row in range(3):
        z=1.616+row*.013
        for col in range(15):
            a=-pi*.90+col*pi*.80/14
            x=.087*cos(a);y=.012+.080*sin(a)
            links.append(((x,y-.001,z),(1,0,0),(0,0,1),.0032,(.30 if row%2 else -.30)))
    _chain_mesh('Coif | visible neck chain relief',links)
