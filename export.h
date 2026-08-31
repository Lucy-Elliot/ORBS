#pragma once

#include "raylib.h"
#include "keyframe.h"
#include "smartcube.h"

#include <vector>

void ExportPath(const std::vector<Keyframe>& path, const SmartCube& cube, int bgType, int colourScheme, char outputName[128], int atomStyle, float atomScale, float clipDistance, char custombackgroundcolour[32]);

