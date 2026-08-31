#include "smartcube.h"
#include "raylib.h"
#include "fileio.h" // global font scale?
#include "rlgl.h"  
#include "raymath.h"
#include "Theme.h"
#include "ui.h"


#include <vector>
#include <string>

void DrawSmartCube(SmartCube &cube, Color accent, Color white) {
    rlPushMatrix();
        rlTranslatef(cube.position.x, cube.position.y, cube.position.z);
        rlScalef(cube.size.x, cube.size.y, cube.size.z);
        
        rlBegin(RL_QUADS);
            auto GetVertexColor = [&](Vector3 v) {
                float t = 0.5f; // Default center
                if (cube.gradientMode == 0) t = v.x + 0.5f;      // X-Axis (-0.5 to 0.5)
                else if (cube.gradientMode == 1) t = v.z + 0.5f; // Y-Axis
                else if (cube.gradientMode == 2) t = v.y + 0.5f; // Z-Axis
                else if (cube.gradientMode == 3) t = (-v.x  + 0.5f); //-X Axis
                else if (cube.gradientMode == 4) t = (-v.z  + 0.5f); //-Y Axis
                else if (cube.gradientMode == 5) t = (-v.y  + 0.5f); //-Z Axis
                
                return (Color){
                    (unsigned char)(accent.r + t * (white.r - accent.r)),
                    (unsigned char)(accent.g + t * (white.g - accent.g)),
                    (unsigned char)(accent.b + t * (white.b - accent.b)), 255
                };
            };

            //define the 8 corners 
            Vector3 v[8] = {
                {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f}, {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, // Front
                {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}  // Back
            };

            // front Face
            for (int i : {0, 1, 2, 3}) { Color c = GetVertexColor(v[i]); rlColor4ub(c.r, c.g, c.b, 255); rlVertex3f(v[i].x, v[i].y, v[i].z); }
            // back Face
            for (int i : {5, 4, 7, 6}) { Color c = GetVertexColor(v[i]); rlColor4ub(c.r, c.g, c.b, 255); rlVertex3f(v[i].x, v[i].y, v[i].z); }
            // top Face
            for (int i : {3, 2, 6, 7}) { Color c = GetVertexColor(v[i]); rlColor4ub(c.r, c.g, c.b, 255); rlVertex3f(v[i].x, v[i].y, v[i].z); }
            // Bottom Face... etc
            for (int i : {1, 0, 4, 5}) { Color c = GetVertexColor(v[i]); rlColor4ub(c.r, c.g, c.b, 255); rlVertex3f(v[i].x, v[i].y, v[i].z); }
            //right
            for (int i : {1, 5, 6, 2}) { Color c = GetVertexColor(v[i]); rlColor4ub(c.r, c.g, c.b, 255); rlVertex3f(v[i].x, v[i].y, v[i].z); }
            //left
            for (int i : {4, 0, 3, 7}) { Color c = GetVertexColor(v[i]); rlColor4ub(c.r, c.g, c.b, 255); rlVertex3f(v[i].x, v[i].y, v[i].z); }
        rlEnd();
    rlPopMatrix();

    if (cube.hoveredFace != -1) {
        DrawCubeWiresV(cube.position, Vector3Add(cube.size, {0.05f, 0.05f, 0.05f}), GREEN);
    }
}

void UpdateSmartCube(SmartCube &cube, Camera camera) {
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    RayCollision col = GetRayCollisionBox(ray, cube.GetBounds());
    float modalX = 100.0f;
    float modalY = 100.0f;
    float modalW = 100.0f;
    Vector2 delta = GetMouseDelta(); // We defined it as 'delta' here

    if (!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        cube.hoveredFace = -1;
        if (col.hit) {
            BoundingBox box = cube.GetBounds();
            float ep = 0.05f;
            if (fabs(col.point.x - box.max.x) < ep) cube.hoveredFace = 0;
            else if (fabs(col.point.x - box.min.x) < ep) cube.hoveredFace = 1;
            else if (fabs(col.point.y - box.max.y) < ep) cube.hoveredFace = 2;
            else if (fabs(col.point.y - box.min.y) < ep) cube.hoveredFace = 3;
            else if (fabs(col.point.z - box.max.z) < ep) cube.hoveredFace = 4;
            else if (fabs(col.point.z - box.min.z) < ep) cube.hoveredFace = 5;
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && col.hit) {
        float currentTime = (float)GetTime();
        if ((currentTime - cube.lastClickTime) < cube.doubleClickThreshold) {
            cube.isMenuOpen = !cube.isMenuOpen;
            cube.menuPos = GetMousePosition();
        }
        cube.lastClickTime = currentTime;
    }

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && cube.hoveredFace != -1 && !cube.isMenuOpen) {
        float sensitivity = 0.1f;
        // Use 'delta' (matching the definition above)
        if (cube.hoveredFace == 0) { float a = delta.x * sensitivity; cube.size.x += a; cube.position.x += a/2; }
        else if (cube.hoveredFace == 1) { float a = -delta.x * sensitivity; cube.size.x += a; cube.position.x -= a/2; }
        else if (cube.hoveredFace == 2) { float a = -delta.y * sensitivity; cube.size.y += a; cube.position.y += a/2; }
        else if (cube.hoveredFace == 3) { float a = delta.y * sensitivity; cube.size.y += a; cube.position.y -= a/2; }
        else if (cube.hoveredFace == 5) { float a = delta.x * sensitivity; cube.size.z += a; cube.position.z -= a/2; }
        else if (cube.hoveredFace == 4) { float a = -delta.x * sensitivity; cube.size.z += a; cube.position.z += a/2; }
        float closeX = (modalX + modalW) - (50.0f * globalFontScale);
        float closeY = modalY + (10.0f * globalFontScale);
        if (AutoButton("X", &closeX, closeY)) cube.isMenuOpen = false;
    }

}