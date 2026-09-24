# Gilded Warden texture provenance

The two diffuse texture images were created with the image generation tool for this asset. Each PNG is 1,254 × 1,254 pixels. Both are packed into `source/knight.blend` and supplied separately beside the exported OBJ/MTL. The Blender builder consumes these images as source inputs; it does not generate or replace them.

The privately supplied knight screenshot was a visual design reference for the original geometry. That reference image is not included in this repository. The prompts below describe material swatches, not a copy of the screenshot.

## `knight_atlas.png`

The atlas has four equal material regions: steel at top left, antique brass/gold at top right, tan cloth at bottom left, and dark leather at bottom right. Geometry UVs stay inset within the selected region. The final generation prompt was:

```text
A usable FLAT 2D game ALBEDO texture atlas, square, with exactly four equal quadrants at the center horizontally and vertically. NO knight or objects. NO perspective. Each quadrant must be mostly a quiet uniform material color, 95 percent smooth color and only 5 percent SUBTLE fine details. TOP LEFT smooth neutral gray steel (average RGB 140,139,133), satin forged plate armor at normal viewing distance with only a FEW faint hairline scuffs. Very low contrast micro-detail. NO cloudy mottling, NO pits, NO dark cracks, NO visible grit, NO concrete/stone texture. TOP RIGHT softly worn antique brass (average RGB 158,130,84), smooth almost uniform albedo, extremely faint fine scratches, no heavy tarnish clouds and no ornaments. BOTTOM LEFT warm tan tightly woven cloth (average RGB145,129,104), subtle very fine cloth grain, NO chunky knit or carpet pattern. BOTTOM RIGHT dark umber smooth leather (average RGB65,48,34), barely visible fine grain. These are flat unlit material-color swatches for realistic medieval armor, not macro material photos. Do not bake specular shine, white light reflections, gradients, light sources or shadows. Do not put bevels, borders, margins, labels, emblems, letters or seams between the four squares. Exact 2x2 grid filling canvas. The most important requirement is clean restrained mostly smooth STEEL and BRASS, not noisy weathered stone.
```

## `knight_mail.png`

The mail material uses a separate repeating image so it can have its own scale on collars, exposed joints, and the skirt. The final generation prompt was:

```text
Asset type: seamless flat DIFFUSE texture for small medieval chainmail on a 3D knight. Square evenly lit orthographic chainmail material swatch filling whole image. Dense historically recognizable interlocking flattened steel rings in orderly alternating offset rows, about 40 small rings across image and 40 rows high. Metal dark neutral gray tarnished iron with delicate pale silver highlights on ring edges, very dark tiny holes between overlapping rings. Fine rings thin wire, consistent realistic four-in-one chainmail weave, not large jewelry chains, no fabric knit, no fish scales. Subtle wear. No objects, no armor outline, no hands, no gradients, no scene/background, no text, no border, no diagonal perspective. Pattern tiles seamlessly on all four edges. Dense tiny readable rings suitable for a realistic medieval knight's collar, elbow gussets, and mail skirt.
```

The geometry supplies the plate contours, borders, embossed tracery, seams, and selected chain-link relief. Material highlights come from the Blender shaders or the software renderer's `Ks`/`Ns` lighting.
