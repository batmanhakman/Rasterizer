"""Reference-matched articulated limbs; Z up, front -Y. Called by the assembler."""
from .common import *
import math
from mathutils import Vector
from math import sin, cos, pi


def _basis(a, b):
    a, b = Vector(a), Vector(b)
    axis = (b-a).normalized()
    u = Vector((1, 0, 0))
    u = (u-axis*u.dot(axis)).normalized()
    v = u.cross(axis).normalized()  # forward (-Y) for downward limbs
    if v.y > 0:
        v = -v
    return a, b, u, v


def _cloth(name, a, b, profiles, mat='cloth', folds=.006, n=24, rows=15):
    a,b,u,v = _basis(a,b)
    verts=[]; uv=[]
    def radii(t):
        for j in range(len(profiles)-1):
            if t <= profiles[j+1][0]:
                t0,x0,y0=profiles[j]; t1,x1,y1=profiles[j+1]
                f=(t-t0)/(t1-t0)
                return x0+(x1-x0)*f, y0+(y1-y0)*f
        return profiles[-1][1:]
    for j in range(rows):
        t=j/(rows-1); rx,ry=radii(t)
        for i in range(n):
            q=2*pi*i/n
            # A few broad gathers wander around the sleeve; localized valleys
            # break their continuity so the cloth never reads as wrapped rings.
            gathers=[(.15,.063,1.1,.80),(.39,.083,-.4,1.0),
                     (.68,.090,2.3,.88),(.89,.057,4.7,.60)]
            if rows<=8:
                gathers=[(.53,.21,1.3,.72)]
            displacement=0.0
            for center,width,phase,strength in gathers:
                shifted=center+.047*sin(q+phase)+.020*sin(2*q-phase)
                around=.23+.77*(.5+.5*cos(q-phase))**2
                distance=(t-shifted)/width
                ridge=math.exp(-distance*distance)
                valley=math.exp(-((distance-1.05)/.67)**2)
                displacement+=strength*around*(ridge-.44*valley)
            # A short diagonal compression crease on one side of the elbow.
            angle=math.atan2(sin(q-.45),cos(q-.45))
            local=math.exp(-(angle/.63)**2)*math.exp(-((t-.55-.055*cos(q))/.055)**2)
            bulk=.16*sin(pi*t)*sin(2*q+.7*t)
            fade=sin(pi*t)**.55
            crease=folds*fade*(.87*displacement+bulk-.25*local)
            p=a.lerp(b,t)+u*((rx+crease)*cos(q))+v*((ry+crease)*sin(q))
            verts.append(tuple(p)); uv.append((i/n,t))
    faces=[]
    for j in range(rows-1):
        for i in range(n):
            k=j*n+i; ni=j*n+(i+1)%n
            faces.append((k,ni,ni+n,k+n))
    faces += [tuple(reversed(range(n))),tuple((rows-1)*n+i for i in range(n))]
    return mesh_object(name,verts,faces,mat,uv,0)


def _cross_ring(a,b,t,rx,ry,n=32):
    a,b,u,v=_basis(a,b); c=a.lerp(b,t)
    return [tuple(c+u*(rx*cos(2*pi*i/n))+v*(ry*sin(2*pi*i/n))) for i in range(n)]


def _shell(name,a,b,rx0,ry0,rx1,ry1,side,mat='steel',wrap=1.28,point=.10):
    """A thin bent plate with a pointed cuff, genuine thickness and crisp rolled edge."""
    a,b,u,v=_basis(a,b)
    mid=pi/2-side*.22
    def surface(t,q,offset=0):
        rx=rx0+(rx1-rx0)*t; ry=ry0+(ry1-ry0)*t
        crown=max(0,1-abs((q-mid)/wrap))
        # The center of each terminal edge projects into a deliberate Gothic point.
        shifted=t-point*crown**2*(1-t)**8+point*.72*crown**2*t**8
        c=a.lerp(b,shifted)
        return c+u*((rx+offset)*cos(q))+v*((ry+offset)*sin(q))
    ts=[0,.018,.065,.20,.42,.66,.87,.96,.985,1]
    n=20; verts=[];uv=[]
    for t in ts:
        for i in range(n+1):
            q=mid-wrap+2*wrap*i/n
            verts.append(tuple(surface(t,q))); uv.append((i/n,t))
    faces=[]
    for j in range(len(ts)-1):
        for i in range(n):
            k=j*(n+1)+i;faces.append((k,k+1,k+n+2,k+n+1))
    mesh_object(name,verts,faces,mat,uv,0,.0032)
    outline=[tuple(surface(0,mid-wrap+2*wrap*i/n,.001)) for i in range(n+1)]
    outline += [tuple(surface(t,mid+wrap,.001)) for t in ts[1:]]
    outline += [tuple(surface(1,mid+wrap-2*wrap*i/n,.001)) for i in range(1,n+1)]
    outline += [tuple(surface(t,mid-wrap,.001)) for t in reversed(ts[1:-1])]
    tube(name+' | narrow brass perimeter',outline,.0017,'gold',True,5)
    return surface, mid


