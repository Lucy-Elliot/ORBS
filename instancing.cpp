#include "instancing.h"
#include "atom.h"
#include "config.h"
#include "raymath.h"
#include <cstdlib> // for malloc and free

Matrix* atomTransforms = nullptr;
Color* atomColors = nullptr;
int atomCount = 0;
float atomScale = 1.0f;
std::vector<Matrix> redTransforms;
std::vector<Matrix> blueTransforms;
Mesh atomMesh;
Material atomMaterial = LoadMaterialDefault();
Material matRed;
Material matBlue;


const char* instance_vshader = 
    "#version 330\n"
    "in vec3 vertexPosition; in vec2 vertexTexCoord; in vec3 vertexNormal; in vec4 vertexColor;"
    "in mat4 instanceTransform;"
    "uniform mat4 mvp; uniform mat4 matNormal;"
    "out vec3 fragPosition; out vec2 fragTexCoord; out vec4 fragColor; out vec3 fragNormal;"
    "void main() {"
    "    fragPosition = vec3(instanceTransform * vec4(vertexPosition, 1.0));"
    "    fragTexCoord = vertexTexCoord; fragColor = vertexColor;"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));"
    "    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);"
    "}";



void UpdateInstancingData() {
    if (atomTransforms) free(atomTransforms);
    if (atomColors) free(atomColors);
    
    atomCount = atoms.size();
    atomTransforms = (Matrix*)malloc(atomCount * sizeof(Matrix));
    atomColors = (Color*)malloc(atomCount * sizeof(Color));

    for (int i = 0; i < atomCount; i++) {
        atomTransforms[i] = MatrixTranslate(atoms[i].position.x, atoms[i].position.y, atoms[i].position.z);
        atomColors[i] = atoms[i].color;
    }
}


