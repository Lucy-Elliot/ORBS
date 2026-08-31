#pragma once

#include "raylib.h"

#include <string>
#include <vector>


struct Atom { Vector3 position; Color color; };


extern std::vector<Atom> atoms;

std::vector<Atom> LoadAtoms(std::string filepath);