def _surface_line(name,surface,coords,mat='gold',radius=.0016):
    points=[]
    for j in range(len(coords)-1):
        t0,q0=coords[j];t1,q1=coords[j+1]
        for i in range(3):
            f=i/3;points.append(tuple(surface(t0+(t1-t0)*f,q0+(q1-q0)*f,.0022)))
    points.append(tuple(surface(*coords[-1],.0022)))
    tube(name,points,radius,mat,False,5)


def _buckle(name,a,b,t,rx,ry,angle):
    a,b,u,v=_basis(a,b); axis=(b-a).normalized()
    normal=(u*cos(angle)+v*sin(angle)).normalized()
    transverse=(-u*sin(angle)+v*cos(angle)).normalized()
    center=a.lerp(b,t)+u*(rx*cos(angle))+v*(ry*sin(angle))+normal*.004
    corners=[center+transverse*x+axis*z for x,z in [(-.010,-.013),(.010,-.013),(.010,.013),(-.010,.013)]]
    tube(name+' | steel frame',[tuple(x) for x in corners],.0013,'steel',True,5)
    tube(name+' | tongue',[tuple(center-transverse*.011),tuple(center+transverse*.009)],.001,'steel',False,5)
    # Strap tail and punched holes sit on the fabric side of the arm, beside the plate.
    verts=[tuple(center+transverse*x+axis*z-normal*.002) for x,z in [(-.008,.015),(.008,.015),(.008,.05),(-.008,.05)]]
    mesh_object(name+' | loose leather tail',verts,[(0,1,2,3)],'leather',None,0,.0015)
    for j in range(2):
        c=center+axis*(.026+j*.011)-normal*.0005
        pts=[tuple(c+transverse*(.0012*cos(i*2*pi/8))+axis*(.0012*sin(i*2*pi/8))) for i in range(8)]
        mesh_object(name+' | strap hole '+str(j),pts,[tuple(range(8))],'shadow',None,0)


def _mail_patch(name,a,b,t0,t1,rx,ry,angle0=.30,angle1=2.84,cols=10,rows=2):
    a,b,u,v=_basis(a,b); axis=(b-a).normalized()
    for j in range(rows):
        t=t0+(t1-t0)*(j+.5)/rows
        for i in range(cols):
            q=angle0+(angle1-angle0)*(i+.5+(j%2)*.30)/cols
            transverse=(-u*sin(q)+v*cos(q)).normalized()
            normal=(u*cos(q)+v*sin(q)).normalized()
            c=a.lerp(b,t)+u*(rx*cos(q))+v*(ry*sin(q))
            pts=[]
            for k in range(8):
                aa=2*pi*k/8
                p=c+transverse*(.0040*cos(aa))+axis*(.0052*sin(aa))+normal*(.0012*sin(aa)*(1 if j%2 else -1))
                pts.append(tuple(p))
            tube(name+' link %d-%d'%(j,i),pts,.0008,'mail',True,4)


