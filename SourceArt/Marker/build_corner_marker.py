"""Build the Blueprint-X Corner marker model in Blender and export it to FBX.

Run (no Blender UI needed):
    blender -b --factory-startup --python build_corner_marker.py

Writes next to this script:
    SM_BXCornerPost.blend  - the model, for editing by hand later
    SM_BXCornerPost.fbx    - the file imported into the Unreal editor
    SM_BXCornerPost.png    - a quick preview render (not committed)

Layout, in Unreal axes (X forward, Y right, Z up), sizes in metres:
    - pivot at the base centre: this point is the corner of the Blueprint-X box
    - dark octagonal base plate, 0.84 m across, fits one 1 m grid cell
    - orange post with two dark bands and an orange diamond beacon on top, 3 m high in total
    - four orange floor arrows, one per direction: the box can grow from the corner either way,
      so the marker is symmetric and does not suggest a side
    - simple box collision (UCX_ meshes)

Blender is Z up and right-handed; the FBX exporter plus the Unreal importer map
Blender (x, y, z) to Unreal (x, -y, z), so Unreal +Y is built along Blender -Y.
"""
import math
import os

import bmesh
import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
NAME = "SM_BXCornerPost"
BLEND_PATH = os.path.join(HERE, NAME + ".blend")
FBX_PATH = os.path.join(HERE, NAME + ".fbx")
PNG_PATH = os.path.join(HERE, NAME + ".png")

# Material slot order matters: slot 0 (orange) is also used by the box preview beams.
ORANGE = ("M_BXCorner", (1.0, 0.25, 0.02, 1.0))
DARK = ("M_BXCornerDark", (0.03, 0.03, 0.035, 1.0))
SLOT_ORANGE = 0
SLOT_DARK = 1

# Sizes in metres.
TOTAL_HEIGHT = 3.0
BASE_RADIUS = 0.42  # octagon circumradius
BASE_HEIGHT = 0.08
POST_HALF = 0.07
BAND_HALF = 0.08
BAND_HEIGHT = 0.12
BAND_Z = (1.0, 2.0)
ARROW_Z = (BASE_HEIGHT, BASE_HEIGHT + 0.05)
ARROW_HALF_WIDTH = 0.05
ARROW_SHAFT_END = 0.30
ARROW_HEAD_HALF_WIDTH = 0.12
ARROW_TIP = 0.46
BEACON_HALF = 0.2   # half width at the beacon's widest point
BEACON_BOTTOM = 2.45
BEACON_MIDDLE = 2.72


def to_blender(x, y, z):
    """Unreal-axis point (metres) to Blender coordinates."""
    return (x, -y, z)


def add_prism(bm, outline, z0, z1, slot):
    """Extrude a 2D outline (list of (x, y) in Unreal axes) from z0 to z1."""
    bottom = [bm.verts.new(to_blender(x, y, z0)) for x, y in outline]
    top = [bm.verts.new(to_blender(x, y, z1)) for x, y in outline]
    faces = [bm.faces.new(bottom), bm.faces.new(top)]
    count = len(outline)
    for i in range(count):
        j = (i + 1) % count
        faces.append(bm.faces.new((bottom[i], bottom[j], top[j], top[i])))
    for face in faces:
        face.material_index = slot
    return faces


def add_box(bm, x0, x1, y0, y1, z0, z1, slot):
    return add_prism(bm, [(x0, y0), (x1, y0), (x1, y1), (x0, y1)], z0, z1, slot)


def add_diamond(bm, half, z_bottom, z_middle, z_top, slot):
    """Octahedron: a square ring at z_middle (corners on the X and Y axes) and two apexes."""
    ring = [bm.verts.new(to_blender(x, y, z_middle)) for x, y in
            ((half, 0.0), (0.0, half), (-half, 0.0), (0.0, -half))]
    bottom = bm.verts.new(to_blender(0.0, 0.0, z_bottom))
    top = bm.verts.new(to_blender(0.0, 0.0, z_top))
    for i in range(4):
        j = (i + 1) % 4
        for face in (bm.faces.new((ring[i], ring[j], top)), bm.faces.new((ring[j], ring[i], bottom))):
            face.material_index = slot


def arrow_outline(half_width, shaft_end, head_half_width, tip):
    """Arrow along +X starting at the post, as a 2D outline (concave, split later)."""
    return [(0.0, -half_width), (shaft_end, -half_width), (shaft_end, -head_half_width),
            (tip, 0.0), (shaft_end, head_half_width), (shaft_end, half_width), (0.0, half_width)]


