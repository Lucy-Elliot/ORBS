#pragma once
#include "raylib.h"
#include "Animation.h"
#include <vector>

// --- structs for loading animations ---- 
struct MatrixDrop { float x; float y; float speed; char character; };
struct Bat { float x; float y; float offset; };
struct Star { float x; float y; float brightness; };

extern AnimatedLayer layerWindow;
extern AnimatedLayer layerShelf;
extern AnimatedLayer layerWizard;
extern AnimatedLayer layerUnicorn;
extern AnimatedLayer layerOrb;
extern Texture2D texTwinklingWindow;
extern Texture2D texWizardDesk;
extern Texture2D texPotionShelves;
extern Texture2D texRunningUnicorn;
extern Texture2D texWizardWindow;
extern int frameWidth;
extern int frameCount ; // Number of frames in Aseprite animation
extern int currentFrame;
extern float frameTimer ;
extern float frameSpeed;

extern std::vector<MatrixDrop> matrixRain;
extern std::vector<Bat> vampireBats;
extern std::vector<Star> wizardStars;

extern bool isExporting;
extern float exportTimer;
// =--- blender declarations ---
extern float blenderProgress;
extern bool isBlenderRunning;
extern bool runBlenderAutomatically;


//----- func. in ui.cpp, declared here -----
bool AutoButton(const char* text, float* currentX, float y);
void DrawWizardLofi(int cx, int cy, float progress);
void DrawUnicornLoading(int w, int h, float progress, float responsiveScale);
void UpdateBlenderProgress();
void DrawLoadingScreen();
void InitVisuals();
void DrawWizardLoading(int w, int h, float progress, float responsiveScale);
void InitWizardTheme();
void InitUnicornTheme();
