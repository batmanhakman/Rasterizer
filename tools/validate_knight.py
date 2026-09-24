"""Validate the exported OBJ independently of the renderer, without dependencies."""
from pathlib import Path
import math
import sys

path = Path(sys.argv[1]) if len(sys.argv)>1 else Path(__file__).resolve().parents[1]/'assets/knight/knight.obj'
positions, texcoords, normals, triangles = [], [], [], []
materials = set()
current = None
for line_number, line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(), 1):
    fields = line.split()
    if not fields or fields[0].startswith('#'):
        continue
    kind = fields[0]
    if kind in ('v', 'vt', 'vn'):
        values = tuple(float(x) for x in fields[1:])
        if not all(math.isfinite(x) for x in values):
            raise ValueError(f'{path}:{line_number}: non-finite {kind}')
        {'v':positions, 'vt':texcoords, 'vn':normals}[kind].append(values)
    elif kind == 'usemtl':
        current = ' '.join(fields[1:]); materials.add(current)
    elif kind == 'f':
        if len(fields)!=4:
            raise ValueError(f'{path}:{line_number}: expected triangulated geometry')
        corners=[]
        for token in fields[1:]:
            indices=token.split('/')
            if len(indices)!=3 or not all(indices):
                raise ValueError(f'{path}:{line_number}: missing UV/normal index')
            corner=tuple(int(index)-1 for index in indices)
            if any(i<0 or i>=len(items) for i,items in zip(corner,(positions,texcoords,normals))):
                raise ValueError(f'{path}:{line_number}: invalid index')
            corners.append(corner)
        if not current:
            raise ValueError(f'{path}:{line_number}: missing material')
        triangles.append(corners)
assert len(triangles)>100, 'No usable model geometry'
assert all(len(v)==3 for v in positions+normals), 'Invalid vector dimensions'
assert all(len(uv)>=2 for uv in texcoords), 'Invalid UV dimensions'
# Atlas regions stay inset; the standalone chainmail texture intentionally repeats.
assert all(abs(sum(x*x for x in n)-1)<0.002 for n in normals), 'Non-unit normal'
lo=[min(v[k] for v in positions) for k in range(3)]
hi=[max(v[k] for v in positions) for k in range(3)]
print(f'{len(positions):,} positions, {len(triangles):,} triangles, {len(materials)} materials')
print('Bounds: '+str(lo)+' -> '+str(hi))
print('All exported face indices, UVs, normals, and numeric values are valid.')
