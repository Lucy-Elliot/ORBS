#include "raylib.h"
#include "raymath.h"
#include "Config.h"
#include "keyframe.h"
#include "log.h"
#include "fileio.h"
#include "smartcube.h"
#include "Theme.h"

#include <fstream>
#include <algorithm>
#include <vector>
#include <string>


void ExportPath(const std::vector<Keyframe>& path, const SmartCube& cube, int bgType, int colourScheme, char outputName[128], int atomStyle, float atomScale, float clipDistance, char custombackgroundcolour[32]) {
    std::vector<std::string> finalExportList;
    if (!foundSequenceFiles.empty()) {
        for (int i = seqStartIndex; i <= seqEndIndex; i++) {
            if (i >= 0 && i < (int)foundSequenceFiles.size()) finalExportList.push_back(foundSequenceFiles[i]);
        }
    } else { Log("ERROR: No files!"); return; }

    std::ofstream camFile("camera_data.txt");
    camFile << path.size() << "\n";
    for (const auto& kf : path) {
        camFile << kf.frameNumber << " " << kf.position.x << " " << kf.position.y << " " << kf.position.z << " "
                << kf.target.x << " " << kf.target.y << " " << kf.target.z << " "
                << kf.up.x << " " << kf.up.y << " " << kf.up.z << "\n";
    }
    camFile.close();

    std::ofstream py("render_job.py");
    py << "import bpy\nimport os\nimport math\nfrom mathutils import Vector, Quaternion\nfrom bpy.app.handlers import persistent\n\n";
    py << "file_sequence = [\n";
    for(const auto& f : finalExportList) {
        std::string cp = f; std::replace(cp.begin(), cp.end(), '\\', '/'); 
        py << "    r'" << cp << "',\n";
    }
    py << "]\n\noutputName = r'" << outputName << "'\nSELECTED_STYLE = " << atomStyle 
       << "\nGLOBAL_SCALE = " << atomScale << "\nCOLOUR_SCHEME = " << colourScheme 
       << "\nBACKGROUND_TYPE = " << bgType << "\nCLIP_DISTANCE = " << clipDistance << "\n";
       // shifting UI values for blender camera, should make it fully centered now.
    py << "UI_SHIFT_X = " << (float)BASE_SIDEBAR_WIDTH * globalFontScale / START_WIDTH << "\n";
    py << "UI_SHIFT_Y = " << (float)BASE_TERMINAL_HEIGHT * globalFontScale / START_HEIGHT << "\n";
    py << "CUBE_POS = (" << cube.position.x << ", " << -cube.position.z << ", " << cube.position.y << ")\n";
    py << "CUBE_SIZE = (" << cube.size.x << ", " << cube.size.z << ", " << cube.size.y << ")\n";
    py << "GRADIENT_MODE = " << cube.gradientMode << "\n";
    py << "ELECTRON_COUNT = 50\n";
    py << "custombackgroundcolour = (" << custombackgroundcolour << ")\n\n";
    py << R"(
bpy.ops.object.select_all(action='DESELECT')
bpy.ops.object.select_by_type(type='MESH')
bpy.ops.object.delete(use_global=False)

## The camera setup is a bit off center because of the sidebar and terminal, so we will fix that with some lens shifting. This was really annoying to figure out lol
cam = bpy.context.scene.camera
cam.data.type = 'PERSP'
cam.data.lens = 35

# We shift the lens so the 'center' moves to match your GUI viewport
cam.data.sensor_fit = 'HORIZONTAL'
cam.data.shift_x = -UI_SHIFT_X / 2.0 
cam.data.shift_y = UI_SHIFT_Y / 2.0


# global variables
FRAMES_PER_FILE = 5 
final_filename = outputName.replace('.mp4', '')
if not final_filename: final_filename = 'render_output'

def create_templates():
    bpy.ops.mesh.primitive_cone_add(vertices=16, radius1=0.4 * GLOBAL_SCALE, depth=0.6 * GLOBAL_SCALE)
    top = bpy.context.active_object
    top.location[2] = 0.6 * GLOBAL_SCALE
    
    bpy.ops.mesh.primitive_cylinder_add(vertices=16, radius=0.2 * GLOBAL_SCALE, depth=1.4 * GLOBAL_SCALE)
    base = bpy.context.active_object
    top.location[2] = 0.7 * GLOBAL_SCALE
    base.location[2] = -0.3 * GLOBAL_SCALE
    
    bpy.ops.object.select_all(action='DESELECT')
    top.select_set(True); base.select_set(True)
    bpy.context.view_layer.objects.active = base
    bpy.ops.object.join()
    
    template = bpy.context.active_object
    template.name = "TemplateArrow"
    
    #make the arrow rotate on center and point up the Z axis by default, also move it up a bit so the base is at the origin for better rotation
    template.location[2] += (0.4 * GLOBAL_SCALE)
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    template.location = (100000, 100000, 100000)

    # Atom Template!
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=0.6 * GLOBAL_SCALE)
    atom_tmpl = bpy.context.active_object
    atom_tmpl.name = "TemplateAtom"
    bpy.ops.object.shade_smooth()
    atom_tmpl.location = (1000, 1000, 1000)

    #Electrons template - emmission shader with sphere
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=0.1 * GLOBAL_SCALE)
    electron_tmpl = bpy.context.active_object
    electron_tmpl.name = "TemplateElectron"
    electron_tmpl.location = (10000, 10000, 1000000)

    el_mat = bpy.data.materials.new("ElectronMat")
    el_mat.use_nodes = True
    nodes_e = el_mat.node_tree.nodes
    nodes_e.clear()
    node_out = nodes_e.new("ShaderNodeOutputMaterial")
    nodes_em = nodes_e.new("ShaderNodeEmission")
    nodes_em.inputs[0].default_value = (0.5, 0.8, 1.0, 1.0) # light blue colour for electrons
    nodes_em.inputs[1].default_value = 5.0 # emission strength
    el_mat.node_tree.links.new(nodes_em.outputs[0], node_out.inputs[0])
    electron_tmpl.data.materials.append(el_mat)
    return template, atom_tmpl, electron_tmpl

