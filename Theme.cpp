#include "Theme.h"

ThemeType currentTheme = THEME_BASIC;

Font fontWizard = { 0 };
Font fontVampire = { 0 };
Font fontCyber = { 0 };
Font fontPink = { 0 };
Font currentFont = { 0 };

Color COL_BACKGROUND = { 20, 10, 30, 255 };
Color COL_SIDEBAR    = { 40, 20, 60, 255 };
Color COL_TERMINAL   = { 10, 5, 15, 255 };
Color COL_TEXT       = { 200, 200, 255, 255 };
Color COL_ACCENT     = { 0, 255, 255, 255 };
Color SPARKLE_COLOR  = { 255, 255, 255, 255 };

void DrawThemeText(const char* text, int x, int y, int fontSize, Color color) {
	Font font = (currentFont.texture.id == 0) ? GetFontDefault() : currentFont;
	float finalSize = (float)fontSize * globalFontScale;
	DrawTextEx(font, text, {(float)x, (float)y}, finalSize, 1.0f, color);
}

void SetTheme(ThemeType type) {
	currentTheme = type;

	if (type == THEME_WIZARD) {
		COL_BACKGROUND = { 20, 10, 30, 255 };
		COL_SIDEBAR = { 45, 20, 60, 255 };
		COL_TERMINAL = { 15, 5, 20, 255 };
		COL_TEXT = { 220, 220, 255, 255 };
		COL_ACCENT = { 100, 255, 218, 255 };
		SPARKLE_COLOR = { 96, 52, 140, 255 };
		currentFont = fontWizard;
	} else if (type == THEME_VAMPIRE) {
		COL_BACKGROUND = { 20, 5, 5, 255 };
		COL_SIDEBAR = { 60, 10, 10, 255 };
		COL_TERMINAL = { 15, 0, 0, 255 };
		COL_TEXT = { 255, 200, 200, 255 };
		COL_ACCENT = { 255, 0, 0, 255 };
		currentFont = fontVampire;
	} else if (type == THEME_BASIC) {
		COL_BACKGROUND = { 10, 10, 10, 255 };
		COL_SIDEBAR = { 30, 30, 50, 255 };
		COL_TERMINAL = { 5, 5, 15, 255 };
		COL_TEXT = { 255, 255, 255, 255 };
		COL_ACCENT = { 220, 220, 255, 255 };
		currentFont = GetFontDefault();
	} else if (type == THEME_PINK) {
		COL_BACKGROUND = { 253, 230, 255, 255 };
		COL_SIDEBAR = { 225, 194, 242, 255 };
		COL_TERMINAL = { 225, 194, 242, 255 };
		COL_TEXT = { 89, 34, 115, 255 };
		COL_ACCENT = { 176, 62, 184, 255 };
		SPARKLE_COLOR = { 255, 255, 255, 255 };
		currentFont = fontPink;
	} else {
		COL_BACKGROUND = { 5, 5, 10, 255 };
		COL_SIDEBAR = { 20, 30, 40, 255 };
		COL_TERMINAL = { 0, 10, 15, 255 };
		COL_TEXT = { 0, 255, 0, 255 };
		COL_ACCENT = { 0, 255, 0, 255 };
		currentFont = fontCyber;
	}
}