#pragma once

#include "raylib.h"
#include <vector>


extern std::vector<Matrix> redTransforms;
extern std::vector<Matrix> blueTransforms;
extern Matrix* atomTransforms;
extern Color* atomColors;
extern int atomCount;
extern float atomScale;
extern Mesh atomMesh;
extern Material atomMaterial;
extern Material matRed;
extern Material matBlue;

extern const char* instance_vshader;

void UpdateInstancingData();