arrow_tmpl, atom_tmpl, electron_tmpl = create_templates()

def setup_scene():
    bpy.ops.object.select_by_type(type='LIGHT'); bpy.ops.object.delete()
    if not file_sequence: return
    pos = []
    with open(file_sequence[0], 'r') as f: lines = f.readlines()
    s_idx = 1 if len(lines[0].split()) == 1 else 0
    for line in lines[s_idx:]:
        p = line.split()
        if len(p) >= 3: pos.append((float(p[0]), -float(p[2]), float(p[1])))
    
    mesh = bpy.data.meshes.new("DataMesh")
    obj = bpy.data.objects.new("VampireData", mesh)
    bpy.context.collection.objects.link(obj)
    mesh.from_pydata(pos, [], [])
    
    # Def Attributes 
    mesh.attributes.new(name="rot_q", type='FLOAT_VECTOR', domain='POINT')
    mesh.attributes.new(name="col", type='FLOAT_COLOR', domain='POINT')

    mod = obj.modifiers.new(name="GeoNodes", type='NODES')
    nt = bpy.data.node_groups.new("VampireTree", 'GeometryNodeTree')
    mod.node_group = nt
    nt.interface.new_socket(name="Geometry", in_out='OUTPUT', socket_type='NodeSocketGeometry')
    nt.interface.new_socket(name="Geometry", in_out='INPUT', socket_type='NodeSocketGeometry')

    nodes = nt.nodes; links = nt.links
    in_n = nodes.new("NodeGroupInput"); out_n = nodes.new("NodeGroupOutput")
    join_n = nodes.new("GeometryNodeJoinGeometry")
    
    # Color Attribute Node
    cap = nodes.new("GeometryNodeCaptureAttribute")
    cap.data_type = 'FLOAT_COLOR'; cap.domain = 'POINT'
    col_in = nodes.new("GeometryNodeInputNamedAttribute")
    col_in.data_type = 'FLOAT_COLOR'; col_in.inputs[0].default_value = "col"
    links.new(in_n.outputs[0], cap.inputs[0])
    links.new(col_in.outputs[0], cap.inputs[1])

    # Setup Rotation Attribute Node
    rot_in = nodes.new("GeometryNodeInputNamedAttribute")
    rot_in.data_type = 'FLOAT_VECTOR'
    rot_in.inputs[0].default_value = "rot_q"

    def branch(tmpl, use_rot):
        inst = nodes.new("GeometryNodeInstanceOnPoints")
        info = nodes.new("GeometryNodeObjectInfo"); info.inputs[0].default_value = tmpl
        links.new(cap.outputs[0], inst.inputs["Points"])
        links.new(info.outputs["Geometry"], inst.inputs["Instance"])
        if use_rot: 
            links.new(rot_in.outputs["Attribute"], inst.inputs["Rotation"])
        return inst

    if SELECTED_STYLE in [0, 2]: 
        links.new(branch(atom_tmpl, False).outputs[0], join_n.inputs[0])
        
    if SELECTED_STYLE in [1, 2]: 
        links.new(branch(arrow_tmpl, True).outputs[0], join_n.inputs[0])

    realize = nodes.new("GeometryNodeRealizeInstances")
    set_mat = nodes.new("GeometryNodeSetMaterial")
    
    # material setup --- add way to add in own .inc file l8r
    mat = bpy.data.materials.new("DataMat")
    mat.use_nodes = True
    nodes_m = mat.node_tree.nodes; nodes_m.clear()
    out_m = nodes_m.new("ShaderNodeOutputMaterial")
    em_m = nodes_m.new("ShaderNodeEmission")
    attr_m = nodes_m.new("ShaderNodeAttribute")
    attr_m.attribute_name = "col"
    mat.node_tree.links.new(attr_m.outputs["Color"], em_m.inputs["Color"])
    mat.node_tree.links.new(em_m.outputs["Emission"], out_m.inputs["Surface"])
    
    set_mat.inputs[2].default_value = mat
    links.new(join_n.outputs[0], realize.inputs[0])
    links.new(realize.outputs[0], set_mat.inputs[0])
    links.new(set_mat.outputs[0], out_n.inputs[0])

    #background colours 
    bg = bpy.context.scene.world.node_tree.nodes.get("Background")
    if BACKGROUND_TYPE == 0: bg.inputs[0].default_value = (0.02, 0.02, 0.02, 1)
    elif BACKGROUND_TYPE == 1: bg.inputs[0].default_value = (1, 1, 1, 1)
    elif BACKGROUND_TYPE == 2: bg.inputs[0].default_value = (0.3, 0, 0, 1)
    elif BACKGROUND_TYPE == 3: bg.inputs[0].default_value = custombackgroundcolour

    #Glowingness of electrons
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE' # force eevee for fast glows
    scene.eevee.use_bloom = True
    scene.eevee.bloom_intensity = 0.1  
    scene.eevee.bloom_threshold = 0.5

    #Electron summonning logic
    electron_group = bpy.data.collections.new("Electrons")
    bpy.context.scene.collection.children.link(electron_group)
    import random
    for i in  range(ELECTRON_COUNT):
        el = bpy.data.objects.new(f"Electron_{i}", electron_tmpl.data)
        el.location = [
        CUBE_POS[j] + (random.random() - 0.5) * CUBE_SIZE[j] for j in range(3)
        ]
        electron_group.objects.link(el)


