#pragma once
#include "raylib.h"



struct SmartCube {
    Vector3 position;
    Vector3 size;
    int hoveredFace = -1; 
    int gradientMode = 2;   // Defaulting to z
    bool isMenuOpen = false;
    Vector2 menuPos = { 0, 0 };
    float lastClickTime = 0.0f;
    const float doubleClickThreshold = 0.25f;

    BoundingBox GetBounds() {
        return (BoundingBox){
            { position.x - size.x/2.0f, position.y - size.y/2.0f, position.z - size.z/2.0f },
            { position.x + size.x/2.0f, position.y + size.y/2.0f, position.z + size.z/2.0f }
        };
    }
};


void DrawSmartCube(SmartCube &cube, Color accent, Color white);

void UpdateSmartCube(SmartCube &cube, Camera camera);