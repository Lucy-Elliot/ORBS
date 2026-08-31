
#include "fileio.h"
#include "log.h"
#include "Theme.h"

#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
int seqStartIndex = 0;
int seqEndIndex = 0;

namespace fs = std::filesystem;


std::vector<std::string> fileList;
std::string currentLoadedFile = "None";
std::string currentDirectory = "."; 

void LoadSettings() {
    std::ifstream inFile("config.txt");
    if (inFile.is_open()) {
        int themeInt;
        float scale;
        if (inFile >> themeInt >> scale) {
            // applying the loaded values
            currentTheme = (ThemeType)themeInt;
            globalFontScale = scale;
        }
        inFile.close();
    }
}

void LoadFiles() {
    fileList.clear();
    std::error_code ec;
    fs::path p = fs::canonical(currentDirectory, ec);
    if (ec) { currentDirectory = "."; p = fs::current_path(); } else { currentDirectory = p.string(); }

    if (p.has_parent_path() && p != p.root_path()) fileList.push_back(".. (Go Up)");

    try {
        if (fs::exists(p) && fs::is_directory(p)) {
            auto options = fs::directory_options::skip_permission_denied;
            for (const auto& entry : fs::directory_iterator(p, options, ec)) {
                if (ec) continue;
                std::string filename = entry.path().filename().string();
                if (filename[0] == '.') continue; 
                if (entry.is_directory()) fileList.push_back(filename + "/");
                else if (filename.find(".txt") != std::string::npos) fileList.push_back(filename);
            }
        }
    } catch (...) { Log("ERROR: Access Denied"); }
    std::sort(fileList.begin(), fileList.end());
}


void SaveSettings() {
    std::ofstream outFile("config.txt");
    if (outFile.is_open()) {
        outFile << (int)currentTheme << "\n";
        outFile << globalFontScale << "\n";
        outFile.close();
    }
}

void ScanDirectoryForSequence(std::string folderPath, std::string referenceFile) {
    foundSequenceFiles.clear();
    fs::path refPath(referenceFile);
    std::string refName = refPath.filename().string();
    
    std::string prefix = "";
    size_t firstDigit = refName.find_first_of("0123456789");
    if (firstDigit != std::string::npos) {
        prefix = refName.substr(0, firstDigit);
    } else {
        prefix = refName.substr(0, refName.find_last_of("."));
    }

    if (fs::exists(folderPath)) {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (!entry.is_regular_file()) continue;
            std::string fname = entry.path().filename().string();
            // Must be .txt AND start with prefix AND not be CMakeLists, made with vdc --text. can handle some files but for some that are too big the lag is too much . overcoming soon :)
            if (fname.find(".txt") != std::string::npos && fname.find(prefix) == 0 && fname.find("CMake") == std::string::npos) {
                //
                foundSequenceFiles.push_back(fs::absolute(entry.path()).string());
            }
        }
    }
    std::sort(foundSequenceFiles.begin(), foundSequenceFiles.end());
    
    seqStartIndex = 0;
    seqEndIndex = (int)foundSequenceFiles.size() - 1;
    if (seqEndIndex < 0) seqEndIndex = 0;
}