def rotate_quarter(outline, turns):
    """Rotate a 2D outline by turns x 90 degrees around the pivot."""
    for _ in range(turns):
        outline = [(-y, x) for x, y in outline]
    return outline


def build_visual_mesh():
    bm = bmesh.new()

    octagon = [(BASE_RADIUS * math.cos(math.radians(22.5 + 45 * i)),
                BASE_RADIUS * math.sin(math.radians(22.5 + 45 * i))) for i in range(8)]
    add_prism(bm, octagon, 0.0, BASE_HEIGHT, SLOT_DARK)

    # The post reaches into the beacon so no gap shows between them.
    add_box(bm, -POST_HALF, POST_HALF, -POST_HALF, POST_HALF, BASE_HEIGHT, BEACON_MIDDLE, SLOT_ORANGE)
    add_diamond(bm, BEACON_HALF, BEACON_BOTTOM, BEACON_MIDDLE, TOTAL_HEIGHT, SLOT_ORANGE)
    for z in BAND_Z:
        add_box(bm, -BAND_HALF, BAND_HALF, -BAND_HALF, BAND_HALF, z, z + BAND_HEIGHT, SLOT_DARK)

    # Shafts start under the post, so the four arrows read as coming out of it.
    floor_arrow = arrow_outline(ARROW_HALF_WIDTH, ARROW_SHAFT_END, ARROW_HEAD_HALF_WIDTH, ARROW_TIP)
    for turns in range(4):
        add_prism(bm, rotate_quarter(floor_arrow, turns), ARROW_Z[0], ARROW_Z[1], SLOT_ORANGE)

    # Concave arrow caps and all quads become triangles here, so Unreal gets a clean mesh.
    bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method="BEAUTY", ngon_method="BEAUTY")
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    return bm


def make_material(name, color):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    bsdf = mat.node_tree.nodes.get("Principled BSDF") if mat.node_tree else None
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = color
    return mat


def mesh_object(name, bm):
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def collision_object(index, x0, x1, y0, y1, z0, z1):
    """Convex box named so Unreal uses it as simple collision for NAME."""
    bm = bmesh.new()
    add_box(bm, x0, x1, y0, y1, z0, z1, 0)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    obj = mesh_object(f"UCX_{NAME}_{index:02d}", bm)
    obj.display_type = "WIRE"
    obj.hide_render = True
    return obj


def build_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0

    visual = mesh_object(NAME, build_visual_mesh())
    for name, color in (ORANGE, DARK):
        visual.data.materials.append(make_material(name, color))

    collisions = [
        collision_object(0, -BASE_RADIUS, BASE_RADIUS, -BASE_RADIUS, BASE_RADIUS, 0.0, ARROW_Z[1]),
        collision_object(1, -BAND_HALF, BAND_HALF, -BAND_HALF, BAND_HALF, 0.0, TOTAL_HEIGHT),
        collision_object(2, -BEACON_HALF, BEACON_HALF, -BEACON_HALF, BEACON_HALF, BEACON_BOTTOM, TOTAL_HEIGHT),
    ]
    return visual, collisions


def export_fbx(objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.export_scene.fbx(
        filepath=FBX_PATH,
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_NONE",
        mesh_smooth_type="FACE",
        use_mesh_modifiers=True,
        add_leaf_bones=False,
        bake_anim=False,
        path_mode="STRIP",
    )


def render_preview(visual):
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.render.resolution_x = 768
    scene.render.resolution_y = 768
    scene.render.film_transparent = False
    scene.render.filepath = PNG_PATH

    cam_data = bpy.data.cameras.new("PreviewCamera")
    cam_data.lens = 40
    cam = bpy.data.objects.new("PreviewCamera", cam_data)
    scene.collection.objects.link(cam)
    # Three-quarter view from -X/-Y (Unreal axes).
    cam.location = to_blender(-4.6, -2.4, 2.6)
    target = to_blender(0.1, 0.1, 1.45)
    direction = [t - c for t, c in zip(target, cam.location)]
    cam.rotation_euler = Vector(direction).to_track_quat("-Z", "Y").to_euler()
    scene.camera = cam
    bpy.ops.render.render(write_still=True)


def main():
    visual, collisions = build_scene()
    triangles = sum(len(p.vertices) - 2 for p in visual.data.polygons)
    export_fbx([visual] + collisions)
    bpy.context.preferences.filepaths.save_version = 0  # no .blend1 backup next to the model
    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH, compress=True)
    render_preview(visual)
    print(f"BXMODEL triangles={triangles} fbx={FBX_PATH} blend={BLEND_PATH}")


main()
