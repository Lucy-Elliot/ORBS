// includes
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cmath> 
#include <cstdlib>
#include <cstring>
namespace fs = std::filesystem;

// GLSL Shader for Hardware Instancing -- otherwise the shading doesnt work and atoms dont show up in 3D window.
const char* instance_vshader = 
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;"
    "in mat4 instanceTransform;"
    "uniform mat4 mvp; uniform mat4 matNormal;"
    "out vec3 fragPosition; out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal;"
    "void main() {"
    "    fragPosition = vec3(instanceTransform * vec4(vertexPosition, 1.0));"
    "    fragTexCoord = vertexTexCoord; fragColor = vertexColor;"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));"
    "    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);"
    "}";

// --- CONSTANTS ---
const int START_WIDTH = 1600;
const int START_HEIGHT = 900;
const int BASE_SIDEBAR_WIDTH = 350;
const int BASE_TERMINAL_HEIGHT = 200;
const int BASE_TOP_BAR_HEIGHT = 50;
const char* colourSchemeNames[] = { "Default (Red/Blue)", "Blue-Gold", "Jet (Heatmap)", "Custom .inc File" };
// --- ENUMS & THEME STATE ---
enum ThemeType { THEME_BASIC, THEME_WIZARD, THEME_VAMPIRE, THEME_CYBER, THEME_PINK};
ThemeType currentTheme = THEME_BASIC;

// --- COLORS ---
Color COL_BACKGROUND = { 20, 10, 30, 255 };
Color COL_SIDEBAR    = { 40, 20, 60, 255 };
Color COL_TERMINAL   = { 10, 5, 15, 255 };
Color COL_TEXT       = { 200, 200, 255, 255 };
Color COL_ACCENT     = { 0, 255, 255, 255 };
Color SPARKLE_COLOR = { 255, 255, 255, 255 };

// --- FONTS ---
Font fontWizard;
Font fontVampire;
Font fontCyber;
Font fontPink;
Font currentFont;


// --- a lot of variables ---
bool showHelpModal = false;
bool showSettingsModal = false;
bool isExporting = false;
float exportTimer = 0.0f;
bool isDropdownOpen = false;
bool showRenderModal = false; 
int selectedBg = 0; // preselected background in render modal
bool runBlenderAutomatically = false; // Option to just generate files
int selectedColourScheme = 0; // 0=Default, 1=Blue-Gold, 2=Jet, 3=Custom .inc
char customIncPath[1024] = { 0 }; //For custom colour scheme .inc file  ----- FIX so people can add own  .inc file, maybe incl custom file path for when uploading to HPC
char outputFileName[128] = "render_output"; // Default name
int letterCount = 13; // Match the length of the default name
int textBoxActive = 0; // 0 = none, 1 = output name, 2 = custom inc path, 3 = custom bg colour
int selectedAtoms = 0; // 0=Atoms, 1=Arrows, 2 = Both, 
float atomScale = 1.0f; // scale for atoms and arrows
float clipDistance = 1000.0f; //clipping distance for Blender camera
float blenderProgress = 0.0f; // 0.0 to 1.0
bool isBlenderRunning = false; // used in blender running progress bar coming soon.
float blenderCheckTimer = 0.0f; //checking blender progress
char frameEditBuffer[16] = { 0 }; // for editing keyframe frame numbers in the sidebar
float filescrolloffset = 0.0f; // for scrolling through file list when too long - good for checking what tlooks like in the camera thing
float framescrolloffset = 0.0f; // for scrolling through frames in sidebar when too long
char custombackgroundcolour[32] = { 0 }; // for custom background colour in .inc format, e.g. "1,0,0, 1" for red, "0.5,0.5,0.5, 1" for grey etc.
float fileScrollY = 0.0f; // for scrolling through file list in render modal when too long
float keyframeScrollY = 0.0f; // for scrolling through keyframe list in sidebar when too long
float lastClickTime = 0.0f; // for detecting double clicks in the keyframe list for editing
int editingKeyframeIndex = -1; // for editing keyframe index in the keyframe list when double clicked, -1 when not editing
char kfEditBuffer[16] = { 0 }; // for editing keyframe frame numbers in the sidebar when double clicked
int kfLetterCount = 0; // for editing keyframe frame numbers in the sidebar when double clicked

// creating a geometry nodes style thing for the atoms/arrows, hopefully makes less laggy
Material atomMaterial = LoadMaterialDefault();
Matrix* atomTransforms = nullptr;
Color* atomColors = nullptr;
int atomCount = 0;
Mesh atomMesh;
Material matRed;
Material matBlue;
std::vector<Matrix> redTransforms;
std::vector<Matrix> blueTransforms;
// making sure the animated  layer has frames/speed/timer etc. self explanatory ngl
struct AnimatedLayer {
    Texture2D texture;
    int frameCount;
    float frameSpeed; 
    float timer;
    int currentFrame;
    Vector2 offset;   
    float scale;

    void Update(float delta) {
        if (texture.id == 0) return;
        timer += delta;
        if (timer >= frameSpeed) {
            timer = 0.0f;
            currentFrame = (currentFrame + 1) % frameCount;
        }
    }

    void Draw(int screenCx, int screenCy, float dynamicScale) {
        if (texture.id == 0) return;
        float frameWidth = (float)texture.width / frameCount;
        Rectangle source = { (float)currentFrame * frameWidth, 0, frameWidth, (float)texture.height };
        Rectangle dest = { 
            (float)screenCx + offset.x * scale * dynamicScale, 
            (float)screenCy + offset.y * scale * dynamicScale, 
            frameWidth * scale * dynamicScale, 
            (float)texture.height * scale * dynamicScale
        };
        Vector2 origin = { (dest.width / 2), (dest.height / 2) };
        DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
    }
};

// sequence Logic
std::vector<std::string> foundSequenceFiles;
int seqStartIndex = 0;
int seqEndIndex = 0;
float rendermodalscroll = 0.0f;


// Global Font Scale
float globalFontScale = 1.0f; 
int targetFPS = 60;

//DATA STRUCTURES
struct Atom { Vector3 position; Color color; };
struct Keyframe { 
    int id; 
    int frameNumber; 
    Vector3 position; 
    Vector3 target; 
    Vector3 up;
};
struct MatrixDrop { float x; float y; float speed; char character; };
struct Bat { float x; float y; float offset; };
struct Star { float x; float y; float brightness; };

// Texture globals
Texture2D texTwinklingWindow;
Texture2D texWizardDesk;
Texture2D texPotionShelves;
Texture2D texRunningUnicorn;
Texture2D texWizardWindow;
int frameWidth;
int frameCount = 13; // Number of frames in Aseprite animation
int currentFrame = 0;
float frameTimer = 0.0f;
float frameSpeed = 0.1f;

float loadingProgress = 0.0f; // 0.0 to 1.0

// --- loading screen sprite
AnimatedLayer layerWindow;
AnimatedLayer layerShelf;
AnimatedLayer layerWizard;
AnimatedLayer layerOrb;

void DrawThemeText(const char* text, int x, int y, int fontSize, Color color) {
    Font f = (currentFont.texture.id == 0) ? GetFontDefault() : currentFont;
    float finalSize = (float)fontSize * globalFontScale;
    DrawTextEx(f, text, (Vector2){(float)x, (float)y}, finalSize, 1.0f, color);
}

