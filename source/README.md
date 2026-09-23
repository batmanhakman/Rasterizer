# Editable knight source

`knight.blend` is the editable Blender project for the bundled knight. Open it in Blender to inspect the mesh, materials, camera, and lighting. The color atlas is packed into the project.

The Blender scene uses Z-up, with the knight facing negative Y. Its OBJ export uses Y-up and faces negative Z, matching the viewer's import convention. The runtime OBJ, MTL, and PNG are in `../assets/knight/`.

To regenerate the source project and exported assets using Blender:

```powershell
blender --background --python tools/create_knight_blender.py
```

Run that command from the repository root, with Blender available on PATH (or replace `blender` with the full path to `blender.exe`). Blender is only needed to edit or regenerate the asset; building and running the rasterizer does not require it.

The model is a static display asset. Its separate armor surfaces and cloth are editable; it does not include a character rig.
