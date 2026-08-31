#include "atom.h"
#include "instancing.h" // for the Transforms
#include "log.h" // for Log()
#include "raylib.h"
#include "raymath.h"
#include "fileio.h" // for currentLoadedFile
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;


std::vector<Atom> LoadAtoms(std::string filepath) {
    std::vector<Atom> newAtoms;
    
    // clear the existing batch data
    redTransforms.clear();
    blueTransforms.clear();
    
    std::ifstream file(filepath);
    if (!file.is_open()) { Log("ERROR: Open Failed"); return newAtoms; }
    
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        float x, y, z, sx, sy, sz;
        if (ss >> x >> y >> z >> sx >> sy >> sz) {
            Color c = (sz > 0 ? RED : BLUE);
            newAtoms.push_back({ (Vector3){x, y, z}, c });
            
            Matrix m = MatrixMultiply(MatrixScale(atomScale, atomScale, atomScale), MatrixTranslate(x, y, z));
            if (sz > 0) redTransforms.push_back(m);
            else blueTransforms.push_back(m);
        }
    }
    
    atoms = newAtoms; // update global atoms lists
    currentLoadedFile = fs::path(filepath).filename().string(); 
    Log(TextFormat("LOADED: %d atoms", (int)atoms.size()));
    return newAtoms;
}
