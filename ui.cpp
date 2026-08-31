#include "raylib.h"
#include "ui.h"
#include "Theme.h"
#include "Animation.h"


#include <string>
#include <cmath>
#include <vector>
#include <fstream>

AnimatedLayer layerWindow;
AnimatedLayer layerShelf;
AnimatedLayer layerWizard;
AnimatedLayer layerUnicorn;
AnimatedLayer layerOrb;
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

// --- loading animation ---- 
std::vector<MatrixDrop> matrixRain;
std::vector<Bat> vampireBats;
std::vector<Star> wizardStars;


// --- blenda ----
float blenderProgress = 0.0f;
bool isBlenderRunning = false;
bool isExporting = false;
float exportTimer = 0.0f;

// ---- coming soon ?? ---
bool runBlenderAutomatically = false;


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
void InitWizardTheme() {
    // Window: 13 frames, slow twinkle, with wizard tower yayyy
    layerWindow = { LoadTexture("./resources/Wizard_Window.png"), 13, 0.2f, 0, 0, {0, -70}, 4.0f };
    // Shelf: Static or slight potion bubble (1 frame or more)
    //layerShelf  = { LoadTexture("shelf_sheet.png"), 1, 0.1f, 0, 0, {120, -20}, 4.0f };
    // Wizard: 30 frames, wizarding around
    layerWizard = { LoadTexture("./resources/wizard_person1.png"), 30, 0.08f, 0, 0, {0, 30}, 6.0f };

}

void InitUnicornTheme() {
    // Unicorn: 12 frames, running around
    layerUnicorn = { LoadTexture("../resources/running_unicorn.png"), 12, 0.1f, 0, 0, {0, 40}, 6.0f };
}

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

void DrawUnicornLoading(int w, int h, float progress, float responsiveScale) {
    int cx = w / 2;
    int cy = h / 2 - (h * 0.1f); // shift art up to make room for bar
    float dt = GetFrameTime();

    // update all animation
    layerUnicorn.Update(dt);

    // draw Layers with the passed-in responsiveScale, which will make them scale based on screen height, only got wizard theme atm.
    layerUnicorn.Draw(cx, cy, responsiveScale);  

    // text placed above the bar (Bottom area)
    DrawThemeText("Sparkle On...", cx - 120, h - 110, 30, COL_TEXT);
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
        DrawThemeText("Using Bash or something...", cx - 80, cy + 150, 30, COL_TEXT);
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
        DrawUnicornLoading(w, h, progress, responsiveScale);
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


