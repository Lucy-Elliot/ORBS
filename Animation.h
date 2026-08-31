#include "raylib.h"
#pragma once   


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