// --- GLOBAL VARIABLES ---
std::vector<std::string> consoleLog;
std::vector<std::string> fileList;
std::string currentLoadedFile = "None";
std::string currentDirectory = "."; 
std::vector<Atom> atoms;

std::vector<MatrixDrop> matrixRain;
std::vector<Bat> vampireBats;
std::vector<Star> wizardStars;


void Log(std::string msg) {
    consoleLog.push_back(msg);
    if (consoleLog.size() > 8) consoleLog.erase(consoleLog.begin());
}

// Set the current theme based on the provided type
void SetTheme(ThemeType type) {
    currentTheme = type;
    if (type == THEME_WIZARD) {
        COL_BACKGROUND = { 20, 10, 30, 255 };
        COL_SIDEBAR    = { 45, 20, 60, 255 };
        COL_TERMINAL   = { 15, 5, 20, 255 };
        COL_TEXT       = { 220, 220, 255, 255 };
        COL_ACCENT     = { 100, 255, 218, 255 }; 
        SPARKLE_COLOR = { 96, 52, 140, 255 };
        currentFont = fontWizard;                
    } else if (type == THEME_VAMPIRE) {
        COL_BACKGROUND = { 20, 5, 5, 255 };
        COL_SIDEBAR    = { 60, 10, 10, 255 };
        COL_TERMINAL   = { 15, 0, 0, 255 };
        COL_TEXT       = { 255, 200, 200, 255 };
        COL_ACCENT     = { 255, 0, 0, 255 };     
        currentFont = fontVampire;               
    } else if(type == THEME_BASIC) {
        COL_BACKGROUND = { 10, 10, 10, 255 };
        COL_SIDEBAR    = { 30, 30, 50, 255 };
        COL_TERMINAL   = { 5, 5, 15, 255 };
        COL_TEXT       = { 255, 255, 255, 255 };
        COL_ACCENT     = { 220, 220, 255, 255 };
        currentFont = GetFontDefault();
    } 
    else if (type == THEME_PINK) {
        COL_BACKGROUND = { 253, 230, 255, 255 };
        COL_SIDEBAR    = { 225, 194, 242, 255 };
        COL_TERMINAL   = { 225, 194, 242, 255 };
        COL_TEXT       = { 89, 34, 115, 255 };
        COL_ACCENT     = { 176, 62, 184, 255 }; 
        SPARKLE_COLOR = { 255, 255, 255, 255 };
        currentFont = fontPink;
    } else { 
        COL_BACKGROUND = { 5, 5, 10, 255 };
        COL_SIDEBAR    = { 20, 30, 40, 255 };
        COL_TERMINAL   = { 0, 10, 15, 255 };
        COL_TEXT       = { 0, 255, 0, 255 };
        COL_ACCENT     = { 0, 255, 0, 255 };     
        currentFont = fontCyber;                 
    }
}

// Geonodes type thing in  raylib
void UpdateInstancingData() {
    if (atomTransforms) free(atomTransforms);
    if (atomColors) free(atomColors);
    
    atomCount = atoms.size();
    atomTransforms = (Matrix*)malloc(atomCount * sizeof(Matrix));
    atomColors = (Color*)malloc(atomCount * sizeof(Color));

    for (int i = 0; i < atomCount; i++) {
        atomTransforms[i] = MatrixTranslate(atoms[i].position.x, atoms[i].position.y, atoms[i].position.z);
        atomColors[i] = atoms[i].color;
    }
}

// lOADING SCREENS :)
void DrawWizardLofi(int cx, int cy, float progress) {
    // draw the Window 
    Rectangle sourceRec = { (float)(cx % 4) * (texTwinklingWindow.width / 4), 0, 
                            (float)texTwinklingWindow.width / 4, (float)texTwinklingWindow.height };
    Vector2 pos = { (float)cx - 100, (float)cy - 150 };
    DrawTextureEx(texTwinklingWindow, pos, 0.0f, 4.0f, WHITE);

    // draw the Desk
    if (progress > 0.3f) {
        DrawTextureEx(texWizardDesk, (Vector2){pos.x - 20, pos.y + 100}, 0.0f, 4.0f, WHITE);
    }

    // draw Shelves 
    if (progress > 0.7f) {
        float alpha = (progress - 0.7f) * 3.33f; // Simple linear fade
        DrawTextureEx(texPotionShelves, (Vector2){pos.x + 150, pos.y - 20}, 0.0f, 4.0f, Fade(WHITE, alpha));
    }
    
    DrawThemeText("Pondering the Orb...", cx - 100, cy + 200, 30, COL_TEXT);
}


// --- update loading progress for the blender render bar
void UpdateBlenderProgress() {
    std::ifstream pFile("render_progress.txt");
    if (pFile.is_open()) {
        float p;
        if (pFile >> p) blenderProgress = p;
        pFile.close();
        isBlenderRunning = true;
    } else {
        isBlenderRunning = false;
    }
}

bool AutoButton(const char* text, float* currentX, float y) {
    Font f = (currentFont.texture.id == 0) ? GetFontDefault() : currentFont;
    float fontSize = 30.0f * globalFontScale;
    
    static std::string lastText = "";
    static Vector2 cachedSize = {0, 0};
    if (lastText != text) {
        cachedSize = MeasureTextEx(f, text, fontSize, 1.0f);
        lastText = text;
    }
    
    float padding = 20.0f * globalFontScale;
    float width = cachedSize.x + padding;
    float height = 30.0f * globalFontScale;

    Rectangle rect = { *currentX, y, width, height };
    bool clicked = false;
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    
    // optimization: Only draw outline if hovering to save fill-rate
    DrawRectangleRec(rect, hover ? COL_ACCENT : COL_SIDEBAR);
    if (hover) DrawRectangleLinesEx(rect, 1, COL_SIDEBAR);
    else DrawRectangleLinesEx(rect, 1, COL_ACCENT);
    
    DrawTextEx(f, text, (Vector2){rect.x + (padding/2), rect.y + (height - cachedSize.y)/2}, fontSize, 1.0f, hover ? COL_SIDEBAR : COL_TEXT);
    
    if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) clicked = true;
    *currentX += width + (25.0f * globalFontScale);
    return clicked;
}

void InitVisuals() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();
    matrixRain.clear();
    for (int i = 0; i < w / 15; i++) matrixRain.push_back({ (float)i * 15, (float)GetRandomValue(-500, h), (float)GetRandomValue(5, 12), (GetRandomValue(0, 1) == 0) ? '0' : '1' });
    vampireBats.clear();
    for (int i = 0; i < 15; i++) vampireBats.push_back({ (float)GetRandomValue(0, w), (float)GetRandomValue(0, h), (float)GetRandomValue(0, 100) / 10.0f });
    wizardStars.clear();
    for (int i = 0; i < 300; i++) wizardStars.push_back({ (float)GetRandomValue(0, w), (float)GetRandomValue(0, h), (float)GetRandomValue(1, 10) / 10.0f });
}


