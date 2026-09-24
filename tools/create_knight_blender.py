#!/usr/bin/env python3
"""Build the Gilded Warden reference-inspired knight in Blender 5.x.

Blender: Z up/front -Y. Export: Y up/front -Z. Run from any directory:
  blender --background --python tools/create_knight_blender.py
Use -- --preview-only --parts helmet --output build/helmet.png for component QA.
"""
import sys, argparse, importlib, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(Path(__file__).resolve().parent))
from knight_parts.common import *
argsv=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
parser=argparse.ArgumentParser()
parser.add_argument('--preview-only',action='store_true')
parser.add_argument('--parts',default='helmet,body,limbs')
parser.add_argument('--portrait',action='store_true')
parser.add_argument('--output',default='build/knight-blender.png')
args=parser.parse_args(argsv)
for part in args.parts.split(','):
    importlib.import_module('knight_parts.'+part.strip()).build()

scene=bpy.context.scene
world=bpy.data.worlds.new('Neutral charcoal studio');scene.world=world;world.use_nodes=True
world.node_tree.nodes['Background'].inputs[0].default_value=(.018,.020,.023,1)
world.node_tree.nodes['Background'].inputs[1].default_value=.35

def look_at(obj,target):obj.rotation_euler=(Vector(target)-obj.location).to_track_quat('-Z','Y').to_euler()
def area_light(name,position,power,color,size,target=(0,0,1.3)):
    data=bpy.data.lights.new(name,'AREA');data.energy=power;data.color=color;data.shape='DISK';data.size=size
    obj=bpy.data.objects.new(name,data);scene.collection.objects.link(obj);obj.location=position;look_at(obj,target)
area_light('Key | broad neutral softbox',(-3,-4,4.5),650,(1,.96,.89),3.0)
area_light('Fill | restrained silver',(3,-2,2.5),210,(.84,.88,1),2.5)
area_light('Rim | edge on forged plates',(1,2.5,3.5),800,(1,.94,.82),2.2)
camdata=bpy.data.cameras.new('Asset camera');cam=bpy.data.objects.new('Asset camera',camdata);scene.collection.objects.link(cam)
camdata.type='ORTHO';scene.camera=cam
head_only=args.parts.strip()=='helmet'
if head_only:
    cam.location=(1.7,-5,2.35);look_at(cam,(0,-.01,1.86));camdata.ortho_scale=.57
elif args.portrait:
    cam.location=(2.1,-6.5,3.0);look_at(cam,(0,-.025,1.51));camdata.ortho_scale=1.39
else:
    cam.location=(2.0,-6.7,2.6);look_at(cam,(0,-.015,1.055));camdata.ortho_scale=2.34
# Studio ground is deliberately outside the export collection.
mesh=bpy.data.meshes.new('Studio floor');mesh.from_pydata([(-200,-200,.007),(200,-200,.007),(200,200,.007),(-200,200,.007)],[],[(0,1,2,3)])
ground=bpy.data.objects.new('Studio floor | not exported',mesh);scene.collection.objects.link(ground)
mat=bpy.data.materials.new('Charcoal matte floor');mat.use_nodes=True
mat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.012,.015,.020,1)
mat.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.97
ground.data.materials.append(mat)
scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=True
scene.render.resolution_x=960;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
scene.view_settings.view_transform='AgX'
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(ROOT/args.output)
(ROOT/'source').mkdir(exist_ok=True);(ROOT/'build').mkdir(exist_ok=True)
scene['asset']='Gilded Warden | reference-inspired ornate plate harness'
scene['export_axes']='OBJ Y up, front -Z'
scene['construction_notes']='Shaped steel shells, traced gold inlays, layered cloth and chainmail; editable individual surfaces.'
bpy.ops.object.select_all(action='DESELECT')
for screen in bpy.data.screens:
    for a in screen.areas:
        if a.type=='VIEW_3D':
            a.spaces.active.region_3d.view_distance=3.0
            a.spaces.active.region_3d.view_location=(0,0,1.06)
            a.spaces.active.region_3d.view_rotation=cam.rotation_euler.to_quaternion()
            a.spaces.active.shading.type='MATERIAL'

def export_obj():
    depsgraph=bpy.context.evaluated_depsgraph_get()
    positions=[];texcoords=[];normals=[];pids={};uids={};nids={};faces=[];triangles=0
    def indexed(values,table,records):
        key=tuple(round(float(v),7) for v in values)
        if key not in table:table[key]=len(records)+1;records.append(key)
        return table[key]
    for obj in model.objects:
        if obj.type!='MESH':continue
        evaluated=obj.evaluated_get(depsgraph)
        mesh=evaluated.to_mesh(preserve_all_data_layers=True,depsgraph=depsgraph)
        mesh.calc_loop_triangles();uv=mesh.uv_layers.active.data
        faces.append('o '+obj.name.replace(' ','_').replace('|',''))
        faces.append('usemtl '+obj.data.materials[0].name)
        normal_matrix=obj.matrix_world.to_3x3().inverted_safe().transposed()
        for tri in mesh.loop_triangles:
            indices=[]
            for li in reversed(tri.loops):
                vi=mesh.loops[li].vertex_index;p=obj.matrix_world@mesh.vertices[vi].co
                n=normal_matrix@mesh.corner_normals[li].vector;n.normalize();t=uv[li].uv
                a=indexed((p.x,p.z,p.y),pids,positions);b=indexed((t.x,t.y),uids,texcoords);c=indexed((n.x,n.z,n.y),nids,normals)
                indices.append('%d/%d/%d'%(a,b,c))
            faces.append('f '+' '.join(indices));triangles+=1
        evaluated.to_mesh_clear()
    lines=['# Gilded Warden | original reference-inspired Blender model','mtllib knight.mtl']
    lines+=['v %.7f %.7f %.7f'%v for v in positions]
    lines+=['vt %.7f %.7f'%uv for uv in texcoords]
    lines+=['vn %.7f %.7f %.7f'%n for n in normals]
    (OUT/'knight.obj').write_text('\n'.join(lines+faces)+'\n',encoding='utf-8')
    mtl=[]
    for name,(kd,ks,ns,metal,rough) in MATERIAL_SPECS.items():
        mtl+=['newmtl '+name,'Ka 0.15 0.15 0.15','Kd %s %s %s'%kd,'Ks %s %s %s'%ks,'Ns '+str(ns),'illum 2','map_Kd '+('knight_mail.png' if name=='mail' else 'knight_atlas.png'),'']
    (OUT/'knight.mtl').write_text('\n'.join(mtl),encoding='utf-8')
    stats={'positions':len(positions),'triangles':triangles,'components':len(model.objects),'materials':len(MATERIAL_SPECS)}
    (ROOT/'build'/'knight-stats.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
    print('EXPORTED '+json.dumps(stats),flush=True)
if not args.preview_only:
    bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'source'/'knight.blend'))
    export_obj()
bpy.ops.render.render(write_still=True)
print('PREVIEW '+scene.render.filepath,flush=True)