def _gauntlet(name,wrist,side):
    original_objects=set(model.objects)
    x,y,z=wrist
    # Human palm rather than a cuboid mitten. Finger lengths keep their natural hierarchy.
    palm_a=(x,y,z+.007);palm_b=(x+side*.009,y+.003,z-.077)
    bone(name+' | soft leather palm',palm_a,palm_b,[(0,.028,.022),(.12,.033,.023),(.69,.037,.023),(1,.032,.019)],'leather',12,1)
    cx=x+side*.006
    outline=[(cx-.030,z-.003),(cx+.030,z-.003),(cx+.036,z-.043),(cx+.027,z-.069),(cx-.028,z-.071),(cx-.036,z-.047)]
    plate(name+' | shaped metacarpal shield',outline,y-.024,.008,'steel',0,.002)
    tube(name+' | gauntlet edge inlay',[(a,y-.024,b) for a,b in outline],.0013,'gold',True,5)
    tube(name+' | dorsal raised keel',[(cx,y-.031,z-.005),(cx,y-.035,z-.032),(cx,y-.029,z-.064)],.0011,'gold',False,5)
    # Thumb is on the anatomical inner side; slight curling leaves each finger readable.
    finger_offsets=[-.025,-.008,.009,.025]
    lengths=[.054,.067,.065,.049]
    for i,(offset,length) in enumerate(zip(finger_offsets,lengths)):
        root=Vector((cx+offset,y+.001,z-.066-(.003 if i in (0,3) else 0)))
        drift=side*.003+(i-1.5)*.001
        points=[root,root+Vector((drift,.003,-length*.36)),root+Vector((drift*1.5,.010,-length*.73)),root+Vector((drift*1.8,.018,-length))]
        for j in range(3):
            r=.0082-j*.0008
            bone(name+' | finger %d leather segment %d'%(i,j),points[j],points[j+1],[(0,r,r),(.15,r*1.06,r),(.80,r*.91,r*.93),(1,r*.78,r*.86)],'leather',8,0)
            a,b,u,v=_basis(points[j],points[j+1])
            verts=[];uv=[];n=6
            for t in [0,.08,.88,1]:
                c=a.lerp(b,t)
                for k in range(n+1):
                    aa=.15+(pi-.30)*k/n
                    p=c+u*((r+.001)*cos(aa))+v*((r+.0015)*sin(aa))
                    verts.append(tuple(p));uv.append((k/n,t))
            faces=[]
            for row in range(3):
                for k in range(n):
                    q=row*(n+1)+k;faces.append((q,q+1,q+n+2,q+n+1))
            mesh_object(name+' | overlapping finger lame %d-%d'%(i,j),verts,faces,'steel',uv,0,.001)
        bolt(name+' | knuckle rivet '+str(i),(root.x,y-.009,root.z+.004),.0035,'steel')
    thumb=[Vector((cx-side*.030,y+.001,z-.021)),Vector((cx-side*.052,y-.001,z-.046)),Vector((cx-side*.047,y+.013,z-.074)),Vector((cx-side*.040,y+.020,z-.090))]
    for j in range(3):
        r=.011-j*.0018
        bone(name+' | relaxed thumb '+str(j),thumb[j],thumb[j+1],[(0,r,r),(.16,r,r),(.78,r*.92,r*.95),(1,r*.79,r*.87)],'leather',8,0)
        p=thumb[j].lerp(thumb[j+1],.45)
        bolt(name+' | thumb articulation '+str(j),(p.x,p.y-r-.001,p.z),r*.7,'steel')
    # Scale the complete glove from its attached wrist, including every plate and digit.
    pivot=Vector(wrist)
    for obj in set(model.objects)-original_objects:
        if obj.type=='MESH':
            for vert in obj.data.vertices:
                vert.co=pivot+(vert.co-pivot)*1.20
            obj.data.update()


def _sabatons(name,ankle,side):
    x,y,z=ankle
    turn=-side*.10
    def world(xx,yy,zz):
        return (x+xx*cos(turn)-yy*sin(turn),y+xx*sin(turn)+yy*cos(turn),zz)
    sections=[(.063,.041,.083),(.051,.048,.097),(.0,.057,.132),(-.049,.056,.113),(-.105,.050,.082),(-.156,.037,.060),(-.198,.020,.044),(-.214,.012,.039)]
    n=16;verts=[];uv=[]
    for j,(yy,w,top) in enumerate(sections):
        for i in range(n):
            aa=2*pi*i/n
            zz=.039+(top-.039)*max(0,sin(aa))+.012*min(0,sin(aa))
            verts.append(world(w*cos(aa),yy,zz));uv.append((i/n,j/(len(sections)-1)))
    faces=[]
    for j in range(len(sections)-1):
        for i in range(n):
            k=j*n+i;faces.append((k,j*n+(i+1)%n,(j+1)*n+(i+1)%n,k+n))
    faces += [tuple(reversed(range(n))),tuple((len(sections)-1)*n+i for i in range(n))]
    mesh_object(name+' | fitted leather sole and boot',verts,faces,'leather',uv,0)
    # Separate overlapping curved plates cover the instep and toes, not merely painted lines.
    for j in range(1,7):
        yy0,w0,h0=sections[j];yy1,w1,h1=sections[j+1]
        vs=[];uv=[];nn=14
        for row,t in enumerate([0,.075,.90,1]):
            yy=yy0+(yy1-yy0)*t-.003;w=w0+(w1-w0)*t+.0015;top=h0+(h1-h0)*t+.003
            for k in range(nn+1):
                aa=pi*k/nn
                vs.append(world(w*cos(aa),yy,.040+(top-.040)*sin(aa)));uv.append((k/nn,t))
        fs=[]
        for row in range(3):
            for k in range(nn):
                q=row*(nn+1)+k;fs.append((q,q+1,q+nn+2,q+nn+1))
        mesh_object(name+' | articulated toe plate '+str(j),vs,fs,'steel',uv,0,.002)
        seam=[world((w1+.002)*cos(pi*k/14),yy1-.004,.040+(h1+.005-.040)*sin(pi*k/14)) for k in range(15)]
        tube(name+' | toe rolled border '+str(j),seam,.0013,'steel',False,5)
    loft(name+' | fitted heel and ankle cuff',[(x,y,.070,.046,.048),(x,y,.080,.046,.049),(x,y,.142,.038,.043),(x,y,.159,.038,.042)],'steel',18,0)
    ring_trim(name+' | ankle brass welt',(x,y,.159,.039,.043),.0015,'gold')