std::vector<Atom> LoadAtoms(std::string filepath) {
    std::vector<Atom> newAtoms;
    
    // clear the existing batch data
    redTransforms.clear();
    blueTransforms.clear();
    
    std::ifstream file(filepath);
    if (!file.is_open()) { Log("ERROR: Open Failed"); return newAtoms; }
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        float x, y, z, sx, sy, sz;
        if (ss >> x >> y >> z >> sx >> sy >> sz) {
            Color c = (sz > 0 ? RED : BLUE);
            newAtoms.push_back({ (Vector3){x, y, z}, c });
            
            // build the matrix for instancing
            // inside LoadAtoms while loop
Matrix m = MatrixMultiply(MatrixScale(atomScale, atomScale, atomScale), MatrixTranslate(x, y, z));
            if (sz > 0) redTransforms.push_back(m);
            else blueTransforms.push_back(m);
        }
    }
    
    atoms = newAtoms; // update global atoms lists
    currentLoadedFile = fs::path(filepath).filename().string(); 
    Log(TextFormat("LOADED: %d atoms", (int)atoms.size()));
    return newAtoms;
}

//  file scanner, 
void ScanDirectoryForSequence(std::string folderPath, std::string referenceFile) {
    foundSequenceFiles.clear();
    fs::path refPath(referenceFile);
    std::string refName = refPath.filename().string();
    
    std::string prefix = "";
    size_t firstDigit = refName.find_first_of("0123456789");
    if (firstDigit != std::string::npos) {
        prefix = refName.substr(0, firstDigit);
    } else {
        prefix = refName.substr(0, refName.find_last_of("."));
    }

    if (fs::exists(folderPath)) {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (!entry.is_regular_file()) continue;
            std::string fname = entry.path().filename().string();
            // Must be .txt AND start with prefix AND not be CMakeLists, made with vdc --text. can handle some files but for some that are too big the lag is too much . overcoming soon :)
            if (fname.find(".txt") != std::string::npos && fname.find(prefix) == 0 && fname.find("CMake") == std::string::npos) {
                //
                foundSequenceFiles.push_back(fs::absolute(entry.path()).string());
            }
        }
    }
    std::sort(foundSequenceFiles.begin(), foundSequenceFiles.end());
    
    seqStartIndex = 0;
    seqEndIndex = (int)foundSequenceFiles.size() - 1;
    if (seqEndIndex < 0) seqEndIndex = 0;
}

