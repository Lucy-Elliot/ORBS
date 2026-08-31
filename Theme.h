#pragma once
#include "raylib.h"

enum ThemeType { THEME_BASIC, THEME_WIZARD, THEME_VAMPIRE, THEME_CYBER, THEME_PINK };

extern ThemeType currentTheme;

extern Color COL_BACKGROUND;
extern Color COL_SIDEBAR;
extern Color COL_TERMINAL;
extern Color COL_TEXT;
extern Color COL_ACCENT;
extern Color SPARKLE_COLOR;


// extern Font fonts for the theme!
extern Font fontWizard;
extern Font fontVampire;
extern Font fontCyber;
extern Font fontPink;
extern Font currentFont;
extern float globalFontScale;

void SetTheme(ThemeType type);
void DrawThemeText(const char* text, int x, int y, int fontSize, Color color);