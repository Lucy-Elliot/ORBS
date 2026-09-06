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
#include "rlgl.h"       
#include <stdlib.h> 
#define RAYGUI_IMPLEMENTATION
#ifndef TextToFloat
    #define TextToFloat(text) (float)atof(text)
#endif
#include "raygui.h"

#include "Config.h"
#include "Animation.h"
#include "Theme.h"
#include "fileio.h"
#include "atom.h"
#include "log.h"
#include "instancing.h"
#include "export.h"
#include "ui.h"
#include "smartcube.h"

namespace fs = std::filesystem;

// --- CONSTANTS ---
const char* colourSchemeNames[] = { "Default (Red/Blue)", "Blue-Gold", "Jet (Heatmap)", "Custom .inc File" };


// --- a lot of variables ---
bool showHelpModal = false;
bool showSettingsModal = false;
bool isDropdownOpen = false;
bool showRenderModal = false; 
int selectedBg = 0; // preselected background in render modal
int selectedColourScheme = 0; // 0=Default, 1=Blue-Gold, 2=Jet, 3=Custom .inc
char customIncPath[1024] = { 0 }; //For custom colour scheme .inc file  ----- FIX so people can add own  .inc file, maybe incl custom file path for when uploading to HPC
char outputFileName[128] = "render_output"; // Default name
int letterCount = 13; // Match the length of the default name
int textBoxActive = 0; // 0 = none, 1 = output name, 2 = custom inc path, 3 = custom bg colour
int selectedAtoms = 0; // 0=Atoms, 1=Arrows, 2 = Both, 
float clipDistance = 1000.0f; //clipping distance for Blender camera
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
int delCamera = 0; // for marking a camera for deletion in the keyframe list in the sidebar, stores the id of the camera to delete, 0 when not marking any camera for deletion
bool showGreenMesh = false;



// creating a geometry nodes style thing for the atoms/arrows, hopefully makes less laggy


std::vector<std::string> foundSequenceFiles;

float rendermodalscroll = 0.0f;


// Global Font Scale
float globalFontScale = 1.0f;

int targetFPS = 60;

float loadingProgress = 0.0f; // 0.0 to 1.0

std::vector<Atom> atoms;

static Font LoadProjectFont(const char* filename) {
    std::vector<fs::path> candidates = {
        fs::current_path() / filename,
        fs::current_path() / "resources" / filename,
        fs::current_path() / ".." / filename,
        fs::current_path() / ".." / "resources" / filename,
        fs::path(filename)
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return LoadFont(candidate.string().c_str());
        }
    }

    Log(TextFormat("Font not found: %s; using default font", filename));
    return GetFontDefault();
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

    SmartCube myCube; 
    myCube.position = (Vector3){ 0.0f, 2.0f, 0.0f }; // Start it at a visible spot
    myCube.size = (Vector3){ 2.0f, 2.0f, 2.0f };     // Give it a default size

    LoadSettings();
    // font loading
    fontWizard  = LoadProjectFont("wizard.ttf");
    fontVampire = LoadProjectFont("vampire.ttf");
    fontCyber   = LoadProjectFont("cyber.ttf");
    fontPink    = LoadProjectFont("pink.ttf");
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
    InitUnicornTheme();
    LoadFiles();
    std::vector<Keyframe> path;
    bool showCubes = true;
    int currentFrame = 0;


    // loading stuff
    texWizardWindow = LoadTexture("../resources/Wizard_Window.png");
    frameWidth = texWizardWindow.width / frameCount;

    while (!WindowShouldClose()) {
        if (IsWindowResized()) InitVisuals();

        UpdateSmartCube(myCube, camera);
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
            if(IsKeyPressed(KEY_E)){
                showGreenMesh = !showGreenMesh;
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
                    if (showGreenMesh) {
                        DrawSmartCube(myCube, COL_ACCENT, WHITE);
                    }
                } else {
                    for (const auto& a : atoms) DrawPoint3D(a.position, a.color);
                }
                DrawGrid(100, 1.0f);
            EndMode3D();

        if (myCube.isMenuOpen) {
        int x = myCube.menuPos.x;
        int y = myCube.menuPos.y;
        DrawRectangle(x, y, 200, 80, Fade(RAYWHITE, 0.9f));
        if (GuiButton({(float)x + 5, (float)y + 5, 90, 20}, "Axis: X")) myCube.gradientMode = 0;
        if (GuiButton({(float)x + 5, (float)y + 30, 90, 20}, "Axis: Y")) myCube.gradientMode = 1;
        if (GuiButton({(float)x + 5, (float)y + 55, 90, 20}, "Axis: Z")) myCube.gradientMode = 2;
        if (GuiButton({(float)x + 110, (float)y + 5, 90, 20}, "Axis: -X")) myCube.gradientMode = 3;
        if (GuiButton({(float)x + 110, (float)y + 30, 90, 20}, "Axis: -Y")) myCube.gradientMode = 4;
        if (GuiButton({(float)x + 110, (float)y + 55, 90, 20}, "Axis: -Z")) myCube.gradientMode = 5;
        }
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
                Rectangle kfRect = { (float)renderW + 10, (float)kfY, currentSidebarWidth - 200, 25 * globalFontScale };
                bool hovering = CheckCollisionPointRec(mousePos, kfRect) && CheckCollisionPointRec(mousePos, kfView);

                if (editingKeyframeIndex == i) {
                    // edit mode -- edits the keyframes frame number.
                    DrawRectangleRec(kfRect , COL_TERMINAL);
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
                    
                    //float optX = mX + 20 * globalFontScale;
                    float buttonX = kfRect.x + 250* globalFontScale;
                    if (AutoButton(delCamera == 0 ? "[X] " : "[ ] ", &buttonX , kfRect.y)){
                        if (delCamera == 0) {
                            path.erase(path.begin() + i);
                            editingKeyframeIndex = -1;
                        } else {
                            delCamera = 1 - delCamera; // toggle delete mode.
                        }  
                    };

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

        // ----------------------------------
        //  ---------  Terminal!!  ----------
        //-----------------------------------
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
            DrawThemeText("Double Click Keyframe to Edit Frame Number, \n Single Click to Snap Camera", modalX + 20, modalY + 410, 20, COL_TEXT);
            DrawThemeText("Press E to bring up a cube which spawns electrons, \n double click to bring up axis menu, \nelectrons move from white to the themes accent colour", modalX+20, modalY + 470, 20, COL_TEXT);
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
                ExportPath(path, myCube, selectedBg, selectedColourScheme, outputFileName, selectedAtoms, atomScale, clipDistance, custombackgroundcolour);
                
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