#pragma once

#include "raylib.h"
#include "keyframe.h"
#include "smartcube.h"

#include <vector>

// the long export fuction that generates the python script for blender to render the animation
void ExportPath(const std::vector<Keyframe>& path, const SmartCube& cube, int bgType, int colourScheme, char outputName[128], int atomStyle, float atomScale, float clipDistance, char custombackgroundcolour[32]);