def build():
    for side in [-1,1]:
        name='Right' if side<0 else 'Left'
        sh=Vector((side*.275,0,1.515));el=Vector((side*.38,-.01,1.18));wr=Vector((side*.43,-.025,.945))
        sleeve_top=sh.lerp(el,.49)
        _cloth(name+' arm | gathered tan upper sleeve',sleeve_top,el.lerp(wr,.13),[(0,.061,.060),(.25,.065,.063),(.65,.057,.055),(1,.054,.051)],folds=.0075,rows=17)
        # A doubled woven edge falls irregularly across the elbow, matching the reference.
        _cloth(name+' arm | folded sleeve hem',sh.lerp(el,.83),el.lerp(wr,.025),[(0,.061,.058),(.13,.064,.060),(.87,.062,.059),(1,.059,.056)],folds=.003,rows=6)
        _cloth(name+' arm | tan forearm arming sleeve',el,wr,[(0,.052,.049),(.27,.051,.046),(.66,.039,.037),(1,.029,.029)],folds=.0045,rows=15)
        bone(name+' arm | mail elbow gusset',sh.lerp(el,.90),el.lerp(wr,.15),[(0,.054,.052),(.50,.054,.052),(1,.052,.049)],'mail',20,0)
        _mail_patch(name+' arm | elbow mail',sh,el,.93,1.015,.056,.053,cols=8,rows=2)
        start=el.lerp(wr,.11);end=el.lerp(wr,1.04)
        surface,mid=_shell(name+' arm | pointed traced vambrace',start,end,.057,.053,.034,.034,side,wrap=1.11,point=.12)
        _surface_line(name+' arm | central Gothic spear',surface,[(.035,mid),(.31,mid-.13*side),(.65,mid+.08*side),(.97,mid)],radius=.0017)
        for sign in [-1,1]:
            _surface_line(name+' arm | swept tracery '+str(sign),surface,[(.035,mid),(.23,mid+sign*.55),(.40,mid+sign*.39),(.65,mid+sign*.25),(.97,mid)],radius=.0016)
            _surface_line(name+' arm | crossing tracery '+str(sign),surface,[(.23,mid+sign*.55),(.43,mid-sign*.28),(.60,mid+sign*.29)],radius=.0015)
        for j,t in enumerate([.30,.73]):
            f=(t-.27)/(.66-.27) if t<=.66 else (t-.66)/(1-.66)
            rx=(.051+(.039-.051)*f) if t<=.66 else (.039+(.029-.039)*f)
            ry=(.046+(.037-.046)*f) if t<=.66 else (.037+(.029-.037)*f)
            rx+=.005;ry+=.005
            aa,bb,u,v=_basis(el,wr)
            strap_verts=[];strap_uv=[];nn=28
            for row,tt in enumerate([t-.038,t+.038]):
                for ii in range(nn+1):
                    q=mid+1.16+(2*pi-2.32)*ii/nn
                    p=aa.lerp(bb,tt)+u*(rx*cos(q))+v*(ry*sin(q))
                    strap_verts.append(tuple(p));strap_uv.append((ii/nn,row))
            strap_faces=[(ii,ii+1,ii+nn+2,ii+nn+1) for ii in range(nn)]
            mesh_object(name+' arm | leather securing strap '+str(j),strap_verts,strap_faces,'leather',strap_uv,0,.0015)
            _buckle(name+' arm | buckle '+str(j),el,wr,t,rx,ry,pi-.22 if side>0 else .22)
        _gauntlet(name+' hand',tuple(wr),side)

        # Slight sagittal asymmetry keeps the stance planted without changing the reference's relaxed pose.
        hip=Vector((side*.105,.015,1.01))
        knee=Vector((side*.130,-.010+(.021 if side>0 else -.008),.57))
        ankle=Vector((side*.145,.015+(.030 if side>0 else -.012),.13))
        _cloth(name+' leg | tailored tan chausses',hip,knee,[(0,.078,.077),(.36,.077,.076),(.78,.057,.055),(1,.053,.054)],folds=.003,rows=15)
        _cloth(name+' leg | knee and calf articulation',knee,ankle,[(0,.051,.051),(.24,.048,.051),(.60,.039,.042),(1,.027,.032)],folds=.0025,rows=15)
        upper=hip.lerp(knee,.16);lower=hip.lerp(knee,.88)
        thigh,mid=_shell(name+' leg | fitted cuisse',upper,lower,.085,.081,.060,.063,side,wrap=1.62,point=.045)
        _surface_line(name+' leg | cuisse fluted keel',thigh,[(.04,mid),(.34,mid-.06),(.71,mid+.05),(.96,mid)],radius=.0013)
        for sign in [-1,1]:
            _surface_line(name+' leg | cuisse pointed inlay '+str(sign),thigh,[(.05,mid),(.27,mid+sign*.43),(.53,mid+sign*.23),(.96,mid)],radius=.0012)
        # Angular, low-relief poleyn follows the kneecap; it is not a spherical joint.
        x,y,z=knee
        outline=[(x-.047,z+.043),(x,z+.062),(x+.047,z+.043),(x+.064+(max(side,0)*.018),z+.006),(x+.042,z-.041),(x,z-.060),(x-.042,z-.041),(x-.064+(min(side,0)*.018),z+.006)]
        plate(name+' knee | sculpted angular poleyn',outline,y-.054,.019,'steel',0,.003)
        tube(name+' knee | brass rolled outline',[(xx,y-.054,zz) for xx,zz in outline],.0015,'gold',True,5)
        tube(name+' knee | medial pointed engraving',[(x,y-.076,z+.046),(x+side*.021,y-.074,z+.003),(x,y-.076,z-.043)],.0012,'gold',False,5)
        bolt(name+' knee | lateral hinge',(x+side*.061,y-.058,z+.009),.0042,'gold')
        # Mail peeks out below the cuisse, above the knee shell.
        bone(name+' knee | mail gusset',hip.lerp(knee,.92),knee.lerp(ankle,.12),[(0,.055,.054),(.50,.056,.055),(1,.053,.055)],'mail',20,0)
        _mail_patch(name+' knee | linked mail',hip,knee,.92,.99,.057,.056,cols=9,rows=2)
        for t in [-.09,.11]:
            c=knee.lerp(ankle,t)
            bone(name+' knee | articulating lame '+str(t),c+Vector((0,0,.017)),c-Vector((0,0,.017)),[(0,.057,.055),(.12,.058,.057),(.88,.055,.054),(1,.054,.053)],'steel',18,0)
        top=knee.lerp(ankle,.16)
        levels=[]
        for t,rx,ry in [(0,.054,.057),(.04,.059,.062),(.23,.062,.067),(.44,.056,.062),(.75,.042,.047),(.97,.035,.041),(1,.036,.042)]:
            c=top.lerp(ankle,t);levels.append((c.x,c.y,c.z,rx,ry))
        # loft goes bottom→top for the UV convention; smooth normals retain its anatomical calf.
        loft(name+' shin | ridged close-fitting greave',list(reversed(levels)),'steel',24,0,.008)
        for idx in [0,-1]:ring_trim(name+' shin | narrow brass lip '+str(idx),levels[idx],.0015,'gold',.008)
        points=[]
        for t in [.02,.16,.36,.60,.83,.98]:
            c=top.lerp(ankle,t);ry=.061*(1-t)+.042*t+.008
            points.append((c.x,c.y-ry,c.z))
        tube(name+' shin | fluted gold medial seam',points,.0013,'gold',False,5)
        bone(name+' ankle | continuous leather articulation',(ankle.x,ankle.y,.075),(ankle.x,ankle.y,.19),[(0,.033,.039),(.4,.035,.040),(1,.036,.042)],'leather',16,0)
        _sabatons(name+' foot',tuple(ankle),side)