// export path (absolute paths + fallback) ------------------------------------
void ExportPath(const std::vector<Keyframe>& path, int bgType, int colourScheme, char outputName[128], int atomStyle, float atomScale, float clipDistance, char custombackgroundcolour[32]) {
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
    return template, atom_tmpl

arrow_tmpl, atom_tmpl = create_templates()

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

    #background colours -- ADD CUSTOM COL OPTION
    bg = bpy.context.scene.world.node_tree.nodes.get("Background")
    if BACKGROUND_TYPE == 0: bg.inputs[0].default_value = (0.02, 0.02, 0.02, 1)
    elif BACKGROUND_TYPE == 1: bg.inputs[0].default_value = (1, 1, 1, 1)
    elif BACKGROUND_TYPE == 2: bg.inputs[0].default_value = (0.3, 0, 0, 1)
    elif BACKGROUND_TYPE == 3: bg.inputs[0].default_value = custombackgroundcolour


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

void LoadFiles() {
    fileList.clear();
    std::error_code ec;
    fs::path p = fs::canonical(currentDirectory, ec);
    if (ec) { currentDirectory = "."; p = fs::current_path(); } else { currentDirectory = p.string(); }

    if (p.has_parent_path() && p != p.root_path()) fileList.push_back(".. (Go Up)");

    try {
        if (fs::exists(p) && fs::is_directory(p)) {
            auto options = fs::directory_options::skip_permission_denied;
            for (const auto& entry : fs::directory_iterator(p, options, ec)) {
                if (ec) continue;
                std::string filename = entry.path().filename().string();
                if (filename[0] == '.') continue; 
                if (entry.is_directory()) fileList.push_back(filename + "/");
                else if (filename.find(".txt") != std::string::npos) fileList.push_back(filename);
            }
        }
    } catch (...) { Log("ERROR: Access Denied"); }
    std::sort(fileList.begin(), fileList.end());
}

// started more animations, but theyre taking a while so im leaving that to later
void InitWizardTheme() {
    // Window: 13 frames, slow twinkle, with wizard tower yayyy
    layerWindow = { LoadTexture("resources/Wizard_Window.png"), 13, 0.2f, 0, 0, {0, -70}, 4.0f };
    // Shelf: Static or slight potion bubble (1 frame or more)
    //layerShelf  = { LoadTexture("shelf_sheet.png"), 1, 0.1f, 0, 0, {120, -20}, 4.0f };
    // Wizard: 30 frames, wizarding around
    layerWizard = { LoadTexture("resources/wizard_person1.png"), 30, 0.08f, 0, 0, {0, 30}, 6.0f };
    // Orb: 6 frames, glowing pulse -- animation coming soon.
    //layerOrb    = { LoadTexture("orb_sheet.png"), 6, 0.08f, 0, 0, {-30, 60}, 4.0f };
}

void DrawWizardLoading(int w, int h, float progress, float responsiveScale) {
    int cx = w / 2;
    int cy = h / 2 - (h * 0.1f); // shift art up to make room for bar
    float dt = GetFrameTime();

    // update all animation
    layerWindow.Update(dt);
    layerShelf.Update(dt);
    layerWizard.Update(dt);
    layerOrb.Update(dt);

    // draw Layers with the passed-in responsiveScale, which will make them scale based on screen height, only got wizard theme atm.
    layerWindow.Draw(cx, cy, responsiveScale); 
    layerShelf.Draw(cx, cy, responsiveScale);  
    layerWizard.Draw(cx, cy, responsiveScale); 
    layerOrb.Draw(cx, cy, responsiveScale);    

    // text placed above the bar (Bottom area)
    DrawThemeText("PONDERING THE ORB...", cx - 120, h - 110, 30, COL_TEXT);
}


// Loading screen for the multiple themes, will be developed to be more descriptive soon <3
void DrawLoadingScreen() {
    int w = GetScreenWidth();
    int h = GetScreenHeight();

    DrawRectangle(0, 0, w, h, (Color){10, 5, 20, 255}); 

    int cx = w / 2;
    int cy = h / 2 - (h * 0.1f); 
    
    // Define these ONCE at the top
    int barWidth = (int)(w * 0.6f); 
    int barHeight = 20;
    int spacing = 40; 
    int baseY = h - 140;

    float dt = GetFrameTime();
    float progress = (exportTimer / 3.0f > 1.0f) ? 1.0f : exportTimer / 3.0f;
    float responsiveScale = (float)h / 2000.0f;
    const int barX = cx - barWidth / 2;

    // - blender Progress Bar (Only if running) -----
    // if (runBlenderAutomatically) {
    //     blenderCheckTimer += dt;
    //     const int blenderY = baseY + 40;
    //     DrawThemeText("BLENDER RENDERING...", barX, blenderY - 25, 20, COL_TEXT);
    //     DrawRectangleLines(barX, blenderY, barWidth, barHeight, COL_ACCENT);
    //     DrawRectangle(barX + 2, blenderY + 2, (int)((barWidth - 4) * blenderProgress), barHeight - 4, COL_ACCENT);
    //     DrawThemeText(TextFormat("%d%%", (int)(blenderProgress * 100)), barX + barWidth + 10, blenderY, 20, COL_TEXT);
    
    //     if (blenderCheckTimer >= 0.5f) { // Only check disk twice per second
    //         UpdateBlenderProgress(); 
    //         blenderCheckTimer = 0.0f;
    //     }

    // }

    

    if (currentTheme == THEME_WIZARD) {
        // draw Stars background - cute :)
        for (const auto& s : wizardStars) {
            float twinkle = sinf((float)GetTime() * 5.0f + s.x) * 0.5f + 0.5f; 
            DrawPixel(s.x, s.y, Fade(WHITE, 2*s.brightness * twinkle));
        }

        // calling the helper function, the responsiveScale
        DrawWizardLoading(w, h, progress, responsiveScale);
    }

    else if (currentTheme == THEME_VAMPIRE) {
        Font currentFont = fontVampire;
        for (const auto& b : vampireBats) {
            float flutter = sinf(GetTime() * 10.0f + b.offset) * 0.5f + 0.5f;
            DrawTriangle((Vector2){b.x, b.y}, (Vector2){b.x - 10*flutter, b.y - 5}, (Vector2){b.x - 10*flutter, b.y + 5}, Fade(COL_ACCENT, flutter));
        }
        DrawThemeText("ORB...", cx - 90, cy + 150, 30, COL_TEXT);
    }
    else if (currentTheme == THEME_CYBER) {
        for (auto& m : matrixRain) {
            DrawTextEx(currentFont, TextFormat("%c", m.character), (Vector2){m.x, m.y}, 20 * globalFontScale, 1.0f, Fade(COL_ACCENT, 0.8f));
            m.y += m.speed;
            if (m.y > h) m.y = (float)GetRandomValue(-100, -20);
            m.character = (GetRandomValue(0, 1) == 0) ? '0' : '1';
        }
        DrawThemeText("HACKING THE ORB...", cx - 80, cy + 150, 30, COL_TEXT);
    }
    else if (currentTheme == THEME_PINK) {
        float pulse = sinf(GetTime() * 3.0f) * 20.0f;
        DrawCircleGradient(cx, cy, 100 + pulse, Fade(COL_ACCENT, 0.3f), Fade(COL_ACCENT, 0.0f));
        DrawCircleGradient(cx, cy, 50, Fade(COL_ACCENT, 0.7f), Fade(COL_SIDEBAR, 0.9f));
        DrawCircleLines(cx, cy, 50, WHITE);
        for (int i = 0; i < 8; i++) {
            float angle = GetTime() * 2.0f + (i * 45 * DEG2RAD);
            float dist = 70.0f + sinf(GetTime() * 4.0f + i) * 5.0f;
            DrawCircle(cx + cosf(angle) * dist, cy + sinf(angle) * dist, 4, Fade(WHITE, 0.8f));
        }
        DrawThemeText("Sparkle On Queen...", cx - 80, cy + 150, 30, COL_TEXT);
    }
    else if (currentTheme == THEME_BASIC) {
        DrawThemeText("EXPORTING...", cx - 60, cy + 150, 30, COL_TEXT);
    }
    
    DrawThemeText(progress < 1.0f ? "WRITING DATA..." : "DATA READY", cx - barWidth/2, baseY - 25, 20, COL_TEXT);
    DrawRectangleLines(cx - barWidth/2, baseY, barWidth, barHeight, COL_ACCENT); 
    DrawRectangle(cx - barWidth/2 + 2, baseY + 2, (int)((barWidth-4) * progress), barHeight - 4, COL_ACCENT); 

    // --- EXIT BUTTON --- important when I fix the blender progress bar.
    float exitBtnWidth = 200.0f * globalFontScale;
    float exitX = cx - (exitBtnWidth / 2.0f);
    if (AutoButton(" RETURN TO EDITOR ", &exitX, (float)h - 60)) {
        isExporting = false;
    // blender Bar (Positioned relatively)
    }
}
void SaveSettings() {
    std::ofstream outFile("config.txt");
    if (outFile.is_open()) {
        outFile << (int)currentTheme << "\n";
        outFile << globalFontScale << "\n";
        outFile.close();
    }
}

void LoadSettings() {
    std::ifstream inFile("config.txt");
    if (inFile.is_open()) {
        int themeInt;
        float scale;
        if (inFile >> themeInt >> scale) {
            // applying the loaded values
            currentTheme = (ThemeType)themeInt;
            globalFontScale = scale;
        }
        inFile.close();
    }
}
//-----------------
// ----- main -----
//-----------------
int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(START_WIDTH, START_HEIGHT, "ORBS");
    // geonodes style cubes
    atomMesh = GenMeshCube(0.8f, 0.8f, 0.8f);

    // instancing shader loaded earlier
    Shader shader = LoadShaderFromMemory(instance_vshader, NULL);
    // locate the transform attribute for instancing
    shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(shader, "instanceTransform");

    // setup Materials using the shader
    matRed = LoadMaterialDefault();
    matRed.shader = shader;
    matRed.maps[MATERIAL_MAP_DIFFUSE].color = RED;

    matBlue = LoadMaterialDefault();
    matBlue.shader = shader;
    matBlue.maps[MATERIAL_MAP_DIFFUSE].color = BLUE;
    //fps etc
    SetTargetFPS(targetFPS);
    InitVisuals(); 

    LoadSettings();
    // font loading
    fontWizard  = LoadFont("wizard.ttf");
    fontVampire = LoadFont("vampire.ttf");
    fontCyber   = LoadFont("cyber.ttf");
    fontPink   = LoadFont("pink.ttf");
    SetTheme(currentTheme);
    // general setup
    float currentSidebarWidth = BASE_SIDEBAR_WIDTH * globalFontScale;
    float currentTopBarHeight = BASE_TOP_BAR_HEIGHT * globalFontScale;
    float currentTerminalHeight = BASE_TERMINAL_HEIGHT * globalFontScale;
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 20.0f, 20.0f, 20.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    InitWizardTheme();

    LoadFiles();
    std::vector<Keyframe> path;
    bool showCubes = true;
    int currentFrame = 0;


    // loading stuff
    texWizardWindow = LoadTexture("resources/Wizard_Window.png");
    frameWidth = texWizardWindow.width / frameCount;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) InitVisuals();

        float currentSidebarWidth = BASE_SIDEBAR_WIDTH * globalFontScale;
        float currentTopBarHeight = BASE_TOP_BAR_HEIGHT * globalFontScale;
        float currentTerminalHeight = BASE_TERMINAL_HEIGHT * globalFontScale;

        // logic for movement in camera in edit mode
        if (!isExporting && !showHelpModal && !showSettingsModal && !showRenderModal) {
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                UpdateCamera(&camera, CAMERA_FIRST_PERSON);
            } else {
                float speed = IsKeyDown(KEY_LEFT_SHIFT) ? 2.0f : 0.5f;
                Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
                Vector3 right = Vector3CrossProduct(forward, camera.up);
                if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(forward, speed));
                if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, speed));
                if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position, Vector3Scale(right, speed));
                if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position, Vector3Scale(right, speed));
                if (IsKeyDown(KEY_SPACE)) camera.position.y += speed; 
                if (IsKeyDown(KEY_LEFT_SHIFT)) camera.position.y -= speed; 
                camera.target = Vector3Add(camera.position, forward); 
            }
            if (IsKeyPressed(KEY_ENTER)) {
                int NextFrame = 0;
                if (!path.empty()){
                    NextFrame = path.back().frameNumber + targetFPS;
                }
                path.push_back({(int)path.size(), NextFrame, camera.position, camera.target, camera.up});
                currentFrame = NextFrame + targetFPS;
                Log(TextFormat("KF added at frame %d", NextFrame));
            }
        }
        if (IsKeyPressed(KEY_M)) showCubes = !showCubes;

        // drawing background
        BeginDrawing();
        ClearBackground(COL_BACKGROUND);

        int renderW = GetScreenWidth() - (int)currentSidebarWidth;
        int renderH = GetScreenHeight() - (int)currentTerminalHeight;

        //3D Viewport
        BeginScissorMode(0, (int)currentTopBarHeight, renderW, renderH - (int)currentTopBarHeight);
            BeginMode3D(camera);
                if (showCubes) {
                    if (!redTransforms.empty()) {
                        DrawMeshInstanced(atomMesh, matRed, redTransforms.data(), (int)redTransforms.size());
                    }
                    if (!blueTransforms.empty()) {
                        DrawMeshInstanced(atomMesh, matBlue, blueTransforms.data(), (int)blueTransforms.size());
                    }
                } else {
                    for (const auto& a : atoms) DrawPoint3D(a.position, a.color);
                }
                DrawGrid(100, 1.0f);
            EndMode3D();
        EndScissorMode();
        //---------------------------------------------------
        // ----------------- Sidebar ------------------------
        //---------------------------------------------------
        Vector2 mousePos = GetMousePosition();

        // sidebar backgrounds
        DrawRectangle(renderW, (int)currentTopBarHeight, (int)currentSidebarWidth, GetScreenHeight(), COL_SIDEBAR);
        DrawRectangleLines(renderW, (int)currentTopBarHeight, (int)currentSidebarWidth, GetScreenHeight(), COL_TERMINAL);

        // --- keyframes ---
        int keyframeSectionHeight = renderH / 2;
        Rectangle kfView = { (float)renderW, (float)currentTopBarHeight, (float)currentSidebarWidth, (float)keyframeSectionHeight - currentTopBarHeight };

        // scrolling and headers ONLY if mouse is in the top half 
        if (CheckCollisionPointRec(mousePos, kfView)) {
            keyframeScrollY += GetMouseWheelMove() * 50.0f;
            if (keyframeScrollY > 0) keyframeScrollY = 0;
        }

        DrawThemeText(TextFormat("KEYFRAMES %d", (int)targetFPS), renderW + 10, (int)currentTopBarHeight + 10, 30, COL_ACCENT);

        // scissor mode for keyframe list -  uused for clipping the keyframe list if it exceeds the view area, also prevents mouse interaction outside of it, good for scrolling
        BeginScissorMode(kfView.x, kfView.y + (50 * globalFontScale), kfView.width, kfView.height - (50 * globalFontScale));
            int kfY = (int)currentTopBarHeight + (50 * globalFontScale) + keyframeScrollY;
    
            for (int i = 0; i < (int)path.size(); i++) {
                Rectangle kfRect = { (float)renderW + 10, (float)kfY, currentSidebarWidth - 20, 25 * globalFontScale };
                bool hovering = CheckCollisionPointRec(mousePos, kfRect) && CheckCollisionPointRec(mousePos, kfView);

                if (editingKeyframeIndex == i) {
                    // edit mode -- edits the keyframes frame number.
                    DrawRectangleRec(kfRect, COL_TERMINAL);
                    DrawRectangleLinesEx(kfRect, 1, COL_ACCENT);
                    DrawThemeText(kfEditBuffer, kfRect.x + 5, kfRect.y + 5, 20, COL_TEXT);

                    int key = GetCharPressed();
                    while (key > 0) {
                        if ((key >= '0') && (key <= '9') && (kfLetterCount < 10)) {
                            kfEditBuffer[kfLetterCount++] = (char)key;
                            kfEditBuffer[kfLetterCount] = '\0';
                        }
                        key = GetCharPressed();
                    }
                    if (IsKeyPressed(KEY_BACKSPACE) && kfLetterCount > 0) kfEditBuffer[--kfLetterCount] = '\0';
            
                    if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !hovering)) {
                        path[i].frameNumber = atoi(kfEditBuffer);
                        editingKeyframeIndex = -1;
                    }
                } else {
                    // view mode shows keyframe info and allows snapping camera to it on click, also shows hover effect
                    Color textColor = hovering ? COL_ACCENT : COL_TEXT;
                    DrawThemeText(TextFormat("[%d] Fr:%d", path[i].id, path[i].frameNumber), kfRect.x, kfRect.y, 25, textColor);
            
                    if (hovering && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        float currentTime = GetTime();
                        // Double Click Logic
                        if (currentTime - lastClickTime < 0.3f) {
                            editingKeyframeIndex = i;
                            kfLetterCount = sprintf(kfEditBuffer, "%d", path[i].frameNumber);
                        } else {
                            // SINGLE CLICK: Snap camera to this keyframe!
                            camera.position = path[i].position;
                            camera.target = path[i].target;
                            camera.up = path[i].up;
                            Log(TextFormat("Snapped to Keyframe %d", i));
                        }
                        lastClickTime = currentTime;
                    }
                }
                kfY += (30 * globalFontScale);
            }
        EndScissorMode();

        // file Explorer
        int fileSectionY = renderH / 2;
        Rectangle fileView = { (float)renderW, (float)fileSectionY, (float)currentSidebarWidth, (float)(GetScreenHeight() - fileSectionY) };
        
        if (CheckCollisionPointRec(GetMousePosition(), fileView)) {
            fileScrollY += GetMouseWheelMove() * 50.0f;
        }
        if (fileScrollY > 0) fileScrollY = 0;
        
        DrawRectangle(renderW, fileSectionY, (int)currentSidebarWidth, 2, COL_ACCENT);
        DrawThemeText("FILES", renderW + 10, fileSectionY + 10, 30, COL_ACCENT);
        
        BeginScissorMode(fileView.x, fileView.y -20 + (50* globalFontScale), fileView.width, fileView.height - (50* globalFontScale));
            int itemY = fileSectionY + (40 * globalFontScale) + fileScrollY;
            for (const auto& f : fileList) {
                if (itemY > GetScreenHeight() - 20) break;
                Rectangle itemRect = {(float)renderW, (float)itemY, currentSidebarWidth, 20.0f * globalFontScale};
                if (CheckCollisionPointRec(GetMousePosition(), itemRect)) {
                    DrawRectangleRec(itemRect, Fade(COL_ACCENT, 0.2f));
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (f == ".. (Go Up)") { 
                            currentDirectory = fs::path(currentDirectory).parent_path().string();
                            LoadFiles(); 
                        }
                        else if (f.back() == '/') { 
                            currentDirectory += "/" + f.substr(0, f.size()-1); 
                            LoadFiles(); 
                        }
                        else { 
                            std::string full = currentDirectory + "/" + f;
                            atoms = LoadAtoms(full); 
                        }
                    }
                }
                Color itemColor = (f.find(".txt") != std::string::npos) ? COL_TEXT : COL_ACCENT;
                DrawThemeText(f.c_str(), renderW + 10, itemY, 25, itemColor);
                itemY += (20 * globalFontScale);


            }
        EndScissorMode();


        //  Terminal!!
        DrawRectangle(0, renderH, renderW, (int)currentTerminalHeight, COL_TERMINAL);
        int logY = GetScreenHeight() - (25 * globalFontScale);
        for (int i = consoleLog.size() - 1; i >= 0; i--) {
            DrawThemeText(consoleLog[i].c_str(), 10, logY, 25, COL_TEXT);
            logY -= (20 * globalFontScale);
        }

            if (currentTheme == THEME_WIZARD || currentTheme == THEME_PINK){
                for (const auto& s : wizardStars) {
                float twinkle = sinf(GetTime() * 2.0f + s.x) * 1.5f + 1.5f; 
                Vector2 linestart = {2*(s.x - 3), 2*s.y };
                Vector2 lineend = {2*(s.x + 3), 2*s.y };
                DrawLineEx(linestart, lineend, 1, Fade(SPARKLE_COLOR, 1*s.brightness * twinkle));
                Vector2 linestart2 = {2*s.x, 2*(s.y - 5)};
                Vector2 lineend2 = {2*s.x, 2*(s.y + 4)};
                DrawLineEx(linestart2, lineend2, 1, Fade(SPARKLE_COLOR, 1*s.brightness * twinkle));
                Vector2 linestart3 = {2*(s.x-1), 2*(s.y - 1)};
                Vector2 lineend3 = {2*(s.x+1), 2*(s.y + 1)};
                DrawLineEx(linestart3, lineend3, 1, Fade(SPARKLE_COLOR, 1*s.brightness * twinkle));
                Vector2 linestart4 = {2*(s.x-1), 2*(s.y + 1)};
                Vector2 lineend4 = {2*(s.x+1), 2*(s.y - 1)};
                DrawLineEx(linestart4, lineend4, 1, Fade(SPARKLE_COLOR, 1*s.brightness * twinkle));
                }
            }
                    
            if (currentTheme == THEME_WIZARD || currentTheme == THEME_PINK){
                for (const auto& s : wizardStars) {
                float twinkle = sinf(GetTime() * 5.0f + s.x) * 1.5f + 1.5f; 
                DrawPixel(2*s.y*(sin(s.x)), s.y, Fade(WHITE, 3*s.brightness * twinkle));
                DrawPixel(s.x, s.y*(cos(s.x)), Fade(COL_ACCENT, 3*s.brightness * twinkle));

                }
            }
        // Top Bar - info help and settings etc.
        DrawRectangle(0, 0, GetScreenWidth(), (int)currentTopBarHeight, COL_SIDEBAR);
        DrawLine(0, (int)currentTopBarHeight, GetScreenWidth(), (int)currentTopBarHeight, COL_ACCENT);
        DrawThemeText("O.R.B.S.", 20, 10 * globalFontScale, 30, COL_ACCENT);

        // buttons on top bar
        float currentButtonX = 150.0f * globalFontScale;
        float btnY = 10.0f * globalFontScale;

        if (AutoButton("HELP", &currentButtonX, btnY)) showHelpModal = !showHelpModal;

        float themeButtonX = currentButtonX+(15 * globalFontScale); 
        if (AutoButton("THEME", &themeButtonX, btnY)) isDropdownOpen = !isDropdownOpen;
        
        float settingsButtonX = themeButtonX + (15 * globalFontScale); 
        if (AutoButton("SETTINGS", &settingsButtonX, btnY)) {
            showSettingsModal = !showSettingsModal;
            if(showSettingsModal) { showRenderModal = false; }
        }

        float renderBtnX = GetScreenWidth() - (150 * globalFontScale);
        if (AutoButton("RENDER", &renderBtnX, btnY)) {
            showRenderModal = !showRenderModal;
            if (showRenderModal) {
                showSettingsModal = false;
                std::string baseFile = (currentLoadedFile == "None" ? "test.txt" : currentLoadedFile);
                std::string fullPath = currentDirectory + "/" + baseFile;
                ScanDirectoryForSequence(currentDirectory, fullPath);
            }
        }
        

        // dropdown Logic
        if (isDropdownOpen) {
            float dropY = currentTopBarHeight; 
            const char* themes[] = { "Basic", "Wizard", "Vampire", "Cyber", "Pink" };
            float dropX = themeButtonX - (120.0f* globalFontScale); 
            for(int i=0; i<5; i++) {
                float tempX = dropX; 
                if (AutoButton(themes[i], &tempX, dropY)) {
                    if(i==1) SetTheme(THEME_WIZARD);
                    if(i==2) SetTheme(THEME_VAMPIRE);
                    if(i==3) SetTheme(THEME_CYBER);
                    if(i==4) SetTheme(THEME_PINK);
                    if(i==0) SetTheme(THEME_BASIC);
                    Log("THEME CHANGED");
                    isDropdownOpen = false;
                }
                dropY += (35 * globalFontScale);
            }
        }

        // --- MODALS ---
        // Help modal 
        if (showHelpModal) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0,0,0, 200});
            float modalX = 100.0f;
            float modalY = 100.0f;
            float modalW = 650.0f * globalFontScale;
            float modalH = 450.0f * globalFontScale;
            DrawRectangle(modalX, modalY, modalW, modalH, COL_SIDEBAR);
            DrawRectangleLines(modalX, modalY, modalW, modalH, COL_ACCENT);
            DrawThemeText("CONTROLS ", modalX + 20, modalY + 20, 30, COL_ACCENT);
            DrawThemeText("WASD: Move", modalX + 20, modalY + 50, 20, COL_TEXT);
            DrawThemeText("Shift + Space: Move Up/Down", modalX + 20, modalY + 90, 20, COL_TEXT);
            DrawThemeText("Right Click: Look Around", modalX + 20, modalY + 130, 20, COL_TEXT);
            DrawThemeText("Enter: Add Keyframe", modalX + 20, modalY + 170, 20, COL_TEXT);
            DrawThemeText("M: Toggle Atoms View", modalX + 20, modalY + 210, 20, COL_TEXT);
            DrawThemeText("Click Files to Load Atoms", modalX + 20, modalY + 250, 20, COL_TEXT);
            DrawThemeText("Render Settings: Choose sequence range and ", modalX + 20, modalY + 290, 20, COL_TEXT);
            DrawThemeText("background/colour options for Blender render", modalX + 20, modalY + 330, 20, COL_TEXT);
            DrawThemeText("Settings: Adjust font size and target FPS for different animation outputs", modalX + 20, modalY + 370, 20, COL_TEXT);
            DrawThemeText("Double Click Keyframe to Edit Frame Number, Single Click to Snap Camera", modalX + 20, modalY + 410, 20, COL_TEXT);
            float closeX = (modalX + modalW) - (50.0f * globalFontScale);
            float closeY = modalY + (10.0f * globalFontScale);
            if (AutoButton("X", &closeX, closeY)) showHelpModal = false;
        }
            // settings modal
        if (showSettingsModal) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0,0,0, 200});
            float mW = 400 * globalFontScale;
            float mH = 300 * globalFontScale;
            float mX = (GetScreenWidth() - mW) /2 ;
            float mY = (GetScreenHeight() - mH)/2 ;

            DrawRectangle(mX, mY, mW, mH, COL_SIDEBAR);
            DrawRectangleLines(mX, mY, mW, mH, COL_ACCENT);
            DrawThemeText("SETTINGS", mX + 20, mY + 20, 30, COL_ACCENT);
            DrawThemeText(TextFormat("Font Size: %.1f x", globalFontScale), mX + 20, mY + 80, 20, COL_TEXT);

            float btnX = mX + 20;
            float btnY = mY + 120 * globalFontScale;
            
            if (AutoButton(" - ", &btnX, btnY)) { if (globalFontScale > 0.6f) globalFontScale -= 0.1f; }
            if (AutoButton(" + ", &btnX, btnY)) { if (globalFontScale < 3.0f) globalFontScale += 0.1f; }
            
            DrawThemeText("Target FPS", mX + 20, mY + 180 * globalFontScale, 20, COL_TEXT);
            btnX = mX + 20;
            btnY = mY + 220 * globalFontScale;

            if (AutoButton(" 30 ", &btnX, btnY)) { targetFPS = 30; SetTargetFPS(targetFPS); }
            if (AutoButton(" 60 ", &btnX, btnY)) { targetFPS = 60; SetTargetFPS(targetFPS); }
            if (AutoButton(" 120 ", &btnX, btnY)) { targetFPS = 120; SetTargetFPS(targetFPS); }

            float closeX = mX + mW - (50* globalFontScale); 
            if (AutoButton("X", &closeX, mY + 10)) showSettingsModal = false;
        }
        // render modal
        if (showRenderModal) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0,0,0, 200});
            float mW = 800 * globalFontScale;
            float mH = 500 * globalFontScale;
            float mX = (GetScreenWidth() - mW) / 2;
            float mY = (GetScreenHeight() - mH) / 2;

            DrawRectangle(mX, mY, mW, mH, COL_SIDEBAR);
            DrawRectangleLines(mX, mY, mW, mH, COL_ACCENT);
            DrawThemeText("RENDER SETTINGS - scroll to [] run blender", mX + 20, mY + 20, 30, COL_ACCENT);

            //  Scroll
            if (CheckCollisionPointRec(GetMousePosition(), (Rectangle){mX, mY, mW, mH})) {
                rendermodalscroll += GetMouseWheelMove() * 50.0f;
            }
            if (rendermodalscroll > 0) rendermodalscroll = 0;
            if (rendermodalscroll < -400 * globalFontScale) rendermodalscroll = -400 * globalFontScale;

            BeginScissorMode((int)mX, (int)(mY + 60 * globalFontScale), (int)mW, (int)(mH - 130 * globalFontScale));
                float rowY = mY + 70 * globalFontScale + rendermodalscroll;

                DrawThemeText(TextFormat("Found: %d files", (int)foundSequenceFiles.size()), mX + 20, rowY, 20, COL_TEXT);
                rowY += 40 * globalFontScale;
                
                // Indices
                DrawThemeText(TextFormat("Start Index: %d", seqStartIndex), mX + 20, rowY, 20, COL_TEXT);
                float btnX = mX + 250 * globalFontScale;
                if (AutoButton(" - ", &btnX, rowY)) { if(seqStartIndex > 0) seqStartIndex--; }
                if (AutoButton(" + ", &btnX, rowY)) { if(seqStartIndex < seqEndIndex) seqStartIndex++; }
                rowY += 50 * globalFontScale;

                DrawThemeText(TextFormat("End Index: %d", seqEndIndex), mX + 20, rowY, 20, COL_TEXT);
                btnX = mX + 250 * globalFontScale;
                if (AutoButton(" - ", &btnX, rowY)) { if(seqEndIndex > seqStartIndex) seqEndIndex--; }
                if (AutoButton(" + ", &btnX, rowY)) { if(seqEndIndex < (int)foundSequenceFiles.size()-1) seqEndIndex++; }
                rowY += 60 * globalFontScale;

                // Background
                DrawThemeText("Background:", mX + 20, rowY, 20, COL_TEXT);
                float optX = mX + 200 * globalFontScale;
                if (AutoButton(selectedBg == 0 ? "[X] Dark" : "[ ] Dark", &optX, rowY)) selectedBg = 0;
                if (AutoButton(selectedBg == 1 ? "[X] White" : "[ ] White", &optX, rowY)) selectedBg = 1;
                if (AutoButton(selectedBg == 2 ? "[X] Red" : "[ ] Red", &optX, rowY)) selectedBg = 2;
                if (AutoButton(selectedBg == 3 ? "[X] Custom" : "[ ] Custom", &optX, rowY)) selectedBg = 3;
                if (selectedBg == 3) {
                    rowY += 20 * globalFontScale;
                    DrawThemeText("Custom colour: input R G B (0-1) in the form r, g, b, 1 :", mX + 20, rowY + 30, 18, COL_TEXT);
                    Rectangle bgTextbox = { mX + 170 * globalFontScale, rowY + 55, 250 * globalFontScale, 30 * globalFontScale };
                    if (custombackgroundcolour[0] == '\0') {
                        strcpy(custombackgroundcolour, "1, 1, 1, 1");
                    }

                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (CheckCollisionPointRec(GetMousePosition(), bgTextbox)) textBoxActive = 3;
                        else textBoxActive = 0;
                    }
                    DrawRectangleRec(bgTextbox, COL_TERMINAL);
                    DrawRectangleLinesEx(bgTextbox, 2, (textBoxActive == 3) ? COL_ACCENT : Fade(COL_TEXT, 0.3f));
                    DrawThemeText(custombackgroundcolour, (int)bgTextbox.x + 5, (int)bgTextbox.y + 5, 20, COL_TEXT);

                    // Typing Logic for background color
                    if (textBoxActive == 3) {
                        SetMouseCursor(MOUSE_CURSOR_IBEAM);
                        int key = GetCharPressed();
                        while (key > 0) {
                            if ((key >= 32) && (key <= 125) && (strlen(custombackgroundcolour) < 30)) {
                                int len = strlen(custombackgroundcolour);
                                custombackgroundcolour[len] = (char)key;
                                custombackgroundcolour[len + 1] = '\0';
                            }
                            key = GetCharPressed();
                        }
                        if (IsKeyPressed(KEY_BACKSPACE) && strlen(custombackgroundcolour) > 0) {
                            custombackgroundcolour[strlen(custombackgroundcolour) - 1] = '\0';
                        }
                    }
                rowY += 60 * globalFontScale;
                }
                

                // scheme
                rowY += 60 * globalFontScale;
                DrawThemeText("Colour Scheme:", mX + 20, rowY, 20, COL_TEXT);
                float optY = mX + 200 * globalFontScale;
                if (AutoButton(selectedColourScheme == 0 ? "[X] Default" : "[ ] Default", &optY, rowY)) selectedColourScheme = 0;
                if (AutoButton(selectedColourScheme == 1 ? "[X] Jet" : "[ ] Jet", &optY, rowY)) selectedColourScheme = 1;
                if (AutoButton(selectedColourScheme == 2 ? "[X] Blue-Gold" : "[ ] Blue-Gold", &optY, rowY)) selectedColourScheme = 2;
                //if (AutoButton(selectedColourScheme == 3 ? "[X] Custom" : "[ ] Custom", &optY, rowY)) selectedColourScheme = 3; <-- coming soon to allow parsing .inc files
        //         if (selectedColourScheme == 3){
        //                 DrawThemeText("Custom palette in folders: select .inc files with R G B (0-255) per line, up to 256 lines.\n This will be read by the Blender script.", mX + 20, rowY + 30, 18, COL_TEXT);
        //                 int fileY = renderH / 2;
                        
        //                 DrawRectangle(renderW, fileY, (int)currentSidebarWidth, 2, COL_ACCENT);
        //                 DrawThemeText("FILES", renderW + 10, fileY + 10, 30, COL_ACCENT);
        
        //                 int itemY = fileY + (40 * globalFontScale);
        //                 for (const auto& f : fileList) {
        //                     if (itemY > GetScreenHeight() - 20) break;
        //                     Rectangle itemRect = {(float)renderW, (float)itemY, currentSidebarWidth, 20.0f * globalFontScale};
        //                     if (CheckCollisionPointRec(GetMousePosition(), itemRect)) {
        //                         DrawRectangleRec(itemRect, Fade(COL_ACCENT, 0.2f));
        //                         if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        //                             if (f == ".. (Go Up)") { 
        //                                 currentDirectory = fs::path(currentDirectory).parent_path().string();
        //                                 LoadFiles(); 
        //                             }
        //                             else if (f.back() == '/') { 
        //                                 currentDirectory += "/" + f.substr(0, f.size()-1); 
        //                                 LoadFiles(); 
        //                             }
        //                             else { 
        //                                 std::string full = currentDirectory + "/" + f;
        //                                 atoms = LoadAtoms(full); 
        //             }
        //         }
        //     }
        //     Color itemColor = (f.find(".txt") != std::string::npos) ? COL_TEXT : COL_ACCENT;
        //     DrawThemeText(f.c_str(), renderW + 10, itemY, 25, itemColor);
        //     itemY += (20 * globalFontScale);
        // }
        //             } 
                
                rowY += 50 * globalFontScale;

                // style
                DrawThemeText("Style:", mX + 20, rowY, 20, COL_TEXT);
                float styleX = mX + 200 * globalFontScale;
                if (AutoButton(selectedAtoms == 0 ? "[X] Atoms" : "[ ] Atoms", &styleX, rowY)) selectedAtoms = 0;
                if (AutoButton(selectedAtoms == 1 ? "[X] Arrows" : "[ ] Arrows", &styleX, rowY)) selectedAtoms = 1;
                if (AutoButton(selectedAtoms == 2 ? "[X] Both" : "[ ] Both", &styleX, rowY)) selectedAtoms = 2;
                rowY += 50 * globalFontScale;

                // Scales
                DrawThemeText(TextFormat("Scale: %.1f", atomScale), mX + 20, rowY, 20, COL_TEXT);
                float scX = mX + 250 * globalFontScale;
                if (AutoButton(" - ", &scX, rowY)) { if(atomScale > 0.1f) atomScale -= 0.1f; }
                if (AutoButton(" + ", &scX, rowY)) { if(atomScale < 5.0f) atomScale += 0.1f; }
                rowY += 50 * globalFontScale;

                DrawThemeText(TextFormat("Clipping: %.1f", clipDistance), mX + 20, rowY, 20, COL_TEXT);
                float clX = mX + 250 * globalFontScale;
                if (AutoButton(" - ", &clX, rowY)) { if(clipDistance > 1.0f) clipDistance -= 1.0f; }
                if (AutoButton(" + ", &clX, rowY)) { if(clipDistance < 5000.0f) clipDistance += 100.0f; }
                rowY += 70 * globalFontScale;

                // output name

                DrawThemeText("Output Name:", mX + 20, rowY, 20, COL_TEXT);
                Rectangle nameTextbox = { mX + 170 * globalFontScale, rowY - 5, 250 * globalFontScale, 30 * globalFontScale };

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (CheckCollisionPointRec(GetMousePosition(), nameTextbox)) textBoxActive = 1;
                }

                DrawRectangleRec(nameTextbox, COL_TERMINAL);
                DrawRectangleLinesEx(nameTextbox, 2, (textBoxActive == 1) ? COL_ACCENT : Fade(COL_TEXT, 0.3f));
                DrawThemeText(outputFileName, (int)nameTextbox.x + 5, (int)nameTextbox.y + 5, 20, COL_TEXT);

                if (textBoxActive == 1) {
                    SetMouseCursor(MOUSE_CURSOR_IBEAM);
                    int key = GetCharPressed();
                    while (key > 0) {
                        if ((key >= 32) && (key <= 125) && (letterCount < 100)) {
                            outputFileName[letterCount] = (char)key;
                            outputFileName[letterCount + 1] = '\0';
                            letterCount++;
                        }
                        key = GetCharPressed();
                    }
                    if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0) {
                        letterCount--;
                        outputFileName[letterCount] = '\0';
                    }
                }
                else { SetMouseCursor(MOUSE_CURSOR_DEFAULT); }
                rowY += 60 * globalFontScale;

                // blender Toggle
                float optB = mX + 20;
                if (AutoButton(runBlenderAutomatically ? "[X] Run Blender" : "[ ] Run Blender", &optB, rowY)) {
                    runBlenderAutomatically = !runBlenderAutomatically;
                }
            
                EndScissorMode();

            // pinned Footer, stays at bottom while things scroll past.
            float goX = mX + 20;
            float goY = mY + mH - 60 * globalFontScale;
            if (AutoButton("GENERATE & RUN", &goX, goY)) {
                isExporting = true;
                exportTimer = 0.0f;
                // Pass arguments to ExportPath
                ExportPath(path, selectedBg, selectedColourScheme, outputFileName, selectedAtoms, atomScale, clipDistance, custombackgroundcolour);
                
                if (runBlenderAutomatically) {
                    #ifdef _WIN32
                        system("start blender -b -P render_job.py");
                    #else
                        system("blender -b -P render_job.py &");
                    #endif
                }
                showRenderModal = false;
            }

            float closeX = mX + mW - 60 * globalFontScale;
            if (AutoButton(" X ", &closeX, mY + 10)) showRenderModal = false;
        } // end showRenderModal

        if (isExporting) {
            exportTimer += GetFrameTime();
            DrawLoadingScreen(); 
            if (exportTimer > 3.0f) isExporting = false;
        }

        EndDrawing();
    }
    SaveSettings();
    // --- clean up fonts ---
    UnloadFont(fontWizard); 
    UnloadFont(fontVampire); 
    UnloadFont(fontCyber);
    UnloadFont(fontPink);
    CloseWindow();

    return 0;
}