@persistent
def update_handler(scene):
    obj = bpy.data.objects.get("VampireData")
    if not obj or not file_sequence: return
    progress = scene.frame_current / scene.frame_end
    with open("render_progress.txt", "w") as f:
        f.write(str(progress))
    
    idx = max(0, min(len(file_sequence)-1, (scene.frame_current - 1) // FRAMES_PER_FILE))
    try:
        with open(file_sequence[idx], 'r') as f: lines = f.readlines()
        s_idx = 1 if len(lines[0].split()) == 1 else 0
        mesh = obj.data
        rot_attr = mesh.attributes.get("rot_q")
        col_attr = mesh.attributes.get("col")
        
        for i, line in enumerate(lines[s_idx:]):
            if i >= len(mesh.vertices): break
            p = list(map(float, line.split()))
            
            # position update
            mesh.vertices[i].co = (p[0], -p[2], p[1])
            
            # Fluid Rotation Math -- doesnt really work lol uhhh
            axis = Vector((0.0, 1.0, 0.0))
            vec = Vector((p[3], -p[5], p[4]))
            if vec.length > 0.001:
                difference = axis.rotation_difference(vec.normalized()).to_euler()
                rot_attr.data[i].vector = (difference.x, difference.y, difference.z)
            
            # Color Scheme Math
            sz = p[5]
            sr, sg, sb = 1.0, 1.0, 1.0
            if abs(p[3]) + abs(p[5]) + abs(p[4]) == 0.0:
                sr, sg, sb = 0.0, 0.0, 0.0
            else:
                if COLOUR_SCHEME == 0:
                    z = abs(sz)
                    sr, sg, sb = ((1-z**1.5), (1-z), z) if sz >= -0.7 else (z, (1-z), (1-z**1.5))
                elif COLOUR_SCHEME == 1:
                    ir = sz * 0.5 + 0.5
                    pts = [(0,0,.56),(0,.05,1),(0,.56,1),(.05,1,.93),(.56,1,.43),(1,.93,0),(1,.43,0),(.93,0,0),(.49,0,0)]
                    sr, sg, sb = pts[max(0, min(8, int(ir * 8)))]
                elif COLOUR_SCHEME == 2:
                    frac = (sz + 1.0) / 2.0; sr, sg, sb = frac, frac * 0.8, 1.0 - frac
            
            col_attr.data[i].color = (sr, sg, sb, 1.0)
        mesh.update()
    except: pass

    #electron movement stuff
    speed = 1.0
    for i in range(ELECTRON_COUNT):
        el = bpy.data.objects.get(f"Electron_{i}")
        if el:
            if GRADIENT_MODE == 0:
                el.location.x += speed
            elif GRADIENT_MODE ==1:
                el.location.y += speed
            elif GRADIENT_MODE == 2:
                el.location.z += speed
            elif GRADIENT_MODE == 3:
                el.location.x -= speed
            elif GRADIENT_MODE ==4:
                el.location.y -= speed
            elif GRADIENT_MODE == 5:
                el.location.z -= speed
        for j in range(3):
            half = CUBE_SIZE[j] / 2
            if el.location[j] > CUBE_POS[j] +half:
                el.location[j] = CUBE_POS[j] - half
            elif el.location[j] < CUBE_POS[j] - half:
                el.location[j] = CUBE_POS[j] + half

setup_scene()
bpy.app.handlers.frame_change_pre.clear()
bpy.app.handlers.frame_change_pre.append(update_handler)

cam = bpy.context.scene.camera
cam.rotation_mode = 'QUATERNION'
cam.data.clip_end = CLIP_DISTANCE
with open('camera_data.txt', 'r') as f:
    lines = f.readlines()
    for l in lines[1:]:
        p = list(map(float, l.split()))
        bpy.context.scene.frame_set(int(p[0]))
        cam.location = (p[1], (-p[3]), (p[2]))
        target = Vector((p[4], (-p[6]), (p[5])))
        direction = target - cam.location
        if direction.length > 0.001:
            cam.rotation_quaternion = direction.to_track_quat('-Z', 'Y')
        cam.keyframe_insert(data_path="location")
        cam.keyframe_insert(data_path="rotation_quaternion")

scene = bpy.context.scene
scene.frame_end = len(file_sequence) * FRAMES_PER_FILE
scene.render.image_settings.file_format = 'FFMPEG'
scene.render.ffmpeg.format = 'MPEG4'
scene.render.filepath = os.path.join(os.getcwd(), final_filename + ".mp4")
bpy.ops.render.render(animation=True)
)";
    py.close();
    Log(">> GENERATED: render_job.py");
}

