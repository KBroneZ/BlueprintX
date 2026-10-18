"""Render the Blueprint-X mod icon (ficsit.app logo) from the corner marker model.

Run (no Blender UI needed), after SourceArt/Marker/build_corner_marker.py has written the .blend:
    blender -b --factory-startup --python render_mod_icon.py

Writes next to this script:
    BlueprintX_Icon_512.png - square 512 x 512 PNG for the ficsit.app mod page

The scene shows what the mod does: two corner markers at opposite corners of an area, the
orange box between them and a few grey blocks (the buildings that get saved). ficsit.app accepts any PNG/JPEG/GIF and converts it to WebP without
resizing (decision D-023), so a square 512 x 512 PNG is used.
"""
import os

import bmesh
import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
BLEND_PATH = os.path.join(HERE, "..", "Marker", "SM_BXCornerPost.blend")
PNG_PATH = os.path.join(HERE, "BlueprintX_Icon_512.png")
MARKER_NAME = "SM_BXCornerPost"

SIZE = 512
BACKGROUND = (0.035, 0.045, 0.06)
GROUND_COLOR = (0.11, 0.12, 0.14, 1.0)
BEAM_COLOR = (1.0, 0.35, 0.05, 1.0)
BUILDING_COLOR = (0.55, 0.57, 0.6, 1.0)

# Simple grey blocks inside the box, standing for the buildings that get saved:
# (x0, x1, y0, y1, z0, z1) in Unreal axes, metres.
BUILDINGS = (
    (0.7, 3.3, 0.7, 3.3, 0.0, 0.4),
    (1.1, 2.3, 1.0, 2.6, 0.4, 2.0),
    (2.5, 3.1, 1.4, 2.9, 0.4, 1.2),
)

# Box in Unreal axes (X forward, Y right, Z up), metres; the markers stand on two opposite corners.
BOX_X = 4.0
BOX_Y = 4.0
BOX_HEIGHT = 3.4
BEAM_HALF = 0.05


def to_blender(x, y, z):
    """Unreal (x, y, z) -> Blender (x, -y, z), same mapping as the marker script."""
    return Vector((x, -y, z))


def make_material(name, color):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    return mat


def add_beam(bm, start, end):
    """Axis-aligned box from start to end (Unreal axes), BEAM_HALF thick."""
    low = [min(a, b) - BEAM_HALF for a, b in zip(start, end)]
    high = [max(a, b) + BEAM_HALF for a, b in zip(start, end)]
    corners = [(x, y, z) for x in (low[0], high[0]) for y in (low[1], high[1]) for z in (low[2], high[2])]
    verts = [bm.verts.new(to_blender(*c)) for c in corners]
    for face in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
        bm.faces.new([verts[i] for i in face])


def mesh_object(name, bm, material):
    mesh = bpy.data.meshes.new(name)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.to_mesh(mesh)
    bm.free()
    mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def build_box():
    bm = bmesh.new()
    xs, ys, zs = (0.0, BOX_X), (0.0, BOX_Y), (0.0, BOX_HEIGHT)
    for y in ys:
        for z in zs:
            add_beam(bm, (xs[0], y, z), (xs[1], y, z))
    for x in xs:
        for z in zs:
            add_beam(bm, (x, ys[0], z), (x, ys[1], z))
    for x in xs:
        for y in ys:
            add_beam(bm, (x, y, zs[0]), (x, y, zs[1]))
    mesh_object("BoxBeams", bm, make_material("Beam", BEAM_COLOR))


def build_buildings():
    bm = bmesh.new()
    for x0, x1, y0, y1, z0, z1 in BUILDINGS:
        corners = [(x, y, z) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]
        verts = [bm.verts.new(to_blender(*c)) for c in corners]
        for face in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)):
            bm.faces.new([verts[i] for i in face])
    mesh_object("Buildings", bm, make_material("Building", BUILDING_COLOR))


def build_ground():
    bm = bmesh.new()
    margin = 40.0
    verts = [bm.verts.new(to_blender(x, y, -0.01)) for x, y in
             ((-margin, -margin), (margin, -margin), (margin, margin), (-margin, margin))]
    bm.faces.new(verts)
    mesh_object("Ground", bm, make_material("Ground", GROUND_COLOR))


def place_markers():
    marker = bpy.data.objects[MARKER_NAME]
    marker.location = to_blender(0.0, 0.0, 0.0)
    second = marker.copy()
    bpy.context.scene.collection.objects.link(second)
    second.location = to_blender(BOX_X, BOX_Y, 0.0)


def setup_render():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_shadows = True
    scene.display.shading.show_cavity = True
    scene.display.render_aa = "32"
    scene.render.resolution_x = SIZE
    scene.render.resolution_y = SIZE
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = PNG_PATH
    scene.world = scene.world or bpy.data.worlds.new("World")
    scene.world.color = BACKGROUND

    cam_data = bpy.data.cameras.new("IconCamera")
    cam_data.lens = 35
    cam = bpy.data.objects.new("IconCamera", cam_data)
    scene.collection.objects.link(cam)
    # Three-quarter view from -X/+Y, a little above the box: one marker left, one right.
    cam.location = to_blender(-2.6, 6.8, 6.4)
    target = to_blender(BOX_X * 0.5, BOX_Y * 0.5, 1.2)
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam


def main():
    bpy.ops.wm.open_mainfile(filepath=BLEND_PATH)
    place_markers()
    build_box()
    build_buildings()
    build_ground()
    setup_render()
    bpy.ops.render.render(write_still=True)
    print(f"BXICON path={PNG_PATH}")


main()
